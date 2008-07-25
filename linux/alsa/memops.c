/*
    Copyright (C) 2000 Paul Davis 

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    $Id: memops.c,v 1.2 2005/08/29 10:36:28 letz Exp $
*/

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#define _ISOC9X_SOURCE  1
#define _ISOC99_SOURCE  1

#define __USE_ISOC9X    1
#define __USE_ISOC99    1

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <memory.h>
#include <stdlib.h>
#include <limits.h>
#include <endian.h>
#include "memops.h"


#define SAMPLE_MAX_24BIT  8388608.0f
#define SAMPLE_MAX_16BIT  32768.0f

#define f_round(f) lrintf(f)

/* Linear Congruential noise generator. From the music-dsp list
 * less random than rand(), but good enough and 10x faster */
inline unsigned int fast_rand();

inline unsigned int fast_rand() {
	static unsigned int seed = 22222;
	seed = (seed * 96314165) + 907633515;

	return seed;
}


/* functions for native float sample data */


void sample_move_floatLE_sSs (jack_default_audio_sample_t *dst, char *src, unsigned long nsamples, unsigned long src_skip) {
       while (nsamples--) {
               *dst = *((float *) src);
               dst++;
               src += src_skip;
       }
}

void sample_move_dS_floatLE (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state) {
       while (nsamples--) {
               *((float *) dst) = *src;
               dst += dst_skip;
               src++;
       }
}


/* functions for native integer sample data */


void sample_move_d32u24_sSs (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state)

{
        long long y;
	int z;

	while (nsamples--) {
		y = (long long)(*src * SAMPLE_MAX_24BIT) << 8;
		if (y > INT_MAX) {
			z = INT_MAX;
		} else if (y < INT_MIN) {
			z = INT_MIN;
		} else {
			z = (int)y;
		}
#if __BYTE_ORDER == __LITTLE_ENDIAN
		dst[0]=(char)(z>>24);
		dst[1]=(char)(z>>16);
		dst[2]=(char)(z>>8);
		dst[3]=(char)(z);
#elif __BYTE_ORDER == __BIG_ENDIAN
		dst[0]=(char)(z);
		dst[1]=(char)(z>>8);
		dst[2]=(char)(z>>16);
		dst[3]=(char)(z>>24);
#endif
		dst += dst_skip;
		src++;
	}
}	

void sample_move_d32u24_sS (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state)

{
        long long y;

	while (nsamples--) {
		y = (long long)(*src * SAMPLE_MAX_24BIT) << 8;
		if (y > INT_MAX) {
			*((int *) dst) = INT_MAX;
		} else if (y < INT_MIN) {
			*((int *) dst) = INT_MIN;
		} else {
			*((int *) dst) = (int)y;
		}
		dst += dst_skip;
		src++;
	}
}	

void sample_move_dS_s32u24s (jack_default_audio_sample_t *dst, char *src, unsigned long nsamples, unsigned long src_skip)
{
	/* ALERT: signed sign-extension portability !!! */

	while (nsamples--) {
		int x;
#if __BYTE_ORDER == __LITTLE_ENDIAN
		x = (unsigned char)(src[0]);
		x <<= 8;
		x |= (unsigned char)(src[1]);
		x <<= 8;
		x |= (unsigned char)(src[2]);
		x <<= 8;
		x |= (unsigned char)(src[3]);
#elif __BYTE_ORDER == __BIG_ENDIAN
		x = (unsigned char)(src[3]);
		x <<= 8;
		x |= (unsigned char)(src[2]);
		x <<= 8;
		x |= (unsigned char)(src[1]);
		x <<= 8;
		x |= (unsigned char)(src[0]);
#endif
		*dst = (x >> 8) / SAMPLE_MAX_24BIT;
		dst++;
		src += src_skip;
	}
}	

void sample_move_dS_s32u24 (jack_default_audio_sample_t *dst, char *src, unsigned long nsamples, unsigned long src_skip)
{
	/* ALERT: signed sign-extension portability !!! */

	while (nsamples--) {
		*dst = (*((int *) src) >> 8) / SAMPLE_MAX_24BIT;
		dst++;
		src += src_skip;
	}
}	

void sample_move_dither_rect_d32u24_sSs (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state)

