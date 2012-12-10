
/* -*- mode: c; c-file-style: "linux"; -*- */
/*
NetJack Abstraction.

Copyright (C) 2008 Pieter Palmers <pieterpalmers@users.sourceforge.net>
Copyright (C) 2006 Torben Hohn <torbenh@gmx.de>
Copyright (C) 2003 Robert Ham <rah@bash.sh>
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

$Id: net_driver.c,v 1.17 2006/04/16 20:16:10 torbenh Exp $
*/

#include <math.h>
#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>

#include <jack/types.h>
#include "jack/jslist.h"

#include <sys/types.h>

#ifdef WIN32
#include <winsock.h>
#include <malloc.h>
#define socklen_t int
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#if HAVE_SAMPLERATE
#include <samplerate.h>
#endif

#include "netjack.h"
#include "netjack_packet.h"
#include "JackError.h"

#define MIN(x,y) ((x)<(y) ? (x) : (y))

static int sync_state = 1;
static jack_transport_state_t last_transport_state;

static int
net_driver_sync_cb(jack_transport_state_t state, jack_position_t *pos, void *data)
{
    int retval = sync_state;

    if (state == JackTransportStarting && last_transport_state != JackTransportStarting) {
        retval = 0;
    }
//    if (state == JackTransportStarting)
//		jack_info("Starting sync_state = %d", sync_state);
    last_transport_state = state;
    return retval;
}

