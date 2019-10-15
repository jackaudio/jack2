/* -*- mode: c; c-file-style: "linux"; -*- */
/*
    Copyright (C) 2001 Paul Davis

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

*/


#define __STDC_FORMAT_MACROS   // For inttypes.h to work in C++
#define _GNU_SOURCE            /* for strcasestr() from string.h */

#include <math.h>
#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>

#include "alsa_driver.h"
#include "hammerfall.h"
#include "hdsp.h"
#include "ice1712.h"
#include "usx2y.h"
#include "generic.h"
#include "memops.h"
#include "JackError.h"

#include "alsa_midi_impl.h"

extern void store_work_time (int);
extern void store_wait_time (int);
extern void show_wait_times ();
extern void show_work_times ();

#undef DEBUG_WAKEUP

char* strcasestr(const char* haystack, const char* needle);

/* Delay (in process calls) before jackd will report an xrun */
#define XRUN_REPORT_DELAY 0
/* Max re-try count for Alsa poll timeout handling */
#define MAX_RETRY_COUNT 5

void
jack_driver_init (jack_driver_t *driver)
{
    memset (driver, 0, sizeof (*driver));

    driver->attach = 0;
    driver->detach = 0;
    driver->write = 0;
    driver->read = 0;
    driver->null_cycle = 0;
    driver->bufsize = 0;
    driver->start = 0;
    driver->stop = 0;
}

void
jack_driver_nt_init (jack_driver_nt_t * driver)
{
    memset (driver, 0, sizeof (*driver));

    jack_driver_init ((jack_driver_t *) driver);

    driver->attach = 0;
    driver->detach = 0;
    driver->bufsize = 0;
    driver->stop = 0;
    driver->start = 0;

    driver->nt_bufsize = 0;
    driver->nt_start = 0;
    driver->nt_stop = 0;
    driver->nt_attach = 0;
    driver->nt_detach = 0;
    driver->nt_run_cycle = 0;
}

static int
alsa_driver_prepare (snd_pcm_t *handle, int is_capture)
{
	int res = 0;

#ifndef __QNXNTO__
	res = snd_pcm_prepare (handle);
#else
	res = snd_pcm_plugin_prepare(handle, is_capture);
#endif
	if (res < 0) {
		jack_error("error preparing: %s", snd_strerror(res));
	}

	return res;
}

static void
alsa_driver_release_channel_dependent_memory (alsa_driver_t *driver)
{
	bitset_destroy (&driver->channels_done);
	bitset_destroy (&driver->channels_not_done);

	if (driver->playback_addr) {
		free (driver->playback_addr);
		driver->playback_addr = 0;
	}

	if (driver->capture_addr) {
		free (driver->capture_addr);
		driver->capture_addr = 0;
	}

#ifdef __QNXNTO__
	if (driver->playback_areas_ptr) {
		free(driver->playback_areas_ptr);
		driver->playback_areas = NULL;
		driver->playback_areas_ptr = NULL;
	}

	if (driver->capture_areas_ptr) {
		free(driver->capture_areas_ptr);
		driver->capture_areas = NULL;
		driver->capture_areas_ptr = NULL;
	}
#endif

	if (driver->playback_interleave_skip) {
		free (driver->playback_interleave_skip);
		driver->playback_interleave_skip = NULL;
	}

	if (driver->capture_interleave_skip) {
		free (driver->capture_interleave_skip);
		driver->capture_interleave_skip = NULL;
	}

	if (driver->silent) {
		free (driver->silent);
		driver->silent = 0;
	}

	if (driver->dither_state) {
		free (driver->dither_state);
		driver->dither_state = 0;
	}
}

#ifndef __QNXNTO__

static int
alsa_driver_check_capabilities (alsa_driver_t *driver)
{
	return 0;
}

char* get_control_device_name(const char * device_name);

static int
alsa_driver_check_card_type (alsa_driver_t *driver)
{
	int err;
	snd_ctl_card_info_t *card_info;
	char * ctl_name;

	snd_ctl_card_info_alloca (&card_info);

	ctl_name = get_control_device_name(driver->alsa_name_playback);

	// XXX: I don't know the "right" way to do this. Which to use
	// driver->alsa_name_playback or driver->alsa_name_capture.
	if ((err = snd_ctl_open (&driver->ctl_handle, ctl_name, 0)) < 0) {
		jack_error ("control open \"%s\" (%s)", ctl_name,
			    snd_strerror(err));
	} else if ((err = snd_ctl_card_info(driver->ctl_handle, card_info)) < 0) {
		jack_error ("control hardware info \"%s\" (%s)",
			    driver->alsa_name_playback, snd_strerror (err));
		snd_ctl_close (driver->ctl_handle);
	}

	driver->alsa_driver = strdup(snd_ctl_card_info_get_driver (card_info));

	free(ctl_name);

	return alsa_driver_check_capabilities (driver);
}

static int
alsa_driver_hammerfall_hardware (alsa_driver_t *driver)
{
	driver->hw = jack_alsa_hammerfall_hw_new (driver);
	return 0;
}

static int
alsa_driver_hdsp_hardware (alsa_driver_t *driver)
{
	driver->hw = jack_alsa_hdsp_hw_new (driver);
	return 0;
}

static int
alsa_driver_ice1712_hardware (alsa_driver_t *driver)
{
        driver->hw = jack_alsa_ice1712_hw_new (driver);
        return 0;
}

// JACK2
/*
static int
alsa_driver_usx2y_hardware (alsa_driver_t *driver)
{
    driver->hw = jack_alsa_usx2y_hw_new (driver);
    return 0;
}
*/

static int
alsa_driver_generic_hardware (alsa_driver_t *driver)
{
	driver->hw = jack_alsa_generic_hw_new (driver);
	return 0;
}

static int
alsa_driver_hw_specific (alsa_driver_t *driver, int hw_monitoring,
			 int hw_metering)
{
	int err;

	if (!strcmp(driver->alsa_driver, "RME9652")) {
		if ((err = alsa_driver_hammerfall_hardware (driver)) != 0) {
			return err;
		}
	} else if (!strcmp(driver->alsa_driver, "H-DSP")) {
                if ((err = alsa_driver_hdsp_hardware (driver)) !=0) {
                        return err;
                }
	} else if (!strcmp(driver->alsa_driver, "ICE1712")) {
                if ((err = alsa_driver_ice1712_hardware (driver)) !=0) {
                        return err;
                }
	}
    // JACK2
    /*
        else if (!strcmp(driver->alsa_driver, "USB US-X2Y")) {
		if ((err = alsa_driver_usx2y_hardware (driver)) !=0) {
				return err;
		}
	}
    */
       else {
	        if ((err = alsa_driver_generic_hardware (driver)) != 0) {
			return err;
		}
	}

	if (driver->hw->capabilities & Cap_HardwareMonitoring) {
		driver->has_hw_monitoring = TRUE;
		/* XXX need to ensure that this is really FALSE or
		 * TRUE or whatever*/
		driver->hw_monitoring = hw_monitoring;
	} else {
		driver->has_hw_monitoring = FALSE;
		driver->hw_monitoring = FALSE;
	}

	if (driver->hw->capabilities & Cap_ClockLockReporting) {
		driver->has_clock_sync_reporting = TRUE;
	} else {
		driver->has_clock_sync_reporting = FALSE;
	}

	if (driver->hw->capabilities & Cap_HardwareMetering) {
		driver->has_hw_metering = TRUE;
		driver->hw_metering = hw_metering;
	} else {
		driver->has_hw_metering = FALSE;
		driver->hw_metering = FALSE;
	}

	return 0;
}
#endif

static void
alsa_driver_setup_io_function_pointers (alsa_driver_t *driver)
{
	if (driver->playback_handle) {
		if (SND_PCM_FORMAT_FLOAT_LE == driver->playback_sample_format) {
			driver->write_via_copy = sample_move_dS_floatLE;
		} else {
			switch (driver->playback_sample_bytes) {
			case 2:
				switch (driver->dither) {
				case Rectangular:
					jack_info("Rectangular dithering at 16 bits");
					driver->write_via_copy = driver->quirk_bswap?
						sample_move_dither_rect_d16_sSs:
						sample_move_dither_rect_d16_sS;
					break;

				case Triangular:
					jack_info("Triangular dithering at 16 bits");
					driver->write_via_copy = driver->quirk_bswap?
						sample_move_dither_tri_d16_sSs:
						sample_move_dither_tri_d16_sS;
					break;

				case Shaped:
					jack_info("Noise-shaped dithering at 16 bits");
					driver->write_via_copy = driver->quirk_bswap?
						sample_move_dither_shaped_d16_sSs:
						sample_move_dither_shaped_d16_sS;
					break;

				default:
					driver->write_via_copy = driver->quirk_bswap?
						sample_move_d16_sSs :
						sample_move_d16_sS;
					break;
				}
				break;

			case 3: /* NO DITHER */
				driver->write_via_copy = driver->quirk_bswap?
					sample_move_d24_sSs:
					sample_move_d24_sS;

				break;

			case 4: /* NO DITHER */
				driver->write_via_copy = driver->quirk_bswap?
					sample_move_d32u24_sSs:
					sample_move_d32u24_sS;
				break;

			default:
				jack_error ("impossible sample width (%d) discovered!",
						driver->playback_sample_bytes);
				exit (1);
			}
		}
	}

	if (driver->capture_handle) {
		if (SND_PCM_FORMAT_FLOAT_LE == driver->capture_sample_format) {
			driver->read_via_copy = sample_move_floatLE_sSs;
		} else {
			switch (driver->capture_sample_bytes) {
			case 2:
				driver->read_via_copy = driver->quirk_bswap?
					sample_move_dS_s16s:
					sample_move_dS_s16;
				break;
			case 3:
				driver->read_via_copy = driver->quirk_bswap?
					sample_move_dS_s24s:
					sample_move_dS_s24;
				break;
			case 4:
				driver->read_via_copy = driver->quirk_bswap?
					sample_move_dS_s32u24s:
					sample_move_dS_s32u24;
				break;
			}
		}
	}
}

#ifdef __QNXNTO__

static int
alsa_driver_allocate_buffer(alsa_driver_t *driver, int frames, int channels, bool is_capture)
{
	const long ALIGNMENT = 32;

	// TODO driver->playback_sample_bytes
	char* const fBuffer = malloc(channels * ((sizeof(alsa_driver_default_format_t)) * frames) + ALIGNMENT);
	if(fBuffer) {
		/* Provide an 32 byte aligned buffer */
		char* const aligned_buffer = (char*)((uintptr_t)fBuffer & ~(ALIGNMENT-1)) + ALIGNMENT;

		if(is_capture) {
			driver->capture_areas_ptr = fBuffer;
			driver->capture_areas = aligned_buffer;
		} else {
			driver->playback_areas_ptr = fBuffer;
			driver->playback_areas = aligned_buffer;
		}

		return 0;
	}

	jack_error ("ALSA: could not allocate audio buffer");
	return -1;
}