{
	/* ALERT: signed sign-extension portability !!! */
	jack_default_audio_sample_t  x;
	long long y;
	int z;

	while (nsamples--) {
		x = *src * SAMPLE_MAX_16BIT;
		x -= (float)fast_rand() / (float)INT_MAX;
		y = (long long)f_round(x);
		y <<= 16;
		if (y > INT_MAX) {
			z = INT_MAX;
		} else if (y < INT_MIN) {
			z = INT_MIN;
		} else {
			z = (int)y;
		}
#if __BYTE_ORDER == __LITTLE_ENDIAN
		dst[0]=(char)(z>>24);
		dst[1]=(char)(z>>16);
		dst[2]=(char)(z>>8);
		dst[3]=(char)(z);
#elif __BYTE_ORDER == __BIG_ENDIAN
		dst[0]=(char)(z);
		dst[1]=(char)(z>>8);
		dst[2]=(char)(z>>16);
		dst[3]=(char)(z>>24);
#endif
		dst += dst_skip;
		src++;
	}
}	

void sample_move_dither_rect_d32u24_sS (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state)

{
	/* ALERT: signed sign-extension portability !!! */
	jack_default_audio_sample_t  x;
	long long y;

	while (nsamples--) {
		x = *src * SAMPLE_MAX_16BIT;
		x -= (float)fast_rand() / (float)INT_MAX;
		y = (long long)f_round(x);
		y <<= 16;
		if (y > INT_MAX) {
			*((int *) dst) = INT_MAX;
		} else if (y < INT_MIN) {
			*((int *) dst) = INT_MIN;
		} else {
			*((int *) dst) = (int)y;
		}
		dst += dst_skip;
		src++;
	}
}	

void sample_move_dither_tri_d32u24_sSs (char *dst,  jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state)
	
{
	jack_default_audio_sample_t  x;
	float     r;
	float     rm1 = state->rm1;
	long long y;
	int z;

	while (nsamples--) {
		x = *src * (float)SAMPLE_MAX_16BIT;
		r = 2.0f * (float)fast_rand() / (float)INT_MAX - 1.0f;
		x += r - rm1;
		rm1 = r;
		y = (long long)f_round(x);
		y <<= 16;

		if (y > INT_MAX) {
			z = INT_MAX;
		} else if (y < INT_MIN) {
			z = INT_MIN;
		} else {
			z = (int)y;
		}
#if __BYTE_ORDER == __LITTLE_ENDIAN
		dst[0]=(char)(z>>24);
		dst[1]=(char)(z>>16);
		dst[2]=(char)(z>>8);
		dst[3]=(char)(z);
#elif __BYTE_ORDER == __BIG_ENDIAN
		dst[0]=(char)(z);
		dst[1]=(char)(z>>8);
		dst[2]=(char)(z>>16);
		dst[3]=(char)(z>>24);
#endif
		dst += dst_skip;
		src++;
	}
	state->rm1 = rm1;
}

void sample_move_dither_tri_d32u24_sS (char *dst,  jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state)
	
{
	jack_default_audio_sample_t  x;
	float     r;
	float     rm1 = state->rm1;
	long long y;

	while (nsamples--) {
		x = *src * (float)SAMPLE_MAX_16BIT;
		r = 2.0f * (float)fast_rand() / (float)INT_MAX - 1.0f;
		x += r - rm1;
		rm1 = r;
		y = (long long)f_round(x);
		y <<= 16;

		if (y > INT_MAX) {
			*((int *) dst) = INT_MAX;
		} else if (y < INT_MIN) {
			*((int *) dst) = INT_MIN;
		} else {
			*((int *) dst) = (int)y;
		}

		dst += dst_skip;
		src++;
	}
	state->rm1 = rm1;
}

void sample_move_dither_shaped_d32u24_sSs (char *dst,  jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state)
	
{
	jack_default_audio_sample_t     x;
	jack_default_audio_sample_t     xe; /* the innput sample - filtered error */
	jack_default_audio_sample_t     xp; /* x' */
	float        r;
	float        rm1 = state->rm1;
	unsigned int idx = state->idx;
	long long    y;
	int z;

	while (nsamples--) {
		x = *src * (float)SAMPLE_MAX_16BIT;
		r = 2.0f * (float)fast_rand() / (float)INT_MAX - 1.0f;
		/* Filter the error with Lipshitz's minimally audible FIR:
		   [2.033 -2.165 1.959 -1.590 0.6149] */
		xe = x
		     - state->e[idx] * 2.033f
		     + state->e[(idx - 1) & DITHER_BUF_MASK] * 2.165f
		     - state->e[(idx - 2) & DITHER_BUF_MASK] * 1.959f
		     + state->e[(idx - 3) & DITHER_BUF_MASK] * 1.590f
		     - state->e[(idx - 4) & DITHER_BUF_MASK] * 0.6149f;
		xp = xe + r - rm1;
		rm1 = r;

		/* This could be some inline asm on x86 */
		y = (long long)f_round(xp);

		/* Intrinsic z^-1 delay */
		idx = (idx + 1) & DITHER_BUF_MASK;
		state->e[idx] = y - xe;

		y <<= 16;

		if (y > INT_MAX) {
			z = INT_MAX;
		} else if (y < INT_MIN) {
			z = INT_MIN;
		} else {
			z = (int)y;
		}
#if __BYTE_ORDER == __LITTLE_ENDIAN
		dst[0]=(char)(z>>24);
		dst[1]=(char)(z>>16);
		dst[2]=(char)(z>>8);
		dst[3]=(char)(z);
#elif __BYTE_ORDER == __BIG_ENDIAN
		dst[0]=(char)(z);
		dst[1]=(char)(z>>8);
		dst[2]=(char)(z>>16);
		dst[3]=(char)(z>>24);
#endif
		dst += dst_skip;
		src++;
	}
	state->rm1 = rm1;
	state->idx = idx;
}

