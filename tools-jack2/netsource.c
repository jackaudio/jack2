/*
NetJack Client

Copyright (C) 2008 Marc-Olivier Barre <marco@marcochapeau.org>
Copyright (C) 2008 Pieter Palmers <pieterpalmers@users.sourceforge.net>
Copyright (C) 2006 Torben Hohn <torbenh@gmx.de>

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

/** @file netsource.c
 *
 * @brief This client connects a remote slave JACK to a local JACK server assumed to be the master
 */


#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#ifdef WIN32
#include <winsock2.h>
#define socklen_t int
#include <malloc.h>
#else
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#endif

/* These two required by FreeBSD. */
#include <sys/types.h>

#include <jack/jack.h>

#include <netjack_packet.h>
#if HAVE_SAMPLERATE
#include <samplerate.h>
#endif

#if HAVE_CELT
#include <celt/celt.h>
#endif

#if HAVE_OPUS
#include <opus/opus.h>
#include <opus/opus_custom.h>
#endif

#include <math.h>

JSList *capture_ports = NULL;
JSList *capture_srcs = NULL;
int capture_channels = 0;
int capture_channels_audio = 2;
int capture_channels_midi = 1;
JSList *playback_ports = NULL;
JSList *playback_srcs = NULL;
int playback_channels = 0;
int playback_channels_audio = 2;
int playback_channels_midi = 1;
int dont_htonl_floats = 0;

int latency = 5;
jack_nframes_t factor = 1;
int bitdepth = 0;
int mtu = 1400;
int reply_port = 0;
int bind_port = 0;
int redundancy = 1;
jack_client_t *client;
packet_cache * packcache = 0;

int state_connected = 0;
int state_latency = 0;
int state_netxruns = 0;
int state_currentframe = 0;
int state_recv_packet_queue_time = 0;

int quit = 0;


int outsockfd;
int insockfd;
#ifdef WIN32
struct sockaddr_in destaddr;
struct sockaddr_in bindaddr;
#else
struct sockaddr destaddr;
struct sockaddr bindaddr;
#endif

int sync_state;
jack_transport_state_t last_transport_state;

int framecnt = 0;

int cont_miss = 0;

int freewheeling = 0;

/**
 * This Function allocates all the I/O Ports which are added the lists.
 */
