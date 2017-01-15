/*
 *  simdtests.c -- test accuraccy and performance of simd optimizations
 *
 *  Copyright (C) 2017 Andreas Mueller.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* We must include all headers memops.c includes to avoid trouble with
 * out namespace game below.
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <memory.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#ifdef __linux__
#include <endian.h>
#endif
#include "memops.h"

#if defined (__SSE2__) && !defined (__sun__)
#include <emmintrin.h>
#ifdef __SSE4_1__
#include <smmintrin.h>
#endif
#endif

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

// our additional headers
#include <time.h>

/* Dirty: include mempos.c twice the second time with SIMD disabled
 * so we can compare aceelerated non accelerated
 */
namespace accelerated {
#include "../common/memops.c"
}

namespace origerated {
#ifdef __SSE2__
#undef __SSE2__
#endif

#ifdef __ARM_NEON__
#undef __ARM_NEON__
#endif

#include "../common/memops.c"
}

// define conversion function types
typedef void (*t_jack_to_integer)(
	char *dst,
	jack_default_audio_sample_t *src,
	unsigned long nsamples,
	unsigned long dst_skip,
	dither_state_t *state);

typedef void (*t_integer_to_jack)(
	jack_default_audio_sample_t *dst,
	char *src,
	unsigned long nsamples,
	unsigned long src_skip);

// define/setup test case data
typedef struct test_case_data {
	uint32_t frame_size;
	uint32_t sample_size;
	bool reverse;
	t_jack_to_integer jack_to_integer_accel;
	t_jack_to_integer jack_to_integer_orig;
	t_integer_to_jack integer_to_jack_accel;
	t_integer_to_jack integer_to_jack_orig;
	dither_state_t *ditherstate;
	const char *name;
} test_case_data_t;

test_case_data_t test_cases[] = {
	{
		4,
		3,
		true,
		accelerated::sample_move_d32u24_sSs,
		origerated::sample_move_d32u24_sSs,
		accelerated::sample_move_dS_s32u24s,
		origerated::sample_move_dS_s32u24s,
		NULL,
		"32u24s" },
	{
		4,
		3,
		false,
		accelerated::sample_move_d32u24_sS,
		origerated::sample_move_d32u24_sS,
		accelerated::sample_move_dS_s32u24,
		origerated::sample_move_dS_s32u24,
		NULL,
		"32u24" },
	{
		3,
		3,
		true,
		accelerated::sample_move_d24_sSs,
		origerated::sample_move_d24_sSs,
		accelerated::sample_move_dS_s24s,
		origerated::sample_move_dS_s24s,
		NULL,
		"24s" },
	{
		3,
		3,
		false,
		accelerated::sample_move_d24_sS,
		origerated::sample_move_d24_sS,
		accelerated::sample_move_dS_s24,
		origerated::sample_move_dS_s24,
		NULL,
		"24" },
	{
		2,
		2,
		true,
		accelerated::sample_move_d16_sSs,
		origerated::sample_move_d16_sSs,
		accelerated::sample_move_dS_s16s,
		origerated::sample_move_dS_s16s,
		NULL,
		"16s" },
	{
		2,
		2,
		false,
		accelerated::sample_move_d16_sS,
		origerated::sample_move_d16_sS,
		accelerated::sample_move_dS_s16,
		origerated::sample_move_dS_s16,
		NULL,
		"16" },
};

// we need to repeat for better accuracy at time measurement
const uint32_t retry_per_case = 1000;

// setup test buffers
#define TESTBUFF_SIZE 1024
jack_default_audio_sample_t jackbuffer_source[TESTBUFF_SIZE];
// integer buffers: max 4 bytes per value / * 2 for stereo
char integerbuffer_accel[TESTBUFF_SIZE*4*2];
char integerbuffer_orig[TESTBUFF_SIZE*4*2];
// float buffers
jack_default_audio_sample_t jackfloatbuffer_accel[TESTBUFF_SIZE];
jack_default_audio_sample_t jackfloatbuffer_orig[TESTBUFF_SIZE];