void sample_move_dither_shaped_d32u24_sS (char *dst,  jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state)
	
{
	jack_default_audio_sample_t     x;
	jack_default_audio_sample_t     xe; /* the innput sample - filtered error */
	jack_default_audio_sample_t     xp; /* x' */
	float        r;
	float        rm1 = state->rm1;
	unsigned int idx = state->idx;
	long long    y;

	while (nsamples--) {
		x = *src * (float)SAMPLE_MAX_16BIT;
		r = 2.0f * (float)fast_rand() / (float)INT_MAX - 1.0f;
		/* Filter the error with Lipshitz's minimally audible FIR:
		   [2.033 -2.165 1.959 -1.590 0.6149] */
		xe = x
		     - state->e[idx] * 2.033f
		     + state->e[(idx - 1) & DITHER_BUF_MASK] * 2.165f
		     - state->e[(idx - 2) & DITHER_BUF_MASK] * 1.959f
		     + state->e[(idx - 3) & DITHER_BUF_MASK] * 1.590f
		     - state->e[(idx - 4) & DITHER_BUF_MASK] * 0.6149f;
		xp = xe + r - rm1;
		rm1 = r;

		/* This could be some inline asm on x86 */
		y = (long long)f_round(xp);

		/* Intrinsic z^-1 delay */
		idx = (idx + 1) & DITHER_BUF_MASK;
		state->e[idx] = y - xe;

		y <<= 16;

		if (y > INT_MAX) {
			*((int *) dst) = INT_MAX;
		} else if (y < INT_MIN) {
			*((int *) dst) = INT_MIN;
		} else {
			*((int *) dst) = y;
		}
		dst += dst_skip;
		src++;
	}
	state->rm1 = rm1;
	state->idx = idx;
}

void sample_move_d24_sSs (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state)

{
        long long y;
	int z;

	while (nsamples--) {
		y = (long long)(*src * SAMPLE_MAX_24BIT);

		if (y > (INT_MAX >> 8 )) {
			z = (INT_MAX >> 8);
		} else if (y < (INT_MIN >> 8 )) {
			z = (INT_MIN >> 8 );
		} else {
			z = (int)y;
		}
#if __BYTE_ORDER == __LITTLE_ENDIAN
		dst[0]=(char)(z>>16);
		dst[1]=(char)(z>>8);
		dst[2]=(char)(z);
#elif __BYTE_ORDER == __BIG_ENDIAN
		dst[0]=(char)(z);
		dst[1]=(char)(z>>8);
		dst[2]=(char)(z>>16);
#endif
		dst += dst_skip;
		src++;
	}
}	

void sample_move_d24_sS (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state)

{
        long long y;

	while (nsamples--) {
		y = (long long)(*src * SAMPLE_MAX_24BIT);

		if (y > (INT_MAX >> 8 )) {
			y = (INT_MAX >> 8);
		} else if (y < (INT_MIN >> 8 )) {
			y = (INT_MIN >> 8 );
		}
#if __BYTE_ORDER == __LITTLE_ENDIAN
		memcpy (dst, &y, 3);
#elif __BYTE_ORDER == __BIG_ENDIAN
		memcpy (dst, (char *)&y + 5, 3);
#endif
		dst += dst_skip;
		src++;
	}
}	

void sample_move_dS_s24s (jack_default_audio_sample_t *dst, char *src, unsigned long nsamples, unsigned long src_skip)
{
	/* ALERT: signed sign-extension portability !!! */

	while (nsamples--) {
		int x;
#if __BYTE_ORDER == __LITTLE_ENDIAN
		x = (unsigned char)(src[0]);
		x <<= 8;
		x |= (unsigned char)(src[1]);
		x <<= 8;
		x |= (unsigned char)(src[2]);
		/* correct sign bit and the rest of the top byte */
		if (src[0] & 0x80) {
			x |= 0xff << 24;
		}
#elif __BYTE_ORDER == __BIG_ENDIAN
		x = (unsigned char)(src[2]);
		x <<= 8;
		x |= (unsigned char)(src[1]);
		x <<= 8;
		x |= (unsigned char)(src[0]);
		/* correct sign bit and the rest of the top byte */
		if (src[0] & 0x80) {
			x |= 0xff << 24;
		}
#endif
		*dst = x / SAMPLE_MAX_24BIT;
		dst++;
		src += src_skip;
	}
}	