int netjack_wait( netjack_driver_state_t *netj )
{
    int we_have_the_expected_frame = 0;
    jack_nframes_t next_frame_avail;
    jack_time_t packet_recv_time_stamp;
    jacknet_packet_header *pkthdr;

    if( !netj->next_deadline_valid ) {
        netj->next_deadline = jack_get_time() + netj->period_usecs;
        netj->next_deadline_valid = 1;
    }

    // Increment expected frame here.

    if( netj->expected_framecnt_valid ) {
        netj->expected_framecnt += 1;
    } else {
        // starting up.... lets look into the packetcache, and fetch the highest packet.
        packet_cache_drain_socket( netj->packcache, netj->sockfd );
        if( packet_cache_get_highest_available_framecnt( netj->packcache, &next_frame_avail ) ) {
            netj->expected_framecnt = next_frame_avail;
            netj->expected_framecnt_valid = 1;
        } else {
            // no packets there... start normally.
            netj->expected_framecnt = 0;
            netj->expected_framecnt_valid = 1;
        }
    }

    //jack_log( "expect %d", netj->expected_framecnt );
    // Now check if required packet is already in the cache.
    // then poll (have deadline calculated)
    // then drain socket, rinse and repeat.
    while(1) {
        if( packet_cache_get_next_available_framecnt( netj->packcache, netj->expected_framecnt, &next_frame_avail) ) {
            if( next_frame_avail == netj->expected_framecnt ) {
                we_have_the_expected_frame = 1;
                if( !netj->always_deadline )
                    break;
            }
        }
        if( ! netjack_poll_deadline( netj->sockfd, netj->next_deadline ) ) {
            break;
        }

        packet_cache_drain_socket( netj->packcache, netj->sockfd );
    }

    // check if we know who to send our packets too.
    if (!netj->srcaddress_valid)
        if( netj->packcache->master_address_valid ) {
            memcpy (&(netj->syncsource_address), &(netj->packcache->master_address), sizeof( struct sockaddr_in ) );
            netj->srcaddress_valid = 1;
        }

    // XXX: switching mode unconditionally is stupid.
    //      if we were running free perhaps we like to behave differently
    //      ie. fastforward one packet etc.
    //      well... this is the first packet we see. hmm.... dunno ;S
    //      it works... so...
    netj->running_free = 0;

    //if( !we_have_the_expected_frame )
    //    jack_error( "netxrun... %d", netj->expected_framecnt );

    if( we_have_the_expected_frame ) {

        jack_time_t now =  jack_get_time();
        if( now < netj->next_deadline )
            netj->time_to_deadline = netj->next_deadline - now;
        else
            netj->time_to_deadline = 0;

        packet_cache_retreive_packet_pointer( netj->packcache, netj->expected_framecnt, (char **) & (netj->rx_buf), netj->rx_bufsize , &packet_recv_time_stamp);
        pkthdr = (jacknet_packet_header *) netj->rx_buf;
        packet_header_ntoh(pkthdr);
        netj->deadline_goodness = (int)pkthdr->sync_state;
        netj->packet_data_valid = 1;

        int want_deadline;
        if( netj->jitter_val != 0 )
            want_deadline = netj->jitter_val;
        else if( netj->latency < 4 )
            want_deadline = -netj->period_usecs / 2;
        else
            want_deadline = (netj->period_usecs / 4 + 10 * (int)netj->period_usecs * netj->latency / 100);

        if( netj->deadline_goodness != MASTER_FREEWHEELS ) {
            if( netj->deadline_goodness < want_deadline ) {
                netj->next_deadline -= netj->period_usecs / 100;
                //jack_log( "goodness: %d, Adjust deadline: --- %d\n", netj->deadline_goodness, (int) netj->period_usecs*netj->latency/100 );
            }
            if( netj->deadline_goodness > want_deadline ) {
                netj->next_deadline += netj->period_usecs / 100;
                //jack_log( "goodness: %d, Adjust deadline: +++ %d\n", netj->deadline_goodness, (int) netj->period_usecs*netj->latency/100 );
            }
        }
//	if( netj->next_deadline < (netj->period_usecs*70/100) ) {
//		jack_error( "master is forcing deadline_offset to below 70%% of period_usecs... increase latency setting on master" );
//		netj->deadline_offset = (netj->period_usecs*90/100);
//	}

        netj->next_deadline += netj->period_usecs;
    } else {
        netj->time_to_deadline = 0;
        netj->next_deadline += netj->period_usecs;
        // bah... the packet is not there.
        // either
        // - it got lost.
        // - its late
        // - sync source is not sending anymore.

        // lets check if we have the next packets, we will just run a cycle without data.
        // in that case.

        if( packet_cache_get_next_available_framecnt( netj->packcache, netj->expected_framecnt, &next_frame_avail) ) {
            jack_nframes_t offset = next_frame_avail - netj->expected_framecnt;

            //XXX: hmm... i need to remember why resync_threshold wasnt right.
            //if( offset < netj->resync_threshold )
            if( offset < 10 ) {
                // ok. dont do nothing. we will run without data.
                // this seems to be one or 2 lost packets.
                //
                // this can also be reordered packet jitter.
                // (maybe this is not happening in real live)
                //  but it happens in netem.

                netj->packet_data_valid = 0;

                // I also found this happening, when the packet queue, is too full.
                // but wtf ? use a smaller latency. this link can handle that ;S
                if( packet_cache_get_fill( netj->packcache, netj->expected_framecnt ) > 80.0 )
                    netj->next_deadline -= netj->period_usecs / 2;


            } else {
                // the diff is too high. but we have a packet in the future.
                // lets resync.
                netj->expected_framecnt = next_frame_avail;
                packet_cache_retreive_packet_pointer( netj->packcache, netj->expected_framecnt, (char **) & (netj->rx_buf), netj->rx_bufsize, NULL );
                pkthdr = (jacknet_packet_header *) netj->rx_buf;
                packet_header_ntoh(pkthdr);
                //netj->deadline_goodness = 0;
                netj->deadline_goodness = (int)pkthdr->sync_state - (int)netj->period_usecs * offset;
                netj->next_deadline_valid = 0;
                netj->packet_data_valid = 1;
            }

        } else {
            // no packets in buffer.
            netj->packet_data_valid = 0;

            //printf( "frame %d No Packet in queue. num_lost_packets = %d \n", netj->expected_framecnt, netj->num_lost_packets );
            if( netj->num_lost_packets < 5 ) {
                // ok. No Packet in queue. The packet was either lost,
                // or we are running too fast.
                //
                // Adjusting the deadline unconditionally resulted in
                // too many xruns on master.
                // But we need to adjust for the case we are running too fast.
                // So lets check if the last packet is there now.
                //
                // It would not be in the queue anymore, if it had been
                // retrieved. This might break for redundancy, but
                // i will make the packet cache drop redundant packets,
                // that have already been retreived.
                //
                if( packet_cache_get_highest_available_framecnt( netj->packcache, &next_frame_avail) ) {
                    if( next_frame_avail == (netj->expected_framecnt - 1) ) {
                        // Ok. the last packet is there now.
                        // and it had not been retrieved.
                        //
                        // TODO: We are still dropping 2 packets.
                        //       perhaps we can adjust the deadline
                        //       when (num_packets lost == 0)

                        // This might still be too much.
                        netj->next_deadline += netj->period_usecs;
                    }
                }
            } else if( (netj->num_lost_packets <= 100) ) {
                // lets try adjusting the deadline harder, for some packets, we might have just ran 2 fast.
                netj->next_deadline += netj->period_usecs * netj->latency / 8;
            } else {

                // But now we can check for any new frame available.
                //
                if( packet_cache_get_highest_available_framecnt( netj->packcache, &next_frame_avail) ) {
                    netj->expected_framecnt = next_frame_avail;
                    packet_cache_retreive_packet_pointer( netj->packcache, netj->expected_framecnt, (char **) & (netj->rx_buf), netj->rx_bufsize, NULL );
                    pkthdr = (jacknet_packet_header *) netj->rx_buf;
                    packet_header_ntoh(pkthdr);
                    netj->deadline_goodness = pkthdr->sync_state;
                    netj->next_deadline_valid = 0;
                    netj->packet_data_valid = 1;
                    netj->running_free = 0;
                    jack_info( "resync after freerun... %d", netj->expected_framecnt );
                } else {
                    if( netj->num_lost_packets == 101 ) {
                        jack_info( "master seems gone... entering freerun mode", netj->expected_framecnt );
                    }

                    netj->running_free = 1;

                    // when we really dont see packets.
                    // reset source address. and open possibility for new master.
                    // maybe dsl reconnect. Also restart of netsource without fix
                    // reply address changes port.
                    if (netj->num_lost_packets > 200 ) {
                        netj->srcaddress_valid = 0;
                        packet_cache_reset_master_address( netj->packcache );
                    }
                }
            }
        }
    }

    int retval = 0;

    if( !netj->packet_data_valid ) {
        netj->num_lost_packets += 1;
        if( netj->num_lost_packets == 1 )
            retval = netj->period_usecs;
    } else {
        if( (netj->num_lost_packets > 1) && !netj->running_free )
            retval = (netj->num_lost_packets - 1) * netj->period_usecs;

        netj->num_lost_packets = 0;
    }

    return retval;
}

