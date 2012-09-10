
/*
    Copyright (C) 2003 Robert Ham <rah@bash.sh>
    Copyright (C) 2005 Torben Hohn <torbenh@gmx.de>

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

#ifndef __NETJACK_H__
#define __NETJACK_H__

#include <unistd.h>

#include <jack/types.h>
#include <jack/jack.h>
#include <jack/transport.h>
#include "jack/jslist.h"

#if HAVE_CELT
#include <celt/celt.h>
#endif

#if HAVE_OPUS
#include <opus/opus.h>
#include <opus/opus_custom.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    struct _packet_cache;

    typedef struct _netjack_driver_state netjack_driver_state_t;

    struct _netjack_driver_state {
        jack_nframes_t  net_period_up;
        jack_nframes_t  net_period_down;

        jack_nframes_t  sample_rate;
        jack_nframes_t  bitdepth;
        jack_nframes_t  period_size;
        jack_time_t	    period_usecs;
        int		    dont_htonl_floats;
        int		    always_deadline;

        jack_nframes_t  codec_latency;

        unsigned int    listen_port;

        unsigned int    capture_channels;
        unsigned int    playback_channels;
        unsigned int    capture_channels_audio;
        unsigned int    playback_channels_audio;
        unsigned int    capture_channels_midi;
        unsigned int    playback_channels_midi;

        JSList	    *capture_ports;
        JSList	    *playback_ports;
        JSList	    *playback_srcs;
        JSList	    *capture_srcs;

        jack_client_t   *client;

#ifdef WIN32
        SOCKET	    sockfd;
        SOCKET	    outsockfd;
#else
        int		    sockfd;
        int		    outsockfd;
#endif

        struct sockaddr_in syncsource_address;

        int		    reply_port;
        int		    srcaddress_valid;

        int sync_state;
        unsigned int handle_transport_sync;

        unsigned int *rx_buf;
        unsigned int rx_bufsize;
        //unsigned int tx_bufsize;
        unsigned int mtu;
        unsigned int latency;
        unsigned int redundancy;

        jack_nframes_t expected_framecnt;
        int		   expected_framecnt_valid;
        unsigned int   num_lost_packets;
        jack_time_t	   next_deadline;
        jack_time_t	   deadline_offset;
        int		   next_deadline_valid;
        int		   packet_data_valid;
        int		   resync_threshold;
        int		   running_free;
        int		   deadline_goodness;
        jack_time_t	   time_to_deadline;
        unsigned int   use_autoconfig;
        unsigned int   resample_factor;
        unsigned int   resample_factor_up;
        int		   jitter_val;
        struct _packet_cache * packcache;
#if HAVE_CELT
        CELTMode	   *celt_mode;
#endif
#if HAVE_OPUS
        OpusCustomMode* opus_mode;
#endif
    };

    int netjack_wait( netjack_driver_state_t *netj );
    void netjack_send_silence( netjack_driver_state_t *netj, int syncstate );
    void netjack_read( netjack_driver_state_t *netj, jack_nframes_t nframes ) ;
    void netjack_write( netjack_driver_state_t *netj, jack_nframes_t nframes, int syncstate );
    void netjack_attach( netjack_driver_state_t *netj );
    void netjack_detach( netjack_driver_state_t *netj );

    netjack_driver_state_t *netjack_init (netjack_driver_state_t *netj,
                                          jack_client_t * client,
                                          const char *name,
                                          unsigned int capture_ports,
                                          unsigned int playback_ports,
                                          unsigned int capture_ports_midi,
                                          unsigned int playback_ports_midi,
                                          jack_nframes_t sample_rate,
                                          jack_nframes_t period_size,
                                          unsigned int listen_port,
                                          unsigned int transport_sync,
                                          unsigned int resample_factor,
                                          unsigned int resample_factor_up,
                                          unsigned int bitdepth,
                                          unsigned int use_autoconfig,
                                          unsigned int latency,
                                          unsigned int redundancy,
                                          int dont_htonl_floats,
                                          int always_deadline,
                                          int jitter_val );

    void netjack_release( netjack_driver_state_t *netj );
    int netjack_startup( netjack_driver_state_t *netj );

#ifdef __cplusplus
}
#endif

#endif