void sample_move_dS_s24 (jack_default_audio_sample_t *dst, char *src, unsigned long nsamples, unsigned long src_skip)
{
	/* ALERT: signed sign-extension portability !!! */

	while (nsamples--) {
		int x;
#if __BYTE_ORDER == __LITTLE_ENDIAN
		memcpy((char*)&x + 1, src, 3);
#elif __BYTE_ORDER == __BIG_ENDIAN
		memcpy(&x, src, 3);
#endif
		x >>= 8;
		*dst = x / SAMPLE_MAX_24BIT;
		dst++;
		src += src_skip;
	}
}	

void sample_move_dither_rect_d24_sSs (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state)

{
	/* ALERT: signed sign-extension portability !!! */
	jack_default_audio_sample_t  x;
	long long y;
	int z;

	while (nsamples--) {
		x = *src * SAMPLE_MAX_16BIT;
		x -= (float)fast_rand() / (float)INT_MAX;
		y = (long long)f_round(x);

		y <<= 8;

		if (y > (INT_MAX >> 8)) {
			z = (INT_MAX >> 8);
		} else if (y < (INT_MIN >> 8)) {
			z = (INT_MIN >> 8);
		} else {
			z = (int)y;
		}
#if __BYTE_ORDER == __LITTLE_ENDIAN
		dst[0]=(char)(z>>16);
		dst[1]=(char)(z>>8);
		dst[2]=(char)(z);
#elif __BYTE_ORDER == __BIG_ENDIAN
		dst[0]=(char)(z);
		dst[1]=(char)(z>>8);
		dst[2]=(char)(z>>16);
#endif
		dst += dst_skip;
		src++;
	}
}	

void sample_move_dither_rect_d24_sS (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state)

{
	/* ALERT: signed sign-extension portability !!! */
	jack_default_audio_sample_t  x;
	long long y;

	while (nsamples--) {
		x = *src * SAMPLE_MAX_16BIT;
		x -= (float)fast_rand() / (float)INT_MAX;
		y = (long long)f_round(x);

		y <<= 8;

		if (y > (INT_MAX >> 8)) {
			y = (INT_MAX >> 8);
		} else if (y < (INT_MIN >> 8)) {
			y = (INT_MIN >> 8);
		}
#if __BYTE_ORDER == __LITTLE_ENDIAN
		memcpy (dst, &y, 3);
#elif __BYTE_ORDER == __BIG_ENDIAN
		memcpy (dst, (char *)&y + 5, 3);
#endif

		dst += dst_skip;
		src++;
	}
}	

void sample_move_dither_tri_d24_sSs (char *dst,  jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state)
	
{
	jack_default_audio_sample_t  x;
	float     r;
	float     rm1 = state->rm1;
	long long y;
	int z;

	while (nsamples--) {
		x = *src * (float)SAMPLE_MAX_16BIT;
		r = 2.0f * (float)fast_rand() / (float)INT_MAX - 1.0f;
		x += r - rm1;
		rm1 = r;
		y = (long long)f_round(x);

		y <<= 8;

		if (y > (INT_MAX >> 8)) {
			z = (INT_MAX >> 8);
		} else if (y < (INT_MIN >> 8)) {
			z = (INT_MIN >> 8);
		} else {
			z = (int)y;
		}
#if __BYTE_ORDER == __LITTLE_ENDIAN
		dst[0]=(char)(z>>16);
		dst[1]=(char)(z>>8);
		dst[2]=(char)(z);
#elif __BYTE_ORDER == __BIG_ENDIAN
		dst[0]=(char)(z);
		dst[1]=(char)(z>>8);
		dst[2]=(char)(z>>16);
#endif
		dst += dst_skip;
		src++;
	}
	state->rm1 = rm1;
}

void sample_move_dither_tri_d24_sS (char *dst,  jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state)
	
{
	jack_default_audio_sample_t  x;
	float     r;
	float     rm1 = state->rm1;
	long long y;

	while (nsamples--) {
		x = *src * (float)SAMPLE_MAX_16BIT;
		r = 2.0f * (float)fast_rand() / (float)INT_MAX - 1.0f;
		x += r - rm1;
		rm1 = r;
		y = (long long)f_round(x);

		y <<= 8;

		if (y > (INT_MAX >> 8)) {
			y = (INT_MAX >> 8);
		} else if (y < (INT_MIN >> 8)) {
			y = (INT_MIN >> 8);
		}
#if __BYTE_ORDER == __LITTLE_ENDIAN
		memcpy (dst, &y, 3);
#elif __BYTE_ORDER == __BIG_ENDIAN
		memcpy (dst, (char *)&y + 5, 3);
#endif

		dst += dst_skip;
		src++;
	}
	state->rm1 = rm1;
}