void
alloc_ports (int n_capture_audio, int n_playback_audio, int n_capture_midi, int n_playback_midi)
{

    int port_flags = JackPortIsOutput;
    int chn;
    jack_port_t *port;
    char buf[32];

    capture_ports = NULL;
    /* Allocate audio capture channels */
    for (chn = 0; chn < n_capture_audio; chn++) {
        snprintf (buf, sizeof (buf) - 1, "capture_%u", chn + 1);
        port = jack_port_register (client, buf, JACK_DEFAULT_AUDIO_TYPE, port_flags, 0);
        if (!port) {
            printf( "jack_netsource: cannot register %s port\n", buf);
            break;
        }
        if (bitdepth == 1000) {
#if HAVE_CELT
#if HAVE_CELT_API_0_11
            CELTMode *celt_mode = celt_mode_create( jack_get_sample_rate( client ), jack_get_buffer_size(client), NULL );
            capture_srcs = jack_slist_append(capture_srcs, celt_decoder_create_custom( celt_mode, 1, NULL ) );
#elif HAVE_CELT_API_0_7 || HAVE_CELT_API_0_8
            CELTMode *celt_mode = celt_mode_create( jack_get_sample_rate( client ), jack_get_buffer_size(client), NULL );
            capture_srcs = jack_slist_append(capture_srcs, celt_decoder_create( celt_mode, 1, NULL ) );
#else
            CELTMode *celt_mode = celt_mode_create( jack_get_sample_rate( client ), 1, jack_get_buffer_size(client), NULL );
            capture_srcs = jack_slist_append(capture_srcs, celt_decoder_create( celt_mode ) );
#endif
#endif
        } else if (bitdepth == 999) {
#if HAVE_OPUS
            int err;
            OpusCustomMode *opus_mode = opus_custom_mode_create(jack_get_sample_rate( client ), jack_get_buffer_size(client), &err);
            if (err != OPUS_OK) { printf("OPUS MODE FAILED\n"); }
            OpusCustomDecoder *decoder = opus_custom_decoder_create(opus_mode, 1, &err);
            if (err != OPUS_OK) { printf("OPUS DECODER FAILED\n"); }
            opus_custom_decoder_init(decoder, opus_mode, 1);
            capture_srcs = jack_slist_append(capture_srcs, decoder);
#endif
        } else {
#if HAVE_SAMPLERATE
            capture_srcs = jack_slist_append (capture_srcs, src_new (SRC_LINEAR, 1, NULL));
#endif
        }
        capture_ports = jack_slist_append (capture_ports, port);
    }

    /* Allocate midi capture channels */
    for (chn = n_capture_audio; chn < n_capture_midi + n_capture_audio; chn++) {
        snprintf (buf, sizeof (buf) - 1, "capture_%u", chn + 1);
        port = jack_port_register (client, buf, JACK_DEFAULT_MIDI_TYPE, port_flags, 0);
        if (!port) {
            printf ("jack_netsource: cannot register %s port\n", buf);
            break;
        }
        capture_ports = jack_slist_append(capture_ports, port);
    }

    /* Allocate audio playback channels */
    port_flags = JackPortIsInput;
    playback_ports = NULL;
    for (chn = 0; chn < n_playback_audio; chn++) {
        snprintf (buf, sizeof (buf) - 1, "playback_%u", chn + 1);
        port = jack_port_register (client, buf, JACK_DEFAULT_AUDIO_TYPE, port_flags, 0);
        if (!port) {
            printf ("jack_netsource: cannot register %s port\n", buf);
            break;
        }
        if( bitdepth == 1000 ) {
#if HAVE_CELT
#if HAVE_CELT_API_0_11
            CELTMode *celt_mode = celt_mode_create( jack_get_sample_rate (client), jack_get_buffer_size(client), NULL );
            playback_srcs = jack_slist_append(playback_srcs, celt_encoder_create_custom( celt_mode, 1, NULL ) );
#elif HAVE_CELT_API_0_7 || HAVE_CELT_API_0_8
            CELTMode *celt_mode = celt_mode_create( jack_get_sample_rate (client), jack_get_buffer_size(client), NULL );
            playback_srcs = jack_slist_append(playback_srcs, celt_encoder_create( celt_mode, 1, NULL ) );
#else
            CELTMode *celt_mode = celt_mode_create( jack_get_sample_rate (client), 1, jack_get_buffer_size(client), NULL );
            playback_srcs = jack_slist_append(playback_srcs, celt_encoder_create( celt_mode ) );
#endif
#endif
        } else if( bitdepth == 999 ) {
#if HAVE_OPUS
            const int kbps = factor;
            printf("new opus encoder %d kbps\n", kbps);
            int err;
            OpusCustomMode *opus_mode = opus_custom_mode_create(jack_get_sample_rate (client), jack_get_buffer_size(client), &err ); // XXX free me
            if (err != OPUS_OK) { printf("OPUS MODE FAILED\n"); }
            OpusCustomEncoder *oe = opus_custom_encoder_create( opus_mode, 1, &err );
            if (err != OPUS_OK) { printf("OPUS ENCODER FAILED\n"); }
            opus_custom_encoder_ctl(oe, OPUS_SET_BITRATE(kbps*1024)); // bits per second
            opus_custom_encoder_ctl(oe, OPUS_SET_COMPLEXITY(10));
            opus_custom_encoder_ctl(oe, OPUS_SET_SIGNAL(OPUS_SIGNAL_MUSIC));
            opus_custom_encoder_ctl(oe, OPUS_SET_SIGNAL(OPUS_APPLICATION_RESTRICTED_LOWDELAY));
            opus_custom_encoder_init(oe, opus_mode, 1);
            playback_srcs = jack_slist_append(playback_srcs, oe);
#endif
        } else {
#if HAVE_SAMPLERATE
            playback_srcs = jack_slist_append (playback_srcs, src_new (SRC_LINEAR, 1, NULL));
#endif
        }
        playback_ports = jack_slist_append (playback_ports, port);
    }

    /* Allocate midi playback channels */
    for (chn = n_playback_audio; chn < n_playback_midi + n_playback_audio; chn++) {
        snprintf (buf, sizeof (buf) - 1, "playback_%u", chn + 1);
        port = jack_port_register (client, buf, JACK_DEFAULT_MIDI_TYPE, port_flags, 0);
        if (!port) {
            printf ("jack_netsource: cannot register %s port\n", buf);
            break;
        }
        playback_ports = jack_slist_append (playback_ports, port);
    }
}