void netjack_send_silence( netjack_driver_state_t *netj, int syncstate )
{
    int tx_size = get_sample_size(netj->bitdepth) * netj->playback_channels * netj->net_period_up + sizeof(jacknet_packet_header);
    unsigned int *packet_buf, *packet_bufX;

    packet_buf = alloca( tx_size);
    jacknet_packet_header *tx_pkthdr = (jacknet_packet_header *)packet_buf;
    jacknet_packet_header *rx_pkthdr = (jacknet_packet_header *)netj->rx_buf;

    //framecnt = rx_pkthdr->framecnt;

    netj->reply_port = rx_pkthdr->reply_port;

    // offset packet_bufX by the packetheader.
    packet_bufX = packet_buf + sizeof(jacknet_packet_header) / sizeof(jack_default_audio_sample_t);

    tx_pkthdr->sync_state = syncstate;
    tx_pkthdr->framecnt = netj->expected_framecnt;

    // memset 0 the payload.
    int payload_size = get_sample_size(netj->bitdepth) * netj->playback_channels * netj->net_period_up;
    memset(packet_bufX, 0, payload_size);

    packet_header_hton(tx_pkthdr);
    if (netj->srcaddress_valid) {
        int r;
        if (netj->reply_port)
            netj->syncsource_address.sin_port = htons(netj->reply_port);

        for( r = 0; r < netj->redundancy; r++ )
            netjack_sendto(netj->outsockfd, (char *)packet_buf, tx_size,
                           0, (struct sockaddr*) & (netj->syncsource_address), sizeof(struct sockaddr_in), netj->mtu);
    }
}

