
/*
 * NetJack - Packet Handling functions
 *
 * used by the driver and the jacknet_client
 *
 * Copyright (C) 2006 Torben Hohn <torbenh@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: net_driver.c,v 1.16 2006/03/20 19:41:37 torbenh Exp $
 *
 */

#ifndef __JACK_NET_PACKET_H__
#define __JACK_NET_PACKET_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <jack/jack.h>
#include <jack/types.h>
#include <jack/jslist.h>
#include <jack/midiport.h>

// The Packet Header.

#define CELT_MODE 1000   // Magic bitdepth value that indicates CELT compression
#define OPUS_MODE  999   // Magic bitdepth value that indicates OPUS compression
#define MASTER_FREEWHEELS 0x80000000

    typedef struct _jacknet_packet_header jacknet_packet_header;

    struct _jacknet_packet_header {
        // General AutoConf Data
        jack_nframes_t capture_channels_audio;
        jack_nframes_t playback_channels_audio;
        jack_nframes_t capture_channels_midi;
        jack_nframes_t playback_channels_midi;
        jack_nframes_t period_size;
        jack_nframes_t sample_rate;

        // Transport Sync
        jack_nframes_t sync_state;
        jack_nframes_t transport_frame;
        jack_nframes_t transport_state;

        // Packet loss Detection, and latency reduction
        jack_nframes_t framecnt;
        jack_nframes_t latency;

        jack_nframes_t reply_port;
        jack_nframes_t mtu;
        jack_nframes_t fragment_nr;
    };

    typedef union _int_float int_float_t;

    union _int_float {
        uint32_t i;
        float    f;
    };

    // fragment reorder cache.
    typedef struct _cache_packet cache_packet;

    struct _cache_packet {
        int		    valid;
        int		    num_fragments;
        int		    packet_size;
        int		    mtu;
        jack_time_t	    recv_timestamp;
        jack_nframes_t  framecnt;
        char *	    fragment_array;
        char *	    packet_buf;
    };

    typedef struct _packet_cache packet_cache;

    struct _packet_cache {
        int size;
        cache_packet *packets;
        int mtu;
        struct sockaddr_in master_address;
        int master_address_valid;
        jack_nframes_t last_framecnt_retreived;
        int last_framecnt_retreived_valid;
    };

    // fragment cache function prototypes
    // XXX: Some of these are private.
    packet_cache *packet_cache_new(int num_packets, int pkt_size, int mtu);
    void	      packet_cache_free(packet_cache *pkt_cache);

    cache_packet *packet_cache_get_packet(packet_cache *pkt_cache, jack_nframes_t framecnt);
    cache_packet *packet_cache_get_oldest_packet(packet_cache *pkt_cache);
    cache_packet *packet_cache_get_free_packet(packet_cache *pkt_cache);

    void	cache_packet_reset(cache_packet *pack);
    void	cache_packet_set_framecnt(cache_packet *pack, jack_nframes_t framecnt);
    void	cache_packet_add_fragment(cache_packet *pack, char *packet_buf, int rcv_len);
    int	cache_packet_is_complete(cache_packet *pack);

    void packet_cache_drain_socket( packet_cache *pcache, int sockfd );
    void packet_cache_reset_master_address( packet_cache *pcache );
    float packet_cache_get_fill( packet_cache *pcache, jack_nframes_t expected_framecnt );
    int packet_cache_retreive_packet_pointer( packet_cache *pcache, jack_nframes_t framecnt, char **packet_buf, int pkt_size, jack_time_t *timestamp );
    int packet_cache_release_packet( packet_cache *pcache, jack_nframes_t framecnt );
    int packet_cache_get_next_available_framecnt( packet_cache *pcache, jack_nframes_t expected_framecnt, jack_nframes_t *framecnt );
    int packet_cache_get_highest_available_framecnt( packet_cache *pcache, jack_nframes_t *framecnt );
    int packet_cache_find_latency( packet_cache *pcache, jack_nframes_t expected_framecnt, jack_nframes_t *framecnt );

    // Function Prototypes

    int netjack_poll_deadline (int sockfd, jack_time_t deadline);
    void netjack_sendto(int sockfd, char *packet_buf, int pkt_size, int flags, struct sockaddr *addr, int addr_size, int mtu);
    int get_sample_size(int bitdepth);
    void packet_header_hton(jacknet_packet_header *pkthdr);
    void packet_header_ntoh(jacknet_packet_header *pkthdr);
    void render_payload_to_jack_ports(int bitdepth, void *packet_payload, jack_nframes_t net_period_down, JSList *capture_ports, JSList *capture_srcs, jack_nframes_t nframes, int dont_htonl_floats );
    void render_jack_ports_to_payload(int bitdepth, JSList *playback_ports, JSList *playback_srcs, jack_nframes_t nframes, void *packet_payload, jack_nframes_t net_period_up, int dont_htonl_floats );

    // XXX: This is sort of deprecated:
    //      This one waits forever. an is not using ppoll
    int netjack_poll(int sockfd, int timeout);

    void decode_midi_buffer (uint32_t *buffer_uint32, unsigned int buffer_size_uint32, jack_default_audio_sample_t* buf);
    void encode_midi_buffer (uint32_t *buffer_uint32, unsigned int buffer_size_uint32, jack_default_audio_sample_t* buf);

#ifdef __cplusplus
}
#endif
#endif