// comparing unsigned makes life easier
uint32_t extract_integer(
	char* buff,
	uint32_t offset,
	uint32_t frame_size,
	uint32_t sample_size,
	bool big_endian)
{
	uint32_t retval = 0;
	unsigned char* curr;
	uint32_t mult = 1;
	if(big_endian) {
		curr = (unsigned char*)buff + offset + sample_size-1;
		for(uint32_t i=0; i<sample_size; i++) {
			retval += *(curr--) * mult;
			mult*=256;
		}
	}
	else {
		curr = (unsigned char*)buff + offset + frame_size-sample_size;
		for(uint32_t i=0; i<sample_size; i++) {
			retval += *(curr++) * mult;
			mult*=256;
		}
	}
	return retval;
}

int main(int argc, char *argv[])
{
//	parse_arguments(argc, argv);
	uint32_t maxerr_displayed = 10;

	// fill jackbuffer
	for(int i=0; i<TESTBUFF_SIZE; i++) {
		// ramp
		jack_default_audio_sample_t value =
			((jack_default_audio_sample_t)((i % TESTBUFF_SIZE) - TESTBUFF_SIZE/2)) / (TESTBUFF_SIZE/2);
		// force clipping
		value *= 1.02;
		jackbuffer_source[i] = value;
	}

	for(uint32_t testcase=0; testcase<sizeof(test_cases)/sizeof(test_case_data_t); testcase++) {
		// test mono/stereo
		for(uint32_t channels=1; channels<=2; channels++) {
			//////////////////////////////////////////////////////////////////////////////
			// jackfloat -> integer

			// clean target buffers
			memset(integerbuffer_accel, 0, sizeof(integerbuffer_accel));
			memset(integerbuffer_orig, 0, sizeof(integerbuffer_orig));
			// accel
			clock_t time_to_integer_accel = clock();
			for(uint32_t repetition=0; repetition<retry_per_case; repetition++)
			{
				test_cases[testcase].jack_to_integer_accel(
					integerbuffer_accel,
					jackbuffer_source,
					TESTBUFF_SIZE,
					test_cases[testcase].frame_size*channels,
					test_cases[testcase].ditherstate);
			}
			float timediff_to_integer_accel = ((float)(clock() - time_to_integer_accel)) / CLOCKS_PER_SEC;
			// orig
			clock_t time_to_integer_orig = clock();
			for(uint32_t repetition=0; repetition<retry_per_case; repetition++)
			{
				test_cases[testcase].jack_to_integer_orig(
					integerbuffer_orig,
					jackbuffer_source,
					TESTBUFF_SIZE,
					test_cases[testcase].frame_size*channels,
					test_cases[testcase].ditherstate);
			}
			float timediff_to_integer_orig = ((float)(clock() - time_to_integer_orig)) / CLOCKS_PER_SEC;
			// output performance results
			printf(
				"JackFloat->Integer @%7.7s/%u: Orig %7.6f sec / Accel %7.6f sec -> Win: %5.2f %%\n",
				test_cases[testcase].name,
				channels,
				timediff_to_integer_orig,
				timediff_to_integer_accel,
				(timediff_to_integer_orig/timediff_to_integer_accel-1)*100.0);
			uint32_t int_deviation_max = 0;
			uint32_t int_error_count = 0;
			// output error (avoid spam -> limit error lines per test case)
			for(uint32_t sample=0; sample<TESTBUFF_SIZE; sample++) {
				uint32_t sample_offset = sample*test_cases[testcase].frame_size*channels;
				// compare both results
				uint32_t intval_accel=extract_integer(
					integerbuffer_accel,
					sample_offset,
					test_cases[testcase].frame_size,
					test_cases[testcase].sample_size,
#if __BYTE_ORDER == __BIG_ENDIAN
					!test_cases[testcase].reverse);
#else
					test_cases[testcase].reverse);
#endif
				uint32_t intval_orig=extract_integer(
					integerbuffer_orig,
					sample_offset,
					test_cases[testcase].frame_size,
					test_cases[testcase].sample_size,
#if __BYTE_ORDER == __BIG_ENDIAN
					!test_cases[testcase].reverse);
#else
					test_cases[testcase].reverse);
#endif
				if(intval_accel != intval_orig) {
					if(int_error_count<maxerr_displayed) {
						printf("Value error sample %u:", sample);
						printf(" Orig 0x");
						char formatstr[10];
						sprintf(formatstr, "%%0%uX", test_cases[testcase].sample_size*2);
						printf(formatstr, intval_orig);
						printf(" Accel 0x");
						printf(formatstr, intval_accel);
						printf("\n");
					}
					int_error_count++;
					uint32_t int_deviation;
					if(intval_accel > intval_orig)
						int_deviation = intval_accel-intval_orig;
					else
						int_deviation = intval_orig-intval_accel;
					if(int_deviation > int_deviation_max)
						int_deviation_max = int_deviation;
				}
			}
			printf(
				"JackFloat->Integer @%7.7s/%u: Errors: %u Max deviation %u\n",
				test_cases[testcase].name,
				channels,
				int_error_count,
				int_deviation_max);

			//////////////////////////////////////////////////////////////////////////////
			// integer -> jackfloat

			// clean target buffers
			memset(jackfloatbuffer_accel, 0, sizeof(jackfloatbuffer_accel));
			memset(jackfloatbuffer_orig, 0, sizeof(jackfloatbuffer_orig));
			// accel
			clock_t time_to_float_accel = clock();
			for(uint32_t repetition=0; repetition<retry_per_case; repetition++)
			{
				test_cases[testcase].integer_to_jack_accel(
					jackfloatbuffer_accel,
					integerbuffer_orig,
					TESTBUFF_SIZE,
					test_cases[testcase].frame_size*channels);
			}
			float timediff_to_float_accel = ((float)(clock() - time_to_float_accel)) / CLOCKS_PER_SEC;
			// orig
			clock_t time_to_float_orig = clock();
			for(uint32_t repetition=0; repetition<retry_per_case; repetition++)
			{
				test_cases[testcase].integer_to_jack_orig(
					jackfloatbuffer_orig,
					integerbuffer_orig,
					TESTBUFF_SIZE,
					test_cases[testcase].frame_size*channels);
			}
			float timediff_to_float_orig = ((float)(clock() - time_to_float_orig)) / CLOCKS_PER_SEC;
			// output performance results
			printf(
				"Integer->JackFloat @%7.7s/%u: Orig %7.6f sec / Accel %7.6f sec -> Win: %5.2f %%\n",
				test_cases[testcase].name,
				channels,
				timediff_to_float_orig,
				timediff_to_float_accel,
				(timediff_to_float_orig/timediff_to_float_accel-1)*100.0);
			jack_default_audio_sample_t float_deviation_max = 0.0;
			uint32_t float_error_count = 0;
			// output error (avoid spam -> limit error lines per test case)
			for(uint32_t sample=0; sample<TESTBUFF_SIZE; sample++) {
				// For easier estimation/readabilty we scale floats back to integer
				jack_default_audio_sample_t sample_scaling;
				switch(test_cases[testcase].sample_size) {
					case 2:
						sample_scaling = SAMPLE_16BIT_SCALING;
						break;
					default:
						sample_scaling = SAMPLE_24BIT_SCALING;
						break;
				}
				jack_default_audio_sample_t floatval_accel = jackfloatbuffer_accel[sample] * sample_scaling;
				jack_default_audio_sample_t floatval_orig = jackfloatbuffer_orig[sample] * sample_scaling;
				// compare both results
				jack_default_audio_sample_t float_deviation;
				if(floatval_accel > floatval_orig)
					float_deviation = floatval_accel-floatval_orig;
				else
					float_deviation = floatval_orig-floatval_accel;
				if(float_deviation > float_deviation_max)
					float_deviation_max = float_deviation;
				// deviation > half bit => error
				if(float_deviation > 0.5) {
					if(float_error_count<maxerr_displayed) {
						printf("Value error sample %u:", sample);
						printf(" Orig %8.1f Accel %8.1f\n", floatval_orig, floatval_accel);
					}
					float_error_count++;
				}
			}
			printf(
				"Integer->JackFloat @%7.7s/%u: Errors: %u Max deviation %f\n",
				test_cases[testcase].name,
				channels,
				float_error_count,
				float_deviation_max);

			printf("\n");
		}
	}
	return 0;
}