void netjack_attach( netjack_driver_state_t *netj )
{
    //puts ("net_driver_attach");
    jack_port_t * port;
    char buf[32];
    unsigned int chn;
    int port_flags;

    if( netj->bitdepth == CELT_MODE ) {
#if HAVE_CELT
#if HAVE_CELT_API_0_7 || HAVE_CELT_API_0_8 || HAVE_CELT_API_0_11
        celt_int32 lookahead;
        netj->celt_mode = celt_mode_create( netj->sample_rate, netj->period_size, NULL );
#else
        celt_int32_t lookahead;
        netj->celt_mode = celt_mode_create( netj->sample_rate, 1, netj->period_size, NULL );
#endif
        celt_mode_info( netj->celt_mode, CELT_GET_LOOKAHEAD, &lookahead );
        netj->codec_latency = 2 * lookahead;
#endif
    }
    if( netj->bitdepth == OPUS_MODE ) {
#if HAVE_OPUS
        netj->opus_mode = opus_custom_mode_create(netj->sample_rate, netj->period_size, NULL);
#endif
    }

    if (netj->handle_transport_sync)
        jack_set_sync_callback(netj->client, (JackSyncCallback) net_driver_sync_cb, NULL);

    port_flags = JackPortIsOutput | JackPortIsPhysical | JackPortIsTerminal;

    for (chn = 0; chn < netj->capture_channels_audio; chn++) {
        snprintf (buf, sizeof(buf) - 1, "capture_%u", chn + 1);

        port = jack_port_register (netj->client, buf,
                                   JACK_DEFAULT_AUDIO_TYPE,
                                   port_flags, 0);
        if (!port) {
            jack_error ("NET: cannot register port for %s", buf);
            break;
        }

        netj->capture_ports =
            jack_slist_append (netj->capture_ports, port);

        if( netj->bitdepth == CELT_MODE ) {
#if HAVE_CELT
#if HAVE_CELT_API_0_11
            netj->capture_srcs = jack_slist_append(netj->capture_srcs, celt_decoder_create_custom( netj->celt_mode, 1, NULL ) );
#elif HAVE_CELT_API_0_7 || HAVE_CELT_API_0_8
            netj->capture_srcs = jack_slist_append(netj->capture_srcs, celt_decoder_create( netj->celt_mode, 1, NULL ) );
#else
            netj->capture_srcs = jack_slist_append(netj->capture_srcs, celt_decoder_create( netj->celt_mode ) );
#endif
#endif
        } else if( netj->bitdepth == OPUS_MODE ) {
#if HAVE_OPUS
            OpusCustomDecoder *decoder = opus_custom_decoder_create( netj->opus_mode, 1, NULL );
            netj->capture_srcs = jack_slist_append(netj->capture_srcs, decoder );
#endif
        } else {
#if HAVE_SAMPLERATE
            netj->capture_srcs = jack_slist_append(netj->capture_srcs, src_new(SRC_LINEAR, 1, NULL));
#endif
        }
    }

    for (chn = netj->capture_channels_audio; chn < netj->capture_channels; chn++) {
        snprintf (buf, sizeof(buf) - 1, "capture_%u", chn + 1);

        port = jack_port_register (netj->client, buf,
                                   JACK_DEFAULT_MIDI_TYPE,
                                   port_flags, 0);
        if (!port) {
            jack_error ("NET: cannot register port for %s", buf);
            break;
        }

        netj->capture_ports =
            jack_slist_append (netj->capture_ports, port);
    }

    port_flags = JackPortIsInput | JackPortIsPhysical | JackPortIsTerminal;

    for (chn = 0; chn < netj->playback_channels_audio; chn++) {
        snprintf (buf, sizeof(buf) - 1, "playback_%u", chn + 1);

        port = jack_port_register (netj->client, buf,
                                   JACK_DEFAULT_AUDIO_TYPE,
                                   port_flags, 0);

        if (!port) {
            jack_error ("NET: cannot register port for %s", buf);
            break;
        }

        netj->playback_ports =
            jack_slist_append (netj->playback_ports, port);
        if( netj->bitdepth == CELT_MODE ) {
#if HAVE_CELT
#if HAVE_CELT_API_0_11
            CELTMode *celt_mode = celt_mode_create( netj->sample_rate, netj->period_size, NULL );
            netj->playback_srcs = jack_slist_append(netj->playback_srcs, celt_decoder_create_custom( celt_mode, 1, NULL ) );
#elif HAVE_CELT_API_0_7 || HAVE_CELT_API_0_8
            CELTMode *celt_mode = celt_mode_create( netj->sample_rate, netj->period_size, NULL );
            netj->playback_srcs = jack_slist_append(netj->playback_srcs, celt_encoder_create( celt_mode, 1, NULL ) );
#else
            CELTMode *celt_mode = celt_mode_create( netj->sample_rate, 1, netj->period_size, NULL );
            netj->playback_srcs = jack_slist_append(netj->playback_srcs, celt_encoder_create( celt_mode ) );
#endif
#endif
        } else if( netj->bitdepth == OPUS_MODE ) {
#if HAVE_OPUS
            const int kbps = netj->resample_factor;
						jack_log( "OPUS %dkbps\n", kbps);

            OpusCustomMode *opus_mode = opus_custom_mode_create( netj->sample_rate, netj->period_size, NULL ); // XXX free me in the end
            OpusCustomEncoder *oe = opus_custom_encoder_create( opus_mode, 1, NULL );
            opus_custom_encoder_ctl(oe, OPUS_SET_BITRATE(kbps*1024)); // bits per second
            opus_custom_encoder_ctl(oe, OPUS_SET_COMPLEXITY(10));
            opus_custom_encoder_ctl(oe, OPUS_SET_SIGNAL(OPUS_SIGNAL_MUSIC));
            opus_custom_encoder_ctl(oe, OPUS_SET_SIGNAL(OPUS_APPLICATION_RESTRICTED_LOWDELAY));
            netj->playback_srcs = jack_slist_append(netj->playback_srcs, oe );
#endif
        } else {
#if HAVE_SAMPLERATE
            netj->playback_srcs = jack_slist_append(netj->playback_srcs, src_new(SRC_LINEAR, 1, NULL));
#endif
        }
    }
    for (chn = netj->playback_channels_audio; chn < netj->playback_channels; chn++) {
        snprintf (buf, sizeof(buf) - 1, "playback_%u", chn + 1);

        port = jack_port_register (netj->client, buf,
                                   JACK_DEFAULT_MIDI_TYPE,
                                   port_flags, 0);

        if (!port) {
            jack_error ("NET: cannot register port for %s", buf);
            break;
        }

        netj->playback_ports =
            jack_slist_append (netj->playback_ports, port);
    }

    jack_activate (netj->client);
}