/**
 * The Sync callback... sync state is set elsewhere...
 * we will see if this is working correctly.
 * i dont really believe in it yet.
 */
int
sync_cb (jack_transport_state_t state, jack_position_t *pos, void *arg)
{
    static int latency_count = 0;
    int retval = sync_state;

    if (! state_connected) {
        return 1;
    }
    if (latency_count) {
        latency_count--;
        retval = 0;
    }

    else if (state == JackTransportStarting && last_transport_state != JackTransportStarting) {
        retval = 0;
        latency_count = latency - 1;
    }

    last_transport_state = state;
    return retval;
}

void
freewheel_cb (int starting, void *arg)
{
    freewheeling = starting;
}

int deadline_goodness = 0;
/**
 * The process callback for this JACK application.
 * It is called by JACK at the appropriate times.
 */
int
process (jack_nframes_t nframes, void *arg)
{
    jack_nframes_t net_period;
    int rx_bufsize, tx_bufsize;

    jack_default_audio_sample_t *buf;
    jack_port_t *port;
    JSList *node;
    int chn;
    int size, i;
    const char *porttype;
    int input_fd;

    jack_position_t local_trans_pos;

    uint32_t *packet_buf_tx, *packet_bufX;
    uint32_t *rx_packet_ptr;
    jack_time_t packet_recv_timestamp;

    if( bitdepth == 1000 || bitdepth == 999)
        net_period = (factor * jack_get_buffer_size(client) * 1024 / jack_get_sample_rate(client) / 8) & (~1) ;
    else
        net_period = (float) nframes / (float) factor;

    rx_bufsize =  get_sample_size (bitdepth) * capture_channels * net_period + sizeof (jacknet_packet_header);
    tx_bufsize =  get_sample_size (bitdepth) * playback_channels * net_period + sizeof (jacknet_packet_header);

    /* Allocate a buffer where both In and Out Buffer will fit */
    packet_buf_tx = alloca (tx_bufsize);

    jacknet_packet_header *pkthdr_tx = (jacknet_packet_header *) packet_buf_tx;

    /*
     * for latency==0 we need to send out the packet before we wait on the reply.
     * but this introduces a cycle of latency, when netsource is connected to itself.
     * so we send out before read only in zero latency mode.
     *
     */

    if( latency == 0 ) {
        /* reset packet_bufX... */
        packet_bufX = packet_buf_tx + sizeof (jacknet_packet_header) / sizeof (jack_default_audio_sample_t);

        /* ---------- Send ---------- */
        render_jack_ports_to_payload (bitdepth, playback_ports, playback_srcs, nframes,
                                      packet_bufX, net_period, dont_htonl_floats);

        /* fill in packet hdr */
        pkthdr_tx->transport_state = jack_transport_query (client, &local_trans_pos);
        pkthdr_tx->transport_frame = local_trans_pos.frame;
        pkthdr_tx->framecnt = framecnt;
        pkthdr_tx->latency = latency;
        pkthdr_tx->reply_port = reply_port;
        pkthdr_tx->sample_rate = jack_get_sample_rate (client);
        pkthdr_tx->period_size = nframes;

        /* playback for us is capture on the other side */
        pkthdr_tx->capture_channels_audio = playback_channels_audio;
        pkthdr_tx->playback_channels_audio = capture_channels_audio;
        pkthdr_tx->capture_channels_midi = playback_channels_midi;
        pkthdr_tx->playback_channels_midi = capture_channels_midi;
        pkthdr_tx->mtu = mtu;
        if( freewheeling != 0 )
            pkthdr_tx->sync_state = (jack_nframes_t)MASTER_FREEWHEELS;
        else
            pkthdr_tx->sync_state = (jack_nframes_t)deadline_goodness;
        //printf("goodness=%d\n", deadline_goodness );

        packet_header_hton (pkthdr_tx);
        if (cont_miss < 3 * latency + 5) {
            int r;
            for( r = 0; r < redundancy; r++ )
                netjack_sendto (outsockfd, (char *) packet_buf_tx, tx_bufsize, 0, &destaddr, sizeof (destaddr), mtu);
        } else if (cont_miss > 50 + 5 * latency) {
            state_connected = 0;
            packet_cache_reset_master_address( packcache );
            //printf ("Frame %d  \tRealy too many packets missed (%d). Let's reset the counter\n", framecnt, cont_miss);
            cont_miss = 0;
        }
    }

    /*
     * ok... now the RECEIVE code.
     *
     */


    if( reply_port )
        input_fd = insockfd;
    else
        input_fd = outsockfd;

    // for latency == 0 we can poll.
    if( (latency == 0) || (freewheeling != 0)  ) {
        jack_time_t deadline = jack_get_time() + 1000000 * jack_get_buffer_size(client) / jack_get_sample_rate(client);
        // Now loop until we get the right packet.
        while(1) {
            jack_nframes_t got_frame;
            if ( ! netjack_poll_deadline( input_fd, deadline ) )
                break;

            packet_cache_drain_socket(packcache, input_fd);

            if (packet_cache_get_next_available_framecnt( packcache, framecnt - latency, &got_frame ))
                if( got_frame == (framecnt - latency) )
                    break;
        }
    } else {
        // normally:
        // only drain socket.
        packet_cache_drain_socket(packcache, input_fd);
    }

    size = packet_cache_retreive_packet_pointer( packcache, framecnt - latency, (char**)&rx_packet_ptr, rx_bufsize, &packet_recv_timestamp );
    /* First alternative : we received what we expected. Render the data
     * to the JACK ports so it can be played. */
    if (size == rx_bufsize) {
        uint32_t *packet_buf_rx = rx_packet_ptr;
        jacknet_packet_header *pkthdr_rx = (jacknet_packet_header *) packet_buf_rx;
        packet_bufX = packet_buf_rx + sizeof (jacknet_packet_header) / sizeof (jack_default_audio_sample_t);
        // calculate how much time there would have been, if this packet was sent at the deadline.

        int recv_time_offset = (int) (jack_get_time() - packet_recv_timestamp);
        packet_header_ntoh (pkthdr_rx);
        deadline_goodness = recv_time_offset - (int)pkthdr_rx->latency;
        //printf( "deadline goodness = %d ---> off: %d\n", deadline_goodness, recv_time_offset );

        if (cont_miss) {
            //printf("Frame %d  \tRecovered from dropouts\n", framecnt);
            cont_miss = 0;
        }
        render_payload_to_jack_ports (bitdepth, packet_bufX, net_period,
                                      capture_ports, capture_srcs, nframes, dont_htonl_floats);

        state_currentframe = framecnt;
        state_recv_packet_queue_time = recv_time_offset;
        state_connected = 1;
        sync_state = pkthdr_rx->sync_state;
        packet_cache_release_packet( packcache, framecnt - latency );
    }
    /* Second alternative : we've received something that's not
     * as big as expected or we missed a packet. We render silence
     * to the ouput ports */
    else {
        jack_nframes_t latency_estimate;
        if( packet_cache_find_latency( packcache, framecnt, &latency_estimate ) )
            //if( (state_latency == 0) || (latency_estimate < state_latency) )
            state_latency = latency_estimate;

        // Set the counters up.
        state_currentframe = framecnt;
        //state_latency = framecnt - pkthdr->framecnt;
        state_netxruns += 1;

        //printf ("Frame %d  \tPacket missed or incomplete (expected: %d bytes, got: %d bytes)\n", framecnt, rx_bufsize, size);
        //printf ("Frame %d  \tPacket missed or incomplete\n", framecnt);
        cont_miss += 1;
        chn = 0;
        node = capture_ports;
        while (node != NULL) {
            port = (jack_port_t *) node->data;
            buf = jack_port_get_buffer (port, nframes);
            porttype = jack_port_type (port);
            if (strncmp (porttype, JACK_DEFAULT_AUDIO_TYPE, jack_port_type_size ()) == 0)
                for (i = 0; i < nframes; i++)
                    buf[i] = 0.0;
            else if (strncmp (porttype, JACK_DEFAULT_MIDI_TYPE, jack_port_type_size ()) == 0)
                jack_midi_clear_buffer (buf);
            node = jack_slist_next (node);
            chn++;
        }
    }
    if (latency != 0) {
        /* reset packet_bufX... */
        packet_bufX = packet_buf_tx + sizeof (jacknet_packet_header) / sizeof (jack_default_audio_sample_t);

        /* ---------- Send ---------- */
        render_jack_ports_to_payload (bitdepth, playback_ports, playback_srcs, nframes,
                                      packet_bufX, net_period, dont_htonl_floats);

        /* fill in packet hdr */
        pkthdr_tx->transport_state = jack_transport_query (client, &local_trans_pos);
        pkthdr_tx->transport_frame = local_trans_pos.frame;
        pkthdr_tx->framecnt = framecnt;
        pkthdr_tx->latency = latency;
        pkthdr_tx->reply_port = reply_port;
        pkthdr_tx->sample_rate = jack_get_sample_rate (client);
        pkthdr_tx->period_size = nframes;

        /* playback for us is capture on the other side */
        pkthdr_tx->capture_channels_audio = playback_channels_audio;
        pkthdr_tx->playback_channels_audio = capture_channels_audio;
        pkthdr_tx->capture_channels_midi = playback_channels_midi;
        pkthdr_tx->playback_channels_midi = capture_channels_midi;
        pkthdr_tx->mtu = mtu;
        if( freewheeling != 0 )
            pkthdr_tx->sync_state = (jack_nframes_t)MASTER_FREEWHEELS;
        else
            pkthdr_tx->sync_state = (jack_nframes_t)deadline_goodness;
        //printf("goodness=%d\n", deadline_goodness );

        packet_header_hton (pkthdr_tx);
        if (cont_miss < 3 * latency + 5) {
            int r;
            for( r = 0; r < redundancy; r++ )
                netjack_sendto (outsockfd, (char *) packet_buf_tx, tx_bufsize, 0, &destaddr, sizeof (destaddr), mtu);
        } else if (cont_miss > 50 + 5 * latency) {
            state_connected = 0;
            packet_cache_reset_master_address( packcache );
            //printf ("Frame %d  \tRealy too many packets missed (%d). Let's reset the counter\n", framecnt, cont_miss);
            cont_miss = 0;
        }
    }

    framecnt++;
    return 0;
}

