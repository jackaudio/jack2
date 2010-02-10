/*
 *   FireWire Backend for Jack
 *   using FFADO
 *   FFADO = Firewire (pro-)audio for linux
 *
 *   http://www.ffado.org
 *   http://www.jackaudio.org
 *
 *   Copyright (C) 2005-2007 Pieter Palmers
 *   Copyright (C) 2009 Devin Anderson
 *
 *   adapted for JackMP by Pieter Palmers
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Main Jack driver entry routines
 *
 */

#ifndef __JACK_FFADO_DRIVER_H__
#define __JACK_FFADO_DRIVER_H__

#include <libffado/ffado.h>

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <poll.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <endian.h>

#include <pthread.h>
#include <semaphore.h>

#include <driver.h>
#include <types.h>

#include <assert.h>
//#include <jack/midiport.h>

// debug print control flags
#define DEBUG_LEVEL_BUFFERS           	(1<<0)
#define DEBUG_LEVEL_HANDLERS			(1<<1)
#define DEBUG_LEVEL_XRUN_RECOVERY     	(1<<2)
#define DEBUG_LEVEL_WAIT     			(1<<3)

#define DEBUG_LEVEL_RUN_CYCLE         	(1<<8)

#define DEBUG_LEVEL_PACKETCOUNTER		(1<<16)
#define DEBUG_LEVEL_STARTUP				(1<<17)
#define DEBUG_LEVEL_THREADS				(1<<18)

//#define DEBUG_ENABLED
#ifdef DEBUG_ENABLED

// default debug level
#define DEBUG_LEVEL (  DEBUG_LEVEL_RUN_CYCLE | \
	(DEBUG_LEVEL_XRUN_RECOVERY)| DEBUG_LEVEL_STARTUP | DEBUG_LEVEL_WAIT | DEBUG_LEVEL_PACKETCOUNTER)

#warning Building debug build!

#define printMessage(format, args...) jack_error( "firewire MSG: %s:%d (%s): " format,  __FILE__, __LINE__, __FUNCTION__, ##args )
#define printError(format, args...) jack_error( "firewire ERR: %s:%d (%s): " format,  __FILE__, __LINE__, __FUNCTION__, ##args )

#define printEnter() jack_error( "FWDRV ENTERS: %s (%s)\n", __FUNCTION__,  __FILE__)
#define printExit() jack_error( "FWDRV EXITS: %s (%s)\n", __FUNCTION__,  __FILE__)
#define printEnter()
#define printExit()

#define debugError(format, args...) jack_error( "firewire ERR: %s:%d (%s): " format,  __FILE__, __LINE__, __FUNCTION__, ##args )
#define debugPrint(Level, format, args...) if(DEBUG_LEVEL & (Level))  jack_error("DEBUG %s:%d (%s) :"  format, __FILE__, __LINE__, __FUNCTION__, ##args );
#define debugPrintShort(Level, format, args...) if(DEBUG_LEVEL & (Level))  jack_error( format,##args );
#define debugPrintWithTimeStamp(Level, format, args...) if(DEBUG_LEVEL & (Level)) jack_error( "%16lu: "format, debugGetCurrentUTime(),##args );
#define SEGFAULT int *test=NULL;	*test=1;
#else
#define DEBUG_LEVEL

#define printMessage(format, args...) if(g_verbose) \
	                                         jack_error("firewire MSG: " format, ##args )
#define printError(format, args...)   jack_error("firewire ERR: " format, ##args )

#define printEnter()
#define printExit()

#define debugError(format, args...)
#define debugPrint(Level, format, args...)
#define debugPrintShort(Level, format, args...)
#define debugPrintWithTimeStamp(Level, format, args...)
#endif

// thread priority setup
#define FFADO_RT_PRIORITY_PACKETIZER_RELATIVE	5

typedef struct _ffado_driver ffado_driver_t;

/*
 * Jack Driver command line parameters
 */

typedef struct _ffado_jack_settings ffado_jack_settings_t;
struct _ffado_jack_settings
{
    int verbose_level;

    int period_size_set;
    jack_nframes_t period_size;

    int sample_rate_set;
    int sample_rate;

    int buffer_size_set;
    jack_nframes_t buffer_size;

    int playback_ports;
    int capture_ports;

    jack_nframes_t capture_frame_latency;
    jack_nframes_t playback_frame_latency;

    int slave_mode;
    int snoop_mode;

    char *device_info;
};

typedef struct _ffado_capture_channel
{
    ffado_streaming_stream_type stream_type;
    uint32_t *midi_buffer;
    void *midi_input;
}
ffado_capture_channel_t;

typedef struct _ffado_playback_channel
{
    ffado_streaming_stream_type stream_type;
    uint32_t *midi_buffer;
    void *midi_output;
}
ffado_playback_channel_t;

/*
 * JACK driver structure
 */
struct _ffado_driver
{
    JACK_DRIVER_NT_DECL;

    jack_nframes_t  sample_rate;
    jack_nframes_t  period_size;
    unsigned long   wait_time;

    jack_time_t                   wait_last;
    jack_time_t                   wait_next;
    int wait_late;

    jack_client_t  *client;

    int		xrun_detected;
    int		xrun_count;

    int process_count;

    /* settings from the command line */
    ffado_jack_settings_t settings;

    /* the firewire virtual device */
    ffado_device_t *dev;

    channel_t                     playback_nchannels;
    channel_t                     capture_nchannels;

    ffado_playback_channel_t *playback_channels;
    ffado_capture_channel_t  *capture_channels;
    ffado_sample_t *nullbuffer;
    ffado_sample_t *scratchbuffer;

    jack_nframes_t  playback_frame_latency;
    jack_nframes_t  capture_frame_latency;

    ffado_device_info_t device_info;
    ffado_options_t device_options;

};

#endif /* __JACK_FFADO_DRIVER_H__ */


