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

    $Id: alsa_driver.h 945 2006-05-04 15:14:45Z pbd $
*/

#ifndef __jack_alsa_driver_h__
#define __jack_alsa_driver_h__

#ifdef __QNXNTO__
#include <sys/asoundlib.h>
#else
#include <alsa/asoundlib.h>
#endif

#include "bitset.h"

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define IS_LE 0
#define IS_BE 1
#elif __BYTE_ORDER == __BIG_ENDIAN
#define IS_LE 1
#define IS_BE 0
#endif

#define TRUE 1
#define FALSE 0

#include "types.h"
#include "hardware.h"
#include "driver.h"
#include "memops.h"
#include "alsa_midi.h"

#ifdef __QNXNTO__
#define SND_PCM_FORMAT_S16_LE      SND_PCM_SFMT_S16_LE
#define SND_PCM_FORMAT_S16_BE      SND_PCM_SFMT_S16_BE
#define SND_PCM_FORMAT_S24_LE      SND_PCM_SFMT_S24_LE
#define SND_PCM_FORMAT_S24_BE      SND_PCM_SFMT_S24_BE
#define SND_PCM_FORMAT_S32_LE      SND_PCM_SFMT_S32_LE
#define SND_PCM_FORMAT_S32_BE      SND_PCM_SFMT_S32_BE
#define SND_PCM_FORMAT_FLOAT_LE    SND_PCM_SFMT_FLOAT_LE
#define SND_PCM_STATE_PREPARED     SND_PCM_STATUS_PREPARED
#define SND_PCM_STATE_SUSPENDED    SND_PCM_STATUS_SUSPENDED
#define SND_PCM_STATE_XRUN         SND_PCM_STATUS_UNDERRUN
#define SND_PCM_STATE_RUNNING      SND_PCM_STATUS_RUNNING
#define SND_PCM_STATE_NOTREADY     SND_PCM_STATUS_NOTREADY
#define SND_PCM_STREAM_PLAYBACK    SND_PCM_CHANNEL_PLAYBACK
#define SND_PCM_STREAM_CAPTURE     SND_PCM_CHANNEL_CAPTURE

typedef unsigned long              snd_pcm_uframes_t;
typedef signed long                snd_pcm_sframes_t;
typedef int32_t                    alsa_driver_default_format_t;
#else
#define SND_PCM_STATE_NOTREADY     (SND_PCM_STATE_LAST + 1)
#endif

#define ALSA_DRIVER_FEAT_CLOSE_IDLE_DEVS   (1 << 1)