void netjack_detach( netjack_driver_state_t *netj )
{
    JSList * node;

    for (node = netj->capture_ports; node; node = jack_slist_next (node))
        jack_port_unregister (netj->client,
                              ((jack_port_t *) node->data));

    jack_slist_free (netj->capture_ports);
    netj->capture_ports = NULL;

    for (node = netj->capture_srcs; node; node = jack_slist_next (node)) {
#if HAVE_CELT
        if( netj->bitdepth == CELT_MODE ) {
            CELTDecoder * decoder = node->data;
            celt_decoder_destroy(decoder);
        } else
#endif
#if HAVE_OPUS
        if ( netj->bitdepth == OPUS_MODE ) {
            OpusCustomDecoder * decoder = node->data;
            opus_custom_decoder_destroy(decoder);
        } else
#endif
        {
#if HAVE_SAMPLERATE
            SRC_STATE * src = node->data;
            src_delete(src);
#endif
        }
    }
    jack_slist_free (netj->capture_srcs);
    netj->playback_srcs = NULL;

    for (node = netj->playback_ports; node; node = jack_slist_next (node))
        jack_port_unregister (netj->client,
                              ((jack_port_t *) node->data));

    jack_slist_free (netj->playback_ports);
    netj->playback_ports = NULL;

    for (node = netj->playback_srcs; node; node = jack_slist_next (node)) {
#if HAVE_CELT
        if( netj->bitdepth == CELT_MODE ) {
            CELTEncoder * encoder = node->data;
            celt_encoder_destroy(encoder);
        } else
#endif
#if HAVE_OPUS
        if ( netj->bitdepth == OPUS_MODE ) {
            OpusCustomEncoder * encoder = node->data;
            opus_custom_encoder_destroy(encoder);
        } else
#endif
        {
#if HAVE_SAMPLERATE
            SRC_STATE * src = node->data;
            src_delete(src);
#endif
        }
    }
    jack_slist_free (netj->playback_srcs);
    netj->playback_srcs = NULL;

#if HAVE_CELT
    if( netj->bitdepth == CELT_MODE )
        celt_mode_destroy(netj->celt_mode);
#endif
#if HAVE_OPUS
    if( netj->bitdepth == OPUS_MODE )
        opus_custom_mode_destroy(netj->opus_mode);
#endif
}


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
                                      int jitter_val )
{

    // Fill in netj values.
    // might be subject to autoconfig...
    // so dont calculate anything with them...


    netj->sample_rate = sample_rate;
    netj->period_size = period_size;
    netj->dont_htonl_floats = dont_htonl_floats;

    netj->listen_port   = listen_port;

    netj->capture_channels  = capture_ports + capture_ports_midi;
    netj->capture_channels_audio  = capture_ports;
    netj->capture_channels_midi   = capture_ports_midi;
    netj->capture_ports     = NULL;
    netj->playback_channels = playback_ports + playback_ports_midi;
    netj->playback_channels_audio = playback_ports;
    netj->playback_channels_midi = playback_ports_midi;
    netj->playback_ports    = NULL;
    netj->codec_latency = 0;

    netj->handle_transport_sync = transport_sync;
    netj->mtu = 1400;
    netj->latency = latency;
    netj->redundancy = redundancy;
    netj->use_autoconfig = use_autoconfig;
    netj->always_deadline = always_deadline;


    netj->client = client;


    if ((bitdepth != 0) && (bitdepth != 8) && (bitdepth != 16) && (bitdepth != CELT_MODE) && (bitdepth != OPUS_MODE)) {
        jack_info ("Invalid bitdepth: %d (8, 16 or 0 for float) !!!", bitdepth);
        return NULL;
    }
    netj->bitdepth = bitdepth;


    if (resample_factor_up == 0)
        resample_factor_up = resample_factor;

    netj->resample_factor = resample_factor;
    netj->resample_factor_up = resample_factor_up;

    netj->jitter_val = jitter_val;

    return netj;
}

