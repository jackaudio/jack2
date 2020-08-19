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

#ifdef __QNXNTO__

char qnx_channel_error_str[9][35] = {
	"SND_PCM_STATUS_NOTREADY",
	"SND_PCM_STATUS_READY",
	"SND_PCM_STATUS_PREPARED",
	"SND_PCM_STATUS_RUNNING",
	"SND_PCM_STATUS_PAUSED",
	"SND_PCM_STATUS_SUSPENDED",
	"SND_PCM_STATUS_UNDERRUN",
	"SND_PCM_STATUS_OVERRUN",
	"SND_PCM_STATUS_UNKNOWN",
};

static char* alsa_channel_status_error_str(int code)
{
	switch(code) {
		case SND_PCM_STATUS_NOTREADY:
			return qnx_channel_error_str[0];
		case SND_PCM_STATUS_READY:
			return qnx_channel_error_str[1];
		case SND_PCM_STATUS_PREPARED:
			return qnx_channel_error_str[2];
		case SND_PCM_STATUS_RUNNING:
			return qnx_channel_error_str[3];
		case SND_PCM_STATUS_PAUSED:
			return qnx_channel_error_str[4];
		case SND_PCM_STATUS_SUSPENDED:
			return qnx_channel_error_str[5];
		case SND_PCM_STATUS_UNDERRUN:
			return qnx_channel_error_str[6];
		case SND_PCM_STATUS_OVERRUN:
			return qnx_channel_error_str[7];
	}
	return qnx_channel_error_str[8];
}
#endif

static int alsa_driver_link (alsa_driver_t *driver);
static int alsa_driver_open_device (alsa_driver_t *driver, alsa_device_t *device, bool is_capture);
static int alsa_driver_get_state (snd_pcm_t *handle, int is_capture);

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
alsa_driver_release_channel_dependent_memory (alsa_driver_t *driver, alsa_device_t *device)
{
	bitset_destroy (&device->channels_done);
	bitset_destroy (&device->channels_not_done);

	device->capture_channel_offset = 0;
	device->playback_channel_offset = 0;

	/* if we have only 1 device reuse user requested channels, otherwise 0 will attemp to allocate max channels on next setup pass */
	if (driver->devices_count != 1) {
		driver->capture_nchannels = 0;
		driver->playback_nchannels = 0;
		device->capture_nchannels = 0;
		device->playback_nchannels = 0;
	}

	if (device->playback_addr) {
		free (device->playback_addr);
		device->playback_addr = 0;
	}

	if (device->capture_addr) {
		free (device->capture_addr);
		device->capture_addr = 0;
	}

#ifdef __QNXNTO__
	if (device->playback_areas_ptr) {
		free(device->playback_areas_ptr);
		device->playback_areas_ptr = NULL;
	}

	if (device->capture_areas_ptr) {
		free(device->capture_areas_ptr);
		device->capture_areas_ptr = NULL;
	}
#endif

	if (device->playback_interleave_skip) {
		free (device->playback_interleave_skip);
		device->playback_interleave_skip = NULL;
	}

	if (device->capture_interleave_skip) {
		free (device->capture_interleave_skip);
		device->capture_interleave_skip = NULL;
	}

	if (device->silent) {
		free (device->silent);
		device->silent = 0;
	}

	if (driver->dither_state) {
		free (driver->dither_state);
		driver->dither_state = 0;
	}
}

#ifndef __QNXNTO__

static int
alsa_driver_check_capabilities (alsa_driver_t *driver, alsa_device_t *device)
{
	return 0;
}

char* get_control_device_name(const char * device_name);

static int
alsa_driver_check_card_type (alsa_driver_t *driver, alsa_device_t *device)
{
	int err;
	snd_ctl_card_info_t *card_info;
	char * ctl_name;

	snd_ctl_card_info_alloca (&card_info);

	ctl_name = get_control_device_name(device->playback_name);

	// XXX: I don't know the "right" way to do this. Which to use
	// driver->alsa_name_playback or driver->alsa_name_capture.
	if ((err = snd_ctl_open (&device->ctl_handle, ctl_name, 0)) < 0) {
		jack_error ("control open \"%s\" (%s)", ctl_name,
			    snd_strerror(err));
	} else if ((err = snd_ctl_card_info(device->ctl_handle, card_info)) < 0) {
		jack_error ("control hardware info \"%s\" (%s)",
			    device->playback_name, snd_strerror (err));
		snd_ctl_close (device->ctl_handle);
	}

	device->alsa_driver = strdup(snd_ctl_card_info_get_driver (card_info));

	free(ctl_name);

	return alsa_driver_check_capabilities (driver, device);
}

static int
alsa_driver_hammerfall_hardware (alsa_driver_t *driver, alsa_device_t *device)
{
	device->hw = jack_alsa_hammerfall_hw_new (device);
	return 0;
}

static int
alsa_driver_hdsp_hardware (alsa_driver_t *driver, alsa_device_t *device)
{
	device->hw = jack_alsa_hdsp_hw_new (device);
	return 0;
}

static int
alsa_driver_ice1712_hardware (alsa_driver_t *driver, alsa_device_t *device)
{
        device->hw = jack_alsa_ice1712_hw_new (device);
        return 0;
}

// JACK2
/*
static int
alsa_driver_usx2y_hardware (alsa_driver_t *driver, alsa_device_t *device)
{
    driver->hw = jack_alsa_usx2y_hw_new (device);
    return 0;
}
*/

static int
alsa_driver_generic_hardware (alsa_driver_t *driver, alsa_device_t *device)
{
	device->hw = jack_alsa_generic_hw_new (device);
	return 0;
}