static int
alsa_driver_get_setup (alsa_driver_t *driver, snd_pcm_channel_setup_t *setup, bool is_capture)
{
	int err = 0;

	memset(setup, 0, sizeof(*setup));
	setup->channel = is_capture;

	if(is_capture) {
		err = snd_pcm_plugin_setup(driver->capture_handle, setup);
	} else {
		err = snd_pcm_plugin_setup(driver->playback_handle, setup);
	}
	if (err < 0) {
		jack_error("couldn't get channel setup for %s, err = %s ",
			   is_capture ? driver->alsa_name_capture : driver->alsa_name_playback,
			   strerror(err));
		return -1;
	}

	return 0;
}

static int
alsa_driver_configure_stream (alsa_driver_t *driver, char *device_name,
			      const char *stream_name,
			      snd_pcm_t *handle,
			      unsigned int *nperiodsp,
			      channel_t *nchns,
			      unsigned long sample_width,
			      bool is_capture)
{
	int err = 0;
	snd_pcm_channel_info_t ch_info;
	snd_pcm_channel_params_t ch_params;
	const unsigned long sample_size = is_capture ? driver->capture_sample_bytes
	                                             : driver->playback_sample_bytes;

	memset(&ch_info, 0, sizeof(ch_info));
	/*A pointer to a snd_pcm_channel_info_t structure that snd_pcm_plugin_info() fills in with information about the PCM channel.
	 * Before calling snd_pcm_plugin_info(), set the info structure's channel member to specify the direction.
	 * This function sets all the other members.*/
	ch_info.channel = is_capture;
	if ((err = snd_pcm_plugin_info(handle, &ch_info)) < 0) {
		jack_error("couldn't get channel info for %s, %s, err = (%s)", stream_name, device_name,  snd_strerror(err));
		alsa_driver_delete(driver);
		return -1;
	}

	if (!*nchns) {
		*nchns = ch_info.max_voices;
	}

	ch_params.mode = SND_PCM_MODE_BLOCK;
	ch_params.start_mode = SND_PCM_START_GO;
	ch_params.stop_mode = SND_PCM_STOP_STOP;
	ch_params.buf.block.frag_size = driver->frames_per_cycle * *nchns * sample_size;

	*nperiodsp = driver->user_nperiods;
	ch_params.buf.block.frags_min = 1;
	/* the maximal available periods (-1 due to one period is always processed
	 * by DMA and therefore not free)
	 */
	ch_params.buf.block.frags_max = *nperiodsp - 1;
	ch_params.format.interleave = 1;
	ch_params.format.rate = driver->frame_rate;
	ch_params.format.voices = *nchns;
	ch_params.channel = is_capture;

	ch_params.format.format = (sample_width == 4) ? SND_PCM_SFMT_S32_LE : SND_PCM_SFMT_S16_LE;
	/*Set the configurable parameters for a PCM channel*/
	if ((err = snd_pcm_plugin_params(handle, &ch_params)) < 0) {
		jack_error("snd_pcm_plugin_params failed for %s %s with err = (%s)", snd_strerror(err), stream_name, device_name);
		alsa_driver_delete(driver);
		return -1;
	}

	/*
	 * The buffer has to be able to hold a full HW audio buffer
	 * (periods * period_size) because the silence prefill will fill the
	 * complete buffer
	 */
	return alsa_driver_allocate_buffer(driver, driver->frames_per_cycle * *nperiodsp, *nchns, is_capture);
}
#else
static int
alsa_driver_configure_stream (alsa_driver_t *driver, char *device_name,
			      const char *stream_name,
			      snd_pcm_t *handle,
			      snd_pcm_hw_params_t *hw_params,
			      snd_pcm_sw_params_t *sw_params,
			      unsigned int *nperiodsp,
			      channel_t *nchns,
			      unsigned long sample_width)
{
	int err, format;
	unsigned int frame_rate;
	snd_pcm_uframes_t stop_th;
	static struct {
		char Name[40];
		snd_pcm_format_t format;
		int swapped;
	} formats[] = {
		{"32bit float little-endian", SND_PCM_FORMAT_FLOAT_LE, IS_LE},
		{"32bit integer little-endian", SND_PCM_FORMAT_S32_LE, IS_LE},
		{"32bit integer big-endian", SND_PCM_FORMAT_S32_BE, IS_BE},
		{"24bit little-endian in 3bytes format", SND_PCM_FORMAT_S24_3LE, IS_LE},
		{"24bit big-endian in 3bytes format", SND_PCM_FORMAT_S24_3BE, IS_BE},
		{"24bit little-endian", SND_PCM_FORMAT_S24_LE, IS_LE},
		{"24bit big-endian", SND_PCM_FORMAT_S24_BE, IS_BE},
		{"16bit little-endian", SND_PCM_FORMAT_S16_LE, IS_LE},
		{"16bit big-endian", SND_PCM_FORMAT_S16_BE, IS_BE},
	};
#define NUMFORMATS (sizeof(formats)/sizeof(formats[0]))
#define FIRST_16BIT_FORMAT 5

	if ((err = snd_pcm_hw_params_any (handle, hw_params)) < 0)  {
		jack_error ("ALSA: no playback configurations available (%s)",
			    snd_strerror (err));
		return -1;
	}

	if ((err = snd_pcm_hw_params_set_periods_integer (handle, hw_params))
	    < 0) {
		jack_error ("ALSA: cannot restrict period size to integral"
			    " value.");
		return -1;
	}

	if ((err = snd_pcm_hw_params_set_access (handle, hw_params, SND_PCM_ACCESS_MMAP_NONINTERLEAVED)) < 0) {
		if ((err = snd_pcm_hw_params_set_access (handle, hw_params, SND_PCM_ACCESS_MMAP_INTERLEAVED)) < 0) {
			if ((err = snd_pcm_hw_params_set_access (
				     handle, hw_params,
				     SND_PCM_ACCESS_MMAP_COMPLEX)) < 0) {
				jack_error ("ALSA: mmap-based access is not possible"
					    " for the %s "
					    "stream of this audio interface",
					    stream_name);
				return -1;
			}
		}
	}

	format = (sample_width == 4) ? 0 : NUMFORMATS - 1;

	while (1) {
		if ((err = snd_pcm_hw_params_set_format (
			     handle, hw_params, formats[format].format)) < 0) {

			if ((sample_width == 4
			     ? format++ >= NUMFORMATS - 1
			     : format-- <= 0)) {
				jack_error ("Sorry. The audio interface \"%s\""
					    " doesn't support any of the"
					    " hardware sample formats that"
					    " JACK's alsa-driver can use.",
					    device_name);
				return -1;
			}
		} else {
			if (formats[format].swapped) {
				driver->quirk_bswap = 1;
			} else {
				driver->quirk_bswap = 0;
			}
			jack_info ("ALSA: final selected sample format for %s: %s", stream_name, formats[format].Name);
			break;
		}
	}

	frame_rate = driver->frame_rate ;
	err = snd_pcm_hw_params_set_rate_near (handle, hw_params,
					       &frame_rate, NULL) ;
	driver->frame_rate = frame_rate ;
	if (err < 0) {
		jack_error ("ALSA: cannot set sample/frame rate to %"
			    PRIu32 " for %s", driver->frame_rate,
			    stream_name);
		return -1;
	}
	if (!*nchns) {
		/*if not user-specified, try to find the maximum
		 * number of channels */
		unsigned int channels_max ;
		err = snd_pcm_hw_params_get_channels_max (hw_params,
							  &channels_max);
		*nchns = channels_max ;

		if (*nchns > 1024) {

			/* the hapless user is an unwitting victim of
			   the "default" ALSA PCM device, which can
			   support up to 16 million channels. since
			   they can't be bothered to set up a proper
			   default device, limit the number of
			   channels for them to a sane default.
			*/

			jack_error (
"You appear to be using the ALSA software \"plug\" layer, probably\n"
"a result of using the \"default\" ALSA device. This is less\n"
"efficient than it could be. Consider using a hardware device\n"
"instead rather than using the plug layer. Usually the name of the\n"
"hardware device that corresponds to the first sound card is hw:0\n"
				);
			*nchns = 2;
		}
	}

	if ((err = snd_pcm_hw_params_set_channels (handle, hw_params,
						   *nchns)) < 0) {
		jack_error ("ALSA: cannot set channel count to %u for %s",
			    *nchns, stream_name);
		return -1;
	}

	if ((err = snd_pcm_hw_params_set_period_size (handle, hw_params,
						      driver->frames_per_cycle,
						      0))
	    < 0) {
		jack_error ("ALSA: cannot set period size to %" PRIu32
			    " frames for %s", driver->frames_per_cycle,
			    stream_name);
		return -1;
	}

	*nperiodsp = driver->user_nperiods;
	snd_pcm_hw_params_set_periods_min (handle, hw_params, nperiodsp, NULL);
	if (*nperiodsp < driver->user_nperiods)
		*nperiodsp = driver->user_nperiods;
	if (snd_pcm_hw_params_set_periods_near (handle, hw_params,
						nperiodsp, NULL) < 0) {
		jack_error ("ALSA: cannot set number of periods to %u for %s",
			    *nperiodsp, stream_name);
		return -1;
	}

	if (*nperiodsp < driver->user_nperiods) {
		jack_error ("ALSA: got smaller periods %u than %u for %s",
			    *nperiodsp, (unsigned int) driver->user_nperiods,
			    stream_name);
		return -1;
	}
	jack_info ("ALSA: use %d periods for %s", *nperiodsp, stream_name);
#if 0
	if (!jack_power_of_two(driver->frames_per_cycle)) {
		jack_error("JACK: frames must be a power of two "
			   "(64, 512, 1024, ...)\n");
		return -1;
	}
#endif

	if ((err = snd_pcm_hw_params_set_buffer_size (handle, hw_params,
						      *nperiodsp *
						      driver->frames_per_cycle))
	    < 0) {
		jack_error ("ALSA: cannot set buffer length to %" PRIu32
			    " for %s",
			    *nperiodsp * driver->frames_per_cycle,
			    stream_name);
		return -1;
	}

	if ((err = snd_pcm_hw_params (handle, hw_params)) < 0) {
		jack_error ("ALSA: cannot set hardware parameters for %s",
			    stream_name);
		return -1;
	}

	snd_pcm_sw_params_current (handle, sw_params);

	if ((err = snd_pcm_sw_params_set_start_threshold (handle, sw_params,
							  0U)) < 0) {
		jack_error ("ALSA: cannot set start mode for %s", stream_name);
		return -1;
	}

	stop_th = *nperiodsp * driver->frames_per_cycle;
	if (driver->soft_mode) {
		stop_th = (snd_pcm_uframes_t)-1;
	}

	if ((err = snd_pcm_sw_params_set_stop_threshold (
		     handle, sw_params, stop_th)) < 0) {
		jack_error ("ALSA: cannot set stop mode for %s",
			    stream_name);
		return -1;
	}

	if ((err = snd_pcm_sw_params_set_silence_threshold (
		     handle, sw_params, 0)) < 0) {
		jack_error ("ALSA: cannot set silence threshold for %s",
			    stream_name);
		return -1;
	}

#if 0
	jack_info ("set silence size to %lu * %lu = %lu",
		 driver->frames_per_cycle, *nperiodsp,
		 driver->frames_per_cycle * *nperiodsp);

	if ((err = snd_pcm_sw_params_set_silence_size (
		     handle, sw_params,
		     driver->frames_per_cycle * *nperiodsp)) < 0) {
		jack_error ("ALSA: cannot set silence size for %s",
			    stream_name);
		return -1;
	}
#endif

	if (handle == driver->playback_handle)
		err = snd_pcm_sw_params_set_avail_min (
			handle, sw_params,
			driver->frames_per_cycle
			* (*nperiodsp - driver->user_nperiods + 1));
	else
		err = snd_pcm_sw_params_set_avail_min (
			handle, sw_params, driver->frames_per_cycle);

	if (err < 0) {
		jack_error ("ALSA: cannot set avail min for %s", stream_name);
		return -1;
	}

	err = snd_pcm_sw_params_set_tstamp_mode(handle, sw_params, SND_PCM_TSTAMP_ENABLE);
	if (err < 0) {
		jack_info("Could not enable ALSA time stamp mode for %s (err %d)",
			  stream_name, err);
	}

#if SND_LIB_MAJOR >= 1 && SND_LIB_MINOR >= 1
	err = snd_pcm_sw_params_set_tstamp_type(handle, sw_params, SND_PCM_TSTAMP_TYPE_MONOTONIC);
	if (err < 0) {
		jack_info("Could not use monotonic ALSA time stamps for %s (err %d)",
			  stream_name, err);
	}
#endif

	if ((err = snd_pcm_sw_params (handle, sw_params)) < 0) {
		jack_error ("ALSA: cannot set software parameters for %s\n",
			    stream_name);
		return -1;
	}

	return 0;
}
#endif