void sample_move_dither_shaped_d24_sSs (char *dst,  jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state)
	
{
	jack_default_audio_sample_t     x;
	jack_default_audio_sample_t     xe; /* the innput sample - filtered error */
	jack_default_audio_sample_t     xp; /* x' */
	float        r;
	float        rm1 = state->rm1;
	unsigned int idx = state->idx;
	long long    y;
	int z;

	while (nsamples--) {
		x = *src * (float)SAMPLE_MAX_16BIT;
		r = 2.0f * (float)fast_rand() / (float)INT_MAX - 1.0f;
		/* Filter the error with Lipshitz's minimally audible FIR:
		   [2.033 -2.165 1.959 -1.590 0.6149] */
		xe = x
		     - state->e[idx] * 2.033f
		     + state->e[(idx - 1) & DITHER_BUF_MASK] * 2.165f
		     - state->e[(idx - 2) & DITHER_BUF_MASK] * 1.959f
		     + state->e[(idx - 3) & DITHER_BUF_MASK] * 1.590f
		     - state->e[(idx - 4) & DITHER_BUF_MASK] * 0.6149f;
		xp = xe + r - rm1;
		rm1 = r;

		/* This could be some inline asm on x86 */
		y = (long long)f_round(xp);

		/* Intrinsic z^-1 delay */
		idx = (idx + 1) & DITHER_BUF_MASK;
		state->e[idx] = y - xe;

		y <<= 8;

		if (y > (INT_MAX >> 8)) {
			z = (INT_MAX >> 8);
		} else if (y < (INT_MIN >> 8)) {
			z = (INT_MIN >> 8);
		} else {
			z = (int)y;
		}
#if __BYTE_ORDER == __LITTLE_ENDIAN
		dst[0]=(char)(z>>16);
		dst[1]=(char)(z>>8);
		dst[2]=(char)(z);
#elif __BYTE_ORDER == __BIG_ENDIAN
		dst[0]=(char)(z);
		dst[1]=(char)(z>>8);
		dst[2]=(char)(z>>16);
#endif
		dst += dst_skip;
		src++;
	}
	state->rm1 = rm1;
	state->idx = idx;
}

void sample_move_dither_shaped_d24_sS (char *dst,  jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state)
	
{
	jack_default_audio_sample_t     x;
	jack_default_audio_sample_t     xe; /* the innput sample - filtered error */
	jack_default_audio_sample_t     xp; /* x' */
	float        r;
	float        rm1 = state->rm1;
	unsigned int idx = state->idx;
	long long    y;

	while (nsamples--) {
		x = *src * (float)SAMPLE_MAX_16BIT;
		r = 2.0f * (float)fast_rand() / (float)INT_MAX - 1.0f;
		/* Filter the error with Lipshitz's minimally audible FIR:
		   [2.033 -2.165 1.959 -1.590 0.6149] */
		xe = x
		     - state->e[idx] * 2.033f
		     + state->e[(idx - 1) & DITHER_BUF_MASK] * 2.165f
		     - state->e[(idx - 2) & DITHER_BUF_MASK] * 1.959f
		     + state->e[(idx - 3) & DITHER_BUF_MASK] * 1.590f
		     - state->e[(idx - 4) & DITHER_BUF_MASK] * 0.6149f;
		xp = xe + r - rm1;
		rm1 = r;

		/* This could be some inline asm on x86 */
		y = (long long)f_round(xp);

		/* Intrinsic z^-1 delay */
		idx = (idx + 1) & DITHER_BUF_MASK;
		state->e[idx] = y - xe;

		y <<= 8;

		if (y > (INT_MAX >> 8)) {
			y = (INT_MAX >> 8);
		} else if (y < (INT_MIN >> 8)) {
			y = (INT_MIN >> 8);
		}
#if __BYTE_ORDER == __LITTLE_ENDIAN
		memcpy (dst, &y, 3);
#elif __BYTE_ORDER == __BIG_ENDIAN
		memcpy (dst, (char *)&y + 5, 3);
#endif

		dst += dst_skip;
		src++;
	}
	state->rm1 = rm1;
	state->idx = idx;
}

void sample_move_d16_sSs (char *dst,  jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state)
	
