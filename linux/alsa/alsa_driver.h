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

#include <alsa/asoundlib.h>
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

typedef struct _alsa_driver {

    JACK_DRIVER_NT_DECL

    int                           poll_timeout;
    jack_time_t                   poll_last;
    jack_time_t                   poll_next;
    char                        **playback_addr;
    char                        **capture_addr;
    const snd_pcm_channel_area_t *capture_areas;
    const snd_pcm_channel_area_t *playback_areas;
    struct pollfd                *pfd;
    unsigned int                  playback_nfds;
    unsigned int                  capture_nfds;
    unsigned long                 interleave_unit;
    unsigned long                *capture_interleave_skip;
    unsigned long                *playback_interleave_skip;
    channel_t                     max_nchannels;
    channel_t                     user_nchannels;
    channel_t                     playback_nchannels;
    channel_t                     capture_nchannels;
    unsigned long                 playback_sample_bytes;
    unsigned long                 capture_sample_bytes;

    jack_nframes_t                frame_rate;
    jack_nframes_t                frames_per_cycle;
    jack_nframes_t                capture_frame_latency;
    jack_nframes_t                playback_frame_latency;

    unsigned long                *silent;
    char                         *alsa_name_playback;
    char                         *alsa_name_capture;
    char                         *alsa_driver;
    bitset_t			  channels_not_done;
    bitset_t			  channels_done;
    snd_pcm_format_t              playback_sample_format;
    snd_pcm_format_t              capture_sample_format;
    float                         max_sample_val;
    unsigned long                 user_nperiods;
    unsigned int                  playback_nperiods;
    unsigned int                  capture_nperiods;
    unsigned long                 last_mask;
    snd_ctl_t                    *ctl_handle;
    snd_pcm_t                    *playback_handle;
    snd_pcm_t                    *capture_handle;
    snd_pcm_hw_params_t          *playback_hw_params;
    snd_pcm_sw_params_t          *playback_sw_params;
    snd_pcm_hw_params_t          *capture_hw_params;
    snd_pcm_sw_params_t          *capture_sw_params;
    jack_hardware_t              *hw;
    ClockSyncStatus              *clock_sync_data;
    jack_client_t                *client;
    JSList                       *capture_ports;
    JSList                       *playback_ports;
    JSList                       *monitor_ports;

    unsigned long input_monitor_mask;

    char soft_mode;
    char hw_monitoring;
    char hw_metering;
    char all_monitor_in;
    char capture_and_playback_not_synced;
    char playback_interleaved;
    char capture_interleaved;
    char with_monitor_ports;
    char has_clock_sync_reporting;
    char has_hw_monitoring;
    char has_hw_metering;
    char quirk_bswap;

    ReadCopyFunction read_via_copy;
    WriteCopyFunction write_via_copy;

    int             dither;
    dither_state_t *dither_state;

    SampleClockMode clock_mode;
    JSList *clock_sync_listeners;
    pthread_mutex_t clock_sync_lock;
    unsigned long next_clock_sync_listener_id;

    int running;
    int run;

    int poll_late;
    int xrun_count;
    int process_count;

    alsa_midi_t *midi;
    int xrun_recovery;

} alsa_driver_t;

static inline void
alsa_driver_mark_channel_done (alsa_driver_t *driver, channel_t chn) {
	bitset_remove (driver->channels_not_done, chn);
	driver->silent[chn] = 0;
}

static inline void
alsa_driver_silence_on_channel (alsa_driver_t *driver, channel_t chn,
				jack_nframes_t nframes) {
	if (driver->playback_interleaved) {
		memset_interleave
			(driver->playback_addr[chn],
			 0, nframes * driver->playback_sample_bytes,
			 driver->interleave_unit,
			 driver->playback_interleave_skip[chn]);
	} else {
		memset (driver->playback_addr[chn], 0,
			nframes * driver->playback_sample_bytes);
	}
	alsa_driver_mark_channel_done (driver,chn);
}

static inline void
alsa_driver_silence_on_channel_no_mark (alsa_driver_t *driver, channel_t chn,
					jack_nframes_t nframes) {
	if (driver->playback_interleaved) {
		memset_interleave
			(driver->playback_addr[chn],
			 0, nframes * driver->playback_sample_bytes,
			 driver->interleave_unit,
			 driver->playback_interleave_skip[chn]);
	} else {
		memset (driver->playback_addr[chn], 0,
			nframes * driver->playback_sample_bytes);
	}
}

static inline void
alsa_driver_read_from_channel (alsa_driver_t *driver,
			       channel_t channel,
			       jack_default_audio_sample_t *buf,
			       jack_nframes_t nsamples)
{
	driver->read_via_copy (buf,
			       driver->capture_addr[channel],
			       nsamples,
			       driver->capture_interleave_skip[channel]);
}

static inline void
alsa_driver_write_to_channel (alsa_driver_t *driver,
			      channel_t channel,
			      jack_default_audio_sample_t *buf,
			      jack_nframes_t nsamples)
{
	driver->write_via_copy (driver->playback_addr[channel],
				buf,
				nsamples,
				driver->playback_interleave_skip[channel],
				driver->dither_state+channel);
	alsa_driver_mark_channel_done (driver, channel);
}

void  alsa_driver_silence_untouched_channels (alsa_driver_t *driver,
					      jack_nframes_t nframes);
void  alsa_driver_set_clock_sync_status (alsa_driver_t *driver, channel_t chn,
					 ClockSyncStatus status);
int   alsa_driver_listen_for_clock_sync_status (alsa_driver_t *,
						ClockSyncListenerFunction,
						void *arg);
int   alsa_driver_stop_listen_for_clock_sync_status (alsa_driver_t *,
						     unsigned int);
void  alsa_driver_clock_sync_notify (alsa_driver_t *, channel_t chn,
				     ClockSyncStatus);

int
alsa_driver_reset_parameters (alsa_driver_t *driver,
			      jack_nframes_t frames_per_cycle,
			      jack_nframes_t user_nperiods,
			      jack_nframes_t rate);

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
		 );
void
alsa_driver_delete (alsa_driver_t *driver);

int
alsa_driver_start (alsa_driver_t *driver);

int
alsa_driver_stop (alsa_driver_t *driver);

jack_nframes_t
alsa_driver_wait (alsa_driver_t *driver, int extra_fd, int *status, float
		  *delayed_usecs);

int
alsa_driver_read (alsa_driver_t *driver, jack_nframes_t nframes);

int
alsa_driver_write (alsa_driver_t* driver, jack_nframes_t nframes);

jack_time_t jack_get_microseconds(void);

// Code implemented in JackAlsaDriver.cpp

void ReadInput(jack_nframes_t orig_nframes, snd_pcm_sframes_t contiguous, snd_pcm_sframes_t nread);
void MonitorInput();
void ClearOutput();
void WriteOutput(jack_nframes_t orig_nframes, snd_pcm_sframes_t contiguous, snd_pcm_sframes_t nwritten);
void SetTime(jack_time_t time);
int Restart();

#ifdef __cplusplus
}
#endif


#endif /* __jack_alsa_driver_h__ */