void netjack_release( netjack_driver_state_t *netj )
{
    close( netj->sockfd );
    close( netj->outsockfd );

    packet_cache_free( netj->packcache );
    netj->packcache = NULL;
}

int
netjack_startup( netjack_driver_state_t *netj )
{
    int first_pack_len;
    struct sockaddr_in address;
    // Now open the socket, and wait for the first packet to arrive...
    netj->sockfd = socket (AF_INET, SOCK_DGRAM, 0);
#ifdef WIN32
    if (netj->sockfd == INVALID_SOCKET)
#else
    if (netj->sockfd == -1)
#endif
    {
        jack_info ("socket error");
        return -1;
    }
    address.sin_family = AF_INET;
    address.sin_port = htons(netj->listen_port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind (netj->sockfd, (struct sockaddr *) &address, sizeof (address)) < 0) {
        jack_info("bind error");
        return -1;
    }

    netj->outsockfd = socket (AF_INET, SOCK_DGRAM, 0);
#ifdef WIN32
    if (netj->outsockfd == INVALID_SOCKET)
#else
    if (netj->outsockfd == -1)
#endif
    {
        jack_info ("socket error");
        return -1;
    }
    netj->srcaddress_valid = 0;
    if (netj->use_autoconfig) {
        jacknet_packet_header *first_packet = alloca (sizeof (jacknet_packet_header));
#ifdef WIN32
        int address_size = sizeof( struct sockaddr_in );
#else
        socklen_t address_size = sizeof (struct sockaddr_in);
#endif
        //jack_info ("Waiting for an incoming packet !!!");
        //jack_info ("*** IMPORTANT *** Dont connect a client to jackd until the driver is attached to a clock source !!!");

        while(1) {
            if( ! netjack_poll( netj->sockfd, 1000 ) ) {
                jack_info ("Waiting aborted");
                return -1;
            }
            first_pack_len = recvfrom (netj->sockfd, (char *)first_packet, sizeof (jacknet_packet_header), 0, (struct sockaddr*) & netj->syncsource_address, &address_size);
#ifdef WIN32
            if( first_pack_len == -1 ) {
                first_pack_len = sizeof(jacknet_packet_header);
                break;
            }
#else
            if (first_pack_len == sizeof (jacknet_packet_header))
                break;
#endif
        }
        netj->srcaddress_valid = 1;

        if (first_pack_len == sizeof (jacknet_packet_header)) {
            packet_header_ntoh (first_packet);

            jack_info ("AutoConfig Override !!!");
            if (netj->sample_rate != first_packet->sample_rate) {
                jack_info ("AutoConfig Override: Master JACK sample rate = %d", first_packet->sample_rate);
                netj->sample_rate = first_packet->sample_rate;
            }

            if (netj->period_size != first_packet->period_size) {
                jack_info ("AutoConfig Override: Master JACK period size is %d", first_packet->period_size);
                netj->period_size = first_packet->period_size;
            }
            if (netj->capture_channels_audio != first_packet->capture_channels_audio) {
                jack_info ("AutoConfig Override: capture_channels_audio = %d", first_packet->capture_channels_audio);
                netj->capture_channels_audio = first_packet->capture_channels_audio;
            }
            if (netj->capture_channels_midi != first_packet->capture_channels_midi) {
                jack_info ("AutoConfig Override: capture_channels_midi = %d", first_packet->capture_channels_midi);
                netj->capture_channels_midi = first_packet->capture_channels_midi;
            }
            if (netj->playback_channels_audio != first_packet->playback_channels_audio) {
                jack_info ("AutoConfig Override: playback_channels_audio = %d", first_packet->playback_channels_audio);
                netj->playback_channels_audio = first_packet->playback_channels_audio;
            }
            if (netj->playback_channels_midi != first_packet->playback_channels_midi) {
                jack_info ("AutoConfig Override: playback_channels_midi = %d", first_packet->playback_channels_midi);
                netj->playback_channels_midi = first_packet->playback_channels_midi;
            }

            netj->mtu = first_packet->mtu;
            jack_info ("MTU is set to %d bytes", first_packet->mtu);
            netj->latency = first_packet->latency;
        }
    }
    netj->capture_channels  = netj->capture_channels_audio + netj->capture_channels_midi;
    netj->playback_channels = netj->playback_channels_audio + netj->playback_channels_midi;

    if( (netj->capture_channels * netj->period_size * netj->latency * 4) > 100000000 ) {
        jack_error( "autoconfig requests more than 100MB packet cache... bailing out" );
        exit(1);
    }

    if( netj->playback_channels > 1000 ) {
        jack_error( "autoconfig requests more than 1000 playback channels... bailing out" );
        exit(1);
    }


    if( netj->mtu < (2 * sizeof( jacknet_packet_header )) ) {
        jack_error( "bullshit mtu requested by autoconfig" );
        exit(1);
    }

    if( netj->sample_rate == 0 ) {
        jack_error( "sample_rate 0 requested by autoconfig" );
        exit(1);
    }

    // After possible Autoconfig: do all calculations...
    netj->period_usecs =
        (jack_time_t) floor ((((float) netj->period_size) / (float)netj->sample_rate)
                             * 1000000.0f);

    if( netj->latency == 0 )
        netj->deadline_offset = 50 * netj->period_usecs;
    else
        netj->deadline_offset = netj->period_usecs + 10 * netj->latency * netj->period_usecs / 100;

    if( netj->bitdepth == CELT_MODE ) {
        // celt mode.
        // TODO: this is a hack. But i dont want to change the packet header.
        netj->resample_factor = (netj->resample_factor * netj->period_size * 1024 / netj->sample_rate / 8) & (~1);
        netj->resample_factor_up = (netj->resample_factor_up * netj->period_size * 1024 / netj->sample_rate / 8) & (~1);

        netj->net_period_down = netj->resample_factor;
        netj->net_period_up = netj->resample_factor_up;
    } else if( netj->bitdepth == OPUS_MODE ) {
        // Opus mode.
        // TODO: this is a hack. But i dont want to change the packet header, either
        netj->net_period_down = (netj->resample_factor * netj->period_size * 1024 / netj->sample_rate / 8) & (~1);
        netj->net_period_up = (netj->resample_factor_up * netj->period_size * 1024 / netj->sample_rate / 8) & (~1);
    } else {
        netj->net_period_down = (float) netj->period_size / (float) netj->resample_factor;
        netj->net_period_up = (float) netj->period_size / (float) netj->resample_factor_up;
    }

    netj->rx_bufsize = sizeof (jacknet_packet_header) + netj->net_period_down * netj->capture_channels * get_sample_size (netj->bitdepth);
    netj->packcache = packet_cache_new (netj->latency + 50, netj->rx_bufsize, netj->mtu);

    netj->expected_framecnt_valid = 0;
    netj->num_lost_packets = 0;
    netj->next_deadline_valid = 0;
    netj->deadline_goodness = 0;
    netj->time_to_deadline = 0;

    // Special handling for latency=0
    if( netj->latency == 0 )
        netj->resync_threshold = 0;
    else
        netj->resync_threshold = MIN( 15, netj->latency - 1 );

    netj->running_free = 0;

    return 0;
}