{
	int tmp;

	/* ALERT: signed sign-extension portability !!! */

	while (nsamples--) {
		tmp = f_round(*src * SAMPLE_MAX_16BIT);
		if (tmp > SHRT_MAX) {
			tmp = SHRT_MAX;
		} else if (tmp < SHRT_MIN) {
			tmp = SHRT_MIN;
		}
#if __BYTE_ORDER == __LITTLE_ENDIAN
		dst[0]=(char)(tmp>>8);
		dst[1]=(char)(tmp);
#elif __BYTE_ORDER == __BIG_ENDIAN
		dst[0]=(char)(tmp);
		dst[1]=(char)(tmp>>8);
#endif
		dst += dst_skip;
		src++;
	}
}

void sample_move_d16_sS (char *dst,  jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state)
	
{
	int tmp;

	/* ALERT: signed sign-extension portability !!! */

	while (nsamples--) {
		tmp = f_round(*src * SAMPLE_MAX_16BIT);
		if (tmp > SHRT_MAX) {
			*((short *)dst) = SHRT_MAX;
		} else if (tmp < SHRT_MIN) {
			*((short *)dst) = SHRT_MIN;
		} else {
			*((short *) dst) = (short) tmp;
		}
		dst += dst_skip;
		src++;
	}
}

void sample_move_dither_rect_d16_sSs (char *dst,  jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state)
	
{
	jack_default_audio_sample_t val;
	int      tmp;

	while (nsamples--) {
		val = *src * (float)SAMPLE_MAX_16BIT;
		val -= (float)fast_rand() / (float)INT_MAX;
		tmp = f_round(val);
		if (tmp > SHRT_MAX) {
			tmp = SHRT_MAX;
		} else if (tmp < SHRT_MIN) {
			tmp = SHRT_MIN;
		}
#if __BYTE_ORDER == __LITTLE_ENDIAN
		dst[0]=(char)(tmp>>8);
		dst[1]=(char)(tmp);
#elif __BYTE_ORDER == __BIG_ENDIAN
		dst[0]=(char)(tmp);
		dst[1]=(char)(tmp>>8);
#endif
		dst += dst_skip;
		src++;
	}
}

void sample_move_dither_rect_d16_sS (char *dst,  jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state)
	
{
	jack_default_audio_sample_t val;
	int      tmp;

	while (nsamples--) {
		val = *src * (float)SAMPLE_MAX_16BIT;
		val -= (float)fast_rand() / (float)INT_MAX;
		tmp = f_round(val);
		if (tmp > SHRT_MAX) {
			*((short *)dst) = SHRT_MAX;
		} else if (tmp < SHRT_MIN) {
			*((short *)dst) = SHRT_MIN;
		} else {
			*((short *) dst) = (short)tmp;
		}
		dst += dst_skip;
		src++;
	}
}

void sample_move_dither_tri_d16_sSs (char *dst,  jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state)
	
{
	jack_default_audio_sample_t x;
	float    r;
	float    rm1 = state->rm1;
	int      y;

	while (nsamples--) {
		x = *src * (float)SAMPLE_MAX_16BIT;
		r = 2.0f * (float)fast_rand() / (float)INT_MAX - 1.0f;
		x += r - rm1;
		rm1 = r;
		y = f_round(x);

		if (y > SHRT_MAX) {
			y = SHRT_MAX;
		} else if (y < SHRT_MIN) {
			y = SHRT_MIN;
		}
#if __BYTE_ORDER == __LITTLE_ENDIAN
		dst[0]=(char)(y>>8);
		dst[1]=(char)(y);
#elif __BYTE_ORDER == __BIG_ENDIAN
		dst[0]=(char)(y);
		dst[1]=(char)(y>>8);
#endif
		dst += dst_skip;
		src++;
	}
	state->rm1 = rm1;
}

void sample_move_dither_tri_d16_sS (char *dst,  jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state)
	
{
	jack_default_audio_sample_t x;
	float    r;
	float    rm1 = state->rm1;
	int      y;

	while (nsamples--) {
		x = *src * (float)SAMPLE_MAX_16BIT;
		r = 2.0f * (float)fast_rand() / (float)INT_MAX - 1.0f;
		x += r - rm1;
		rm1 = r;
		y = f_round(x);

		if (y > SHRT_MAX) {
			*((short *)dst) = SHRT_MAX;
		} else if (y < SHRT_MIN) {
			*((short *)dst) = SHRT_MIN;
		} else {
			*((short *) dst) = (short)y;
		}

		dst += dst_skip;
		src++;
	}
	state->rm1 = rm1;
}

void sample_move_dither_shaped_d16_sSs (char *dst,  jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state)
	
