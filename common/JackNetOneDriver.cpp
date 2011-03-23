/*
Copyright (C) 2001 Paul Davis
Copyright (C) 2008 Romain Moret at Grame

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

#ifdef WIN32
#include <malloc.h>
#endif

#include "JackNetOneDriver.h"
#include "JackEngineControl.h"
#include "JackGraphManager.h"
#include "JackWaitThreadedDriver.h"
#include "JackTools.h"
#include "driver_interface.h"

#include "netjack.h"
#include "netjack_packet.h"

#if HAVE_SAMPLERATE
#include "samplerate.h"
#endif

#if HAVE_CELT
#include "celt/celt.h"
#endif

#define MIN(x,y) ((x)<(y) ? (x) : (y))

using namespace std;

namespace Jack
{
    JackNetOneDriver::JackNetOneDriver ( const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table,
                                    int port, int mtu, int capture_ports, int playback_ports, int midi_input_ports, int midi_output_ports,
                                    int sample_rate, int period_size, int resample_factor,
                                    const char* net_name, uint transport_sync, int bitdepth, int use_autoconfig,
                                    int latency, int redundancy, int dont_htonl_floats, int always_deadline, int jitter_val )
            : JackAudioDriver ( name, alias, engine, table )
    {
        jack_log ( "JackNetOneDriver::JackNetOneDriver port %d", port );

    #ifdef WIN32
        WSADATA wsa;
        int rc = WSAStartup(MAKEWORD(2,0),&wsa);
    #endif

        netjack_init( & (this->netj),
            NULL, // client
                    name,
                    capture_ports,
                    playback_ports,
                    midi_input_ports,
                    midi_output_ports,
                    sample_rate,
                    period_size,
                    port,
                    transport_sync,
                    resample_factor,
                    0,
                    bitdepth,
            use_autoconfig,
            latency,
            redundancy,
            dont_htonl_floats,
            always_deadline,
            jitter_val);
    }

    JackNetOneDriver::~JackNetOneDriver()
    {
        // No destructor yet.
    }

//open, close, attach and detach------------------------------------------------------
    int JackNetOneDriver::Open ( jack_nframes_t buffer_size, jack_nframes_t samplerate, bool capturing, bool playing,
                              int inchannels, int outchannels, bool monitor,
                              const char* capture_driver_name, const char* playback_driver_name,
                              jack_nframes_t capture_latency, jack_nframes_t playback_latency )
    {
        if ( JackAudioDriver::Open ( buffer_size,
                                     samplerate,
                                     capturing,
                                     playing,
                                     inchannels,
                                     outchannels,
                                     monitor,
                                     capture_driver_name,
                                     playback_driver_name,
                                     capture_latency,
                                     playback_latency ) == 0 )
        {
            fEngineControl->fPeriod = 0;
            fEngineControl->fComputation = 500 * 1000;
            fEngineControl->fConstraint = 500 * 1000;
            return 0;
        }
        else
        {
            jack_error( "open fail" );
            return -1;
        }
    }

    int JackNetOneDriver::Close()
    {
        FreePorts();
        netjack_release( &netj );
        return JackDriver::Close();
    }

    int JackNetOneDriver::Attach()
    {
        return 0;
    }

    int JackNetOneDriver::Detach()
    {
        return 0;
    }

    int JackNetOneDriver::AllocPorts()
    {
        jack_port_id_t port_id;
        char buf[64];
        unsigned int chn;

        //if (netj.handle_transport_sync)
        //    jack_set_sync_callback(netj.client, (JackSyncCallback) net_driver_sync_cb, NULL);

        for (chn = 0; chn < netj.capture_channels_audio; chn++) {
            snprintf (buf, sizeof(buf) - 1, "system:capture_%u", chn + 1);

                if ( ( port_id = fGraphManager->AllocatePort ( fClientControl.fRefNum, buf, JACK_DEFAULT_AUDIO_TYPE,
                                 CaptureDriverFlags, fEngineControl->fBufferSize ) ) == NO_PORT )
                {
                    jack_error ( "driver: cannot register port for %s", buf );
                    return -1;
                }
                //port = fGraphManager->GetPort ( port_id );

            netj.capture_ports =
            jack_slist_append (netj.capture_ports, (void *)(intptr_t)port_id);

            if( netj.bitdepth == CELT_MODE ) {
        #if HAVE_CELT
        #if HAVE_CELT_API_0_7 || HAVE_CELT_API_0_8
                celt_int32 lookahead;
                CELTMode *celt_mode = celt_mode_create( netj.sample_rate, netj.period_size, NULL );
                netj.capture_srcs = jack_slist_append(netj.capture_srcs, celt_decoder_create( celt_mode, 1, NULL ) );
        #else
                celt_int32_t lookahead;
                CELTMode *celt_mode = celt_mode_create( netj.sample_rate, 1, netj.period_size, NULL );
                netj.capture_srcs = jack_slist_append(netj.capture_srcs, celt_decoder_create( celt_mode ) );
        #endif
                celt_mode_info( celt_mode, CELT_GET_LOOKAHEAD, &lookahead );
                netj.codec_latency = 2*lookahead;
        #endif
            } else {
        #if HAVE_SAMPLERATE
                netj.capture_srcs = jack_slist_append(netj.capture_srcs, (void *)src_new(SRC_LINEAR, 1, NULL));
        #endif
            }
        }
        for (chn = netj.capture_channels_audio; chn < netj.capture_channels; chn++) {
            snprintf (buf, sizeof(buf) - 1, "system:capture_%u", chn + 1);

            if ( ( port_id = fGraphManager->AllocatePort ( fClientControl.fRefNum, buf, JACK_DEFAULT_MIDI_TYPE,
                             CaptureDriverFlags, fEngineControl->fBufferSize ) ) == NO_PORT )
            {
                jack_error ( "driver: cannot register port for %s", buf );
                return -1;
            }
            //port = fGraphManager->GetPort ( port_id );

            netj.capture_ports =
            jack_slist_append (netj.capture_ports, (void *)(intptr_t)port_id);
        }

        for (chn = 0; chn < netj.playback_channels_audio; chn++) {
            snprintf (buf, sizeof(buf) - 1, "system:playback_%u", chn + 1);

            if ( ( port_id = fGraphManager->AllocatePort ( fClientControl.fRefNum, buf, JACK_DEFAULT_AUDIO_TYPE,
                             PlaybackDriverFlags, fEngineControl->fBufferSize ) ) == NO_PORT )
            {
                jack_error ( "driver: cannot register port for %s", buf );
                return -1;
            }
            //port = fGraphManager->GetPort ( port_id );

            netj.playback_ports =
            jack_slist_append (netj.playback_ports, (void *)(intptr_t)port_id);

            if( netj.bitdepth == CELT_MODE ) {
        #if HAVE_CELT
        #if HAVE_CELT_API_0_7 || HAVE_CELT_API_0_8
                CELTMode *celt_mode = celt_mode_create( netj.sample_rate, netj.period_size, NULL );
                netj.playback_srcs = jack_slist_append(netj.playback_srcs, celt_encoder_create( celt_mode, 1, NULL ) );
        #else
                CELTMode *celt_mode = celt_mode_create( netj.sample_rate, 1, netj.period_size, NULL );
                netj.playback_srcs = jack_slist_append(netj.playback_srcs, celt_encoder_create( celt_mode ) );
        #endif
        #endif
                } else {
        #if HAVE_SAMPLERATE
                netj.playback_srcs = jack_slist_append(netj.playback_srcs, (void *)src_new(SRC_LINEAR, 1, NULL));
        #endif
            }
        }
        for (chn = netj.playback_channels_audio; chn < netj.playback_channels; chn++) {
            snprintf (buf, sizeof(buf) - 1, "system:playback_%u", chn + 1);

            if ( ( port_id = fGraphManager->AllocatePort ( fClientControl.fRefNum, buf, JACK_DEFAULT_MIDI_TYPE,
                             PlaybackDriverFlags, fEngineControl->fBufferSize ) ) == NO_PORT )
            {
                jack_error ( "driver: cannot register port for %s", buf );
                return -1;
            }
            //port = fGraphManager->GetPort ( port_id );

            netj.playback_ports =
                jack_slist_append (netj.playback_ports, (void *)(intptr_t)port_id);
        }
        return 0;
    }

//init and restart--------------------------------------------------------------------
    bool JackNetOneDriver::Initialize()
    {
        jack_log ( "JackNetOneDriver::Init()" );

        FreePorts();
        netjack_release( &netj );

        //display some additional infos
        jack_info ( "NetOne driver started" );
        if( netjack_startup( &netj ) ) {
            return false;
        }

        //register jack ports
        if ( AllocPorts() != 0 )
        {
            jack_error ( "Can't allocate ports." );
            return false;
        }

        //monitor
        //driver parametering
        JackAudioDriver::SetBufferSize ( netj.period_size );
        JackAudioDriver::SetSampleRate ( netj.sample_rate );

        JackDriver::NotifyBufferSize ( netj.period_size );
        JackDriver::NotifySampleRate ( netj.sample_rate );

        //transport engine parametering
        fEngineControl->fTransport.SetNetworkSync ( true );
        return true;
    }


//jack ports and buffers--------------------------------------------------------------

//driver processes--------------------------------------------------------------------
    int JackNetOneDriver::Read()
    {
        int delay;
        delay = netjack_wait( &netj );
        if( delay ) {
            NotifyXRun(fBeginDateUst, (float) delay);
            jack_error( "netxruns... duration: %dms", delay/1000 );
        }

        if( (netj.num_lost_packets * netj.period_size / netj.sample_rate) > 2 )
            JackTools::ThrowJackNetException();

        //netjack_read( &netj, netj.period_size );
        JackDriver::CycleTakeBeginTime();

        jack_position_t local_trans_pos;
        jack_transport_state_t local_trans_state;

        unsigned int *packet_buf, *packet_bufX;

        if( ! netj.packet_data_valid ) {
            jack_log( "data not valid" );
            render_payload_to_jack_ports (netj.bitdepth, NULL, netj.net_period_down, netj.capture_ports, netj.capture_srcs, netj.period_size, netj.dont_htonl_floats );
            return 0;
        }
        packet_buf = netj.rx_buf;

        jacknet_packet_header *pkthdr = (jacknet_packet_header *)packet_buf;

        packet_bufX = packet_buf + sizeof(jacknet_packet_header) / sizeof(jack_default_audio_sample_t);

        netj.reply_port = pkthdr->reply_port;
        netj.latency = pkthdr->latency;

        // Special handling for latency=0
        if( netj.latency == 0 )
            netj.resync_threshold = 0;
        else
            netj.resync_threshold = MIN( 15, pkthdr->latency-1 );

        // check whether, we should handle the transport sync stuff, or leave trnasports untouched.
        if (netj.handle_transport_sync) {
    #if 1
            unsigned int compensated_tranport_pos = (pkthdr->transport_frame + (pkthdr->latency * netj.period_size) + netj.codec_latency);

            // read local transport info....
            //local_trans_state = jack_transport_query(netj.client, &local_trans_pos);

            local_trans_state = fEngineControl->fTransport.Query ( &local_trans_pos );

            // Now check if we have to start or stop local transport to sync to remote...
            switch (pkthdr->transport_state) {

                case JackTransportStarting:
                    // the master transport is starting... so we set our reply to the sync_callback;
                    if (local_trans_state == JackTransportStopped) {
                        fEngineControl->fTransport.SetCommand ( TransportCommandStart );
                        //jack_transport_start(netj.client);
                        //last_transport_state = JackTransportStopped;
                        netj.sync_state = 0;
                        jack_info("locally stopped... starting...");
                    }

                    if (local_trans_pos.frame != compensated_tranport_pos) {
                        jack_position_t new_pos = local_trans_pos;
                        new_pos.frame = compensated_tranport_pos + 2*netj.period_size;
                        new_pos.valid = (jack_position_bits_t) 0;


                        fEngineControl->fTransport.RequestNewPos ( &new_pos );
                        //jack_transport_locate(netj.client, compensated_tranport_pos);
                        //last_transport_state = JackTransportRolling;
                        netj.sync_state = 0;
                        jack_info("starting locate to %d", compensated_tranport_pos );
                    }
                    break;

                case JackTransportStopped:
                    netj.sync_state = 1;
                    if (local_trans_pos.frame != (pkthdr->transport_frame)) {
                        jack_position_t new_pos = local_trans_pos;
                        new_pos.frame = pkthdr->transport_frame;
                        new_pos.valid = (jack_position_bits_t)0;
                        fEngineControl->fTransport.RequestNewPos ( &new_pos );
                        //jack_transport_locate(netj.client, (pkthdr->transport_frame));
                        jack_info("transport is stopped locate to %d", pkthdr->transport_frame);
                    }
                    if (local_trans_state != JackTransportStopped)
                        //jack_transport_stop(netj.client);
                        fEngineControl->fTransport.SetCommand ( TransportCommandStop );
                    break;

                case JackTransportRolling:
                    netj.sync_state = 1;
        //		    if(local_trans_pos.frame != (pkthdr->transport_frame + (pkthdr->latency) * netj.period_size)) {
        //		        jack_transport_locate(netj.client, (pkthdr->transport_frame + (pkthdr->latency + 2) * netj.period_size));
        //			jack_info("running locate to %d", pkthdr->transport_frame + (pkthdr->latency)*netj.period_size);
        //		    		}
                    if (local_trans_state != JackTransportRolling)
                        fEngineControl->fTransport.SetState ( JackTransportRolling );
                    break;

                case JackTransportLooping:
                    break;
            }
#endif
        }

        render_payload_to_jack_ports (netj.bitdepth, packet_bufX, netj.net_period_down, netj.capture_ports, netj.capture_srcs, netj.period_size, netj.dont_htonl_floats );
        packet_cache_release_packet(netj.packcache, netj.expected_framecnt );
        return 0;
    }

    int JackNetOneDriver::Write()
    {
        int syncstate = netj.sync_state | ((fEngineControl->fTransport.GetState() == JackTransportNetStarting) ? 1 : 0 );
        uint32_t *packet_buf, *packet_bufX;

        int packet_size = get_sample_size(netj.bitdepth) * netj.playback_channels * netj.net_period_up + sizeof(jacknet_packet_header);
        jacknet_packet_header *pkthdr;

        packet_buf = (uint32_t *) alloca(packet_size);
        pkthdr = (jacknet_packet_header *)packet_buf;

        if( netj.running_free ) {
            return 0;
        }

        // offset packet_bufX by the packetheader.
        packet_bufX = packet_buf + sizeof(jacknet_packet_header) / sizeof(jack_default_audio_sample_t);

        pkthdr->sync_state = syncstate;;
        pkthdr->latency = netj.time_to_deadline;
        //printf( "time to deadline = %d  goodness=%d\n", (int)netj.time_to_deadline, netj.deadline_goodness );
        pkthdr->framecnt = netj.expected_framecnt;

        render_jack_ports_to_payload(netj.bitdepth, netj.playback_ports, netj.playback_srcs, netj.period_size, packet_bufX, netj.net_period_up, netj.dont_htonl_floats );

        packet_header_hton(pkthdr);
        if (netj.srcaddress_valid)
        {
            unsigned int r;
            static const int flag = 0;

            if (netj.reply_port)
            netj.syncsource_address.sin_port = htons(netj.reply_port);

            for( r=0; r<netj.redundancy; r++ )
            netjack_sendto(netj.sockfd, (char *)packet_buf, packet_size,
                flag, (struct sockaddr*)&(netj.syncsource_address), sizeof(struct sockaddr_in), netj.mtu);
        }
        return 0;
    }

    void
    JackNetOneDriver::FreePorts ()
    {
        JSList *node = netj.capture_ports;

        while( node != NULL ) {
            JSList *this_node = node;
            jack_port_id_t port_id = (jack_port_id_t)(intptr_t) node->data;
            node = jack_slist_remove_link( node, this_node );
            jack_slist_free_1( this_node );
            fGraphManager->ReleasePort( fClientControl.fRefNum, port_id );
        }
        netj.capture_ports = NULL;

        node = netj.playback_ports;
        while( node != NULL ) {
            JSList *this_node = node;
            jack_port_id_t port_id = (jack_port_id_t)(intptr_t) node->data;
            node = jack_slist_remove_link( node, this_node );
            jack_slist_free_1( this_node );
            fGraphManager->ReleasePort( fClientControl.fRefNum, port_id );
        }
        netj.playback_ports = NULL;

        if( netj.bitdepth == CELT_MODE ) {
    #if HAVE_CELT
        node = netj.playback_srcs;
        while( node != NULL ) {
            JSList *this_node = node;
            CELTEncoder *enc = (CELTEncoder *) node->data;
            node = jack_slist_remove_link( node, this_node );
            jack_slist_free_1( this_node );
            celt_encoder_destroy( enc );
        }
        netj.playback_srcs = NULL;

        node = netj.capture_srcs;
        while( node != NULL ) {
            JSList *this_node = node;
            CELTDecoder *dec = (CELTDecoder *) node->data;
            node = jack_slist_remove_link( node, this_node );
            jack_slist_free_1( this_node );
            celt_decoder_destroy( dec );
        }
        netj.capture_srcs = NULL;
    #endif
        } else {
    #if HAVE_SAMPLERATE
        node = netj.playback_srcs;
        while( node != NULL ) {
            JSList *this_node = node;
            SRC_STATE *state = (SRC_STATE *) node->data;
            node = jack_slist_remove_link( node, this_node );
            jack_slist_free_1( this_node );
            src_delete( state );
        }
        netj.playback_srcs = NULL;

        node = netj.capture_srcs;
        while( node != NULL ) {
            JSList *this_node = node;
            SRC_STATE *state = (SRC_STATE *) node->data;
            node = jack_slist_remove_link( node, this_node );
            jack_slist_free_1( this_node );
            src_delete( state );
        }
        netj.capture_srcs = NULL;
    #endif
        }
    }

//Render functions--------------------------------------------------------------------

// render functions for float
    void
    JackNetOneDriver::render_payload_to_jack_ports_float ( void *packet_payload, jack_nframes_t net_period_down, JSList *capture_ports, JSList *capture_srcs, jack_nframes_t nframes, int dont_htonl_floats)
    {
        uint32_t chn = 0;
        JSList *node = capture_ports;
    #if HAVE_SAMPLERATE
        JSList *src_node = capture_srcs;
    #endif

        uint32_t *packet_bufX = (uint32_t *)packet_payload;

        if( !packet_payload )
            return;

        while (node != NULL)
        {
            unsigned int i;
            int_float_t val;
    #if HAVE_SAMPLERATE
            SRC_DATA src;
    #endif
            jack_port_id_t port_id = (jack_port_id_t)(intptr_t) node->data;
            JackPort *port = fGraphManager->GetPort( port_id );

            jack_default_audio_sample_t* buf =
                (jack_default_audio_sample_t*)fGraphManager->GetBuffer(port_id, fEngineControl->fBufferSize);

            const char *porttype = port->GetType();

            if (strncmp (porttype, JACK_DEFAULT_AUDIO_TYPE, jack_port_type_size()) == 0)
            {
    #if HAVE_SAMPLERATE
                // audio port, resample if necessary
                if (net_period_down != nframes)
                {
                    SRC_STATE *src_state = (SRC_STATE *)src_node->data;
                    for (i = 0; i < net_period_down; i++)
                    {
                        packet_bufX[i] = ntohl (packet_bufX[i]);
                    }

                    src.data_in = (float *) packet_bufX;
                    src.input_frames = net_period_down;

                    src.data_out = buf;
                    src.output_frames = nframes;

                    src.src_ratio = (float) nframes / (float) net_period_down;
                    src.end_of_input = 0;

                    src_set_ratio (src_state, src.src_ratio);
                    src_process (src_state, &src);
                    src_node = jack_slist_next (src_node);
                }
                else
    #endif
                {
                    if( dont_htonl_floats )
                    {
                        memcpy( buf, packet_bufX, net_period_down*sizeof(jack_default_audio_sample_t));
                    }
                    else
                    {
                        for (i = 0; i < net_period_down; i++)
                        {
                            val.i = packet_bufX[i];
                            val.i = ntohl (val.i);
                            buf[i] = val.f;
                        }
                    }
                }
            }
            else if (strncmp (porttype, JACK_DEFAULT_MIDI_TYPE, jack_port_type_size()) == 0)
            {
                // midi port, decode midi events
                // convert the data buffer to a standard format (uint32_t based)
                unsigned int buffer_size_uint32 = net_period_down;
                uint32_t * buffer_uint32 = (uint32_t*)packet_bufX;
                decode_midi_buffer (buffer_uint32, buffer_size_uint32, buf);
            }
            packet_bufX = (packet_bufX + net_period_down);
            node = jack_slist_next (node);
            chn++;
        }
}

    void
    JackNetOneDriver::render_jack_ports_to_payload_float (JSList *playback_ports, JSList *playback_srcs, jack_nframes_t nframes, void *packet_payload, jack_nframes_t net_period_up, int dont_htonl_floats )
    {
        uint32_t chn = 0;
        JSList *node = playback_ports;
    #if HAVE_SAMPLERATE
        JSList *src_node = playback_srcs;
    #endif

        uint32_t *packet_bufX = (uint32_t *) packet_payload;

        while (node != NULL)
        {
    #if HAVE_SAMPLERATE
            SRC_DATA src;
    #endif
            unsigned int i;
            int_float_t val;
            jack_port_id_t port_id = (jack_port_id_t)(intptr_t) node->data;
            JackPort *port = fGraphManager->GetPort( port_id );

            jack_default_audio_sample_t* buf =
                (jack_default_audio_sample_t*)fGraphManager->GetBuffer(port_id, fEngineControl->fBufferSize);

            const char *porttype = port->GetType();

            if (strncmp (porttype, JACK_DEFAULT_AUDIO_TYPE, jack_port_type_size()) == 0)
            {
                // audio port, resample if necessary

    #if HAVE_SAMPLERATE
                if (net_period_up != nframes) {
                    SRC_STATE *src_state = (SRC_STATE *) src_node->data;
                    src.data_in = buf;
                    src.input_frames = nframes;

                    src.data_out = (float *) packet_bufX;
                    src.output_frames = net_period_up;

                    src.src_ratio = (float) net_period_up / (float) nframes;
                    src.end_of_input = 0;

                    src_set_ratio (src_state, src.src_ratio);
                    src_process (src_state, &src);

                    for (i = 0; i < net_period_up; i++)
                    {
                        packet_bufX[i] = htonl (packet_bufX[i]);
                    }
                    src_node = jack_slist_next (src_node);
                }
                else
    #endif
                {
                    if( dont_htonl_floats )
                    {
                        memcpy( packet_bufX, buf, net_period_up*sizeof(jack_default_audio_sample_t) );
                    }
                    else
                    {
                        for (i = 0; i < net_period_up; i++)
                        {
                            val.f = buf[i];
                            val.i = htonl (val.i);
                            packet_bufX[i] = val.i;
                        }
                    }
                }
            }
            else if (strncmp(porttype, JACK_DEFAULT_MIDI_TYPE, jack_port_type_size()) == 0)
            {
                // encode midi events from port to packet
                // convert the data buffer to a standard format (uint32_t based)
                unsigned int buffer_size_uint32 = net_period_up;
                uint32_t * buffer_uint32 = (uint32_t*) packet_bufX;
                encode_midi_buffer (buffer_uint32, buffer_size_uint32, buf);
            }
            packet_bufX = (packet_bufX + net_period_up);
            node = jack_slist_next (node);
            chn++;
        }
    }

    #if HAVE_CELT
    // render functions for celt.
    void
    JackNetOneDriver::render_payload_to_jack_ports_celt (void *packet_payload, jack_nframes_t net_period_down, JSList *capture_ports, JSList *capture_srcs, jack_nframes_t nframes)
    {
        uint32_t chn = 0;
        JSList *node = capture_ports;
        JSList *src_node = capture_srcs;
        unsigned char *packet_bufX = (unsigned char *)packet_payload;

        while (node != NULL)
        {
            jack_port_id_t port_id = (jack_port_id_t) (intptr_t)node->data;
            JackPort *port = fGraphManager->GetPort( port_id );

            jack_default_audio_sample_t* buf =
                (jack_default_audio_sample_t*)fGraphManager->GetBuffer(port_id, fEngineControl->fBufferSize);

            const char *portname = port->GetType();

            if (strncmp(portname, JACK_DEFAULT_AUDIO_TYPE, jack_port_type_size()) == 0)
            {
                // audio port, decode celt data.
                CELTDecoder *decoder = (CELTDecoder *)src_node->data;
                if( !packet_payload )
                    celt_decode_float( decoder, NULL, net_period_down, buf );
                else
                    celt_decode_float( decoder, packet_bufX, net_period_down, buf );

                src_node = jack_slist_next (src_node);
            }
            else if (strncmp(portname, JACK_DEFAULT_MIDI_TYPE, jack_port_type_size()) == 0)
            {
                // midi port, decode midi events
                // convert the data buffer to a standard format (uint32_t based)
                unsigned int buffer_size_uint32 = net_period_down / 2;
                uint32_t * buffer_uint32 = (uint32_t*) packet_bufX;
            if( packet_payload )
                decode_midi_buffer (buffer_uint32, buffer_size_uint32, buf);
            }
            packet_bufX = (packet_bufX + net_period_down);
            node = jack_slist_next (node);
            chn++;
        }
    }

    void
    JackNetOneDriver::render_jack_ports_to_payload_celt (JSList *playback_ports, JSList *playback_srcs, jack_nframes_t nframes, void *packet_payload, jack_nframes_t net_period_up)
    {
        uint32_t chn = 0;
        JSList *node = playback_ports;
        JSList *src_node = playback_srcs;

        unsigned char *packet_bufX = (unsigned char *)packet_payload;

        while (node != NULL)
        {
            jack_port_id_t port_id = (jack_port_id_t) (intptr_t) node->data;
            JackPort *port = fGraphManager->GetPort( port_id );

            jack_default_audio_sample_t* buf =
                (jack_default_audio_sample_t*)fGraphManager->GetBuffer(port_id, fEngineControl->fBufferSize);

            const char *portname = port->GetType();

            if (strncmp (portname, JACK_DEFAULT_AUDIO_TYPE, jack_port_type_size()) == 0)
            {
                // audio port, encode celt data.

            int encoded_bytes;
            jack_default_audio_sample_t *floatbuf = (jack_default_audio_sample_t *)alloca (sizeof(jack_default_audio_sample_t) * nframes );
            memcpy( floatbuf, buf, nframes * sizeof(jack_default_audio_sample_t) );
            CELTEncoder *encoder = (CELTEncoder *)src_node->data;
            encoded_bytes = celt_encode_float( encoder, floatbuf, NULL, packet_bufX, net_period_up );
            if( encoded_bytes != (int)net_period_up )
            jack_error( "something in celt changed. netjack needs to be changed to handle this." );
            src_node = jack_slist_next( src_node );
            }
            else if (strncmp(portname, JACK_DEFAULT_MIDI_TYPE, jack_port_type_size()) == 0)
            {
                // encode midi events from port to packet
                // convert the data buffer to a standard format (uint32_t based)
                unsigned int buffer_size_uint32 = net_period_up / 2;
                uint32_t * buffer_uint32 = (uint32_t*) packet_bufX;
                encode_midi_buffer (buffer_uint32, buffer_size_uint32, buf);
            }
            packet_bufX = (packet_bufX + net_period_up);
            node = jack_slist_next (node);
            chn++;
        }
    }

    #endif
    /* Wrapper functions with bitdepth argument... */
    void
    JackNetOneDriver::render_payload_to_jack_ports (int bitdepth, void *packet_payload, jack_nframes_t net_period_down, JSList *capture_ports, JSList *capture_srcs, jack_nframes_t nframes, int dont_htonl_floats)
    {
    #if HAVE_CELT
        if (bitdepth == CELT_MODE)
            render_payload_to_jack_ports_celt (packet_payload, net_period_down, capture_ports, capture_srcs, nframes);
        else
    #endif
            render_payload_to_jack_ports_float (packet_payload, net_period_down, capture_ports, capture_srcs, nframes, dont_htonl_floats);
    }

    void
    JackNetOneDriver::render_jack_ports_to_payload (int bitdepth, JSList *playback_ports, JSList *playback_srcs, jack_nframes_t nframes, void *packet_payload, jack_nframes_t net_period_up, int dont_htonl_floats)
    {
    #if HAVE_CELT
        if (bitdepth == CELT_MODE)
            render_jack_ports_to_payload_celt (playback_ports, playback_srcs, nframes, packet_payload, net_period_up);
        else
    #endif
            render_jack_ports_to_payload_float (playback_ports, playback_srcs, nframes, packet_payload, net_period_up, dont_htonl_floats);
    }

    //driver loader-----------------------------------------------------------------------

    #ifdef __cplusplus
        extern "C"
        {
    #endif
            SERVER_EXPORT jack_driver_desc_t* driver_get_descriptor ()
        {
            jack_driver_desc_t* desc = ( jack_driver_desc_t* ) calloc ( 1, sizeof ( jack_driver_desc_t ) );
            jack_driver_param_desc_t * params;

            strcpy ( desc->name, "netone" );                             // size MUST be less then JACK_DRIVER_NAME_MAX + 1
            strcpy ( desc->desc, "netjack one slave backend component" ); // size MUST be less then JACK_DRIVER_PARAM_DESC + 1

            desc->nparams = 18;
            params = ( jack_driver_param_desc_t* ) calloc ( desc->nparams, sizeof ( jack_driver_param_desc_t ) );

            int i = 0;
            strcpy (params[i].name, "audio-ins");
            params[i].character  = 'i';
            params[i].type       = JackDriverParamUInt;
            params[i].value.ui   = 2U;
            strcpy (params[i].short_desc, "Number of capture channels (defaults to 2)");
            strcpy (params[i].long_desc, params[i].short_desc);

            i++;
            strcpy (params[i].name, "audio-outs");
            params[i].character  = 'o';
            params[i].type       = JackDriverParamUInt;
            params[i].value.ui   = 2U;
            strcpy (params[i].short_desc, "Number of playback channels (defaults to 2)");
            strcpy (params[i].long_desc, params[i].short_desc);

            i++;
            strcpy (params[i].name, "midi-ins");
            params[i].character  = 'I';
            params[i].type       = JackDriverParamUInt;
            params[i].value.ui   = 1U;
            strcpy (params[i].short_desc, "Number of midi capture channels (defaults to 1)");
            strcpy (params[i].long_desc, params[i].short_desc);

            i++;
            strcpy (params[i].name, "midi-outs");
            params[i].character  = 'O';
            params[i].type       = JackDriverParamUInt;
            params[i].value.ui   = 1U;
            strcpy (params[i].short_desc, "Number of midi playback channels (defaults to 1)");
            strcpy (params[i].long_desc, params[i].short_desc);

            i++;
            strcpy (params[i].name, "rate");
            params[i].character  = 'r';
            params[i].type       = JackDriverParamUInt;
            params[i].value.ui   = 48000U;
            strcpy (params[i].short_desc, "Sample rate");
            strcpy (params[i].long_desc, params[i].short_desc);

            i++;
            strcpy (params[i].name, "period");
            params[i].character  = 'p';
            params[i].type       = JackDriverParamUInt;
            params[i].value.ui   = 1024U;
            strcpy (params[i].short_desc, "Frames per period");
            strcpy (params[i].long_desc, params[i].short_desc);

            i++;
            strcpy (params[i].name, "num-periods");
            params[i].character  = 'n';
            params[i].type       = JackDriverParamUInt;
            params[i].value.ui   = 5U;
            strcpy (params[i].short_desc,
                "Network latency setting in no. of periods");
            strcpy (params[i].long_desc, params[i].short_desc);

            i++;
            strcpy (params[i].name, "listen-port");
            params[i].character  = 'l';
            params[i].type       = JackDriverParamUInt;
            params[i].value.ui   = 3000U;
            strcpy (params[i].short_desc,
                "The socket port we are listening on for sync packets");
            strcpy (params[i].long_desc, params[i].short_desc);

            i++;
            strcpy (params[i].name, "factor");
            params[i].character  = 'f';
            params[i].type       = JackDriverParamUInt;
            params[i].value.ui   = 1U;
            strcpy (params[i].short_desc,
                "Factor for sample rate reduction");
            strcpy (params[i].long_desc, params[i].short_desc);

            i++;
            strcpy (params[i].name, "upstream-factor");
            params[i].character  = 'u';
            params[i].type       = JackDriverParamUInt;
            params[i].value.ui   = 0U;
            strcpy (params[i].short_desc,
                "Factor for sample rate reduction on the upstream");
            strcpy (params[i].long_desc, params[i].short_desc);

            i++;
            strcpy (params[i].name, "celt");
            params[i].character  = 'c';
            params[i].type       = JackDriverParamUInt;
            params[i].value.ui   = 0U;
            strcpy (params[i].short_desc,
                "sets celt encoding and number of kbits per channel");
            strcpy (params[i].long_desc, params[i].short_desc);

            i++;
            strcpy (params[i].name, "bit-depth");
            params[i].character  = 'b';
            params[i].type       = JackDriverParamUInt;
            params[i].value.ui   = 0U;
            strcpy (params[i].short_desc,
                "Sample bit-depth (0 for float, 8 for 8bit and 16 for 16bit)");
            strcpy (params[i].long_desc, params[i].short_desc);

            i++;
            strcpy (params[i].name, "transport-sync");
            params[i].character  = 't';
            params[i].type       = JackDriverParamBool;
            params[i].value.ui   = 1U;
            strcpy (params[i].short_desc,
                "Whether to slave the transport to the master transport");
            strcpy (params[i].long_desc, params[i].short_desc);

            i++;
            strcpy (params[i].name, "autoconf");
            params[i].character  = 'a';
            params[i].type       = JackDriverParamBool;
            params[i].value.ui   = 1U;
            strcpy (params[i].short_desc,
                "Whether to use Autoconfig, or just start.");
            strcpy (params[i].long_desc, params[i].short_desc);

            i++;
            strcpy (params[i].name, "redundancy");
            params[i].character  = 'R';
            params[i].type       = JackDriverParamUInt;
            params[i].value.ui   = 1U;
            strcpy (params[i].short_desc,
                "Send packets N times");
            strcpy (params[i].long_desc, params[i].short_desc);

            i++;
            strcpy (params[i].name, "native-endian");
            params[i].character  = 'e';
            params[i].type       = JackDriverParamBool;
            params[i].value.ui   = 0U;
            strcpy (params[i].short_desc,
                "Dont convert samples to network byte order.");
            strcpy (params[i].long_desc, params[i].short_desc);

            i++;
            strcpy (params[i].name, "jitterval");
            params[i].character  = 'J';
            params[i].type       = JackDriverParamInt;
            params[i].value.i   = 0;
            strcpy (params[i].short_desc,
                "attempted jitterbuffer microseconds on master");
            strcpy (params[i].long_desc, params[i].short_desc);

            i++;
            strcpy (params[i].name, "always-deadline");
            params[i].character  = 'D';
            params[i].type       = JackDriverParamBool;
            params[i].value.ui   = 0U;
            strcpy (params[i].short_desc,
                "always use deadline");
            strcpy (params[i].long_desc, params[i].short_desc);

            desc->params = params;

            return desc;
        }

            SERVER_EXPORT Jack::JackDriverClientInterface* driver_initialize ( Jack::JackLockedEngine* engine, Jack::JackSynchro* table, const JSList* params )
            {
                jack_nframes_t sample_rate = 48000;
                jack_nframes_t resample_factor = 1;
                jack_nframes_t period_size = 1024;
                unsigned int capture_ports = 2;
                unsigned int playback_ports = 2;
                unsigned int capture_ports_midi = 1;
                unsigned int playback_ports_midi = 1;
                unsigned int listen_port = 3000;
                unsigned int bitdepth = 0;
                unsigned int handle_transport_sync = 1;
                unsigned int use_autoconfig = 1;
                unsigned int latency = 5;
                unsigned int redundancy = 1;
                unsigned int mtu = 1400;
            #if HAVE_SAMPLERATE
                unsigned int resample_factor_up = 1;
            #endif
                int dont_htonl_floats = 0;
                int always_deadline = 0;
                int jitter_val = 0;
                const JSList * node;
                const jack_driver_param_t * param;

                for ( node = params; node; node = jack_slist_next ( node ) )
                {
                    param = ( const jack_driver_param_t* ) node->data;
                    switch ( param->character )
                    {
                case 'i':
                    capture_ports = param->value.ui;
                    break;

                case 'o':
                    playback_ports = param->value.ui;
                    break;

                case 'I':
                    capture_ports_midi = param->value.ui;
                    break;

                case 'O':
                    playback_ports_midi = param->value.ui;
                    break;

                case 'r':
                    sample_rate = param->value.ui;
                    break;

                case 'p':
                    period_size = param->value.ui;
                    break;

                case 'l':
                    listen_port = param->value.ui;
                    break;

                case 'f':
            #if HAVE_SAMPLERATE
                    resample_factor = param->value.ui;
            #else
                    jack_error( "not built with libsamplerate support" );
                    return NULL;
            #endif
                    break;

                case 'u':
            #if HAVE_SAMPLERATE
                    resample_factor_up = param->value.ui;
            #else
                    jack_error( "not built with libsamplerate support" );
                    return NULL;
            #endif
                    break;

                case 'b':
                    bitdepth = param->value.ui;
                    break;

                case 'c':
            #if HAVE_CELT
                    bitdepth = CELT_MODE;
                    resample_factor = param->value.ui;
            #else
                    jack_error( "not built with celt support" );
                    return NULL;
            #endif
                    break;

                case 't':
                    handle_transport_sync = param->value.ui;
                    break;

                case 'a':
                    use_autoconfig = param->value.ui;
                    break;

                case 'n':
                    latency = param->value.ui;
                    break;

                case 'R':
                    redundancy = param->value.ui;
                    break;

                case 'H':
                    dont_htonl_floats = param->value.ui;
                    break;

                case 'J':
                    jitter_val = param->value.i;
                    break;

                case 'D':
                    always_deadline = param->value.ui;
                    break;
                    }
                }

                try
                {
                    Jack::JackDriverClientInterface* driver =
                        new Jack::JackWaitThreadedDriver (
                        new Jack::JackNetOneDriver ( "system", "net_pcm", engine, table, listen_port, mtu,
                                                  capture_ports_midi, playback_ports_midi, capture_ports, playback_ports,
                              sample_rate, period_size, resample_factor,
                              "net_pcm", handle_transport_sync, bitdepth, use_autoconfig, latency, redundancy,
                              dont_htonl_floats, always_deadline, jitter_val ) );

                    if ( driver->Open ( period_size, sample_rate, 1, 1, capture_ports, playback_ports,
                                        0, "from_master_", "to_master_", 0, 0 ) == 0 )
                    {
                        return driver;
                    }
                    else
                    {
                        delete driver;
                        return NULL;
                    }

                }
                catch ( ... )
                {
                    return NULL;
                }
            }

    #ifdef __cplusplus
        }
    #endif
}