#ifdef __cplusplus
extern "C"
{
#endif

typedef void (*ReadCopyFunction)  (jack_default_audio_sample_t *dst, char *src,
                                   unsigned long src_bytes,
                                   unsigned long src_skip_bytes);
typedef void (*WriteCopyFunction) (char *dst, jack_default_audio_sample_t *src,
                                   unsigned long src_bytes,
                                   unsigned long dst_skip_bytes,
                                   dither_state_t *state);

typedef struct _alsa_device {
#ifdef __QNXNTO__
    unsigned int                  playback_sample_format;
    unsigned int                  capture_sample_format;
    void                          *capture_areas;
    void                          *playback_areas;
    void                          *capture_areas_ptr;
    void                          *playback_areas_ptr;
#else
    snd_pcm_format_t              playback_sample_format;
    snd_pcm_format_t              capture_sample_format;
    const snd_pcm_channel_area_t *capture_areas;
    const snd_pcm_channel_area_t *playback_areas;
#endif
    snd_pcm_t *playback_handle;
    snd_pcm_t *capture_handle;

    char *playback_name;
    char *capture_name;

    char **playback_addr;
    char **capture_addr;

    channel_t playback_channel_offset;
    channel_t capture_channel_offset;

    channel_t playback_nchannels;
    channel_t capture_nchannels;
    channel_t max_nchannels;
    channel_t user_nchannels;

    bitset_t channels_not_done;
    bitset_t channels_done;

    char quirk_bswap;

    ReadCopyFunction read_via_copy;
    WriteCopyFunction write_via_copy;

    unsigned long interleave_unit;
    unsigned long *capture_interleave_skip;
    unsigned long *playback_interleave_skip;

    char playback_interleaved;
    char capture_interleaved;

    unsigned long *silent;

    unsigned long playback_sample_bytes;
    unsigned long capture_sample_bytes;

    /* device is 'snd_pcm_link' to a group, only 1 group of linked devices is allowed */
    int capture_linked;
    int playback_linked;

    int capture_xrun_count;
    int playback_xrun_count;

    /* desired state of device, decided by JackAlsaDriver */
    int capture_target_state;
    int playback_target_state;

    jack_hardware_t *hw;
    char *alsa_driver;
} alsa_device_t;

typedef struct _alsa_driver {

    JACK_DRIVER_NT_DECL

#ifndef __QNXNTO__
    snd_pcm_hw_params_t          *playback_hw_params;
    snd_pcm_sw_params_t          *playback_sw_params;
    snd_pcm_hw_params_t          *capture_hw_params;
    snd_pcm_sw_params_t          *capture_sw_params;
#endif
    int                           poll_timeout_ms;
    jack_time_t                   poll_last;
    jack_time_t                   poll_next;
    struct pollfd                *pfd;
    unsigned int                  playback_nfds;
    unsigned int                  capture_nfds;
    channel_t                     playback_nchannels;
    channel_t                     capture_nchannels;

    jack_nframes_t                frame_rate;
    jack_nframes_t                frames_per_cycle;
    jack_nframes_t                capture_frame_latency;
    jack_nframes_t                playback_frame_latency;

    unsigned long                 user_nperiods;
    unsigned int                  playback_nperiods;
    unsigned int                  capture_nperiods;
    snd_ctl_t                    *ctl_handle;
    jack_client_t                *client;

    unsigned long input_monitor_mask;

    char soft_mode;
    char hw_monitoring;
    char hw_metering;
    char all_monitor_in;
    char with_monitor_ports;
    char has_clock_sync_reporting;
    char has_hw_monitoring;
    char has_hw_metering;

    int             dither;
    dither_state_t *dither_state;

    SampleClockMode clock_mode;
    JSList *clock_sync_listeners;
    pthread_mutex_t clock_sync_lock;

    int poll_late;
    int xrun_count;
    int process_count;

    alsa_midi_t *midi;
    int xrun_recovery;

    alsa_device_t *devices;
    int devices_count;
    int devices_c_count;
    int devices_p_count;

    int features;
} alsa_driver_t;

typedef struct _alsa_device_info {
    char *capture_name;
    char *playback_name;

    int capture_channels;
    int playback_channels;
} alsa_device_info_t;

typedef struct _alsa_driver_info {
    alsa_device_info_t *devices;
    uint32_t devices_capture_size;
    uint32_t devices_playback_size;

    char *midi_name;
    alsa_midi_t *midi_driver;

    jack_nframes_t frame_rate;
    jack_nframes_t frames_per_period;
    int periods_n;

    DitherAlgorithm dither;

    int shorts_first;

    jack_nframes_t capture_latency;
    jack_nframes_t playback_latency;

    // these 4 should be reworked as struct.features
    int hw_monitoring;
    int hw_metering;
    int monitor;
    int soft_mode;

    int features;
} alsa_driver_info_t;

static inline void
alsa_driver_mark_channel_done (alsa_driver_t *driver, alsa_device_t *device, channel_t chn) {
	bitset_remove (device->channels_not_done, chn);
	device->silent[chn] = 0;
}

static inline void
alsa_driver_silence_on_channel (alsa_driver_t *driver, alsa_device_t *device, channel_t chn,
				jack_nframes_t nframes) {
	if (device->playback_interleaved) {
		memset_interleave
			(device->playback_addr[chn],
			 0, nframes * device->playback_sample_bytes,
			 device->interleave_unit,
			 device->playback_interleave_skip[chn]);
	} else {
		memset (device->playback_addr[chn], 0,
			nframes * device->playback_sample_bytes);
	}
    alsa_driver_mark_channel_done (driver, device, chn);
}

static inline void
alsa_driver_silence_on_channel_no_mark (alsa_driver_t *driver, alsa_device_t *device, channel_t chn,
					jack_nframes_t nframes) {
	if (device->playback_interleaved) {
		memset_interleave
			(device->playback_addr[chn],
			 0, nframes * device->playback_sample_bytes,
			 device->interleave_unit,
			 device->playback_interleave_skip[chn]);
	} else {
		memset (device->playback_addr[chn], 0,
			nframes * device->playback_sample_bytes);
	}
}

static inline void
alsa_driver_read_from_channel (alsa_driver_t *driver,
                   alsa_device_t *device,
			       channel_t channel,
			       jack_default_audio_sample_t *buf,
			       jack_nframes_t nsamples)
{
	device->read_via_copy (buf,
			       device->capture_addr[channel],
			       nsamples,
			       device->capture_interleave_skip[channel]);
}

static inline void
alsa_driver_write_to_channel (alsa_driver_t *driver,
                  alsa_device_t *device,
			      channel_t channel,
			      jack_default_audio_sample_t *buf,
			      jack_nframes_t nsamples)
{
	device->write_via_copy (device->playback_addr[channel],
				buf,
				nsamples,
				device->playback_interleave_skip[channel],
				driver->dither_state+channel);
	alsa_driver_mark_channel_done (driver, device, channel);
}

int
alsa_driver_reset_parameters (alsa_driver_t *driver,
			      jack_nframes_t frames_per_cycle,
			      jack_nframes_t user_nperiods,
			      jack_nframes_t rate);

jack_driver_t *
alsa_driver_new (char *name, alsa_driver_info_t info, jack_client_t *client);

void
alsa_driver_delete (alsa_driver_t *driver);

int
alsa_driver_open (alsa_driver_t *driver);

int
alsa_driver_start (alsa_driver_t *driver);

int
alsa_driver_stop (alsa_driver_t *driver);

int
alsa_driver_close (alsa_driver_t *driver);

jack_nframes_t
alsa_driver_wait (alsa_driver_t *driver, int extra_fd, int *status, float
		  *delayed_usecs);

int
alsa_driver_read (alsa_driver_t *driver, jack_nframes_t nframes);

int
alsa_driver_write (alsa_driver_t* driver, jack_nframes_t nframes);

// Code implemented in JackAlsaDriver.cpp

void ReadInput(alsa_device_t *device, jack_nframes_t orig_nframes, snd_pcm_sframes_t contiguous, snd_pcm_sframes_t nread);
void MonitorInput();
void ClearOutput();
void WriteOutput(alsa_device_t *device, jack_nframes_t orig_nframes, snd_pcm_sframes_t contiguous, snd_pcm_sframes_t nwritten);
void SetTime(jack_time_t time);

int Restart();

#ifdef __cplusplus
}
#endif


#endif /* __jack_alsa_driver_h__ */