{
	jack_default_audio_sample_t     x;
	jack_default_audio_sample_t     xe; /* the innput sample - filtered error */
	jack_default_audio_sample_t     xp; /* x' */
	float        r;
	float        rm1 = state->rm1;
	unsigned int idx = state->idx;
	int          y;

	while (nsamples--) {
		x = *src * (float)SAMPLE_MAX_16BIT;
		r = 2.0f * (float)fast_rand() / (float)INT_MAX - 1.0f;
		/* Filter the error with Lipshitz's minimally audible FIR:
		   [2.033 -2.165 1.959 -1.590 0.6149] */
		xe = x
		     - state->e[idx] * 2.033f
		     + state->e[(idx - 1) & DITHER_BUF_MASK] * 2.165f
		     - state->e[(idx - 2) & DITHER_BUF_MASK] * 1.959f
		     + state->e[(idx - 3) & DITHER_BUF_MASK] * 1.590f
		     - state->e[(idx - 4) & DITHER_BUF_MASK] * 0.6149f;
		xp = xe + r - rm1;
		rm1 = r;

		/* This could be some inline asm on x86 */
		y = f_round(xp);

		/* Intrinsic z^-1 delay */
		idx = (idx + 1) & DITHER_BUF_MASK;
		state->e[idx] = y - xe;

		if (y > SHRT_MAX) {
			y = SHRT_MAX;
		} else if (y < SHRT_MIN) {
			y = SHRT_MIN;
		}
#if __BYTE_ORDER == __LITTLE_ENDIAN
		dst[0]=(char)(y>>8);
		dst[1]=(char)(y);
#elif __BYTE_ORDER == __BIG_ENDIAN
		dst[0]=(char)(y);
		dst[1]=(char)(y>>8);
#endif
		dst += dst_skip;
		src++;
	}
	state->rm1 = rm1;
	state->idx = idx;
}

void sample_move_dither_shaped_d16_sS (char *dst,  jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state)
	
{
	jack_default_audio_sample_t     x;
	jack_default_audio_sample_t     xe; /* the innput sample - filtered error */
	jack_default_audio_sample_t     xp; /* x' */
	float        r;
	float        rm1 = state->rm1;
	unsigned int idx = state->idx;
	int          y;

	while (nsamples--) {
		x = *src * (float)SAMPLE_MAX_16BIT;
		r = 2.0f * (float)fast_rand() / (float)INT_MAX - 1.0f;
		/* Filter the error with Lipshitz's minimally audible FIR:
		   [2.033 -2.165 1.959 -1.590 0.6149] */
		xe = x
		     - state->e[idx] * 2.033f
		     + state->e[(idx - 1) & DITHER_BUF_MASK] * 2.165f
		     - state->e[(idx - 2) & DITHER_BUF_MASK] * 1.959f
		     + state->e[(idx - 3) & DITHER_BUF_MASK] * 1.590f
		     - state->e[(idx - 4) & DITHER_BUF_MASK] * 0.6149f;
		xp = xe + r - rm1;
		rm1 = r;

		/* This could be some inline asm on x86 */
		y = f_round(xp);

		/* Intrinsic z^-1 delay */
		idx = (idx + 1) & DITHER_BUF_MASK;
		state->e[idx] = y - xe;

		if (y > SHRT_MAX) {
			*((short *)dst) = SHRT_MAX;
		} else if (y < SHRT_MIN) {
			*((short *)dst) = SHRT_MIN;
		} else {
			*((short *) dst) = (short)y;
		}
		dst += dst_skip;
		src++;
	}
	state->rm1 = rm1;
	state->idx = idx;
}

void sample_move_dS_s16s (jack_default_audio_sample_t *dst, char *src, unsigned long nsamples, unsigned long src_skip) 
	
{
	short z;

	/* ALERT: signed sign-extension portability !!! */
	while (nsamples--) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
		z = (unsigned char)(src[0]);
		z <<= 8;
		z |= (unsigned char)(src[1]);
#elif __BYTE_ORDER == __BIG_ENDIAN
		z = (unsigned char)(src[1]);
		z <<= 8;
		z |= (unsigned char)(src[0]);
#endif
		*dst = z / SAMPLE_MAX_16BIT;
		dst++;
		src += src_skip;
	}
}	

void sample_move_dS_s16 (jack_default_audio_sample_t *dst, char *src, unsigned long nsamples, unsigned long src_skip) 
	
{
	/* ALERT: signed sign-extension portability !!! */
	while (nsamples--) {
		*dst = (*((short *) src)) / SAMPLE_MAX_16BIT;
		dst++;
		src += src_skip;
	}
}	

void sample_merge_d16_sS (char *dst,  jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state)
{
	short val;

	/* ALERT: signed sign-extension portability !!! */
	
	while (nsamples--) {
		val = (short) (*src * SAMPLE_MAX_16BIT);
		
		if (val > SHRT_MAX - *((short *) dst)) {
			*((short *)dst) = SHRT_MAX;
		} else if (val < SHRT_MIN - *((short *) dst)) {
			*((short *)dst) = SHRT_MIN;
		} else {
			*((short *) dst) += val;
		}
		dst += dst_skip;
		src++;
	}
}	