#ifdef __QNXNTO__
static int
alsa_driver_check_format (unsigned int format)
{
#else
static int
alsa_driver_check_format (snd_pcm_format_t format)
{
#endif

	switch (format) {
#ifndef __QNXNTO__
	case SND_PCM_FORMAT_FLOAT_LE:
	case SND_PCM_FORMAT_S24_3LE:
	case SND_PCM_FORMAT_S24_3BE:
	case SND_PCM_FORMAT_S24_LE:
	case SND_PCM_FORMAT_S24_BE:
	case SND_PCM_FORMAT_S32_BE:
	case SND_PCM_FORMAT_S16_BE:
#endif
	case SND_PCM_FORMAT_S16_LE:
	case SND_PCM_FORMAT_S32_LE:
		break;
	default:
		jack_error ("format not supported %d", format);
		return -1;
	}

	return 0;
}

static void
alsa_driver_set_sample_bytes (alsa_driver_t *driver)
{
#ifdef __QNXNTO__
	driver->playback_sample_bytes =
		snd_pcm_format_width (driver->playback_sample_format)
		/ 8;
	driver->capture_sample_bytes =
		snd_pcm_format_width (driver->capture_sample_format)
		/ 8;
#else
	driver->playback_sample_bytes =
		snd_pcm_format_physical_width (driver->playback_sample_format)
		/ 8;
	driver->capture_sample_bytes =
		snd_pcm_format_physical_width (driver->capture_sample_format)
		/ 8;
#endif
}

static int
alsa_driver_set_parameters (alsa_driver_t *driver,
			    jack_nframes_t frames_per_cycle,
			    jack_nframes_t user_nperiods,
			    jack_nframes_t rate)
{
#ifdef __QNXNTO__
	snd_pcm_channel_setup_t c_setup;
	snd_pcm_channel_setup_t p_setup;
	jack_nframes_t p_periods = 0;
	jack_nframes_t c_periods = 0;
#else
	int dir;
#endif
	snd_pcm_uframes_t p_period_size = 0;
	snd_pcm_uframes_t c_period_size = 0;
	channel_t chn;
	unsigned int pr = 0;
	unsigned int cr = 0;
	int err;

	driver->frame_rate = rate;
	driver->frames_per_cycle = frames_per_cycle;
	driver->user_nperiods = user_nperiods;

	jack_info ("configuring for %" PRIu32 "Hz, period = %"
		 PRIu32 " frames (%.1f ms), buffer = %" PRIu32 " periods",
		 rate, frames_per_cycle, (((float)frames_per_cycle / (float) rate) * 1000.0f), user_nperiods);

	if (driver->capture_handle) {
#ifdef __QNXNTO__
	    err = alsa_driver_configure_stream (
		    driver,
		    driver->alsa_name_capture,
		    "capture",
		    driver->capture_handle,
		    &driver->capture_nperiods,
		    &driver->capture_nchannels,
		    driver->capture_sample_bytes,
		    SND_PCM_CHANNEL_CAPTURE);
#else
	    err = alsa_driver_configure_stream (
		    driver,
		    driver->alsa_name_capture,
		    "capture",
		    driver->capture_handle,
		    driver->capture_hw_params,
		    driver->capture_sw_params,
		    &driver->capture_nperiods,
		    &driver->capture_nchannels,
		    driver->capture_sample_bytes);
#endif
		if (err) {
			jack_error ("ALSA: cannot configure capture channel");
			return -1;
		}
	}

	if (driver->playback_handle) {
#ifdef __QNXNTO__
	    err = alsa_driver_configure_stream (
		    driver,
		    driver->alsa_name_playback,
		    "playback",
		    driver->playback_handle,
		    &driver->playback_nperiods,
		    &driver->playback_nchannels,
		    driver->playback_sample_bytes,
		    SND_PCM_CHANNEL_PLAYBACK);
#else
	    err = alsa_driver_configure_stream (
		    driver,
		    driver->alsa_name_playback,
		    "playback",
		    driver->playback_handle,
		    driver->playback_hw_params,
		    driver->playback_sw_params,
		    &driver->playback_nperiods,
		    &driver->playback_nchannels,
		    driver->playback_sample_bytes);
#endif
		if (err) {
			jack_error ("ALSA: cannot configure playback channel");
			return -1;
		}
	}

#ifdef __QNXNTO__
	if (driver->capture_handle) {
		err = alsa_driver_get_setup(driver, &c_setup, SND_PCM_CHANNEL_CAPTURE);
		if(err < 0) {
			return -1;
		}
		cr = c_setup.format.rate;
		c_period_size = c_setup.buf.block.frag_size / driver->capture_nchannels
		        / driver->capture_sample_bytes;
		c_periods = c_setup.buf.block.frags;
		driver->capture_sample_format = c_setup.format.format;
		driver->capture_interleaved = c_setup.format.interleave;
	}
	if (driver->playback_handle) {
		err = alsa_driver_get_setup(driver, &p_setup, SND_PCM_CHANNEL_PLAYBACK);
		if(err < 0) {
			return -1;
		}
		pr = p_setup.format.rate;
		p_period_size = p_setup.buf.block.frag_size  / driver->playback_nchannels
		        / driver->playback_sample_bytes;
		p_periods = p_setup.buf.block.frags;
		driver->playback_sample_format = p_setup.format.format;
		driver->playback_interleaved = p_setup.format.interleave;
	}
#else

	/* check the rate, since that's rather important */

	if (driver->playback_handle) {
		snd_pcm_hw_params_get_rate (driver->playback_hw_params,
					    &pr, &dir);
	}

	if (driver->capture_handle) {
		snd_pcm_hw_params_get_rate (driver->capture_hw_params,
					    &cr, &dir);
	}
#endif

	if (driver->capture_handle && driver->playback_handle) {
		if (cr != pr) {
			jack_error ("playback and capture sample rates do "
				    "not match (%d vs. %d)", pr, cr);
		}

		/* only change if *both* capture and playback rates
		 * don't match requested certain hardware actually
		 * still works properly in full-duplex with slightly
		 * different rate values between adc and dac
		 */
		if (cr != driver->frame_rate && pr != driver->frame_rate) {
			jack_error ("sample rate in use (%d Hz) does not "
				    "match requested rate (%d Hz)",
				    cr, driver->frame_rate);
			driver->frame_rate = cr;
		}

	}
	else if (driver->capture_handle && cr != driver->frame_rate) {
		jack_error ("capture sample rate in use (%d Hz) does not "
			    "match requested rate (%d Hz)",
			    cr, driver->frame_rate);
		driver->frame_rate = cr;
	}
	else if (driver->playback_handle && pr != driver->frame_rate) {
		jack_error ("playback sample rate in use (%d Hz) does not "
			    "match requested rate (%d Hz)",
			    pr, driver->frame_rate);
		driver->frame_rate = pr;
	}


	/* check the fragment size, since that's non-negotiable */

	if (driver->playback_handle) {
#ifndef __QNXNTO__
 		snd_pcm_access_t access;

 		err = snd_pcm_hw_params_get_period_size (
 			driver->playback_hw_params, &p_period_size, &dir);
 		err = snd_pcm_hw_params_get_format (
 			driver->playback_hw_params,
			&(driver->playback_sample_format));
 		err = snd_pcm_hw_params_get_access (driver->playback_hw_params,
						    &access);
 		driver->playback_interleaved =
			(access == SND_PCM_ACCESS_MMAP_INTERLEAVED)
			|| (access == SND_PCM_ACCESS_MMAP_COMPLEX);
#endif
		if (p_period_size != driver->frames_per_cycle) {
			jack_error ("alsa_pcm: requested an interrupt every %"
				    PRIu32
				    " frames but got %u frames for playback",
				    driver->frames_per_cycle, p_period_size);
			return -1;
		}
#ifdef __QNXNTO__
		if (p_periods != driver->user_nperiods) {
			jack_error ("alsa_pcm: requested %"
				    PRIu32
				    " periods but got %"
				    PRIu32
				    " periods for playback",
				    driver->user_nperiods, p_periods);
			return -1;
		}
#endif
	}

	if (driver->capture_handle) {
#ifndef __QNXNTO__
 		snd_pcm_access_t access;

 		err = snd_pcm_hw_params_get_period_size (
 			driver->capture_hw_params, &c_period_size, &dir);
 		err = snd_pcm_hw_params_get_format (
 			driver->capture_hw_params,
			&(driver->capture_sample_format));
 		err = snd_pcm_hw_params_get_access (driver->capture_hw_params,
						    &access);
 		driver->capture_interleaved =
			(access == SND_PCM_ACCESS_MMAP_INTERLEAVED)
			|| (access == SND_PCM_ACCESS_MMAP_COMPLEX);
#endif
		if (c_period_size != driver->frames_per_cycle) {
			jack_error ("alsa_pcm: requested an interrupt every %"
				    PRIu32
				    " frames but got %u frames for capture",
				    driver->frames_per_cycle, c_period_size);
			return -1;
		}
#ifdef __QNXNTO__
		/* capture buffers can be configured bigger but it should fail
		 * if they are smaller as expected
		 */
		if (c_periods < driver->user_nperiods) {
			jack_error ("alsa_pcm: requested %"
				    PRIu32
				    " periods but got %"
				    PRIu32
				    " periods for capture",
				    driver->user_nperiods, c_periods);
			return -1;
		}
#endif
	}

	alsa_driver_set_sample_bytes(driver);

	if (driver->playback_handle) {
		err = alsa_driver_check_format(driver->playback_sample_format);
		if(err < 0) {
			jack_error ("programming error: unhandled format "
				    "type for playback");
			return -1;
		}
	}

	if (driver->capture_handle) {
		err = alsa_driver_check_format(driver->capture_sample_format);
		if(err < 0) {
			jack_error ("programming error: unhandled format "
				    "type for capture");
			return -1;
		}
	}

	if (driver->playback_interleaved) {
#ifndef __QNXNTO__
		const snd_pcm_channel_area_t *my_areas;
		snd_pcm_uframes_t offset, frames;
		if (snd_pcm_mmap_begin(driver->playback_handle,
				       &my_areas, &offset, &frames) < 0) {
			jack_error ("ALSA: %s: mmap areas info error",
				    driver->alsa_name_playback);
			return -1;
		}

		// TODO does not work for capture only
		driver->interleave_unit =
			snd_pcm_format_physical_width (
				driver->playback_sample_format) / 8;
#else
		driver->interleave_unit = snd_pcm_format_width(
		            driver->playback_sample_format) / 8;
#endif
	} else {

		driver->interleave_unit = 0;  /* NOT USED */
	}

#ifndef __QNXNTO__
	if (driver->capture_interleaved) {
		const snd_pcm_channel_area_t *my_areas;
		snd_pcm_uframes_t offset, frames;
		if (snd_pcm_mmap_begin(driver->capture_handle,
				       &my_areas, &offset, &frames) < 0) {
			jack_error ("ALSA: %s: mmap areas info error",
				    driver->alsa_name_capture);
			return -1;
		}
	}
#endif
	if (driver->playback_nchannels > driver->capture_nchannels) {
		driver->max_nchannels = driver->playback_nchannels;
		driver->user_nchannels = driver->capture_nchannels;
	} else {
		driver->max_nchannels = driver->capture_nchannels;
		driver->user_nchannels = driver->playback_nchannels;
	}

	alsa_driver_setup_io_function_pointers (driver);

	/* Allocate and initialize structures that rely on the
	   channels counts.

	   Set up the bit pattern that is used to record which
	   channels require action on every cycle. any bits that are
	   not set after the engine's process() call indicate channels
	   that potentially need to be silenced.
	*/

	bitset_create (&driver->channels_done, driver->max_nchannels);
	bitset_create (&driver->channels_not_done, driver->max_nchannels);

	if (driver->playback_handle) {
		driver->playback_addr = (char **)
			malloc (sizeof (char *) * driver->playback_nchannels);
		memset (driver->playback_addr, 0,
			sizeof (char *) * driver->playback_nchannels);
		driver->playback_interleave_skip = (unsigned long *)
			malloc (sizeof (unsigned long *) * driver->playback_nchannels);
		memset (driver->playback_interleave_skip, 0,
			sizeof (unsigned long *) * driver->playback_nchannels);
		driver->silent = (unsigned long *)
			malloc (sizeof (unsigned long)
				* driver->playback_nchannels);

		for (chn = 0; chn < driver->playback_nchannels; chn++) {
			driver->silent[chn] = 0;
		}

		for (chn = 0; chn < driver->playback_nchannels; chn++) {
			bitset_add (driver->channels_done, chn);
		}

		driver->dither_state = (dither_state_t *)
			calloc ( driver->playback_nchannels,
				 sizeof (dither_state_t));
	}

	if (driver->capture_handle) {
		driver->capture_addr = (char **)
			malloc (sizeof (char *) * driver->capture_nchannels);
		memset (driver->capture_addr, 0,
			sizeof (char *) * driver->capture_nchannels);
		driver->capture_interleave_skip = (unsigned long *)
			malloc (sizeof (unsigned long *) * driver->capture_nchannels);
		memset (driver->capture_interleave_skip, 0,
			sizeof (unsigned long *) * driver->capture_nchannels);
	}

	driver->clock_sync_data = (ClockSyncStatus *)
		malloc (sizeof (ClockSyncStatus) * driver->max_nchannels);

	driver->period_usecs =
		(jack_time_t) floor ((((float) driver->frames_per_cycle) /
				      driver->frame_rate) * 1000000.0f);
	driver->poll_timeout_ms = (int) floor (1.5f * (driver->period_usecs / 1000.0f));

// JACK2
/*
	if (driver->engine) {
		if (driver->engine->set_buffer_size (driver->engine,
						     driver->frames_per_cycle)) {
			jack_error ("ALSA: Cannot set engine buffer size to %d (check MIDI)", driver->frames_per_cycle);
			return -1;
		}
	}
*/

	return 0;

	// may be unused
	(void)err;
}

int
alsa_driver_reset_parameters (alsa_driver_t *driver,
			      jack_nframes_t frames_per_cycle,
			      jack_nframes_t user_nperiods,
			      jack_nframes_t rate)
{
	/* XXX unregister old ports ? */
	alsa_driver_release_channel_dependent_memory (driver);
	return alsa_driver_set_parameters (driver,
					   frames_per_cycle,
					   user_nperiods, rate);
}

#ifdef __QNXNTO__
static int
snd_pcm_poll_descriptors_count(snd_pcm_t *pcm)
{
	return 1;
}

static int
snd_pcm_poll_descriptors_revents(snd_pcm_t *pcm, struct pollfd *pfds,
                                 unsigned int nfds, unsigned short *revents)
{
	*revents = pfds->revents;

	return 0;
}
#endif

static int
alsa_driver_get_channel_addresses (alsa_driver_t *driver,
				   snd_pcm_uframes_t *capture_avail,
				   snd_pcm_uframes_t *playback_avail,
				   snd_pcm_uframes_t *capture_offset,
				   snd_pcm_uframes_t *playback_offset)
{
	channel_t chn;

	if (capture_avail) {
#ifndef __QNXNTO__
		int err;
		if ((err = snd_pcm_mmap_begin (
			     driver->capture_handle, &driver->capture_areas,
			     (snd_pcm_uframes_t *) capture_offset,
			     (snd_pcm_uframes_t *) capture_avail)) < 0) {
			jack_error ("ALSA: %s: mmap areas info error",
				    driver->alsa_name_capture);
			return -1;
		}

		for (chn = 0; chn < driver->capture_nchannels; chn++) {
			const snd_pcm_channel_area_t *a =
				&driver->capture_areas[chn];
			driver->capture_addr[chn] = (char *) a->addr
				+ ((a->first + a->step * *capture_offset) / 8);
			driver->capture_interleave_skip[chn] = (unsigned long ) (a->step / 8);
		}
#else
		for (chn = 0; chn < driver->capture_nchannels; chn++) {
			char* const a = driver->capture_areas;
			if (driver->capture_interleaved) {
				driver->capture_addr[chn] = &a[chn * driver->capture_sample_bytes];
				driver->capture_interleave_skip[chn] = driver->capture_nchannels *
				        driver->capture_sample_bytes;
			} else {
				driver->capture_addr[chn] = &a[chn *
				        driver->capture_sample_bytes * driver->frames_per_cycle];
				driver->capture_interleave_skip[chn] = driver->capture_sample_bytes;
			}
		}
#endif
	}

	if (playback_avail) {
#ifndef __QNXNTO__
		int err;
		if ((err = snd_pcm_mmap_begin (
			     driver->playback_handle, &driver->playback_areas,
			     (snd_pcm_uframes_t *) playback_offset,
			     (snd_pcm_uframes_t *) playback_avail)) < 0) {
			jack_error ("ALSA: %s: mmap areas info error ",
				    driver->alsa_name_playback);
			return -1;
		}

		for (chn = 0; chn < driver->playback_nchannels; chn++) {
			const snd_pcm_channel_area_t *a =
				&driver->playback_areas[chn];
			driver->playback_addr[chn] = (char *) a->addr
				+ ((a->first + a->step * *playback_offset) / 8);
			driver->playback_interleave_skip[chn] = (unsigned long ) (a->step / 8);
		}
#else
		for (chn = 0; chn < driver->playback_nchannels; chn++) {
			char* const a = driver->playback_areas;
			if (driver->playback_interleaved) {
				driver->playback_addr[chn] = &a[chn * driver->playback_sample_bytes];
				driver->playback_interleave_skip[chn] = driver->playback_nchannels *
				        driver->playback_sample_bytes;
			} else {
				driver->playback_addr[chn] = &a[chn *
				        driver->playback_sample_bytes * driver->frames_per_cycle];
				driver->playback_interleave_skip[chn] = driver->playback_sample_bytes;
			}
		}
#endif
	}

	return 0;
}

#ifdef __QNXNTO__
static int
alsa_driver_stream_start(snd_pcm_t *pcm, bool is_capture)
{
	return snd_pcm_channel_go(pcm, is_capture);
}
#else
static int
alsa_driver_stream_start(snd_pcm_t *pcm, bool is_capture)
{
	return snd_pcm_start(pcm);
}
#endif

int
alsa_driver_start (alsa_driver_t *driver)
{
	int err;
	snd_pcm_uframes_t poffset, pavail;
	channel_t chn;

	driver->poll_last = 0;
	driver->poll_next = 0;

	if (driver->playback_handle) {
		if ((err = alsa_driver_prepare (driver->playback_handle, SND_PCM_STREAM_PLAYBACK)) < 0) {
			jack_error ("ALSA: prepare error for playback on "
				    "\"%s\" (%s)", driver->alsa_name_playback,
				    snd_strerror(err));
			return -1;
		}
	}

	if ((driver->capture_handle && driver->capture_and_playback_not_synced)
	    || !driver->playback_handle) {
		if ((err = alsa_driver_prepare (driver->capture_handle, SND_PCM_STREAM_CAPTURE)) < 0) {
			jack_error ("ALSA: prepare error for capture on \"%s\""
				    " (%s)", driver->alsa_name_capture,
				    snd_strerror(err));
			return -1;
		}
	}

	if (driver->hw_monitoring) {
		if (driver->input_monitor_mask || driver->all_monitor_in) {
			if (driver->all_monitor_in) {
				driver->hw->set_input_monitor_mask (driver->hw, ~0U);
			} else {
				driver->hw->set_input_monitor_mask (
					driver->hw, driver->input_monitor_mask);
			}
		} else {
			driver->hw->set_input_monitor_mask (driver->hw,
							    driver->input_monitor_mask);
		}
	}

	if (driver->playback_handle) {
		driver->playback_nfds =
			snd_pcm_poll_descriptors_count (driver->playback_handle);
	} else {
		driver->playback_nfds = 0;
	}

	if (driver->capture_handle) {
		driver->capture_nfds =
			snd_pcm_poll_descriptors_count (driver->capture_handle);
	} else {
		driver->capture_nfds = 0;
	}

	if (driver->pfd) {
		free (driver->pfd);
	}

	driver->pfd = (struct pollfd *)
		malloc (sizeof (struct pollfd) *
			(driver->playback_nfds + driver->capture_nfds + 2));

	if (driver->midi && !driver->xrun_recovery)
		(driver->midi->start)(driver->midi);

	if (driver->playback_handle) {
		const jack_nframes_t silence_frames = driver->frames_per_cycle *
		        driver->playback_nperiods;
		/* fill playback buffer with zeroes, and mark
		   all fragments as having data.
		*/

#ifndef __QNXNTO__
		pavail = snd_pcm_avail_update (driver->playback_handle);

		if (pavail != silence_frames) {
			jack_error ("ALSA: full buffer not available at start");
			return -1;
		}
#endif

		if (alsa_driver_get_channel_addresses (driver,
					0, &pavail, 0, &poffset)) {
			jack_error("silence failed, get channel addresses");
			return -1;
		}

		/* XXX this is cheating. ALSA offers no guarantee that
		   we can access the entire buffer at any one time. It
		   works on most hardware tested so far, however, buts
		   its a liability in the long run. I think that
		   alsa-lib may have a better function for doing this
		   here, where the goal is to silence the entire
		   buffer.
		*/

		for (chn = 0; chn < driver->playback_nchannels; chn++) {
			alsa_driver_silence_on_channel (
				driver, chn, silence_frames);
		}

#ifdef __QNXNTO__
		const size_t bytes = silence_frames * driver->playback_nchannels *
		        driver->playback_sample_bytes;
		if ((err = snd_pcm_plugin_write(driver->playback_handle,
					       driver->playback_areas, bytes)) < bytes) {
			jack_error ("ALSA: could not complete write of %"
				PRIu32 " frames: error = %d", silence_frames, err);
			return -1;
		}
#else
		snd_pcm_mmap_commit (driver->playback_handle, poffset, silence_frames);
#endif

		if ((err = alsa_driver_stream_start (driver->playback_handle, SND_PCM_STREAM_PLAYBACK)) < 0) {
			jack_error ("ALSA: could not start playback (%s)",
				    snd_strerror (err));
			return -1;
		}
	}

	if ((driver->capture_handle && driver->capture_and_playback_not_synced)
	    || !driver->playback_handle) {
		if ((err = alsa_driver_stream_start (driver->capture_handle, SND_PCM_STREAM_CAPTURE)) < 0) {
			jack_error ("ALSA: could not start capture (%s)",
				    snd_strerror (err));
			return -1;
		}
	}

	return 0;
}

int
alsa_driver_stop (alsa_driver_t *driver)
{
	int err;
//	JSList* node;
//	int chn;

	/* silence all capture port buffers, because we might
	   be entering offline mode.
	*/

// JACK2
/*
	for (chn = 0, node = driver->capture_ports; node;
	     node = jack_slist_next (node), chn++) {

		jack_port_t* port;
		char* buf;
		jack_nframes_t nframes = driver->engine->control->buffer_size;

		port = (jack_port_t *) node->data;
		buf = jack_port_get_buffer (port, nframes);
		memset (buf, 0, sizeof (jack_default_audio_sample_t) * nframes);
	}
*/

// JACK2
    ClearOutput();

	if (driver->playback_handle) {
#ifdef __QNXNTO__
		/* In case of playback: Drain discards the frames */
		err = snd_pcm_plugin_playback_drain(driver->playback_handle);
#else
		err = snd_pcm_drop (driver->playback_handle);
#endif
		if (err < 0) {
			jack_error ("ALSA: channel flush for playback "
				    "failed (%s)", snd_strerror (err));
			return -1;
		}
	}

	if (!driver->playback_handle
	    || driver->capture_and_playback_not_synced) {
		if (driver->capture_handle) {
#ifdef __QNXNTO__
			/* In case of capture: Flush discards the frames */
			err = snd_pcm_plugin_flush(driver->capture_handle, SND_PCM_CHANNEL_CAPTURE);
#else
			err = snd_pcm_drop (driver->capture_handle);
#endif
			if (err < 0) {
				jack_error ("ALSA: channel flush for "
					    "capture failed (%s)",
					    snd_strerror (err));
				return -1;
			}
		}
	}

	if (driver->hw_monitoring) {
		driver->hw->set_input_monitor_mask (driver->hw, 0);
	}

	if (driver->midi && !driver->xrun_recovery)
		(driver->midi->stop)(driver->midi);

	return 0;
}

static int
alsa_driver_restart (alsa_driver_t *driver)
{
	int res;

	driver->xrun_recovery = 1;
    // JACK2
    /*
 	if ((res = driver->nt_stop((struct _jack_driver_nt *) driver))==0)
		res = driver->nt_start((struct _jack_driver_nt *) driver);
    */
    res = Restart();
	driver->xrun_recovery = 0;

	if (res && driver->midi)
		(driver->midi->stop)(driver->midi);

	return res;
}

static int
alsa_driver_get_status (alsa_driver_t *driver)
{
	snd_pcm_t *pcm_handle;

#ifdef __QNXNTO__
	int res;
	snd_pcm_channel_status_t status;
#endif

	if (driver->capture_handle) {
		pcm_handle = driver->capture_handle;
	} else {
		pcm_handle = driver->playback_handle;
	}

#ifdef __QNXNTO__
	memset (&status, 0, sizeof (status));
	status.channel = driver->capture_handle ? SND_PCM_CHANNEL_CAPTURE :
	                                          SND_PCM_CHANNEL_PLAYBACK;
	res = snd_pcm_plugin_status(pcm_handle, &status);
	if (res < 0) {
		jack_error("status error: %s", snd_strerror(res));
		return -1;
	}
	return status.status;
#else
	return snd_pcm_state(pcm_handle);
#endif
}

static int
alsa_driver_xrun_recovery (alsa_driver_t *driver, float *delayed_usecs)
{
	int status;
	int res;

	status = alsa_driver_get_status(driver);
	if (status == SND_PCM_STATE_SUSPENDED) {
		if (driver->capture_handle) {
			if ((res = alsa_driver_prepare(driver->capture_handle, SND_PCM_STREAM_CAPTURE))
			    < 0) {
				jack_error("error preparing after suspend: %s", snd_strerror(res));
			}
		} 
		if (driver->playback_handle) {
			if ((res = alsa_driver_prepare(driver->playback_handle, SND_PCM_STREAM_PLAYBACK))
			    < 0) {
				jack_error("error preparing after suspend: %s", snd_strerror(res));
			}
		}
	}

	// TODO overrun
	if (status == SND_PCM_STATE_XRUN
	    && driver->process_count > XRUN_REPORT_DELAY) {
		driver->xrun_count++;
#ifdef __QNXNTO__
		/* Timestamp api's are not available as per QNX Documentation */
		*delayed_usecs = 0;
#else
		struct timeval now, diff, tstamp;
		snd_pcm_status_get_tstamp(status,&now);
		snd_pcm_status_get_trigger_tstamp(status, &tstamp);
		timersub(&now, &tstamp, &diff);
		*delayed_usecs = diff.tv_sec * 1000000.0 + diff.tv_usec;
#endif
		jack_log("**** alsa_pcm: xrun of at least %.3f msecs",*delayed_usecs / 1000.0);
		if (driver->capture_handle) {
			jack_log("Repreparing capture");
			if ((res = alsa_driver_prepare(driver->capture_handle,
			                               SND_PCM_STREAM_CAPTURE)) < 0) {
				jack_error("error preparing after xrun: %s", snd_strerror(res));
			}
		}
		if (driver->playback_handle) {
			jack_log("Repreparing playback");
			if ((res = alsa_driver_prepare(driver->playback_handle,
			                               SND_PCM_STREAM_PLAYBACK)) < 0) {
				jack_error("error preparing after xrun: %s", snd_strerror(res));
			}
		}
	}

	if (alsa_driver_restart (driver)) {
		jack_error("xrun recovery failed to restart driver");
		return -1;
	}
	return 0;
}

static void
alsa_driver_silence_untouched_channels (alsa_driver_t *driver,
					jack_nframes_t nframes)
{
	channel_t chn;
	jack_nframes_t buffer_frames =
		driver->frames_per_cycle * driver->playback_nperiods;

	for (chn = 0; chn < driver->playback_nchannels; chn++) {
		if (bitset_contains (driver->channels_not_done, chn)) {
			if (driver->silent[chn] < buffer_frames) {
				alsa_driver_silence_on_channel_no_mark (
					driver, chn, nframes);
				driver->silent[chn] += nframes;
			}
		}
	}
}

#ifdef __QNXNTO__
static int
alsa_driver_poll_descriptors(snd_pcm_t *pcm, struct pollfd *pfds, unsigned int space, bool is_capture)
{
	pfds->fd = snd_pcm_file_descriptor (pcm, is_capture);
	pfds->events = POLLHUP|POLLNVAL;
	pfds->events |= (is_capture == SND_PCM_STREAM_PLAYBACK) ? POLLOUT : POLLIN;

	return 0;
}

static snd_pcm_sframes_t
alsa_driver_avail(alsa_driver_t *driver, snd_pcm_t *pcm, bool is_capture)
{
	/* QNX guarantees that after poll() event at least one perido is available */
	return driver->frames_per_cycle;
}
#else
static int
alsa_driver_poll_descriptors(snd_pcm_t *pcm, struct pollfd *pfds, unsigned int space, bool is_capture)
{
	return snd_pcm_poll_descriptors(pcm, pfds, space);
}

static snd_pcm_sframes_t
alsa_driver_avail(alsa_driver_t *driver, snd_pcm_t *pcm, bool is_capture)
{
	return snd_pcm_avail_update(pcm);
}
#endif

static int under_gdb = FALSE;

jack_nframes_t
alsa_driver_wait (alsa_driver_t *driver, int extra_fd, int *status, float
		  *delayed_usecs)
{
	snd_pcm_sframes_t avail = 0;
	snd_pcm_sframes_t capture_avail = 0;
	snd_pcm_sframes_t playback_avail = 0;
	int xrun_detected = FALSE;
	int need_capture;
	int need_playback;
	int retry_cnt = 0;
	unsigned int i;
	jack_time_t poll_enter;
	jack_time_t poll_ret = 0;

	*status = -1;
	*delayed_usecs = 0;

	need_capture = driver->capture_handle ? 1 : 0;

	if (extra_fd >= 0) {
		need_playback = 0;
	} else {
		need_playback = driver->playback_handle ? 1 : 0;
	}

  again:

	while ((need_playback || need_capture) && !xrun_detected) {

		int poll_result;
		unsigned int ci = 0;
		unsigned int nfds;
		unsigned short revents;

		nfds = 0;

		if (need_playback) {
			alsa_driver_poll_descriptors (driver->playback_handle,
						  &driver->pfd[0],
						  driver->playback_nfds, SND_PCM_STREAM_PLAYBACK);
			nfds += driver->playback_nfds;
		}

		if (need_capture) {
			alsa_driver_poll_descriptors (driver->capture_handle,
						  &driver->pfd[nfds],
						  driver->capture_nfds, SND_PCM_STREAM_CAPTURE);
			ci = nfds;
			nfds += driver->capture_nfds;
		}

		/* ALSA doesn't set POLLERR in some versions of 0.9.X */

		for (i = 0; i < nfds; i++) {
			driver->pfd[i].events |= POLLERR;
		}

		if (extra_fd >= 0) {
			driver->pfd[nfds].fd = extra_fd;
			driver->pfd[nfds].events =
				POLLIN|POLLERR|POLLHUP|POLLNVAL;
			nfds++;
		}

		poll_enter = jack_get_microseconds ();

		if (poll_enter > driver->poll_next) {
			/*
			 * This processing cycle was delayed past the
			 * next due interrupt!  Do not account this as
			 * a wakeup delay:
			 */
			driver->poll_next = 0;
			driver->poll_late++;
		}

#ifdef __ANDROID__
		poll_result = poll (driver->pfd, nfds, -1);  //fix for sleep issue
#else
		poll_result = poll (driver->pfd, nfds, driver->poll_timeout_ms);
#endif
		if (poll_result < 0) {

			if (errno == EINTR) {
				const char poll_log[] = "ALSA: poll interrupt";
				// this happens mostly when run
				// under gdb, or when exiting due to a signal
				if (under_gdb) {
					jack_info(poll_log);
					goto again;
				}
				jack_error(poll_log);
				*status = -2;
				return 0;
			}

			jack_error ("ALSA: poll call failed (%s)",
				    strerror (errno));
			*status = -3;
			return 0;

		}

		poll_ret = jack_get_microseconds ();

		if (poll_result == 0) {
			retry_cnt++;
			if(retry_cnt > MAX_RETRY_COUNT) {
				jack_error ("ALSA: poll time out, polled for %" PRIu64
					    " usecs, Reached max retry cnt = %d, Exiting",
					    poll_ret - poll_enter, MAX_RETRY_COUNT);
				*status = -5;
				return 0;
			}
			jack_error ("ALSA: poll time out, polled for %" PRIu64
				    " usecs, Retrying with a recovery, retry cnt = %d",
				    poll_ret - poll_enter, retry_cnt);
			*status = alsa_driver_xrun_recovery (driver, delayed_usecs);
			if(*status != 0) {
				jack_error ("ALSA: poll time out, recovery failed with status = %d", *status);
				return 0;
			}
		}

        // JACK2
        SetTime(poll_ret);

		if (extra_fd < 0) {
			if (driver->poll_next && poll_ret > driver->poll_next) {
				*delayed_usecs = poll_ret - driver->poll_next;
			}
			driver->poll_last = poll_ret;
			driver->poll_next = poll_ret + driver->period_usecs;
// JACK2
/*
			driver->engine->transport_cycle_start (driver->engine,
							       poll_ret);
*/
		}

#ifdef DEBUG_WAKEUP
		fprintf (stderr, "%" PRIu64 ": checked %d fds, started at %" PRIu64 " %" PRIu64 "  usecs since poll entered\n",
			 poll_ret, nfds, poll_enter, poll_ret - poll_enter);
#endif

		/* check to see if it was the extra FD that caused us
		 * to return from poll */

		if (extra_fd >= 0) {

			if (driver->pfd[nfds-1].revents == 0) {
				/* we timed out on the extra fd */

				*status = -4;
				return -1;
			}

			/* if POLLIN was the only bit set, we're OK */

			*status = 0;
			return (driver->pfd[nfds-1].revents == POLLIN) ? 0 : -1;
		}

		if (need_playback) {
			if (snd_pcm_poll_descriptors_revents
			    (driver->playback_handle, &driver->pfd[0],
			     driver->playback_nfds, &revents) < 0) {
				jack_error ("ALSA: playback revents failed");
				*status = -6;
				return 0;
			}

			if (revents & POLLNVAL) {
				jack_error ("ALSA: playback device disconnected");
				*status = -7;
				return 0;
			}

			if (revents & POLLERR) {
				xrun_detected = TRUE;
			}

			if (revents & POLLOUT) {
				need_playback = 0;
#ifdef DEBUG_WAKEUP
				fprintf (stderr, "%" PRIu64
					 " playback stream ready\n",
					 poll_ret);
#endif
			}
		}

		if (need_capture) {
			if (snd_pcm_poll_descriptors_revents
			    (driver->capture_handle, &driver->pfd[ci],
			     driver->capture_nfds, &revents) < 0) {
				jack_error ("ALSA: capture revents failed");
				*status = -6;
				return 0;
			}

			if (revents & POLLNVAL) {
				jack_error ("ALSA: capture device disconnected");
				*status = -7;
				return 0;
			}

			if (revents & POLLERR) {
				xrun_detected = TRUE;
			}

			if (revents & POLLIN) {
				need_capture = 0;
#ifdef DEBUG_WAKEUP
				fprintf (stderr, "%" PRIu64
					 " capture stream ready\n",
					 poll_ret);
#endif
			}
		}
	}

	if (driver->capture_handle) {
		if ((capture_avail = alsa_driver_avail (driver,
			     driver->capture_handle, SND_PCM_STREAM_CAPTURE)) < 0) {
			if (capture_avail == -EPIPE) {
				xrun_detected = TRUE;
			} else {
				jack_error ("unknown ALSA avail_update return"
					    " value (%u)", capture_avail);
			}
		}
	} else {
		/* odd, but see min() computation below */
		capture_avail = INT_MAX;
	}

	if (driver->playback_handle) {
		if ((playback_avail = alsa_driver_avail (driver,
			     driver->playback_handle, SND_PCM_STREAM_PLAYBACK)) < 0) {
			if (playback_avail == -EPIPE) {
				xrun_detected = TRUE;
			} else {
				jack_error ("unknown ALSA avail_update return"
					    " value (%u)", playback_avail);
			}
		}
	} else {
		/* odd, but see min() computation below */
		playback_avail = INT_MAX;
	}

	if (xrun_detected) {
		*status = alsa_driver_xrun_recovery (driver, delayed_usecs);
		return 0;
	}

	*status = 0;
	driver->last_wait_ust = poll_ret;

	avail = capture_avail < playback_avail ? capture_avail : playback_avail;

#ifdef DEBUG_WAKEUP
	fprintf (stderr, "wakeup complete, avail = %lu, pavail = %lu "
		 "cavail = %lu\n",
		 avail, playback_avail, capture_avail);
#endif

	/* mark all channels not done for now. read/write will change this */

	bitset_copy (driver->channels_not_done, driver->channels_done);

	/* constrain the available count to the nearest (round down) number of
	   periods.
	*/

	return avail - (avail % driver->frames_per_cycle);
}


int
alsa_driver_read (alsa_driver_t *driver, jack_nframes_t nframes)
{
	snd_pcm_sframes_t contiguous;
	snd_pcm_sframes_t nread;
	snd_pcm_uframes_t offset;
	jack_nframes_t  orig_nframes;
//	jack_default_audio_sample_t* buf;
//	channel_t chn;
//	JSList *node;
//	jack_port_t* port;
	int err;

	if (nframes > driver->frames_per_cycle) {
		return -1;
	}

// JACK2
/*
	if (driver->engine->freewheeling) {
		return 0;
	}
*/
	if (driver->midi)
		(driver->midi->read)(driver->midi, nframes);

	if (!driver->capture_handle) {
		return 0;
	}

	nread = 0;
	contiguous = 0;
	orig_nframes = nframes;

	while (nframes) {

		contiguous = nframes;

		if (alsa_driver_get_channel_addresses (
			    driver,
			    (snd_pcm_uframes_t *) &contiguous,
			    (snd_pcm_uframes_t *) 0,
			    &offset, 0) < 0) {
			return -1;
		}

#ifdef __QNXNTO__
		const size_t bytes = contiguous * driver->capture_nchannels * driver->capture_sample_bytes;
		if ((err = snd_pcm_plugin_read(driver->capture_handle,
					       driver->capture_areas, bytes)) < bytes) {
			jack_error ("ALSA: could not complete read of %"
				    PRIu32 " frames: error = %d", contiguous, err);
			return -1;
		}
#endif

// JACK2
/*
		for (chn = 0, node = driver->capture_ports; node;
		     node = jack_slist_next (node), chn++) {

			port = (jack_port_t *) node->data;

			if (!jack_port_connected (port)) {
				// no-copy optimization
				continue;
			}
			buf = jack_port_get_buffer (port, orig_nframes);
			alsa_driver_read_from_channel (driver, chn,
				buf + nread, contiguous);
		}
*/
        ReadInput(orig_nframes, contiguous, nread);

#ifndef __QNXNTO__
		if ((err = snd_pcm_mmap_commit (driver->capture_handle,
				offset, contiguous)) < 0) {
			jack_error ("ALSA: could not complete read of %"
				PRIu32 " frames: error = %d", contiguous, err);
			return -1;
		}
#endif

		nframes -= contiguous;
		nread += contiguous;
	}

	return 0;
}

int
alsa_driver_write (alsa_driver_t* driver, jack_nframes_t nframes)
{
//	channel_t chn;
//	JSList *node;
//	JSList *mon_node;
//	jack_default_audio_sample_t* buf;
//	jack_default_audio_sample_t* monbuf;
	jack_nframes_t orig_nframes;
	snd_pcm_sframes_t nwritten;
	snd_pcm_sframes_t contiguous;
	snd_pcm_uframes_t offset;
//	jack_port_t *port;
	int err;

	driver->process_count++;

// JACK2
/*
	if (!driver->playback_handle || driver->engine->freewheeling) {
		return 0;
	}
*/
	if (!driver->playback_handle) {
		return 0;
	}

	if (nframes > driver->frames_per_cycle) {
		return -1;
	}

	if (driver->midi)
		(driver->midi->write)(driver->midi, nframes);

	nwritten = 0;
	contiguous = 0;
	orig_nframes = nframes;

	/* check current input monitor request status */

	driver->input_monitor_mask = 0;

// JACK2
/*
	for (chn = 0, node = driver->capture_ports; node;
	     node = jack_slist_next (node), chn++) {
		if (((jack_port_t *) node->data)->shared->monitor_requests) {
			driver->input_monitor_mask |= (1<<chn);
		}
	}
*/
    MonitorInput();

	if (driver->hw_monitoring) {
		if ((driver->hw->input_monitor_mask
		     != driver->input_monitor_mask)
		    && !driver->all_monitor_in) {
			driver->hw->set_input_monitor_mask (
				driver->hw, driver->input_monitor_mask);
		}
	}

	while (nframes) {

		contiguous = nframes;

		if (alsa_driver_get_channel_addresses (
			    driver,
			    (snd_pcm_uframes_t *) 0,
			    (snd_pcm_uframes_t *) &contiguous,
			    0, &offset) < 0) {
			return -1;
		}

// JACK2
/*
		for (chn = 0, node = driver->playback_ports, mon_node=driver->monitor_ports;
		     node;
		     node = jack_slist_next (node), chn++) {

			port = (jack_port_t *) node->data;

			if (!jack_port_connected (port)) {
				continue;
			}
			buf = jack_port_get_buffer (port, orig_nframes);
			alsa_driver_write_to_channel (driver, chn,
				buf + nwritten, contiguous);

			if (mon_node) {
				port = (jack_port_t *) mon_node->data;
				if (!jack_port_connected (port)) {
					continue;
				}
				monbuf = jack_port_get_buffer (port, orig_nframes);
				memcpy (monbuf + nwritten, buf + nwritten, contiguous * sizeof(jack_default_audio_sample_t));
				mon_node = jack_slist_next (mon_node);
			}
		}
*/

        // JACK2
        WriteOutput(orig_nframes, contiguous, nwritten);

		if (!bitset_empty (driver->channels_not_done)) {
			alsa_driver_silence_untouched_channels (driver,
								contiguous);
		}

#ifdef __QNXNTO__
		const size_t bytes = contiguous * driver->playback_nchannels * driver->playback_sample_bytes;
		if ((err = snd_pcm_plugin_write(driver->playback_handle,
					       driver->playback_areas, bytes)) < bytes) {
			jack_error ("ALSA: could not complete write of %"
				PRIu32 " frames: error = %d", contiguous, err);
			return -1;
		}
#else
		if ((err = snd_pcm_mmap_commit (driver->playback_handle,
				offset, contiguous)) < 0) {
			jack_error ("ALSA: could not complete playback of %"
				PRIu32 " frames: error = %d", contiguous, err);
			if (err != -EPIPE && err != -ESTRPIPE)
				return -1;
		}
#endif

		nframes -= contiguous;
		nwritten += contiguous;
	}

	return 0;
}

#if 0
static int  /* UNUSED */
alsa_driver_change_sample_clock (alsa_driver_t *driver, SampleClockMode mode)
{
	return driver->hw->change_sample_clock (driver->hw, mode);
}

static void  /* UNUSED */
alsa_driver_request_all_monitor_input (alsa_driver_t *driver, int yn)

{
	if (driver->hw_monitoring) {
		if (yn) {
			driver->hw->set_input_monitor_mask (driver->hw, ~0U);
		} else {
			driver->hw->set_input_monitor_mask (
				driver->hw, driver->input_monitor_mask);
		}
	}

	driver->all_monitor_in = yn;
}

static void  /* UNUSED */
alsa_driver_set_hw_monitoring (alsa_driver_t *driver, int yn)
{
	if (yn) {
		driver->hw_monitoring = TRUE;

		if (driver->all_monitor_in) {
			driver->hw->set_input_monitor_mask (driver->hw, ~0U);
		} else {
			driver->hw->set_input_monitor_mask (
				driver->hw, driver->input_monitor_mask);
		}
	} else {
		driver->hw_monitoring = FALSE;
		driver->hw->set_input_monitor_mask (driver->hw, 0);
	}
}

static ClockSyncStatus  /* UNUSED */
alsa_driver_clock_sync_status (channel_t chn)
{
	return Lock;
}
#endif

void
alsa_driver_delete (alsa_driver_t *driver)
{
	JSList *node;

	if (driver->midi)
		(driver->midi->destroy)(driver->midi);

	for (node = driver->clock_sync_listeners; node;
	     node = jack_slist_next (node)) {
		free (node->data);
	}
	jack_slist_free (driver->clock_sync_listeners);

	if (driver->ctl_handle) {
		snd_ctl_close (driver->ctl_handle);
		driver->ctl_handle = 0;
	}

	if (driver->capture_handle) {
		snd_pcm_close (driver->capture_handle);
		driver->capture_handle = 0;
	}

	if (driver->playback_handle) {
		snd_pcm_close (driver->playback_handle);
		driver->capture_handle = 0;
	}

#ifndef __QNXNTO__
	if (driver->capture_hw_params) {
		snd_pcm_hw_params_free (driver->capture_hw_params);
		driver->capture_hw_params = 0;
	}

	if (driver->playback_hw_params) {
		snd_pcm_hw_params_free (driver->playback_hw_params);
		driver->playback_hw_params = 0;
	}

	if (driver->capture_sw_params) {
		snd_pcm_sw_params_free (driver->capture_sw_params);
		driver->capture_sw_params = 0;
	}

	if (driver->playback_sw_params) {
		snd_pcm_sw_params_free (driver->playback_sw_params);
		driver->playback_sw_params = 0;
	}
#endif

	if (driver->pfd) {
		free (driver->pfd);
	}

	if (driver->hw) {
		driver->hw->release (driver->hw);
		driver->hw = 0;
	}
	free(driver->alsa_name_playback);
	free(driver->alsa_name_capture);
	free(driver->alsa_driver);

	alsa_driver_release_channel_dependent_memory (driver);
    //JACK2
    //jack_driver_nt_finish ((jack_driver_nt_t *) driver);
	free (driver);
}

static char*
discover_alsa_using_apps ()
{
        char found[2048];
        char command[5192];
        char* path = getenv ("PATH");
        char* dir;
        size_t flen = 0;
        int card;
        int device;
        size_t cmdlen = 0;

        if (!path) {
                return NULL;
        }

        /* look for lsof and give up if its not in PATH */

        path = strdup (path);
        dir = strtok (path, ":");
        while (dir) {
                char maybe[PATH_MAX+1];
                snprintf (maybe, sizeof(maybe), "%s/lsof", dir);
                if (access (maybe, X_OK) == 0) {
                        break;
                }
                dir = strtok (NULL, ":");
        }
        free (path);

        if (!dir) {
                return NULL;
        }

        snprintf (command, sizeof (command), "lsof -Fc0 ");
        cmdlen = strlen (command);

        for (card = 0; card < 8; ++card) {
                for (device = 0; device < 8; ++device)  {
                        char buf[32];

                        snprintf (buf, sizeof (buf), "/dev/snd/pcmC%dD%dp", card, device);
                        if (access (buf, F_OK) == 0) {
                                snprintf (command+cmdlen, sizeof(command)-cmdlen, "%s ", buf);
                        }
                        cmdlen = strlen (command);

                        snprintf (buf, sizeof (buf), "/dev/snd/pcmC%dD%dc", card, device);
                        if (access (buf, F_OK) == 0) {
                                snprintf (command+cmdlen, sizeof(command)-cmdlen, "%s ", buf);
                        }
                        cmdlen = strlen (command);
                }
        }

        FILE* f = popen (command, "r");

        if (!f) {
                return NULL;
        }

        while (!feof (f)) {
                char buf[1024]; /* lsof doesn't output much */

                if (!fgets (buf, sizeof (buf), f)) {
                        break;
                }

                if (*buf != 'p') {
                        return NULL;
                }

                /* buf contains NULL as a separator between the process field and the command field */
                char *pid = buf;
                ++pid; /* skip leading 'p' */
                char *cmd = pid;

                /* skip to NULL */
                while (*cmd) {
                        ++cmd;
                }
                ++cmd; /* skip to 'c' */
                ++cmd; /* skip to first character of command */

                snprintf (found+flen, sizeof (found)-flen, "%s (process ID %s)\n", cmd, pid);
                flen = strlen (found);

                if (flen >= sizeof (found)) {
                        break;
                }
        }

        pclose (f);

        if (flen) {
                return strdup (found);
        } else {
                return NULL;
        }
}

static int
alsa_driver_open (alsa_driver_t *driver, bool is_capture)
{
	int err = 0;
	char* current_apps;

	if(is_capture) {
#ifdef __QNXNTO__
		err = snd_pcm_open_name (&driver->capture_handle,
					 driver->alsa_name_capture,
					 SND_PCM_OPEN_CAPTURE | SND_PCM_OPEN_NONBLOCK);
#else
		err = snd_pcm_open (&driver->capture_handle,
				    driver->alsa_name_capture,
				    SND_PCM_STREAM_CAPTURE,
				    SND_PCM_NONBLOCK);
#endif
	} else {
#ifdef __QNXNTO__
		err = snd_pcm_open_name (&driver->playback_handle,
					 driver->alsa_name_playback,
					 SND_PCM_OPEN_PLAYBACK | SND_PCM_OPEN_NONBLOCK);
#else
		err = snd_pcm_open (&driver->playback_handle,
				    driver->alsa_name_playback,
				    SND_PCM_STREAM_PLAYBACK,
				    SND_PCM_NONBLOCK);
#endif
	}
	if (err < 0) {
		switch (errno) {
		case EBUSY:
#ifdef __ANDROID__
			jack_error ("\n\nATTENTION: The device \"%s\" is "
				    "already in use. Please stop the"
				    " application using it and "
				    "run JACK again",
				    is_capture ? driver->alsa_name_capture : driver->alsa_name_playback);
#else
			current_apps = discover_alsa_using_apps ();
			if (current_apps) {
				jack_error ("\n\nATTENTION: The device \"%s\" is "
					    "already in use. The following applications "
					    " are using your soundcard(s) so you should "
					    " check them and stop them as necessary before "
					    " trying to start JACK again:\n\n%s",
					    is_capture ? driver->alsa_name_capture : driver->alsa_name_playback,
					    current_apps);
				free (current_apps);
			} else {
				jack_error ("\n\nATTENTION: The device \"%s\" is "
					    "already in use. Please stop the"
					    " application using it and "
					    "run JACK again",
					    is_capture ? driver->alsa_name_capture : driver->alsa_name_playback);
			}
#endif
			break;

		case EPERM:
			jack_error ("you do not have permission to open "
				    "the audio device \"%s\" for playback",
				    is_capture ? driver->alsa_name_capture : driver->alsa_name_playback);
			break;

		case EINVAL:
			jack_error ("the state of handle or the mode is invalid "
				"or invalid state change occured \"%s\" for %s",
				is_capture ? driver->alsa_name_capture : driver->alsa_name_playback,
				is_capture ? "capture" : "playback");
			break;

		case ENOENT:
			jack_error ("device \"%s\"  does not exist for %s",
				is_capture ? driver->alsa_name_capture : driver->alsa_name_playback,
				is_capture ? "capture" : "playback");
			break;

		case ENOMEM:
			jack_error ("Not enough memory available for allocation for \"%s\" for %s",
				is_capture ? driver->alsa_name_capture : driver->alsa_name_playback,
				is_capture ? "capture" : "playback");
			break;

		case SND_ERROR_INCOMPATIBLE_VERSION:
			jack_error ("Version mismatch \"%s\" for %s",
				is_capture ? driver->alsa_name_capture : driver->alsa_name_playback,
				is_capture ? "capture" : "playback");
			break;
		}
		alsa_driver_delete (driver);
		if(is_capture) {
			driver->capture_handle = NULL;
		} else {
			driver->playback_handle = NULL;
		}
	}

	if (is_capture && driver->capture_handle) {
#ifdef __QNXNTO__
		snd_pcm_nonblock_mode (driver->capture_handle, 0);
#else
		snd_pcm_nonblock (driver->capture_handle, 0);
#endif
	} else if(!is_capture && driver->playback_handle) {
#ifdef __QNXNTO__
		snd_pcm_nonblock_mode (driver->playback_handle, 0);
#else
		snd_pcm_nonblock (driver->playback_handle, 0);
#endif
	}

	return err;
}

jack_driver_t *
alsa_driver_new (char *name, char *playback_alsa_device,
		 char *capture_alsa_device,
		 jack_client_t *client,
		 jack_nframes_t frames_per_cycle,
		 jack_nframes_t user_nperiods,
		 jack_nframes_t rate,
		 int hw_monitoring,
		 int hw_metering,
		 int capturing,
		 int playing,
		 DitherAlgorithm dither,
		 int soft_mode,
		 int monitor,
		 int user_capture_nchnls,
		 int user_playback_nchnls,
		 int shorts_first,
		 jack_nframes_t capture_latency,
		 jack_nframes_t playback_latency,
		 alsa_midi_t *midi_driver
		 )
{
	int err;
	alsa_driver_t *driver;

	jack_info ("creating alsa driver ... %s|%s|%" PRIu32 "|%" PRIu32
		"|%" PRIu32"|%" PRIu32"|%" PRIu32 "|%s|%s|%s|%s",
		playing ? playback_alsa_device : "-",
		capturing ? capture_alsa_device : "-",
		frames_per_cycle, user_nperiods, rate,
		user_capture_nchnls,user_playback_nchnls,
		hw_monitoring ? "hwmon": "nomon",
		hw_metering ? "hwmeter":"swmeter",
		soft_mode ? "soft-mode":"-",
		shorts_first ? "16bit":"32bit");

	driver = (alsa_driver_t *) calloc (1, sizeof (alsa_driver_t));

	jack_driver_nt_init ((jack_driver_nt_t *) driver);

    // JACK2
    /*
	driver->nt_attach = (JackDriverNTAttachFunction) alsa_driver_attach;
    driver->nt_detach = (JackDriverNTDetachFunction) alsa_driver_detach;
	driver->read = (JackDriverReadFunction) alsa_driver_read;
	driver->write = (JackDriverReadFunction) alsa_driver_write;
	driver->null_cycle = (JackDriverNullCycleFunction) alsa_driver_null_cycle;
	driver->nt_bufsize = (JackDriverNTBufSizeFunction) alsa_driver_bufsize;
	driver->nt_start = (JackDriverNTStartFunction) alsa_driver_start;
	driver->nt_stop = (JackDriverNTStopFunction) alsa_driver_stop;
	driver->nt_run_cycle = (JackDriverNTRunCycleFunction) alsa_driver_run_cycle;
    */

	driver->playback_handle = NULL;
	driver->capture_handle = NULL;
	driver->ctl_handle = 0;
	driver->hw = 0;
	driver->capture_and_playback_not_synced = FALSE;
	driver->max_nchannels = 0;
	driver->user_nchannels = 0;
	driver->playback_nchannels = user_playback_nchnls;
	driver->capture_nchannels = user_capture_nchnls;
	driver->playback_sample_bytes = (shorts_first ? 2:4);
	driver->capture_sample_bytes = (shorts_first ? 2:4);
	driver->capture_frame_latency = capture_latency;
	driver->playback_frame_latency = playback_latency;

	driver->playback_addr = 0;
	driver->capture_addr = 0;
	driver->playback_interleave_skip = NULL;
	driver->capture_interleave_skip = NULL;
#ifdef __QNXNTO__
	driver->playback_areas = NULL;
	driver->playback_areas_ptr = NULL;
	driver->capture_areas = NULL;
	driver->capture_areas_ptr = NULL;
#endif


	driver->silent = 0;
	driver->all_monitor_in = FALSE;
	driver->with_monitor_ports = monitor;

	driver->clock_mode = ClockMaster; /* XXX is it? */
	driver->input_monitor_mask = 0;   /* XXX is it? */

	driver->capture_ports = 0;
	driver->playback_ports = 0;
	driver->monitor_ports = 0;

	driver->pfd = 0;
	driver->playback_nfds = 0;
	driver->capture_nfds = 0;

	driver->dither = dither;
	driver->soft_mode = soft_mode;

	driver->quirk_bswap = 0;

	pthread_mutex_init (&driver->clock_sync_lock, 0);
	driver->clock_sync_listeners = 0;

	driver->poll_late = 0;
	driver->xrun_count = 0;
	driver->process_count = 0;

	driver->alsa_name_playback = strdup (playback_alsa_device);
	driver->alsa_name_capture = strdup (capture_alsa_device);

	driver->midi = midi_driver;
	driver->xrun_recovery = 0;

#ifndef __QNXNTO__
	if (alsa_driver_check_card_type (driver)) {
		alsa_driver_delete (driver);
		return NULL;
	}

	alsa_driver_hw_specific (driver, hw_monitoring, hw_metering);
#endif

	if (playing) {
		err = alsa_driver_open(driver, SND_PCM_STREAM_PLAYBACK);
		if(err < 0) {
			jack_error ("\n\nATTENTION: Opening of the playback device \"%s\" failed.",
				    playback_alsa_device);
			return NULL;
		}
	}
	if(capturing) {
		err = alsa_driver_open(driver, SND_PCM_STREAM_CAPTURE);
		if(err < 0) {
			jack_error ("\n\nATTENTION: Opening of the capture device \"%s\" failed.",
				    capture_alsa_device);
			return NULL;
		}
	}

	if (driver->playback_handle == NULL) {
		if (playing) {

			/* they asked for playback, but we can't do it */

			jack_error ("ALSA: Cannot open PCM device %s for "
				    "playback. Falling back to capture-only"
				    " mode", name);

			if (driver->capture_handle == NULL) {
				/* can't do anything */
				alsa_driver_delete (driver);
				return NULL;
			}

			playing = FALSE;
		}
	}

	if (driver->capture_handle == NULL) {
		if (capturing) {

			/* they asked for capture, but we can't do it */

			jack_error ("ALSA: Cannot open PCM device %s for "
				    "capture. Falling back to playback-only"
				    " mode", name);

			if (driver->playback_handle == NULL) {
				/* can't do anything */
				alsa_driver_delete (driver);
				return NULL;
			}

			capturing = FALSE;
		}
	}

#ifndef __QNXNTO__
	driver->playback_hw_params = 0;
	driver->capture_hw_params = 0;
	driver->playback_sw_params = 0;
	driver->capture_sw_params = 0;

	if (driver->playback_handle) {
		if ((err = snd_pcm_hw_params_malloc (
			     &driver->playback_hw_params)) < 0) {
			jack_error ("ALSA: could not allocate playback hw"
				    " params structure");
			alsa_driver_delete (driver);
			return NULL;
		}

		if ((err = snd_pcm_sw_params_malloc (
			     &driver->playback_sw_params)) < 0) {
			jack_error ("ALSA: could not allocate playback sw"
				    " params structure");
			alsa_driver_delete (driver);
			return NULL;
		}
	}

	if (driver->capture_handle) {
		if ((err = snd_pcm_hw_params_malloc (
			     &driver->capture_hw_params)) < 0) {
			jack_error ("ALSA: could not allocate capture hw"
				    " params structure");
			alsa_driver_delete (driver);
			return NULL;
		}

		if ((err = snd_pcm_sw_params_malloc (
			     &driver->capture_sw_params)) < 0) {
			jack_error ("ALSA: could not allocate capture sw"
				    " params structure");
			alsa_driver_delete (driver);
			return NULL;
		}
	}
#endif
	if (alsa_driver_set_parameters (driver, frames_per_cycle,
					user_nperiods, rate)) {
		alsa_driver_delete (driver);
		return NULL;
	}

	driver->capture_and_playback_not_synced = FALSE;

	if (driver->capture_handle && driver->playback_handle) {
		if (snd_pcm_link (driver->playback_handle,
				  driver->capture_handle) != 0) {
			driver->capture_and_playback_not_synced = TRUE;
		}
	}

	driver->client = client;

	return (jack_driver_t *) driver;
}

int
alsa_driver_stop_listening_to_clock_sync_status (alsa_driver_t *driver,
						 unsigned int which)

{
	JSList *node;
	int ret = -1;
	pthread_mutex_lock (&driver->clock_sync_lock);
	for (node = driver->clock_sync_listeners; node;
	     node = jack_slist_next (node)) {
		if (((ClockSyncListener *) node->data)->id == which) {
			driver->clock_sync_listeners =
				jack_slist_remove_link (
					driver->clock_sync_listeners, node);
			free (node->data);
			jack_slist_free_1 (node);
			ret = 0;
			break;
		}
	}
	pthread_mutex_unlock (&driver->clock_sync_lock);
	return ret;
}

/* DRIVER "PLUGIN" INTERFACE */

const char driver_client_name[] = "alsa_pcm";

void
driver_finish (jack_driver_t *driver)
{
	alsa_driver_delete ((alsa_driver_t *) driver);
}