/**
 * This is the shutdown callback for this JACK application.
 * It is called by JACK if the server ever shuts down or
 * decides to disconnect the client.
 */

void
jack_shutdown (void *arg)
{
    fprintf(stderr, "JACK shut down, exiting ...\n");
    exit (1);
}

void
init_sockaddr_in (struct sockaddr_in *name , const char *hostname , uint16_t port)
{
    name->sin_family = AF_INET ;
    name->sin_port = htons (port);
    if (hostname) {
        struct hostent *hostinfo = gethostbyname (hostname);
        if (hostinfo == NULL) {
            fprintf (stderr, "init_sockaddr_in: unknown host: %s.\n", hostname);
            fflush( stderr );
        }
#ifdef WIN32
        name->sin_addr.s_addr = inet_addr( hostname );
#else
        name->sin_addr = *(struct in_addr *) hostinfo->h_addr ;
#endif
    } else
        name->sin_addr.s_addr = htonl (INADDR_ANY) ;

}

void
printUsage ()
{
    fprintf (stderr, "usage: jack_netsource [options]\n"
             "\n"
             "  -h this help text\n"
             "  -H <slave host> - Host name of the slave JACK\n"
             "  -o <num channels> - Number of audio playback channels\n"
             "  -i <num channels> - Number of audio capture channels\n"
             "  -O <num channels> - Number of midi playback channels\n"
             "  -I <num channels> - Number of midi capture channels\n"
             "  -n <periods> - Network latency in JACK periods\n"
             "  -p <port> - UDP port that the slave is listening on\n"
             "  -r <reply port> - UDP port that we are listening on\n"
             "  -B <bind port> - reply port, for use in NAT environments\n"
             "  -b <bitdepth> - Set transport to use 16bit or 8bit\n"
             "  -c <kbits> - Use CELT encoding with <kbits> kbits per channel\n"
             "  -P <kbits> - Use Opus encoding with <kbits> kbits per channel\n"
             "  -m <mtu> - Assume this mtu for the link\n"
             "  -R <N> - Redundancy: send out packets N times.\n"
             "  -e - skip host-to-network endianness conversion\n"
             "  -N <jack name> - Reports a different name to jack\n"
             "  -s <server name> - The name of the local jack server\n"
             "\n");
}