void sample_merge_d32u24_sS (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state)

{
	/* ALERT: signed sign-extension portability !!! */

	while (nsamples--) {
		*((int *) dst) += (((int) (*src * SAMPLE_MAX_24BIT)) << 8);
		dst += dst_skip;
		src++;
	}
}	

void memset_interleave (char *dst, char val, unsigned long bytes, 
			unsigned long unit_bytes, 
			unsigned long skip_bytes) 
{
	switch (unit_bytes) {
	case 1:
		while (bytes--) {
			*dst = val;
			dst += skip_bytes;
		}
		break;
	case 2:
		while (bytes) {
			*((short *) dst) = (short) val;
			dst += skip_bytes;
			bytes -= 2;
		}
		break;
	case 4:		    
		while (bytes) {
			*((int *) dst) = (int) val;
			dst += skip_bytes;
			bytes -= 4;
		}
		break;
	default:
		while (bytes) {
			memset(dst, val, unit_bytes);
			dst += skip_bytes;
			bytes -= unit_bytes;
		}
		break;
	}
}

/* COPY FUNCTIONS: used to move data from an input channel to an
   output channel. Note that we assume that the skip distance
   is the same for both channels. This is completely fine
   unless the input and output were on different audio interfaces that
   were interleaved differently. We don't try to handle that.
*/

void 
memcpy_fake (char *dst, char *src, unsigned long src_bytes, unsigned long foo, unsigned long bar)
{
	memcpy (dst, src, src_bytes);
}

void 
merge_memcpy_d16_s16 (char *dst, char *src, unsigned long src_bytes,
		      unsigned long dst_skip_bytes, unsigned long src_skip_bytes)
{
	while (src_bytes) {
		*((short *) dst) += *((short *) src);
		dst += 2;
		src += 2;
		src_bytes -= 2;
	}
}

void 
merge_memcpy_d32_s32 (char *dst, char *src, unsigned long src_bytes,
		      unsigned long dst_skip_bytes, unsigned long src_skip_bytes)

{
	while (src_bytes) {
		*((int *) dst) += *((int *) src);
		dst += 4;
		src += 4;
		src_bytes -= 4;
	}
}

void 
merge_memcpy_interleave_d16_s16 (char *dst, char *src, unsigned long src_bytes, 
				 unsigned long dst_skip_bytes, unsigned long src_skip_bytes)

{
	while (src_bytes) {
		*((short *) dst) += *((short *) src);
		dst += dst_skip_bytes;
		src += src_skip_bytes;
		src_bytes -= 2;
	}
}

void 
merge_memcpy_interleave_d32_s32 (char *dst, char *src, unsigned long src_bytes,
				 unsigned long dst_skip_bytes, unsigned long src_skip_bytes)
{
	while (src_bytes) {
		*((int *) dst) += *((int *) src);
		dst += dst_skip_bytes;
		src += src_skip_bytes;
		src_bytes -= 4;
	}
}

void 
merge_memcpy_interleave_d24_s24 (char *dst, char *src, unsigned long src_bytes,
				 unsigned long dst_skip_bytes, unsigned long src_skip_bytes)
{
	while (src_bytes) {
		int acc = (*(int *)dst & 0xFFFFFF) + (*(int *)src & 0xFFFFFF);
		memcpy(dst, &acc, 3);
		dst += dst_skip_bytes;
		src += src_skip_bytes;
		src_bytes -= 3;
	}
}

void 
memcpy_interleave_d16_s16 (char *dst, char *src, unsigned long src_bytes,
			   unsigned long dst_skip_bytes, unsigned long src_skip_bytes)
{
	while (src_bytes) {
		*((short *) dst) = *((short *) src);
		dst += dst_skip_bytes;
		src += src_skip_bytes;
		src_bytes -= 2;
	}
}

void 
memcpy_interleave_d24_s24 (char *dst, char *src, unsigned long src_bytes,
			   unsigned long dst_skip_bytes, unsigned long src_skip_bytes)

{
	while (src_bytes) {
		memcpy(dst, src, 3);
		dst += dst_skip_bytes;
		src += src_skip_bytes;
		src_bytes -= 3;
	}
}

void 
memcpy_interleave_d32_s32 (char *dst, char *src, unsigned long src_bytes,
			   unsigned long dst_skip_bytes, unsigned long src_skip_bytes)

{
	while (src_bytes) {
		*((int *) dst) = *((int *) src);
		dst += dst_skip_bytes;
		src += src_skip_bytes;
		src_bytes -= 4;
	}
}