static int
alsa_driver_hw_specific (alsa_driver_t *driver, alsa_device_t *device, int hw_monitoring,
			 int hw_metering)
{
	int err;

	if (!strcmp(device->alsa_driver, "RME9652")) {
		if ((err = alsa_driver_hammerfall_hardware (driver, device)) != 0) {
			return err;
		}
	} else if (!strcmp(device->alsa_driver, "H-DSP")) {
                if ((err = alsa_driver_hdsp_hardware (driver, device)) !=0) {
                        return err;
                }
	} else if (!strcmp(device->alsa_driver, "ICE1712")) {
                if ((err = alsa_driver_ice1712_hardware (driver, device)) !=0) {
                        return err;
                }
	}
    // JACK2
    /*
        else if (!strcmp(device->alsa_driver, "USB US-X2Y")) {
		if ((err = alsa_driver_usx2y_hardware (driver, device)) !=0) {
				return err;
		}
	}
    */
       else {
	        if ((err = alsa_driver_generic_hardware (driver, device)) != 0) {
			return err;
		}
	}

	if (device->hw->capabilities & Cap_HardwareMonitoring) {
		driver->has_hw_monitoring = TRUE;
		/* XXX need to ensure that this is really FALSE or
		 * TRUE or whatever*/
		driver->hw_monitoring = hw_monitoring;
	} else {
		driver->has_hw_monitoring = FALSE;
		driver->hw_monitoring = FALSE;
	}

	if (device->hw->capabilities & Cap_ClockLockReporting) {
		driver->has_clock_sync_reporting = TRUE;
	} else {
		driver->has_clock_sync_reporting = FALSE;
	}

	if (device->hw->capabilities & Cap_HardwareMetering) {
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
alsa_driver_setup_io_function_pointers (alsa_driver_t *driver, alsa_device_t *device)
{
	if (device->playback_handle) {
		if (SND_PCM_FORMAT_FLOAT_LE == device->playback_sample_format) {
			device->write_via_copy = sample_move_dS_floatLE;
		} else {
			switch (device->playback_sample_bytes) {
			case 2:
				switch (driver->dither) {
				case Rectangular:
					jack_info("Rectangular dithering at 16 bits");
					device->write_via_copy = device->quirk_bswap?
						sample_move_dither_rect_d16_sSs:
						sample_move_dither_rect_d16_sS;
					break;

				case Triangular:
					jack_info("Triangular dithering at 16 bits");
					device->write_via_copy = device->quirk_bswap?
						sample_move_dither_tri_d16_sSs:
						sample_move_dither_tri_d16_sS;
					break;

				case Shaped:
					jack_info("Noise-shaped dithering at 16 bits");
					device->write_via_copy = device->quirk_bswap?
						sample_move_dither_shaped_d16_sSs:
						sample_move_dither_shaped_d16_sS;
					break;

				default:
					device->write_via_copy = device->quirk_bswap?
						sample_move_d16_sSs :
						sample_move_d16_sS;
					break;
				}
				break;

			case 3: /* NO DITHER */
				device->write_via_copy = device->quirk_bswap?
					sample_move_d24_sSs:
					sample_move_d24_sS;

				break;

			case 4: /* NO DITHER */
				device->write_via_copy = device->quirk_bswap?
					sample_move_d32u24_sSs:
					sample_move_d32u24_sS;
				break;

			default:
				jack_error ("impossible sample width (%d) discovered!",
						device->playback_sample_bytes);
				exit (1);
			}
		}
	}

	if (device->capture_handle) {
		if (SND_PCM_FORMAT_FLOAT_LE == device->capture_sample_format) {
			device->read_via_copy = sample_move_floatLE_sSs;
		} else {
			switch (device->capture_sample_bytes) {
			case 2:
				device->read_via_copy = device->quirk_bswap?
					sample_move_dS_s16s:
					sample_move_dS_s16;
				break;
			case 3:
				device->read_via_copy = device->quirk_bswap?
					sample_move_dS_s24s:
					sample_move_dS_s24;
				break;
			case 4:
				device->read_via_copy = device->quirk_bswap?
					sample_move_dS_s32u24s:
					sample_move_dS_s32u24;
				break;
			}
		}
	}
}

#ifdef __QNXNTO__

static int
alsa_driver_allocate_buffer(alsa_driver_t *driver, alsa_device_t *device, int frames, int channels, bool is_capture)
{
	const long ALIGNMENT = 32;

	// TODO driver->playback_sample_bytes
	char* const fBuffer = malloc(channels * ((sizeof(alsa_driver_default_format_t)) * frames) + ALIGNMENT);
	if(fBuffer) {
		/* Provide an 32 byte aligned buffer */
		char* const aligned_buffer = (char*)((uintptr_t)fBuffer & ~(ALIGNMENT-1)) + ALIGNMENT;

		if(is_capture) {
			device->capture_areas_ptr = fBuffer;
			device->capture_areas = aligned_buffer;
		} else {
			device->playback_areas_ptr = fBuffer;
			device->playback_areas = aligned_buffer;
		}

		return 0;
	}

	jack_error ("ALSA: could not allocate audio buffer");
	return -1;
}

static int
alsa_driver_get_setup (alsa_driver_t *driver, alsa_device_t *device, snd_pcm_channel_setup_t *setup, bool is_capture)
{
	int err = 0;

	memset(setup, 0, sizeof(*setup));
	setup->channel = is_capture;

	if(is_capture) {
		err = snd_pcm_plugin_setup(device->capture_handle, setup);
	} else {
		err = snd_pcm_plugin_setup(device->playback_handle, setup);
	}
	if (err < 0) {
		jack_error("couldn't get channel setup for %s, err = %s ",
			   is_capture ? device->capture_name : device->playback_name,
			   strerror(err));
		return -1;
	}

	return 0;
}

static int
alsa_driver_configure_stream (alsa_driver_t *driver, alsa_device_t *device, char *device_name,
			      const char *stream_name,
			      snd_pcm_t *handle,
			      unsigned int *nperiodsp,
			      channel_t *nchns,
			      unsigned long preferred_sample_bytes,
			      bool is_capture)
{
	int err = 0;
	snd_pcm_channel_info_t ch_info;
	snd_pcm_channel_params_t ch_params;
	const unsigned long sample_size = is_capture ? device->capture_sample_bytes
	                                             : device->playback_sample_bytes;

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

	ch_params.format.format = (preferred_sample_bytes == 4) ? SND_PCM_SFMT_S32_LE : SND_PCM_SFMT_S16_LE;

	ch_params.mode = SND_PCM_MODE_BLOCK;
	ch_params.start_mode = SND_PCM_START_GO;
	ch_params.stop_mode = SND_PCM_STOP_STOP;
	ch_params.buf.block.frag_size = driver->frames_per_cycle * *nchns * snd_pcm_format_width(ch_params.format.format) / 8;

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

	/*Set the configurable parameters for a PCM channel*/
	if ((err = snd_pcm_plugin_params(handle, &ch_params)) < 0) {
		jack_error("snd_pcm_plugin_params failed for %s %s with err = (%s)", snd_strerror(err), stream_name, device_name);
		return -1;
	}

	/*
	 * The buffer has to be able to hold a full HW audio buffer
	 * (periods * period_size) because the silence prefill will fill the
	 * complete buffer
	 */
	return alsa_driver_allocate_buffer(driver, device, driver->frames_per_cycle * *nperiodsp, *nchns, is_capture);
}
#else
static int
alsa_driver_configure_stream (alsa_driver_t *driver, alsa_device_t *device, char *device_name,
			      const char *stream_name,
			      snd_pcm_t *handle,
			      snd_pcm_hw_params_t *hw_params,
			      snd_pcm_sw_params_t *sw_params,
			      unsigned int *nperiodsp,
			      channel_t *nchns,
			      unsigned long preferred_sample_bytes)
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

	format = (preferred_sample_bytes == 4) ? 0 : NUMFORMATS - 1;

	while (1) {
		if ((err = snd_pcm_hw_params_set_format (
			     handle, hw_params, formats[format].format)) < 0) {

			if ((preferred_sample_bytes == 4
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
				device->quirk_bswap = 1;
			} else {
				device->quirk_bswap = 0;
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

	if (handle == device->playback_handle)
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

static int
alsa_driver_set_parameters (alsa_driver_t *driver,
			    alsa_device_t *device,
			    int do_capture,
			    int do_playback,
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

	jack_info ("configuring C: '%s' P: '%s' %" PRIu32 "Hz, period = %"
		 PRIu32 " frames (%.1f ms), buffer = %" PRIu32 " periods",
		 device->capture_name != NULL ? device->capture_name : "-", device->playback_name != NULL ? device->playback_name : "-",
		 rate, frames_per_cycle, (((float)frames_per_cycle / (float) rate) * 1000.0f), user_nperiods);

	if (do_capture) {
		if (!device->capture_handle) {
			jack_error ("ALSA: pcm capture handle not available");
			return -1;
		}
#ifdef __QNXNTO__
		err = alsa_driver_configure_stream (
			driver,
			device,
			device->capture_name,
			"capture",
			device->capture_handle,
			&driver->capture_nperiods,
			&device->capture_nchannels,
			driver->preferred_sample_bytes,
			SND_PCM_CHANNEL_CAPTURE);

		if (err) {
			jack_error ("ALSA: cannot configure capture channel");
			return -1;
		}

		err = alsa_driver_get_setup(driver, device, &c_setup, SND_PCM_CHANNEL_CAPTURE);
		if(err < 0) {
			jack_error ("ALSA: get setup failed");
			return -1;
		}

		cr = c_setup.format.rate;
		c_periods = c_setup.buf.block.frags;
		device->capture_sample_format = c_setup.format.format;
		device->capture_interleaved = c_setup.format.interleave;
		device->capture_sample_bytes = snd_pcm_format_width (c_setup.format.format) / 8;
		c_period_size = c_setup.buf.block.frag_size / device->capture_nchannels / device->capture_sample_bytes;
#else
		err = alsa_driver_configure_stream (
			driver,
			device,
			device->capture_name,
			"capture",
			device->capture_handle,
			driver->capture_hw_params,
			driver->capture_sw_params,
			&driver->capture_nperiods,
			&device->capture_nchannels,
			driver->preferred_sample_bytes);

		if (err) {
			jack_error ("ALSA: cannot configure capture channel");
			return -1;
		}

		snd_pcm_hw_params_get_rate (driver->capture_hw_params,
						&cr, &dir);

		snd_pcm_access_t access;

		err = snd_pcm_hw_params_get_period_size (
			driver->capture_hw_params, &c_period_size, &dir);
		err = snd_pcm_hw_params_get_format (
			driver->capture_hw_params,
		&(device->capture_sample_format));
		err = snd_pcm_hw_params_get_access (driver->capture_hw_params,
						    &access);
		device->capture_interleaved =
			(access == SND_PCM_ACCESS_MMAP_INTERLEAVED)
			|| (access == SND_PCM_ACCESS_MMAP_COMPLEX);
		device->capture_sample_bytes = snd_pcm_format_physical_width (device->capture_sample_format) / 8;
#endif
		if (err) {
			jack_error ("ALSA: cannot configure capture channel");
			return -1;
		}
	}

	if (do_playback) {
		if (!device->playback_handle) {
			jack_error ("ALSA: pcm playback handle not available");
			return -1;
		}
#ifdef __QNXNTO__
		err = alsa_driver_configure_stream (
			driver,
			device,
			device->playback_name,
			"playback",
			device->playback_handle,
			&driver->playback_nperiods,
			&device->playback_nchannels,
			driver->preferred_sample_bytes,
			SND_PCM_CHANNEL_PLAYBACK);

		if (err) {
			jack_error ("ALSA: cannot configure playback channel");
			return -1;
		}

		err = alsa_driver_get_setup(driver, device, &p_setup, SND_PCM_CHANNEL_PLAYBACK);
		if(err < 0) {
			jack_error ("ALSA: get setup failed");
			return -1;
		}

		pr = p_setup.format.rate;
		p_periods = p_setup.buf.block.frags;
		device->playback_sample_format = p_setup.format.format;
		device->playback_interleaved = p_setup.format.interleave;
		device->playback_sample_bytes = snd_pcm_format_width (p_setup.format.format) / 8;
		p_period_size = p_setup.buf.block.frag_size / device->playback_nchannels / device->playback_sample_bytes;
#else

		err = alsa_driver_configure_stream (
			driver,
			device,
			device->playback_name,
			"playback",
			device->playback_handle,
			driver->playback_hw_params,
			driver->playback_sw_params,
			&driver->playback_nperiods,
			&device->playback_nchannels,
			driver->preferred_sample_bytes);

		if (err) {
			jack_error ("ALSA: cannot configure playback channel");
			return -1;
		}

		/* check the rate, since that's rather important */
		snd_pcm_hw_params_get_rate (driver->playback_hw_params,
					    &pr, &dir);

		snd_pcm_access_t access;

		err = snd_pcm_hw_params_get_period_size (
			driver->playback_hw_params, &p_period_size, &dir);
		err = snd_pcm_hw_params_get_format (
			driver->playback_hw_params,
		&(device->playback_sample_format));
		err = snd_pcm_hw_params_get_access (driver->playback_hw_params,
						    &access);
		device->playback_interleaved =
			(access == SND_PCM_ACCESS_MMAP_INTERLEAVED)
			|| (access == SND_PCM_ACCESS_MMAP_COMPLEX);
		device->playback_sample_bytes = snd_pcm_format_physical_width (device->playback_sample_format) / 8;
#endif
	}

	/* original checks done for single device mode */
	if (driver->devices_count == 1) {
		if (do_capture && do_playback) {
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
		else if (do_capture && cr != driver->frame_rate) {
			jack_error ("capture sample rate in use (%d Hz) does not "
					"match requested rate (%d Hz)",
					cr, driver->frame_rate);
			driver->frame_rate = cr;
		}
		else if (do_playback && pr != driver->frame_rate) {
			jack_error ("playback sample rate in use (%d Hz) does not "
					"match requested rate (%d Hz)",
					pr, driver->frame_rate);
			driver->frame_rate = pr;
		}
	} else {
		if (do_capture && cr != driver->frame_rate) {
			jack_error ("capture sample rate in use (%d Hz) does not "
					"match requested rate (%d Hz)",
					cr, driver->frame_rate);
			return -1;
		}
		if (do_playback && pr != driver->frame_rate) {
			jack_error ("playback sample rate in use (%d Hz) does not "
					"match requested rate (%d Hz)",
					pr, driver->frame_rate);
			return -1;
		}
	}


	/* check the fragment size, since that's non-negotiable */

	if (do_playback) {
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

	if (do_capture) {
#ifndef __QNXNTO__
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

	if (do_playback) {
		err = alsa_driver_check_format(device->playback_sample_format);
		if(err < 0) {
			jack_error ("programming error: unhandled format "
				    "type for playback");
			return -1;
		}
	}

	if (do_capture) {
		err = alsa_driver_check_format(device->capture_sample_format);
		if(err < 0) {
			jack_error ("programming error: unhandled format "
				    "type for capture");
			return -1;
		}
	}

	if (device->playback_interleaved && do_playback) {
#ifndef __QNXNTO__
		const snd_pcm_channel_area_t *my_areas;
		snd_pcm_uframes_t offset, frames;
		if (snd_pcm_mmap_begin(device->playback_handle,
				       &my_areas, &offset, &frames) < 0) {
			jack_error ("ALSA: %s: mmap areas info error",
				    device->playback_name);
			return -1;
		}

		// TODO does not work for capture only
		device->interleave_unit =
			snd_pcm_format_physical_width (
				device->playback_sample_format) / 8;
#else
		device->interleave_unit = snd_pcm_format_width(
		            device->playback_sample_format) / 8;
#endif
	} else if (do_playback) {
		device->interleave_unit = 0;  /* NOT USED */
	}

#ifndef __QNXNTO__
	if (device->capture_interleaved && do_capture) {
		const snd_pcm_channel_area_t *my_areas;
		snd_pcm_uframes_t offset, frames;
		if (snd_pcm_mmap_begin(device->capture_handle,
				       &my_areas, &offset, &frames) < 0) {
			jack_error ("ALSA: %s: mmap areas info error",
				    device->capture_name);
			return -1;
		}
	}
#endif

	alsa_driver_setup_io_function_pointers (driver, device);

	/* do only on first start */
	if (device->max_nchannels == 0) {

		/* Allocate and initialize structures that rely on the
		   channels counts.

		   Set up the bit pattern that is used to record which
		   channels require action on every cycle. any bits that are
		   not set after the engine's process() call indicate channels
		   that potentially need to be silenced.
		*/

		device->max_nchannels = device->playback_nchannels > device->capture_nchannels ?
			device->playback_nchannels : device->capture_nchannels;

		/* device local channel offset to offsets in driver, used by Jack2 */
		device->capture_channel_offset = driver->capture_nchannels;
		device->playback_channel_offset = driver->playback_nchannels;

		driver->capture_nchannels += device->capture_nchannels;
		driver->playback_nchannels += device->playback_nchannels;

		bitset_create (&device->channels_done, device->max_nchannels);
		bitset_create (&device->channels_not_done, device->max_nchannels);

		if (device->playback_name) {
			device->playback_addr = (char **)
				malloc (sizeof (char *) * device->playback_nchannels);
			memset (device->playback_addr, 0,
				sizeof (char *) * device->playback_nchannels);
			device->playback_interleave_skip = (unsigned long *)
				malloc (sizeof (unsigned long *) * device->playback_nchannels);
			memset (device->playback_interleave_skip, 0,
				sizeof (unsigned long *) * device->playback_nchannels);
			device->silent = (unsigned long *)
				malloc (sizeof (unsigned long)
					* device->playback_nchannels);

			for (chn = 0; chn < device->playback_nchannels; chn++) {
				device->silent[chn] = 0;
			}

			for (chn = 0; chn < device->playback_nchannels; chn++) {
				bitset_add (device->channels_done, chn);
			}

			driver->dither_state = (dither_state_t *) calloc (device->playback_nchannels, sizeof (dither_state_t));
		}

		if (device->capture_name) {
			device->capture_addr = (char **)
				malloc (sizeof (char *) * device->capture_nchannels);
			memset (device->capture_addr, 0,
				sizeof (char *) * device->capture_nchannels);
			device->capture_interleave_skip = (unsigned long *)
				malloc (sizeof (unsigned long *) * device->capture_nchannels);
			memset (device->capture_interleave_skip, 0,
				sizeof (unsigned long *) * device->capture_nchannels);
		}
	}

	driver->period_usecs =
		(jack_time_t) floor ((((float) driver->frames_per_cycle) /
				      driver->frame_rate) * 1000000.0f);
	driver->poll_timeout_ms = (int) (1.5f * (driver->period_usecs / 1000.0f) + 0.5f);

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
	int err = 0;

	jack_info ("reset parameters");

	/* XXX unregister old ports ? */
	for (int i = 0; i < driver->devices_count; ++i) {
		alsa_device_t *device = &driver->devices[i];
		alsa_driver_release_channel_dependent_memory (driver, device);
		if ((err = alsa_driver_set_parameters (driver, device, 1, 1, frames_per_cycle, user_nperiods, rate)) != 0) {
			return err;
		}
	}

	return err;
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
				   alsa_device_t *device,
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
			     device->capture_handle, &device->capture_areas,
			     (snd_pcm_uframes_t *) capture_offset,
			     (snd_pcm_uframes_t *) capture_avail)) < 0) {
			jack_error ("ALSA: %s: mmap areas info error",
				    device->capture_name);
			return -1;
		}

		for (chn = 0; chn < device->capture_nchannels; chn++) {
			const snd_pcm_channel_area_t *a =
				&device->capture_areas[chn];
			device->capture_addr[chn] = (char *) a->addr
				+ ((a->first + a->step * *capture_offset) / 8);
			device->capture_interleave_skip[chn] = (unsigned long ) (a->step / 8);
		}
#else
		for (chn = 0; chn < device->capture_nchannels; chn++) {
			char* const a = device->capture_areas;
			if (device->capture_interleaved) {
				device->capture_addr[chn] = &a[chn * device->capture_sample_bytes];
				device->capture_interleave_skip[chn] = device->capture_nchannels *
				        device->capture_sample_bytes;
			} else {
				device->capture_addr[chn] = &a[chn *
				        device->capture_sample_bytes * driver->frames_per_cycle];
				device->capture_interleave_skip[chn] = device->capture_sample_bytes;
			}
		}
#endif
	}

	if (playback_avail) {
#ifndef __QNXNTO__
		int err;
		if ((err = snd_pcm_mmap_begin (
			     device->playback_handle, &device->playback_areas,
			     (snd_pcm_uframes_t *) playback_offset,
			     (snd_pcm_uframes_t *) playback_avail)) < 0) {
			jack_error ("ALSA: %s: mmap areas info error ",
				    device->playback_name);
			return -1;
		}

		for (chn = 0; chn < device->playback_nchannels; chn++) {
			const snd_pcm_channel_area_t *a =
				&device->playback_areas[chn];
			device->playback_addr[chn] = (char *) a->addr
				+ ((a->first + a->step * *playback_offset) / 8);
			device->playback_interleave_skip[chn] = (unsigned long ) (a->step / 8);
		}
#else
		for (chn = 0; chn < device->playback_nchannels; chn++) {
			char* const a = device->playback_areas;
			if (device->playback_interleaved) {
				device->playback_addr[chn] = &a[chn * device->playback_sample_bytes];
				device->playback_interleave_skip[chn] = device->playback_nchannels *
				        device->playback_sample_bytes;
			} else {
				device->playback_addr[chn] = &a[chn *
				        device->playback_sample_bytes * driver->frames_per_cycle];
				device->playback_interleave_skip[chn] = device->playback_sample_bytes;
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
alsa_driver_open (alsa_driver_t *driver)
{
	int err = 0;

	driver->poll_last = 0;
	driver->poll_next = 0;

	for (int i = 0; i < driver->devices_count; ++i) {
		alsa_device_t *device = &driver->devices[i];
		int do_capture = 0, do_playback = 0;

		if (!device->capture_handle && (i < driver->devices_c_count) && (device->capture_target_state != SND_PCM_STATE_NOTREADY)) {
			jack_info("open C: %s", device->capture_name);

			err = alsa_driver_open_device (driver, &driver->devices[i], SND_PCM_STREAM_CAPTURE);
			if (err < 0) {
				jack_error ("\n\nATTENTION: Opening of the capture device \"%s\" failed.",
						driver->devices[i].capture_name);
				return -1;
			}

			do_capture = 1;
		}

		if (!device->playback_handle && (i < driver->devices_p_count) && (device->playback_target_state != SND_PCM_STATE_NOTREADY)) {
			jack_info("open P: %s", device->playback_name);

			err = alsa_driver_open_device (driver, &driver->devices[i], SND_PCM_STREAM_PLAYBACK);
			if (err < 0) {
				jack_error ("\n\nATTENTION: Opening of the playback device \"%s\" failed.",
						driver->devices[i].playback_name);
				return -1;
			}

			do_playback = 1;
		}

		if (alsa_driver_set_parameters (driver, device, do_capture, do_playback, driver->frames_per_cycle, driver->user_nperiods, driver->frame_rate)) {
			jack_error ("ALSA: failed to set parameters");
			return -1;
		}
	}

	if (!(driver->features & ALSA_DRIVER_FEAT_UNLINKED_DEVS)) {
		jack_info ("alsa driver linking enabled");
		alsa_driver_link(driver);
	} else {
		jack_info ("alsa driver linking disabled");
	}

	return 0;
}

static int
alsa_driver_link (alsa_driver_t *driver)
{
	snd_pcm_t *group_handle = NULL;

	for (int i = 0; i < driver->devices_c_count; ++i) {
		alsa_device_t *device = &driver->devices[i];

		if (!device->capture_handle) {
			continue;
		}

		jack_info("link C: %s", device->capture_name);

		if (device->capture_target_state != SND_PCM_STATE_RUNNING) {
			jack_info("link skipped, device unused");
			continue;
		}

		if (group_handle == NULL) {
			jack_info("link, device is group master");
			group_handle = device->capture_handle;
			device->capture_linked = 1;
			continue;
		}

		if (device->capture_linked) {
			jack_info("link skipped, already done");
			continue;
		}

		if (group_handle == device->capture_handle) {
			jack_info("link skipped, master already done");
			device->capture_linked = 1;
			continue;
		}

		if (snd_pcm_link (group_handle, device->capture_handle) != 0) {
			jack_error ("failed to add device to link group C: '%s'", device->capture_name);
			continue;
		}
		device->capture_linked = 1;
	}

	for (int i = 0; i < driver->devices_p_count; ++i) {
		alsa_device_t *device = &driver->devices[i];

		if (!device->playback_handle) {
			continue;
		}

		jack_info("link P: %s", device->playback_name);

		if (device->playback_target_state != SND_PCM_STATE_RUNNING) {
			jack_info("link skipped, device unused");
			continue;
		}

		if (group_handle == NULL) {
			jack_info("link, device is group master");
			group_handle = device->playback_handle;
			device->playback_linked = 1;
			continue;
		}

		if (device->playback_linked) {
			jack_info("link skipped, already done");
			continue;
		}

		if (group_handle == device->playback_handle) {
			jack_info("link skipped, master already done");
			device->playback_linked = 1;
			continue;
		}

		if (snd_pcm_link (group_handle, device->playback_handle) != 0) {
			jack_error ("failed to add device to link group P: '%s'", device->playback_name);
			continue;
		}
		device->playback_linked = 1;
	}

	return 0;
}

int
alsa_driver_start (alsa_driver_t *driver)
{
	int err;
	snd_pcm_uframes_t poffset, pavail;
	channel_t chn;

	driver->capture_nfds = 0;
	driver->playback_nfds = 0;

	int group_done = 0;

	for (int i = 0; i < driver->devices_c_count; ++i) {
		alsa_device_t *device = &driver->devices[i];

		if (!device->capture_handle) {
			continue;
		}

		jack_info("prepare C: %s", device->capture_name);

		if (device->capture_target_state == SND_PCM_STATE_NOTREADY) {
			jack_info("prepare skipped, device unused");
			continue;
		}

		if (device->capture_target_state == SND_PCM_STATE_RUNNING) {
			driver->capture_nfds += snd_pcm_poll_descriptors_count (device->capture_handle);
		}

		if (group_done && device->capture_linked) {
			jack_info("prepare skipped, already done by link group");
			continue;
		}

		if (alsa_driver_get_state(device->capture_handle, 1) == SND_PCM_STATE_PREPARED) {
			jack_info("prepare skipped, already prepared");
			continue;
		}

		if (device->capture_linked) {
			group_done = 1;
		}

		if ((err = alsa_driver_prepare (device->capture_handle, SND_PCM_STREAM_CAPTURE)) < 0) {
			jack_error ("ALSA: failed to prepare device '%s' (%s)", device->capture_name, snd_strerror(err));
			return -1;
		}
	}

	for (int i = 0; i < driver->devices_p_count; ++i) {
		alsa_device_t *device = &driver->devices[i];

		if (!device->playback_handle) {
			continue;
		}

		jack_info("prepare P: %s", device->playback_name);

		if (device->playback_target_state == SND_PCM_STATE_NOTREADY) {
			jack_info("prepare skipped, device unused");
			continue;
		}

		if (device->playback_target_state == SND_PCM_STATE_RUNNING) {
			driver->playback_nfds += snd_pcm_poll_descriptors_count (device->playback_handle);
		}

		if (group_done && device->playback_linked) {
			jack_info("prepare skipped, already done by link group");
			continue;
		}

		if (alsa_driver_get_state(device->playback_handle, 0) == SND_PCM_STATE_PREPARED) {
			jack_info("prepare skipped, already prepared");
			continue;
		}

		if (device->playback_linked) {
			group_done = 1;
		}

		if ((err = alsa_driver_prepare (device->playback_handle, SND_PCM_STREAM_PLAYBACK)) < 0) {
			jack_error ("ALSA: failed to prepare device '%s' (%s)", device->playback_name, snd_strerror(err));
			return -1;
		}
	}

	// TODO amiartus
//	if (driver->hw_monitoring) {
//		if (driver->input_monitor_mask || driver->all_monitor_in) {
//			if (driver->all_monitor_in) {
//				driver->hw->set_input_monitor_mask (driver->hw, ~0U);
//			} else {
//				driver->hw->set_input_monitor_mask (
//					driver->hw, driver->input_monitor_mask);
//			}
//		} else {
//			driver->hw->set_input_monitor_mask (driver->hw,
//							    driver->input_monitor_mask);
//		}
//	}

	if (driver->pfd) {
		free (driver->pfd);
	}

	driver->pfd = (struct pollfd *)
		malloc (sizeof (struct pollfd) *
			(driver->playback_nfds + driver->capture_nfds + 2));

	if (driver->midi && !driver->xrun_recovery)
		(driver->midi->start)(driver->midi);

	for (int i = 0; i < driver->devices_p_count; ++i) {
		alsa_device_t *device = &driver->devices[i];

		if (!device->playback_handle) {
			continue;
		}

		jack_info("silence P: %s", device->playback_name);

		if (device->playback_target_state != SND_PCM_STATE_RUNNING) {
			jack_info("silence skipped, device unused");
			continue;
		}

		const jack_nframes_t silence_frames = driver->frames_per_cycle *
		        driver->playback_nperiods;
		/* fill playback buffer with zeroes, and mark
		   all fragments as having data.
		*/

#ifndef __QNXNTO__
		pavail = snd_pcm_avail_update (device->playback_handle);
		if (pavail != silence_frames) {
			jack_error ("ALSA: full buffer not available at start");
			return -1;
		}
#endif

		if (alsa_driver_get_channel_addresses (driver, device,
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

		for (chn = 0; chn < device->playback_nchannels; chn++) {
			alsa_driver_silence_on_channel (
				driver, device, chn, silence_frames);
		}

#ifdef __QNXNTO__
		const size_t bytes = silence_frames * device->playback_nchannels *
		        device->playback_sample_bytes;
		if ((err = snd_pcm_plugin_write(device->playback_handle,
					       device->playback_areas, bytes)) < bytes) {
			jack_error ("ALSA: could not complete silence %s of %"
				PRIu32 " frames: %u, error = %d", device->playback_name, silence_frames, err, errno);
			return -1;
		}
#else
		snd_pcm_mmap_commit (device->playback_handle, poffset, silence_frames);
#endif
	}

	group_done = 0;

	for (int i = 0; i < driver->devices_c_count; ++i) {
		alsa_device_t *device = &driver->devices[i];

		if (!device->capture_handle) {
			continue;
		}

		jack_info("start C: %s", device->capture_name);

		if (device->capture_target_state != SND_PCM_STATE_RUNNING) {
			jack_info("start skipped, device unused");
			continue;
		}

		if (group_done && device->capture_linked) {
			jack_info("start skipped, already done by link group");
			continue;
		}

		if (device->capture_linked) {
			group_done = 1;
		}

		if ((err = alsa_driver_stream_start (device->capture_handle, SND_PCM_STREAM_CAPTURE)) < 0) {
			jack_error ("ALSA: failed to start device C: '%s' (%s)", device->capture_name,
					snd_strerror(err));
			return -1;
		}
	}

	for (int i = 0; i < driver->devices_p_count; ++i) {
		alsa_device_t *device = &driver->devices[i];

		if (!device->playback_handle) {
			continue;
		}

		jack_info("start P: %s", device->playback_name);

		if (device->playback_target_state != SND_PCM_STATE_RUNNING) {
			jack_info("start skipped, device unused");
			continue;
		}

		if (group_done && device->playback_linked) {
			jack_info("start skipped, already done by link group");
			continue;
		}

		if (device->playback_linked) {
			group_done = 1;
		}

		if ((err = alsa_driver_stream_start (device->playback_handle, SND_PCM_STREAM_PLAYBACK)) < 0) {
			jack_error ("ALSA: failed to start device P: '%s' (%s)", device->playback_name,
					snd_strerror(err));
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

	int group_done = 0;

	for (int i = 0; i < driver->devices_c_count; ++i) {
		alsa_device_t *device = &driver->devices[i];
		if (!device->capture_handle) {
			continue;
		}

		jack_info("stop C: %s", device->capture_name);

		if (group_done && device->capture_linked) {
			jack_info("stop skipped, already done by link group");
			continue;
		}

		if (alsa_driver_get_state(device->capture_handle, 1) != SND_PCM_STATE_RUNNING) {
			jack_info("stop skipped, device not running");
			continue;
		}

#ifdef __QNXNTO__
		/* In case of capture: Flush discards the frames */
		err = snd_pcm_plugin_flush(device->capture_handle, SND_PCM_CHANNEL_CAPTURE);
#else
		if (device->capture_linked) {
			group_done = 1;
		}

		err = snd_pcm_drop (device->capture_handle);
#endif
		if (err < 0) {
			jack_error ("ALSA: failed to flush device (%s)", snd_strerror (err));
			return -1;
		}
	}

	for (int i = 0; i < driver->devices_p_count; ++i) {
		alsa_device_t *device = &driver->devices[i];
		if (!device->playback_handle) {
			continue;
		}

		jack_info("stop P: %s", device->playback_name);

		if (group_done && device->playback_linked) {
			jack_info("stop skipped, already done by link group");
			continue;
		}

		if (alsa_driver_get_state(device->playback_handle, 0) != SND_PCM_STATE_RUNNING) {
			jack_info("stop skipped, device not running");
			continue;
		}

#ifdef __QNXNTO__
		/* In case of playback: Drain discards the frames */
		err = snd_pcm_plugin_playback_drain(device->playback_handle);
#else
		if (device->playback_linked) {
			group_done = 1;
		}

		err = snd_pcm_drop (device->playback_handle);
#endif
		if (err < 0) {
			jack_error ("ALSA: failed to flush device (%s)", snd_strerror (err));
			return -1;
		}
	}

// TODO: amiartus
//	if (driver->hw_monitoring) {
//		driver->hw->set_input_monitor_mask (driver->hw, 0);
//	}

//	if (driver->midi && !driver->xrun_recovery)
//		(driver->midi->stop)(driver->midi);

	return 0;
}

int
alsa_driver_close (alsa_driver_t *driver)
{
	for (int i = 0; i < driver->devices_c_count; ++i) {
		alsa_device_t *device = &driver->devices[i];
		if (!device->capture_handle) {
			continue;
		}

		if (device->capture_linked) {
			snd_pcm_unlink(device->capture_handle);
			device->capture_linked = 0;
		}

		if (device->capture_target_state != SND_PCM_STATE_NOTREADY) {
			continue;
		}

		device->capture_sample_bytes = 0;
		device->capture_sample_format = SND_PCM_FORMAT_UNKNOWN;

		snd_pcm_close(device->capture_handle);
		device->capture_handle = NULL;
	}

	for (int i = 0; i < driver->devices_p_count; ++i) {
		alsa_device_t *device = &driver->devices[i];
		if (!device->playback_handle) {
			continue;
		}


		if (device->playback_linked) {
			snd_pcm_unlink(device->playback_handle);
			device->playback_linked = 0;
		}

		if (device->playback_target_state != SND_PCM_STATE_NOTREADY) {
			continue;
		}

		device->playback_sample_bytes = 0;
		device->playback_sample_format = SND_PCM_FORMAT_UNKNOWN;

		snd_pcm_close(device->playback_handle);
		device->playback_handle = NULL;
	}

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
alsa_driver_get_state (snd_pcm_t *handle, int is_capture)
{
#ifdef __QNXNTO__
	int res;
	snd_pcm_channel_status_t status;

	memset (&status, 0, sizeof (status));
	status.channel = is_capture;
	res = snd_pcm_plugin_status(handle, &status);
	if (res < 0) {
		jack_error("status error: %s", snd_strerror(res));
		return -1;
	}
	return status.status;
#else
	return snd_pcm_state(handle);
#endif
}

int
alsa_driver_xrun_recovery (alsa_driver_t *driver, float *delayed_usecs)
{
	int state;

	for (int i = 0; i < driver->devices_count; ++i) {
		alsa_device_t *device = &driver->devices[i];
		if (device->capture_handle) {
			state = alsa_driver_get_state(device->capture_handle, SND_PCM_STREAM_CAPTURE);
			// TODO overrun
			if (state == SND_PCM_STATE_XRUN) {
				driver->xrun_count++;
#ifdef __QNXNTO__
				/* Timestamp api's are not available as per QNX Documentation */
				*delayed_usecs = 0;
#else
				snd_pcm_status_t *status;
				snd_pcm_status_alloca(&status);
				snd_pcm_status(device->capture_handle, status);
				struct timeval now, diff, tstamp;
				snd_pcm_status_get_tstamp(status,&now);
				snd_pcm_status_get_trigger_tstamp(status, &tstamp);
				timersub(&now, &tstamp, &diff);
				*delayed_usecs = diff.tv_sec * 1000000.0 + diff.tv_usec;
#endif
				jack_log("**** alsa_pcm: xrun of at least %.3f msecs",*delayed_usecs / 1000.0);
			}
		}

		if (device->playback_handle) {
			state = alsa_driver_get_state(device->playback_handle, SND_PCM_STREAM_PLAYBACK);
			// TODO overrun
			if (state == SND_PCM_STATE_XRUN) {
				driver->xrun_count++;
#ifdef __QNXNTO__
				/* Timestamp api's are not available as per QNX Documentation */
				*delayed_usecs = 0;
#else
				snd_pcm_status_t *status;
				snd_pcm_status_alloca(&status);
				snd_pcm_status(device->playback_handle, status);
				struct timeval now, diff, tstamp;
				snd_pcm_status_get_tstamp(status,&now);
				snd_pcm_status_get_trigger_tstamp(status, &tstamp);
				timersub(&now, &tstamp, &diff);
				*delayed_usecs = diff.tv_sec * 1000000.0 + diff.tv_usec;
#endif
				jack_log("**** alsa_pcm: xrun of at least %.3f msecs",*delayed_usecs / 1000.0);
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
alsa_driver_silence_untouched_channels (alsa_driver_t *driver, alsa_device_t *device,
					jack_nframes_t nframes)
{
	channel_t chn;
	jack_nframes_t buffer_frames =
		driver->frames_per_cycle * driver->playback_nperiods;

	for (chn = 0; chn < device->playback_nchannels; chn++) {
		if (bitset_contains (device->channels_not_done, chn)) {
			if (device->silent[chn] < buffer_frames) {
				alsa_driver_silence_on_channel_no_mark (
					driver, device, chn, nframes);
				device->silent[chn] += nframes;
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

	return snd_pcm_poll_descriptors_count(pcm);
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
alsa_driver_wait (alsa_driver_t *driver, int extra_fd, alsa_driver_wait_status_t *status, float
		  *delayed_usecs)
{
	snd_pcm_sframes_t avail = 0;
	snd_pcm_sframes_t capture_avail = 0;
	snd_pcm_sframes_t playback_avail = 0;
	jack_time_t poll_enter;
	jack_time_t poll_ret = 0;

	*status = -1;
	*delayed_usecs = 0;

	int cap_revents[driver->devices_c_count];
	memset(cap_revents, 0, sizeof(cap_revents));
	int play_revents[driver->devices_p_count];
	memset(play_revents, 0, sizeof(play_revents));

	int pfd_cap_count[driver->devices_c_count];
	int pfd_play_count[driver->devices_p_count];
	/* In case if extra_fd is positive number then should be added to pfd_count
	 * since at present extra_fd is always negative this is not changed now.
	 */
	int pfd_count = driver->capture_nfds + driver->playback_nfds;

	/* special case where all devices are stopped */
	if (pfd_count == 0) {

		driver->poll_last = jack_get_microseconds ();

		if (driver->poll_next > driver->poll_last) {
			struct timespec duration, remain;
			duration.tv_sec = 0;
			duration.tv_nsec = (int64_t) ((driver->poll_next - driver->poll_last) * 1000);
			nanosleep(&duration, &remain);

			driver->poll_last = jack_get_microseconds ();
		}

		SetTime(driver->poll_last);
		driver->poll_next = driver->poll_last + driver->period_usecs;

		*status = ALSA_DRIVER_WAIT_OK;

		return driver->frames_per_cycle;
	}

	while (pfd_count > 0) {
		int poll_result;
		int pfd_index = 0;

		/* collect capture poll descriptors */
		for (int i = 0; i < driver->devices_c_count; ++i) {
			/* this device already triggered poll event before */
			if (cap_revents[i]) {
				continue;
			}

			alsa_device_t *device = &driver->devices[i];
			if (!device->capture_handle) {
				continue;
			}

			if (device->capture_target_state != SND_PCM_STATE_RUNNING) {
				continue;
			}

			pfd_cap_count[i] = alsa_driver_poll_descriptors (device->capture_handle,
				&driver->pfd[pfd_index],
				pfd_count - pfd_index,
				SND_PCM_STREAM_CAPTURE);
			if (pfd_cap_count[i] < 0) {
				/* In case of xrun -EPIPE is returned perform xrun recovery*/
				if (pfd_cap_count[i] == -EPIPE) {
					jack_error("poll descriptors xrun C: %s pfd_cap_count[%d]=%d", device->capture_name, i, pfd_cap_count[i]);
					*status = ALSA_DRIVER_WAIT_XRUN;
					return 0;
				}
				/* for any other error return negative wait status to caller */
				jack_error("poll descriptors error C: %s pfd_cap_count[%d]=%d", device->capture_name, i, pfd_cap_count[i]);
				*status = ALSA_DRIVER_WAIT_ERROR;
				return 0;
			} else {
				pfd_index += pfd_cap_count[i];
			}
		}

		/* collect playback poll descriptors */
		for (int i = 0; i < driver->devices_p_count; ++i) {
			/* this device already triggered poll event before */
			if (play_revents[i]) {
				continue;
			}

			alsa_device_t *device = &driver->devices[i];
			if (!device->playback_handle) {
				continue;
			}

			if (device->playback_target_state != SND_PCM_STATE_RUNNING) {
				continue;
			}

			pfd_play_count[i] = alsa_driver_poll_descriptors (device->playback_handle,
				&driver->pfd[pfd_index],
				pfd_count - pfd_index,
				SND_PCM_STREAM_PLAYBACK);
			if (pfd_play_count[i] < 0) {
				/* In case of xrun -EPIPE is returned perform xrun recovery*/
				if (pfd_cap_count[i] == -EPIPE) {
					jack_error("poll descriptors xrun P: %s pfd_cap_count[%d]=%d", device->playback_name, i, pfd_play_count[i]);
					*status = ALSA_DRIVER_WAIT_XRUN;
					return 0;
				}
				/* for any other error return negative wait status to caller */
				jack_error("poll descriptors error P: %s pfd_cap_count[%d]=%d", device->playback_name, i, pfd_play_count[i]);
				*status = ALSA_DRIVER_WAIT_ERROR;
				return 0;
			} else {
				pfd_index += pfd_play_count[i];
			}
		}

		if (extra_fd >= 0) {
			driver->pfd[pfd_index].fd = extra_fd;
			driver->pfd[pfd_index].events =
				POLLIN|POLLERR|POLLHUP|POLLNVAL;
			pfd_index++;
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
		poll_result = poll (driver->pfd,(unsigned int) pfd_count, driver->poll_timeout_ms);
#endif
		if (poll_result < 0) {

			if (errno == EINTR) {
				const char poll_log[] = "ALSA: poll interrupt";
				// this happens mostly when run
				// under gdb, or when exiting due to a signal
				if (under_gdb) {
					jack_info(poll_log);
					continue;
				}
				jack_error(poll_log);
				*status = ALSA_DRIVER_WAIT_ERROR;
				return 0;
			}

			jack_error ("ALSA: poll call failed (%s)",
				    strerror (errno));
			*status = ALSA_DRIVER_WAIT_ERROR;
			return 0;

		}

		poll_ret = jack_get_microseconds ();

		if (poll_result == 0) {
			jack_error ("ALSA: poll time out, polled for %" PRIu64
				    " usecs, Retrying with a recovery",
				    poll_ret - poll_enter);
			for (int i = 0; i < driver->devices_count; ++i) {
				if (driver->devices[i].capture_handle && i < driver->devices_c_count && cap_revents[i] == 0) {
					jack_log("device C: %s poll was requested", driver->devices[i].capture_name);
				}
				if (driver->devices[i].playback_handle && i < driver->devices_p_count && play_revents[i] == 0) {
					jack_log("device P: %s poll was requested", driver->devices[i].playback_name);
				}
			}
			*status = ALSA_DRIVER_WAIT_XRUN;
			return 0;
		}

        // JACK2
        SetTime(poll_ret);

		if (extra_fd < 0) {
			if (driver->poll_next && poll_ret > driver->poll_next) {
				*delayed_usecs = poll_ret - driver->poll_next;
			}
			driver->poll_last = poll_ret;
			driver->poll_next = poll_ret + driver->period_usecs;
		} else {
			/* check to see if it was the extra FD that caused us
			 * to return from poll */
			if (driver->pfd[pfd_index-1].revents == 0) {
				/* we timed out on the extra fd */
				jack_error("extra fd error");
				*status = ALSA_DRIVER_WAIT_ERROR;
				return -1;
			}
			/* if POLLIN was the only bit set, we're OK */
			*status = ALSA_DRIVER_WAIT_OK;
			return (driver->pfd[pfd_index-1].revents == POLLIN) ? 0 : -1;
		}

#ifdef DEBUG_WAKEUP
		fprintf (stderr, "%" PRIu64 ": checked %d fds, started at %" PRIu64 " %" PRIu64 "  usecs since poll entered\n",
			 poll_ret, desc_count, poll_enter, poll_ret - poll_enter);
#endif

		pfd_index = 0;

		for (int i = 0; i < driver->devices_c_count; ++i) {
			/* this device already triggered poll event before */
			if (cap_revents[i]) {
				continue;
			}

			alsa_device_t *device = &driver->devices[i];
			if (!device->capture_handle) {
				continue;
			}

			if (device->capture_target_state != SND_PCM_STATE_RUNNING) {
				continue;
			}

			unsigned short collect_revs = 0;
			if (snd_pcm_poll_descriptors_revents (device->capture_handle, &driver->pfd[pfd_index],
					pfd_cap_count[i], &collect_revs) != 0) {
				jack_error ("ALSA: capture revents failed");
				*status = ALSA_DRIVER_WAIT_ERROR;
				return 0;
			}

			pfd_index += pfd_cap_count[i];
			if (collect_revs & (POLLERR | POLLIN)) {
				if (collect_revs & POLLERR) {
					/* optimization, no point in polling more if we already have xrun on one device */
					jack_error ("xrun C: '%s'", device->capture_name);
					*status = ALSA_DRIVER_WAIT_XRUN;
					return 0;
				}
				if (collect_revs & POLLIN) {
				}
				/* on next poll round skip fds from this device */
				cap_revents[i] = collect_revs;
				pfd_count -= pfd_cap_count[i];
			}
		}

		for (int i = 0; i < driver->devices_p_count; ++i) {
			/* this device already triggered poll event before */
			if (play_revents[i]) {
				continue;
			}

			alsa_device_t *device = &driver->devices[i];
			if (!device->playback_handle) {
				continue;
			}

			if (device->playback_target_state != SND_PCM_STATE_RUNNING) {
				continue;
			}

			unsigned short collect_revs = 0;
			if (snd_pcm_poll_descriptors_revents (device->playback_handle, &driver->pfd[pfd_index],
					pfd_play_count[i], &collect_revs) != 0) {
				jack_error ("ALSA: playback revents failed");
				*status = ALSA_DRIVER_WAIT_ERROR;
				return 0;
			}

			pfd_index += pfd_play_count[i];
			if (collect_revs & (POLLERR | POLLOUT)) {
				if (collect_revs & POLLERR) {
					/* optimization, no point in polling more if we already have xrun on one device */
					jack_error ("xrun P: '%s'", device->playback_name);
					*status = ALSA_DRIVER_WAIT_XRUN;
					return 0;
				}
				if (collect_revs & POLLNVAL) {
					jack_error ("ALSA: playback device disconnected");
					*status = ALSA_DRIVER_WAIT_ERROR;
					return 0;
				}
				if (collect_revs & POLLOUT) {
				}
				/* on next poll round skip fds from this device */
				play_revents[i] = collect_revs;
				pfd_count -= pfd_play_count[i];
			}
		}
	}

	/* TODO: amiartus; I assume all devices are snd_pcm_link-ed and running on the same clock source,
	 * therefore should have the same avail frames, however in practice, this might have to be reworked,
	 * since we should check carefully for avail frames on each device, make sure it matches and handle corner cases
	 */

	capture_avail = INT_MAX;

	for (int i = 0; i < driver->devices_c_count; ++i) {
		alsa_device_t *device = &driver->devices[i];

		if (!device->capture_handle) {
			continue;
		}

		if (device->capture_target_state != SND_PCM_STATE_RUNNING) {
			continue;
		}

		snd_pcm_sframes_t avail = 0;
		if ((avail = alsa_driver_avail (driver, device->capture_handle, SND_PCM_STREAM_CAPTURE)) < 0) {
			if (avail == -EPIPE) {
				jack_error ("ALSA: avail_update xrun on capture dev '%s'", device->capture_name);
				*status = ALSA_DRIVER_WAIT_XRUN;
				return 0;
			} else {
				jack_error ("unknown ALSA avail_update return value (%u)", capture_avail);
			}
		}
		capture_avail = capture_avail < avail ? capture_avail : avail;
	}

	playback_avail = INT_MAX;

	for (int i = 0; i < driver->devices_p_count; ++i) {
		alsa_device_t *device = &driver->devices[i];

		if (!device->playback_handle) {
			continue;
		}

		if (device->playback_target_state != SND_PCM_STATE_RUNNING) {
			continue;
		}

		snd_pcm_sframes_t avail = 0;
		if ((avail = alsa_driver_avail (driver, device->playback_handle, SND_PCM_STREAM_PLAYBACK)) < 0) {
			if (avail == -EPIPE) {
				jack_error ("ALSA: avail_update xrun on playback dev '%s'", device->playback_name);
				*status = ALSA_DRIVER_WAIT_XRUN;
				return 0;
			} else {
				jack_error ("unknown ALSA avail_update return value (%u)", playback_avail);
			}
		}
		playback_avail = playback_avail < avail ? playback_avail : avail;
	}

	/* mark all channels not done for now. read/write will change this */
	for (int i = 0; i < driver->devices_p_count; ++i) {
		alsa_device_t *device = &driver->devices[i];

		if (!device->playback_handle) {
			continue;
		}

		if (device->playback_target_state != SND_PCM_STATE_RUNNING) {
			continue;
		}

		bitset_copy (device->channels_not_done, device->channels_done);
	}

	*status = ALSA_DRIVER_WAIT_OK;

	avail = capture_avail < playback_avail ? capture_avail : playback_avail;

#ifdef DEBUG_WAKEUP
	fprintf (stderr, "wakeup complete, avail = %lu, pavail = %lu "
		 "cavail = %lu\n",
		 avail, playback_avail, capture_avail);
#endif

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
	int err;

	if (nframes > driver->frames_per_cycle) {
		return -1;
	}

	for (size_t i = 0; i < driver->devices_c_count; ++i) {
		alsa_device_t *device = &driver->devices[i];

		if (!device->capture_handle) {
			continue;
		}

		if (device->capture_target_state != SND_PCM_STATE_RUNNING) {
			continue;
		}

		nread = 0;
		contiguous = 0;
		jack_nframes_t frames_remain = nframes;

		while (frames_remain) {

			contiguous = frames_remain;

			if (alsa_driver_get_channel_addresses (
					driver,
					device,
					(snd_pcm_uframes_t *) &contiguous,
					(snd_pcm_uframes_t *) 0,
					&offset, 0) < 0) {
				return -1;
			}

#ifdef __QNXNTO__
			const size_t bytes = contiguous * device->capture_nchannels * device->capture_sample_bytes;
			if ((err = snd_pcm_plugin_read(device->capture_handle,
							   device->capture_areas, bytes)) < bytes) {
				jack_error("read C: %s, requested %d, got %d, snd error %s, errno %d",
					device->capture_name,
					bytes,
					err,
					alsa_channel_status_error_str(alsa_driver_get_state(device->capture_handle, 1)),
					errno);
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
            ReadInput(device, nframes, contiguous, nread);

#ifndef __QNXNTO__
			if ((err = snd_pcm_mmap_commit (device->capture_handle,
					offset, contiguous)) < 0) {
				jack_error ("ALSA: could not complete read commit %s of %"
					PRIu32 " frames: error = %d", device->capture_name, contiguous, err);
				return -1;
			}
#endif

			frames_remain -= contiguous;
			nread += contiguous;
		}
	}

	return 0;
}

int
alsa_driver_write (alsa_driver_t* driver, jack_nframes_t nframes)
{
	snd_pcm_sframes_t contiguous;
	snd_pcm_sframes_t nwritten;
	snd_pcm_uframes_t offset;
	int err;

	driver->process_count++;

	if (nframes > driver->frames_per_cycle) {
		return -1;
	}

	for (size_t i = 0; i < driver->devices_p_count; ++i) {
		alsa_device_t *device = &driver->devices[i];

		if (!device->playback_handle) {
			continue;
		}

		if (device->playback_target_state != SND_PCM_STATE_RUNNING) {
			continue;
		}

		if (driver->midi)
			(driver->midi->write)(driver->midi, nframes);

		nwritten = 0;
		contiguous = 0;
		jack_nframes_t frames_remain = nframes;

		/* check current input monitor request status */
		driver->input_monitor_mask = 0;

		MonitorInput();

		if (driver->hw_monitoring) {
			if ((device->hw->input_monitor_mask
				 != driver->input_monitor_mask)
				&& !driver->all_monitor_in) {
				device->hw->set_input_monitor_mask (
						device->hw, driver->input_monitor_mask);
			}
		}

		while (frames_remain) {
	
			contiguous = frames_remain;
	
			if (alsa_driver_get_channel_addresses (
					driver,
					device,
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
            WriteOutput(device, nframes, contiguous, nwritten);

			if (!bitset_empty (device->channels_not_done)) {
				alsa_driver_silence_untouched_channels (driver, device, contiguous);
			}

#ifdef __QNXNTO__
			const size_t bytes = contiguous * device->playback_nchannels * device->playback_sample_bytes;
			if ((err = snd_pcm_plugin_write(device->playback_handle,
							   device->playback_areas, bytes)) < bytes) {
				jack_error("write P: %s, requested %d, got %d, snd error %s, errno %d",
					device->playback_name,
					bytes,
					err,
					alsa_channel_status_error_str(alsa_driver_get_state(device->playback_handle, 0)),
					errno);
				return -1;
			}
#else
			if ((err = snd_pcm_mmap_commit (device->playback_handle,
					offset, contiguous)) < 0) {
				jack_error ("ALSA: could not complete playback commit %s of %"
					PRIu32 " frames: error = %d", device->playback_name, contiguous, err);
				if (err != -EPIPE && err != -ESTRPIPE)
					return -1;
			}
#endif

			frames_remain -= contiguous;
			nwritten += contiguous;
		}
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
	if (driver->midi)
		(driver->midi->destroy)(driver->midi);

	for (int i = 0; i < driver->devices_count; ++i) {
		if (driver->devices[i].capture_handle) {
			snd_pcm_close (driver->devices[i].capture_handle);
			driver->devices[i].capture_handle = 0;
		}

		if (driver->devices[i].playback_handle) {
			snd_pcm_close (driver->devices[i].playback_handle);
			driver->devices[i].playback_handle = 0;
#ifndef __QNXNTO__
			for (JSList *node = driver->devices[i].clock_sync_listeners; node; node = jack_slist_next (node)) {
				free (node->data);
			}
			jack_slist_free (driver->devices[i].clock_sync_listeners);
#endif
		}

		free(driver->devices[i].capture_name);
		free(driver->devices[i].playback_name);
		free(driver->devices[i].alsa_driver);

		alsa_driver_release_channel_dependent_memory (driver, &driver->devices[i]);

		if (driver->devices[i].hw) {
			driver->devices[i].hw->release (driver->devices[i].hw);
			driver->devices[i].hw = 0;
		}

		if (driver->devices[i].ctl_handle) {
			snd_ctl_close (driver->devices[i].ctl_handle);
			driver->devices[i].ctl_handle = 0;
		}
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
alsa_driver_open_device (alsa_driver_t *driver, alsa_device_t *device, bool is_capture)
{
	int err = 0;
	char* current_apps;

	if(is_capture) {
#ifdef __QNXNTO__
		err = snd_pcm_open_name (&device->capture_handle,
					 device->capture_name,
					 SND_PCM_OPEN_CAPTURE | SND_PCM_OPEN_NONBLOCK);
#else
		err = snd_pcm_open (&device->capture_handle,
				    device->capture_name,
				    SND_PCM_STREAM_CAPTURE,
				    SND_PCM_NONBLOCK);
#endif
	} else {
#ifdef __QNXNTO__
		err = snd_pcm_open_name (&device->playback_handle,
					 device->playback_name,
					 SND_PCM_OPEN_PLAYBACK | SND_PCM_OPEN_NONBLOCK);
#else
		err = snd_pcm_open (&device->playback_handle,
				    device->playback_name,
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
				    is_capture ? device->alsa_name_capture : device->alsa_name_playback);
#else
			current_apps = discover_alsa_using_apps ();
			if (current_apps) {
				jack_error ("\n\nATTENTION: The device \"%s\" is "
					    "already in use. The following applications "
					    " are using your soundcard(s) so you should "
					    " check them and stop them as necessary before "
					    " trying to start JACK again:\n\n%s",
					    is_capture ? device->capture_name : device->playback_name,
					    current_apps);
				free (current_apps);
			} else {
				jack_error ("\n\nATTENTION: The device \"%s\" is "
					    "already in use. Please stop the"
					    " application using it and "
					    "run JACK again",
					    is_capture ? device->capture_name : device->playback_name);
			}
#endif
			break;

		case EPERM:
			jack_error ("you do not have permission to open "
				    "the audio device \"%s\" for %s",
				    is_capture ? device->capture_name : device->playback_name,
                    is_capture ? "capture" : "playback");
			break;

		case EINVAL:
			jack_error ("the state of handle or the mode is invalid "
				"or invalid state change occured \"%s\" for %s",
				is_capture ? device->capture_name : device->playback_name,
				is_capture ? "capture" : "playback");
			break;

		case ENOENT:
			jack_error ("device \"%s\"  does not exist for %s",
				is_capture ? device->capture_name : device->playback_name,
				is_capture ? "capture" : "playback");
			break;

		case ENOMEM:
			jack_error ("Not enough memory available for allocation for \"%s\" for %s",
				is_capture ? device->capture_name : device->playback_name,
				is_capture ? "capture" : "playback");
			break;

		case SND_ERROR_INCOMPATIBLE_VERSION:
			jack_error ("Version mismatch \"%s\" for %s",
				is_capture ? device->capture_name : device->playback_name,
				is_capture ? "capture" : "playback");
			break;
		}
		if(is_capture) {
			device->capture_handle = NULL;
		} else {
			device->playback_handle = NULL;
		}
	}

	if (is_capture && device->capture_handle) {
#ifdef __QNXNTO__
		snd_pcm_nonblock_mode (device->capture_handle, 0);
#else
		snd_pcm_nonblock (device->capture_handle, 0);
#endif
	} else if(!is_capture && device->playback_handle) {
#ifdef __QNXNTO__
		snd_pcm_nonblock_mode (device->playback_handle, 0);
#else
		snd_pcm_nonblock (device->playback_handle, 0);
#endif
	}

	return err;
}

jack_driver_t *
alsa_driver_new (char *name, alsa_driver_info_t info, jack_client_t *client)
{
	int err;
	alsa_driver_t *driver;

	jack_info ("creating alsa driver ... %s|%" PRIu32 "|%s|%" PRIu32 "|%" PRIu32 "|%" PRIu32
		"|%" PRIu32"|%" PRIu32"|%" PRIu32 "|%s|%s|%s|%s",
		info.devices_capture_size > 0 ? info.devices[0].capture_name : "-",
		info.devices_capture_size,
		info.devices_playback_size > 0 ? info.devices[0].playback_name : "-",
		info.devices_playback_size,
		info.frames_per_period, info.periods_n, info.frame_rate,
		info.devices[0].capture_channels, info.devices[0].playback_channels,
		info.hw_monitoring ? "hwmon": "nomon",
		info.hw_metering ? "hwmeter":"swmeter",
		info.soft_mode ? "soft-mode":"-",
		info.shorts_first ? "16bit":"32bit");

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

	driver->capture_frame_latency = info.capture_latency;
	driver->playback_frame_latency = info.playback_latency;

	driver->all_monitor_in = FALSE;
	driver->with_monitor_ports = info.monitor;

	driver->clock_mode = ClockMaster; /* XXX is it? */
	driver->input_monitor_mask = 0;   /* XXX is it? */

	driver->pfd = 0;
	driver->playback_nfds = 0;
	driver->capture_nfds = 0;

	driver->dither = info.dither;
	driver->soft_mode = info.soft_mode;

	driver->poll_late = 0;
	driver->xrun_count = 0;
	driver->process_count = 0;

	driver->midi = info.midi_driver;
	driver->xrun_recovery = 0;

	driver->devices_c_count = info.devices_capture_size;
	driver->devices_p_count = info.devices_playback_size;
	driver->devices_count = info.devices_capture_size > info.devices_playback_size ? info.devices_capture_size : info.devices_playback_size;
	driver->devices = (alsa_device_t*) calloc(driver->devices_count, sizeof(*driver->devices));

	driver->frame_rate = info.frame_rate;
	driver->frames_per_cycle = info.frames_per_period;
	driver->user_nperiods = info.periods_n;

	driver->preferred_sample_bytes = info.shorts_first ? 2 : 4;

	driver->features = info.features;

	for (int i = 0; i < driver->devices_count; ++i) {
		alsa_device_t *device = &driver->devices[i];
		if (i < driver->devices_c_count) {
			device->capture_sample_bytes = 0;
			device->capture_sample_format = SND_PCM_FORMAT_UNKNOWN;
			device->capture_name = strdup(info.devices[i].capture_name);
			device->capture_nchannels = info.devices[i].capture_channels;
		}
		if (i < driver->devices_p_count) {
			device->playback_sample_bytes = 0;
			device->playback_sample_format = SND_PCM_FORMAT_UNKNOWN;
			device->playback_name = strdup(info.devices[i].playback_name);
			device->playback_nchannels = info.devices[i].playback_channels;
		}
	}

#ifndef __QNXNTO__
	driver->playback_hw_params = 0;
	driver->capture_hw_params = 0;
	driver->playback_sw_params = 0;
	driver->capture_sw_params = 0;

	if (driver->devices_p_count) {
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

	if (driver->devices_c_count) {
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

	driver->client = client;


#ifndef __QNXNTO__
	for (int i = 0; i < driver->devices_p_count; ++i) {
		pthread_mutex_init (&driver->devices[i].clock_sync_lock, 0);
		driver->devices[i].clock_sync_listeners = 0;

		if (alsa_driver_check_card_type (driver, &driver->devices[i])) {
			alsa_driver_delete(driver);
			return NULL;
		}

		alsa_driver_hw_specific (driver, &driver->devices[i], info.hw_monitoring, info.hw_metering);
	}
#endif

	return (jack_driver_t *) driver;
}

int
alsa_driver_listen_for_clock_sync_status (alsa_device_t *device,
					  ClockSyncListenerFunction func,
					  void *arg)
{
	ClockSyncListener *csl;

	csl = (ClockSyncListener *) malloc (sizeof (ClockSyncListener));
	csl->function = func;
	csl->arg = arg;
	csl->id = device->next_clock_sync_listener_id++;

	pthread_mutex_lock (&device->clock_sync_lock);
	device->clock_sync_listeners =
		jack_slist_prepend (device->clock_sync_listeners, csl);
	pthread_mutex_unlock (&device->clock_sync_lock);
	return csl->id;
}

int
alsa_driver_stop_listening_to_clock_sync_status (alsa_device_t *device,
						 unsigned int which)
{
	JSList *node;
	int ret = -1;
	pthread_mutex_lock (&device->clock_sync_lock);
	for (node = device->clock_sync_listeners; node;
	     node = jack_slist_next (node)) {
		if (((ClockSyncListener *) node->data)->id == which) {
			device->clock_sync_listeners =
				jack_slist_remove_link (
					device->clock_sync_listeners, node);
			free (node->data);
			jack_slist_free_1 (node);
			ret = 0;
			break;
		}
	}
	pthread_mutex_unlock (&device->clock_sync_lock);
	return ret;
}

void
alsa_device_clock_sync_notify (alsa_device_t *device, channel_t chn,
			       ClockSyncStatus status)
{
	JSList *node;

	pthread_mutex_lock (&device->clock_sync_lock);
	for (node = device->clock_sync_listeners; node;
	     node = jack_slist_next (node)) {
		ClockSyncListener *csl = (ClockSyncListener *) node->data;
		csl->function (chn, status, csl->arg);
	}
	pthread_mutex_unlock (&device->clock_sync_lock);
}

/* DRIVER "PLUGIN" INTERFACE */

const char driver_client_name[] = "alsa_pcm";

void
driver_finish (jack_driver_t *driver)
{
	alsa_driver_delete ((alsa_driver_t *) driver);
}