void
sigterm_handler( int signal )
{
    quit = 1;
}

int
main (int argc, char *argv[])
{
    /* Some startup related basics */
    char *client_name, *server_name = NULL, *peer_ip;
    int peer_port = 3000;
    jack_options_t options = JackNullOption;
    jack_status_t status;
#ifdef WIN32
    WSADATA wsa;
    int rc = WSAStartup(MAKEWORD(2, 0), &wsa);
#endif
    /* Torben's famous state variables, aka "the reporting API" ! */
    /* heh ? these are only the copies of them ;)                 */
    int statecopy_connected, statecopy_latency, statecopy_netxruns;
    jack_nframes_t net_period;
    /* Argument parsing stuff */
    extern char *optarg;
    extern int optind, optopt;
    int errflg = 0, c;

    if (argc < 3) {
        printUsage ();
        return 1;
    }

    client_name = (char *) malloc (sizeof (char) * 10);
    peer_ip = (char *) malloc (sizeof (char) * 10);
    sprintf(client_name, "netjack");
    sprintf(peer_ip, "localhost");

    while ((c = getopt (argc, argv, ":h:H:o:i:O:I:n:p:r:B:b:c:m:R:e:N:s:P:")) != -1) {
        switch (c) {
            case 'h':
                printUsage();
                exit (0);
                break;
            case 'H':
                free(peer_ip);
                peer_ip = (char *) malloc (sizeof (char) * strlen (optarg) + 1);
                strcpy (peer_ip, optarg);
                break;
            case 'o':
                playback_channels_audio = atoi (optarg);
                break;
            case 'i':
                capture_channels_audio = atoi (optarg);
                break;
            case 'O':
                playback_channels_midi = atoi (optarg);
                break;
            case 'I':
                capture_channels_midi = atoi (optarg);
                break;
            case 'n':
                latency = atoi (optarg);
                break;
            case 'p':
                peer_port = atoi (optarg);
                break;
            case 'r':
                reply_port = atoi (optarg);
                break;
            case 'B':
                bind_port = atoi (optarg);
                break;
            case 'f':
                factor = atoi (optarg);
                printf("This feature is deprecated and will be removed in future netjack versions. CELT offers a superiour way to conserve bandwidth");
                break;
            case 'b':
                bitdepth = atoi (optarg);
                break;
            case 'c':
#if HAVE_CELT
                bitdepth = 1000;
                factor = atoi (optarg);
#else
                printf( "not built with celt support\n" );
                exit(10);
#endif
                break;
            case 'P':
#if HAVE_OPUS
                bitdepth = 999;
                factor = atoi (optarg);
#else
                printf( "not built with opus support\n" );
                exit(10);
#endif
                break;
            case 'm':
                mtu = atoi (optarg);
                break;
            case 'R':
                redundancy = atoi (optarg);
                break;
            case 'e':
                dont_htonl_floats = 1;
                break;
            case 'N':
                free(client_name);
                client_name = (char *) malloc (sizeof (char) * strlen (optarg) + 1);
                strcpy (client_name, optarg);
                break;
            case 's':
                server_name = (char *) malloc (sizeof (char) * strlen (optarg) + 1);
                strcpy (server_name, optarg);
                options |= JackServerName;
                break;
            case ':':
                fprintf (stderr, "Option -%c requires an operand\n", optopt);
                errflg++;
                break;
            case '?':
                fprintf (stderr, "Unrecognized option: -%c\n", optopt);
                errflg++;
        }
    }
    if (errflg) {
        printUsage ();
        exit (2);
    }

    capture_channels = capture_channels_audio + capture_channels_midi;
    playback_channels = playback_channels_audio + playback_channels_midi;

    outsockfd = socket (AF_INET, SOCK_DGRAM, 0);
    insockfd = socket (AF_INET, SOCK_DGRAM, 0);

    if ((outsockfd == -1) || (insockfd == -1)) {
        fprintf (stderr, "cant open sockets\n" );
        return 1;
    }

    init_sockaddr_in ((struct sockaddr_in *) &destaddr, peer_ip, peer_port);
    if (bind_port) {
        init_sockaddr_in ((struct sockaddr_in *) &bindaddr, NULL, bind_port);
        if( bind (outsockfd, &bindaddr, sizeof (bindaddr)) ) {
            fprintf (stderr, "bind failure\n" );
        }
    }
    if (reply_port) {
        init_sockaddr_in ((struct sockaddr_in *) &bindaddr, NULL, reply_port);
        if( bind (insockfd, &bindaddr, sizeof (bindaddr)) ) {
            fprintf (stderr, "bind failure\n" );
        }
    }

    /* try to become a client of the JACK server */
    client = jack_client_open (client_name, options, &status, server_name);
    if (client == NULL) {
        fprintf (stderr, "jack_client_open() failed, status = 0x%2.0x\n"
                 "Is the JACK server running ?\n", status);
        return 1;
    }

    /* Set up jack callbacks */
    jack_set_process_callback (client, process, 0);
    jack_set_sync_callback (client, sync_cb, 0);
    jack_set_freewheel_callback (client, freewheel_cb, 0);
    jack_on_shutdown (client, jack_shutdown, 0);

    alloc_ports (capture_channels_audio, playback_channels_audio, capture_channels_midi, playback_channels_midi);

    if( bitdepth == 1000 || bitdepth == 999)
        net_period = (factor * jack_get_buffer_size(client) * 1024 / jack_get_sample_rate(client) / 8) & (~1) ;
    else
        net_period = ceilf((float) jack_get_buffer_size (client) / (float) factor);

    int rx_bufsize =  get_sample_size (bitdepth) * capture_channels * net_period + sizeof (jacknet_packet_header);
    packcache = packet_cache_new (latency + 50, rx_bufsize, mtu);

    /* tell the JACK server that we are ready to roll */
    if (jack_activate (client)) {
        fprintf (stderr, "Cannot activate client");
        return 1;
    }

    /* Now sleep forever... and evaluate the state_ vars */

    signal( SIGTERM, sigterm_handler );
    signal( SIGINT, sigterm_handler );

    statecopy_connected = 2; // make it report unconnected on start.
    statecopy_latency = state_latency;
    statecopy_netxruns = state_netxruns;

    while ( !quit ) {
#ifdef WIN32
        Sleep (1000);
#else
        sleep(1);
#endif
        if (statecopy_connected != state_connected) {
            statecopy_connected = state_connected;
            if (statecopy_connected) {
                state_netxruns = 1; // We want to reset the netxrun count on each new connection
                printf ("Connected :-)\n");
            } else
                printf ("Not Connected\n");

            fflush(stdout);
        }

        if (statecopy_connected) {
            if (statecopy_netxruns != state_netxruns) {
                statecopy_netxruns = state_netxruns;
                printf ("%s: at frame %06d -> total netxruns %d  (%d%%) queue time= %d\n",
                        client_name,
                        state_currentframe,
                        statecopy_netxruns,
                        100 * statecopy_netxruns / state_currentframe,
                        state_recv_packet_queue_time);

                fflush(stdout);
            }
        } else {
            if (statecopy_latency != state_latency) {
                statecopy_latency = state_latency;
                if (statecopy_latency > 1)
                    printf ("current latency %d\n", statecopy_latency);
                fflush(stdout);
            }
        }
    }

    jack_client_close (client);
    packet_cache_free (packcache);
    exit (0);
}
