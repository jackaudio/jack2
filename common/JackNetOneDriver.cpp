/*
Copyright (C) 2008-2011 Torben Horn

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
#include "JackLockedEngine.h"
#include "JackGraphManager.h"
#include "JackWaitThreadedDriver.h"
#include "JackTools.h"
#include "driver_interface.h"

#include "netjack.h"
#include "netjack_packet.h"

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

#define MIN(x,y) ((x)<(y) ? (x) : (y))

using namespace std;

namespace Jack
{
JackNetOneDriver::JackNetOneDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table,
                                     int port, int mtu, int capture_ports, int playback_ports, int midi_input_ports, int midi_output_ports,
                                     int sample_rate, int period_size, int resample_factor,
                                     const char* net_name, uint transport_sync, int bitdepth, int use_autoconfig,
                                     int latency, int redundancy, int dont_htonl_floats, int always_deadline, int jitter_val)
    : JackWaiterDriver(name, alias, engine, table)
{
    jack_log("JackNetOneDriver::JackNetOneDriver port %d", port);

#ifdef WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 0), &wsa);
#endif

    netjack_init(& (this->netj),
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

int JackNetOneDriver::Close()
{
    // Generic audio driver close
    int res = JackWaiterDriver::Close();

    FreePorts();
    netjack_release(&netj);
    return res;
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
    jack_port_id_t port_index;
    char buf[64];
    unsigned int chn;

    //if (netj.handle_transport_sync)
    //    jack_set_sync_callback(netj.client, (JackSyncCallback) net_driver_sync_cb, NULL);

    for (chn = 0; chn < netj.capture_channels_audio; chn++) {
        snprintf (buf, sizeof(buf) - 1, "system:capture_%u", chn + 1);

        if (fEngine->PortRegister(fClientControl.fRefNum, buf, JACK_DEFAULT_AUDIO_TYPE,
            CaptureDriverFlags, fEngineControl->fBufferSize, &port_index) < 0) {
            jack_error("driver: cannot register port for %s", buf);
            return -1;
        }
        //port = fGraphManager->GetPort(port_index);

        netj.capture_ports = jack_slist_append (netj.capture_ports, (void *)(intptr_t)port_index);

        if (netj.bitdepth == CELT_MODE) {
#if HAVE_CELT
#if HAVE_CELT_API_0_11
            celt_int32 lookahead;
            CELTMode *celt_mode = celt_mode_create(netj.sample_rate, netj.period_size, NULL);
            netj.capture_srcs = jack_slist_append(netj.capture_srcs, celt_decoder_create_custom(celt_mode, 1, NULL));
#elif HAVE_CELT_API_0_7 || HAVE_CELT_API_0_8
            celt_int32 lookahead;
            CELTMode *celt_mode = celt_mode_create(netj.sample_rate, netj.period_size, NULL);
            netj.capture_srcs = jack_slist_append(netj.capture_srcs, celt_decoder_create(celt_mode, 1, NULL));
#else
            celt_int32_t lookahead;
            CELTMode *celt_mode = celt_mode_create(netj.sample_rate, 1, netj.period_size, NULL);
            netj.capture_srcs = jack_slist_append(netj.capture_srcs, celt_decoder_create(celt_mode));
#endif
            celt_mode_info(celt_mode, CELT_GET_LOOKAHEAD, &lookahead);
            netj.codec_latency = 2 * lookahead;
#endif
        } else if (netj.bitdepth == OPUS_MODE) {
#if HAVE_OPUS
            OpusCustomMode *opus_mode = opus_custom_mode_create(netj.sample_rate, netj.period_size, NULL); // XXX free me in the end
            OpusCustomDecoder *decoder = opus_custom_decoder_create( opus_mode, 1, NULL );
            netj.capture_srcs = jack_slist_append(netj.capture_srcs, decoder);
#endif
        } else {
#if HAVE_SAMPLERATE
            netj.capture_srcs = jack_slist_append(netj.capture_srcs, (void *)src_new(SRC_LINEAR, 1, NULL));
#endif
        }
    }

    for (chn = netj.capture_channels_audio; chn < netj.capture_channels; chn++) {
        snprintf (buf, sizeof(buf) - 1, "system:capture_%u", chn + 1);

        if (fEngine->PortRegister(fClientControl.fRefNum, buf, JACK_DEFAULT_MIDI_TYPE,
            CaptureDriverFlags, fEngineControl->fBufferSize, &port_index) < 0) {
            jack_error("driver: cannot register port for %s", buf);
            return -1;
        }
        //port = fGraphManager->GetPort(port_index);

        netj.capture_ports =
            jack_slist_append (netj.capture_ports, (void *)(intptr_t)port_index);
    }

    for (chn = 0; chn < netj.playback_channels_audio; chn++) {
        snprintf (buf, sizeof(buf) - 1, "system:playback_%u", chn + 1);

        if (fEngine->PortRegister(fClientControl.fRefNum, buf, JACK_DEFAULT_AUDIO_TYPE,
            PlaybackDriverFlags, fEngineControl->fBufferSize, &port_index) < 0) {
            jack_error("driver: cannot register port for %s", buf);
            return -1;
        }
        //port = fGraphManager->GetPort(port_index);

        netj.playback_ports = jack_slist_append (netj.playback_ports, (void *)(intptr_t)port_index);
        if (netj.bitdepth == CELT_MODE) {
#if HAVE_CELT
#if HAVE_CELT_API_0_11
            CELTMode *celt_mode = celt_mode_create(netj.sample_rate, netj.period_size, NULL);
            netj.playback_srcs = jack_slist_append(netj.playback_srcs, celt_encoder_create_custom(celt_mode, 1, NULL));
#elif HAVE_CELT_API_0_7 || HAVE_CELT_API_0_8
            CELTMode *celt_mode = celt_mode_create(netj.sample_rate, netj.period_size, NULL);
            netj.playback_srcs = jack_slist_append(netj.playback_srcs, celt_encoder_create(celt_mode, 1, NULL));
#else
            CELTMode *celt_mode = celt_mode_create(netj.sample_rate, 1, netj.period_size, NULL);
            netj.playback_srcs = jack_slist_append(netj.playback_srcs, celt_encoder_create(celt_mode));
#endif
#endif
        } else if (netj.bitdepth == OPUS_MODE) {
#if HAVE_OPUS
            const int kbps = netj.resample_factor;
            jack_error("NEW ONE OPUS ENCODER 128  <> %d!!", kbps);
            int err;
            OpusCustomMode *opus_mode = opus_custom_mode_create( netj.sample_rate, netj.period_size, &err ); // XXX free me in the end
            if (err != OPUS_OK) { jack_error("opus mode failed"); }
            OpusCustomEncoder *oe = opus_custom_encoder_create( opus_mode, 1, &err );
            if (err != OPUS_OK) { jack_error("opus mode failed"); }
            opus_custom_encoder_ctl(oe, OPUS_SET_BITRATE(kbps*1024)); // bits per second
            opus_custom_encoder_ctl(oe, OPUS_SET_COMPLEXITY(10));
            opus_custom_encoder_ctl(oe, OPUS_SET_SIGNAL(OPUS_SIGNAL_MUSIC));
            opus_custom_encoder_ctl(oe, OPUS_SET_SIGNAL(OPUS_APPLICATION_RESTRICTED_LOWDELAY));
            netj.playback_srcs = jack_slist_append(netj.playback_srcs, oe);
#endif
        } else {
#if HAVE_SAMPLERATE
            netj.playback_srcs = jack_slist_append(netj.playback_srcs, (void *)src_new(SRC_LINEAR, 1, NULL));
#endif
        }
    }
    for (chn = netj.playback_channels_audio; chn < netj.playback_channels; chn++) {
        snprintf (buf, sizeof(buf) - 1, "system:playback_%u", chn + 1);

        if (fEngine->PortRegister(fClientControl.fRefNum, buf, JACK_DEFAULT_MIDI_TYPE,
            PlaybackDriverFlags, fEngineControl->fBufferSize, &port_index) < 0) {
            jack_error("driver: cannot register port for %s", buf);
            return -1;
        }
        //port = fGraphManager->GetPort(port_index);

        netj.playback_ports =
            jack_slist_append (netj.playback_ports, (void *)(intptr_t)port_index);
    }
    return 0;
}

//init and restart--------------------------------------------------------------------
bool JackNetOneDriver::Initialize()
{
    jack_log("JackNetOneDriver::Init");

    FreePorts();
    netjack_release(&netj);

    //display some additional infos
    jack_info("NetOne driver started");
    if (netjack_startup(&netj)) {
        return false;
    }

    //register jack ports
    if (AllocPorts() != 0) {
        jack_error("Can't allocate ports.");
        return false;
    }

    //monitor
    //driver parametering
    JackTimedDriver::SetBufferSize(netj.period_size);
    JackTimedDriver::SetSampleRate(netj.sample_rate);

    JackDriver::NotifyBufferSize(netj.period_size);
    JackDriver::NotifySampleRate(netj.sample_rate);

    //transport engine parametering
    fEngineControl->fTransport.SetNetworkSync(true);
    return true;
}


//jack ports and buffers--------------------------------------------------------------

//driver processes--------------------------------------------------------------------

int JackNetOneDriver::Read()
{
    int delay;
    delay = netjack_wait(&netj);
    if (delay) {
        NotifyXRun(fBeginDateUst, (float) delay);
        jack_error("netxruns... duration: %dms", delay / 1000);
    }

    if ((netj.num_lost_packets * netj.period_size / netj.sample_rate) > 2)
        JackTools::ThrowJackNetException();

    //netjack_read(&netj, netj.period_size);
    JackDriver::CycleTakeBeginTime();

    jack_position_t local_trans_pos;
    jack_transport_state_t local_trans_state;

    unsigned int *packet_buf, *packet_bufX;

    if (! netj.packet_data_valid) {
        jack_log("data not valid");
        render_payload_to_jack_ports (netj.bitdepth, NULL, netj.net_period_down, netj.capture_ports, netj.capture_srcs, netj.period_size, netj.dont_htonl_floats);
        return 0;
    }
    packet_buf = netj.rx_buf;

    jacknet_packet_header *pkthdr = (jacknet_packet_header *)packet_buf;

    packet_bufX = packet_buf + sizeof(jacknet_packet_header) / sizeof(jack_default_audio_sample_t);

    netj.reply_port = pkthdr->reply_port;
    netj.latency = pkthdr->latency;

    // Special handling for latency=0
    if (netj.latency == 0)
        netj.resync_threshold = 0;
    else
        netj.resync_threshold = MIN(15, pkthdr->latency - 1);

    // check whether, we should handle the transport sync stuff, or leave trnasports untouched.
    if (netj.handle_transport_sync) {
#if 1
        unsigned int compensated_tranport_pos = (pkthdr->transport_frame + (pkthdr->latency * netj.period_size) + netj.codec_latency);

        // read local transport info....
        //local_trans_state = jack_transport_query(netj.client, &local_trans_pos);

        local_trans_state = fEngineControl->fTransport.Query(&local_trans_pos);

        // Now check if we have to start or stop local transport to sync to remote...
        switch (pkthdr->transport_state) {

            case JackTransportStarting:
                // the master transport is starting... so we set our reply to the sync_callback;
                if (local_trans_state == JackTransportStopped) {
                    fEngineControl->fTransport.SetCommand(TransportCommandStart);
                    //jack_transport_start(netj.client);
                    //last_transport_state = JackTransportStopped;
                    netj.sync_state = 0;
                    jack_info("locally stopped... starting...");
                }

                if (local_trans_pos.frame != compensated_tranport_pos) {
                    jack_position_t new_pos = local_trans_pos;
                    new_pos.frame = compensated_tranport_pos + 2 * netj.period_size;
                    new_pos.valid = (jack_position_bits_t) 0;


                    fEngineControl->fTransport.RequestNewPos(&new_pos);
                    //jack_transport_locate(netj.client, compensated_tranport_pos);
                    //last_transport_state = JackTransportRolling;
                    netj.sync_state = 0;
                    jack_info("starting locate to %d", compensated_tranport_pos);
                }
                break;

            case JackTransportStopped:
                netj.sync_state = 1;
                if (local_trans_pos.frame != (pkthdr->transport_frame)) {
                    jack_position_t new_pos = local_trans_pos;
                    new_pos.frame = pkthdr->transport_frame;
                    new_pos.valid = (jack_position_bits_t)0;
                    fEngineControl->fTransport.RequestNewPos(&new_pos);
                    //jack_transport_locate(netj.client, (pkthdr->transport_frame));
                    jack_info("transport is stopped locate to %d", pkthdr->transport_frame);
                }
                if (local_trans_state != JackTransportStopped)
                    //jack_transport_stop(netj.client);
                    fEngineControl->fTransport.SetCommand(TransportCommandStop);
                break;

            case JackTransportRolling:
                netj.sync_state = 1;
                //		    if(local_trans_pos.frame != (pkthdr->transport_frame + (pkthdr->latency) * netj.period_size)) {
                //		        jack_transport_locate(netj.client, (pkthdr->transport_frame + (pkthdr->latency + 2) * netj.period_size));
                //			jack_info("running locate to %d", pkthdr->transport_frame + (pkthdr->latency)*netj.period_size);
                //		    		}
                if (local_trans_state != JackTransportRolling)
                    fEngineControl->fTransport.SetState(JackTransportRolling);
                break;

            case JackTransportLooping:
                break;
        }
#endif
    }

    render_payload_to_jack_ports (netj.bitdepth, packet_bufX, netj.net_period_down, netj.capture_ports, netj.capture_srcs, netj.period_size, netj.dont_htonl_floats);
    packet_cache_release_packet(netj.packcache, netj.expected_framecnt);
    return 0;
}

int JackNetOneDriver::Write()
{
    int syncstate = netj.sync_state | ((fEngineControl->fTransport.GetState() == JackTransportNetStarting) ? 1 : 0);
    uint32_t *packet_buf, *packet_bufX;

    int packet_size = get_sample_size(netj.bitdepth) * netj.playback_channels * netj.net_period_up + sizeof(jacknet_packet_header);
    jacknet_packet_header *pkthdr;

    packet_buf = (uint32_t *) alloca(packet_size);
    pkthdr = (jacknet_packet_header *)packet_buf;

    if (netj.running_free) {
        return 0;
    }

    // offset packet_bufX by the packetheader.
    packet_bufX = packet_buf + sizeof(jacknet_packet_header) / sizeof(jack_default_audio_sample_t);

    pkthdr->sync_state = syncstate;;
    pkthdr->latency = netj.time_to_deadline;
    //printf("time to deadline = %d  goodness=%d\n", (int)netj.time_to_deadline, netj.deadline_goodness);
    pkthdr->framecnt = netj.expected_framecnt;

    render_jack_ports_to_payload(netj.bitdepth, netj.playback_ports, netj.playback_srcs, netj.period_size, packet_bufX, netj.net_period_up, netj.dont_htonl_floats);

    packet_header_hton(pkthdr);
    if (netj.srcaddress_valid) {
        unsigned int r;
        static const int flag = 0;

        if (netj.reply_port)
            netj.syncsource_address.sin_port = htons(netj.reply_port);

        for (r = 0; r < netj.redundancy; r++)
            netjack_sendto(netj.sockfd, (char *)packet_buf, packet_size,
                           flag, (struct sockaddr*) & (netj.syncsource_address), sizeof(struct sockaddr_in), netj.mtu);
    }
    return 0;
}

void
JackNetOneDriver::FreePorts ()
{
    JSList *node = netj.capture_ports;

    while (node != NULL) {
        JSList *this_node = node;
        jack_port_id_t port_index = (jack_port_id_t)(intptr_t) node->data;
        node = jack_slist_remove_link(node, this_node);
        jack_slist_free_1(this_node);
        fEngine->PortUnRegister(fClientControl.fRefNum, port_index);
    }
    netj.capture_ports = NULL;

    node = netj.playback_ports;
    while (node != NULL) {
        JSList *this_node = node;
        jack_port_id_t port_index = (jack_port_id_t)(intptr_t) node->data;
        node = jack_slist_remove_link(node, this_node);
        jack_slist_free_1(this_node);
        fEngine->PortUnRegister(fClientControl.fRefNum, port_index);
    }
    netj.playback_ports = NULL;

    if (netj.bitdepth == CELT_MODE) {
#if HAVE_CELT
        node = netj.playback_srcs;
        while (node != NULL) {
            JSList *this_node = node;
            CELTEncoder *enc = (CELTEncoder *) node->data;
            node = jack_slist_remove_link(node, this_node);
            jack_slist_free_1(this_node);
            celt_encoder_destroy(enc);
        }
        netj.playback_srcs = NULL;

        node = netj.capture_srcs;
        while (node != NULL) {
            JSList *this_node = node;
            CELTDecoder *dec = (CELTDecoder *) node->data;
            node = jack_slist_remove_link(node, this_node);
            jack_slist_free_1(this_node);
            celt_decoder_destroy(dec);
        }
        netj.capture_srcs = NULL;
#endif
    } else if (netj.bitdepth == OPUS_MODE) {
#if HAVE_OPUS
        node = netj.playback_srcs;
        while (node != NULL) {
            JSList *this_node = node;
            OpusCustomEncoder *enc = (OpusCustomEncoder *) node->data;
            node = jack_slist_remove_link(node, this_node);
            jack_slist_free_1(this_node);
            opus_custom_encoder_destroy(enc);
        }
        netj.playback_srcs = NULL;

        node = netj.capture_srcs;
        while (node != NULL) {
            JSList *this_node = node;
            OpusCustomDecoder *dec = (OpusCustomDecoder *) node->data;
            node = jack_slist_remove_link(node, this_node);
            jack_slist_free_1(this_node);
            opus_custom_decoder_destroy(dec);
        }
        netj.capture_srcs = NULL;
#endif
    } else {
#if HAVE_SAMPLERATE
        node = netj.playback_srcs;
        while (node != NULL) {
            JSList *this_node = node;
            SRC_STATE *state = (SRC_STATE *) node->data;
            node = jack_slist_remove_link(node, this_node);
            jack_slist_free_1(this_node);
            src_delete(state);
        }
        netj.playback_srcs = NULL;

        node = netj.capture_srcs;
        while (node != NULL) {
            JSList *this_node = node;
            SRC_STATE *state = (SRC_STATE *) node->data;
            node = jack_slist_remove_link(node, this_node);
            jack_slist_free_1(this_node);
            src_delete(state);
        }
        netj.capture_srcs = NULL;
#endif
    }
}

//Render functions--------------------------------------------------------------------

// render functions for float
void
JackNetOneDriver::render_payload_to_jack_ports_float(void *packet_payload, jack_nframes_t net_period_down, JSList *capture_ports, JSList *capture_srcs, jack_nframes_t nframes, int dont_htonl_floats)
{
    uint32_t chn = 0;
    JSList *node = capture_ports;
#if HAVE_SAMPLERATE
    JSList *src_node = capture_srcs;
#endif

    uint32_t *packet_bufX = (uint32_t *)packet_payload;

    if (!packet_payload)
        return;

    while (node != NULL) {
        unsigned int i;
        int_float_t val;
#if HAVE_SAMPLERATE
        SRC_DATA src;
#endif
        jack_port_id_t port_index = (jack_port_id_t)(intptr_t) node->data;
        JackPort *port = fGraphManager->GetPort(port_index);

        jack_default_audio_sample_t* buf =
            (jack_default_audio_sample_t*)fGraphManager->GetBuffer(port_index, fEngineControl->fBufferSize);

        const char *porttype = port->GetType();

        if (strncmp (porttype, JACK_DEFAULT_AUDIO_TYPE, jack_port_type_size()) == 0) {
#if HAVE_SAMPLERATE
            // audio port, resample if necessary
            if (net_period_down != nframes) {
                SRC_STATE *src_state = (SRC_STATE *)src_node->data;
                for (i = 0; i < net_period_down; i++) {
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
            } else
#endif
            {
                if (dont_htonl_floats) {
                    memcpy(buf, packet_bufX, net_period_down * sizeof(jack_default_audio_sample_t));
                } else {
                    for (i = 0; i < net_period_down; i++) {
                        val.i = packet_bufX[i];
                        val.i = ntohl (val.i);
                        buf[i] = val.f;
                    }
                }
            }
        } else if (strncmp (porttype, JACK_DEFAULT_MIDI_TYPE, jack_port_type_size()) == 0) {
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
JackNetOneDriver::render_jack_ports_to_payload_float (JSList *playback_ports, JSList *playback_srcs, jack_nframes_t nframes, void *packet_payload, jack_nframes_t net_period_up, int dont_htonl_floats)
{
    uint32_t chn = 0;
    JSList *node = playback_ports;
#if HAVE_SAMPLERATE
    JSList *src_node = playback_srcs;
#endif

    uint32_t *packet_bufX = (uint32_t *) packet_payload;

    while (node != NULL) {
#if HAVE_SAMPLERATE
        SRC_DATA src;
#endif
        unsigned int i;
        int_float_t val;
        jack_port_id_t port_index = (jack_port_id_t)(intptr_t) node->data;
        JackPort *port = fGraphManager->GetPort(port_index);

        jack_default_audio_sample_t* buf =
            (jack_default_audio_sample_t*)fGraphManager->GetBuffer(port_index, fEngineControl->fBufferSize);

        const char *porttype = port->GetType();

        if (strncmp (porttype, JACK_DEFAULT_AUDIO_TYPE, jack_port_type_size()) == 0) {
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

                for (i = 0; i < net_period_up; i++) {
                    packet_bufX[i] = htonl (packet_bufX[i]);
                }
                src_node = jack_slist_next (src_node);
            } else
#endif
            {
                if (dont_htonl_floats) {
                    memcpy(packet_bufX, buf, net_period_up * sizeof(jack_default_audio_sample_t));
                } else {
                    for (i = 0; i < net_period_up; i++) {
                        val.f = buf[i];
                        val.i = htonl (val.i);
                        packet_bufX[i] = val.i;
                    }
                }
            }
        } else if (strncmp(porttype, JACK_DEFAULT_MIDI_TYPE, jack_port_type_size()) == 0) {
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

    while (node != NULL) {
        jack_port_id_t port_index = (jack_port_id_t) (intptr_t)node->data;
        JackPort *port = fGraphManager->GetPort(port_index);

        jack_default_audio_sample_t* buf =
            (jack_default_audio_sample_t*)fGraphManager->GetBuffer(port_index, fEngineControl->fBufferSize);

        const char *portname = port->GetType();

        if (strncmp(portname, JACK_DEFAULT_AUDIO_TYPE, jack_port_type_size()) == 0) {
            // audio port, decode celt data.
            CELTDecoder *decoder = (CELTDecoder *)src_node->data;

#if HAVE_CELT_API_0_8 || HAVE_CELT_API_0_11
            if (!packet_payload)
                celt_decode_float(decoder, NULL, net_period_down, buf, nframes);
            else
                celt_decode_float(decoder, packet_bufX, net_period_down, buf, nframes);
#else
            if (!packet_payload)
                celt_decode_float(decoder, NULL, net_period_down, buf);
            else
                celt_decode_float(decoder, packet_bufX, net_period_down, buf);
#endif

            src_node = jack_slist_next (src_node);
        } else if (strncmp(portname, JACK_DEFAULT_MIDI_TYPE, jack_port_type_size()) == 0) {
            // midi port, decode midi events
            // convert the data buffer to a standard format (uint32_t based)
            unsigned int buffer_size_uint32 = net_period_down / 2;
            uint32_t * buffer_uint32 = (uint32_t*) packet_bufX;
            if (packet_payload)
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

    while (node != NULL) {
        jack_port_id_t port_index = (jack_port_id_t) (intptr_t) node->data;
        JackPort *port = fGraphManager->GetPort(port_index);

        jack_default_audio_sample_t* buf =
            (jack_default_audio_sample_t*)fGraphManager->GetBuffer(port_index, fEngineControl->fBufferSize);

        const char *portname = port->GetType();

        if (strncmp (portname, JACK_DEFAULT_AUDIO_TYPE, jack_port_type_size()) == 0) {
            // audio port, encode celt data.

            int encoded_bytes;
            jack_default_audio_sample_t *floatbuf = (jack_default_audio_sample_t *)alloca (sizeof(jack_default_audio_sample_t) * nframes);
            memcpy(floatbuf, buf, nframes * sizeof(jack_default_audio_sample_t));
            CELTEncoder *encoder = (CELTEncoder *)src_node->data;
#if HAVE_CELT_API_0_8 || HAVE_CELT_API_0_11
            encoded_bytes = celt_encode_float(encoder, floatbuf, nframes, packet_bufX, net_period_up);
#else
            encoded_bytes = celt_encode_float(encoder, floatbuf, NULL, packet_bufX, net_period_up);
#endif
            if (encoded_bytes != (int)net_period_up)
                jack_error("something in celt changed. netjack needs to be changed to handle this.");
            src_node = jack_slist_next(src_node);
        } else if (strncmp(portname, JACK_DEFAULT_MIDI_TYPE, jack_port_type_size()) == 0) {
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

#if HAVE_OPUS
#define CDO (sizeof(short)) ///< compressed data offset (first 2 bytes are length)
// render functions for Opus.
void
JackNetOneDriver::render_payload_to_jack_ports_opus (void *packet_payload, jack_nframes_t net_period_down, JSList *capture_ports, JSList *capture_srcs, jack_nframes_t nframes)
{
    int chn = 0;
    JSList *node = capture_ports;
    JSList *src_node = capture_srcs;

    unsigned char *packet_bufX = (unsigned char *)packet_payload;

    while (node != NULL) {
        jack_port_id_t port_index = (jack_port_id_t) (intptr_t)node->data;
        JackPort *port = fGraphManager->GetPort(port_index);

        jack_default_audio_sample_t* buf =
            (jack_default_audio_sample_t*)fGraphManager->GetBuffer(port_index, fEngineControl->fBufferSize);

        const char *portname = port->GetType();

        if (strncmp(portname, JACK_DEFAULT_AUDIO_TYPE, jack_port_type_size()) == 0) {
            // audio port, decode opus data.
            OpusCustomDecoder *decoder = (OpusCustomDecoder*) src_node->data;
            if( !packet_payload )
                memset(buf, 0, nframes * sizeof(float));
            else {
                unsigned short len;
                memcpy(&len, packet_bufX, CDO);
                len = ntohs(len);
                opus_custom_decode_float( decoder, packet_bufX + CDO, len, buf, nframes );
            }

            src_node = jack_slist_next (src_node);
        } else if (strncmp(portname, JACK_DEFAULT_MIDI_TYPE, jack_port_type_size()) == 0) {
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
JackNetOneDriver::render_jack_ports_to_payload_opus (JSList *playback_ports, JSList *playback_srcs, jack_nframes_t nframes, void *packet_payload, jack_nframes_t net_period_up)
{
    int chn = 0;
    JSList *node = playback_ports;
    JSList *src_node = playback_srcs;

    unsigned char *packet_bufX = (unsigned char *)packet_payload;

    while (node != NULL) {
        jack_port_id_t port_index = (jack_port_id_t) (intptr_t) node->data;
        JackPort *port = fGraphManager->GetPort(port_index);

        jack_default_audio_sample_t* buf =
            (jack_default_audio_sample_t*)fGraphManager->GetBuffer(port_index, fEngineControl->fBufferSize);

        const char *portname = port->GetType();

        if (strncmp (portname, JACK_DEFAULT_AUDIO_TYPE, jack_port_type_size()) == 0) {
            // audio port, encode opus data.

            int encoded_bytes;
            jack_default_audio_sample_t *floatbuf = (jack_default_audio_sample_t *)alloca (sizeof(jack_default_audio_sample_t) * nframes);
            memcpy(floatbuf, buf, nframes * sizeof(jack_default_audio_sample_t));
            OpusCustomEncoder *encoder = (OpusCustomEncoder*) src_node->data;
            encoded_bytes = opus_custom_encode_float( encoder, floatbuf, nframes, packet_bufX + CDO, net_period_up - CDO );
            unsigned short len = htons(encoded_bytes);
            memcpy(packet_bufX, &len, CDO);
            src_node = jack_slist_next( src_node );
        } else if (strncmp(portname, JACK_DEFAULT_MIDI_TYPE, jack_port_type_size()) == 0) {
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
#if HAVE_OPUS
    if (bitdepth == OPUS_MODE)
        render_payload_to_jack_ports_opus (packet_payload, net_period_down, capture_ports, capture_srcs, nframes);
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
#if HAVE_OPUS
    if (bitdepth == OPUS_MODE)
        render_jack_ports_to_payload_opus (playback_ports, playback_srcs, nframes, packet_payload, net_period_up);
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
        jack_driver_desc_t * desc;
        jack_driver_desc_filler_t filler;
        jack_driver_param_value_t value;

        desc = jack_driver_descriptor_construct("netone", JackDriverMaster, "netjack one slave backend component", &filler);

        value.ui = 2U;
        jack_driver_descriptor_add_parameter(desc, &filler, "audio-ins", 'i', JackDriverParamUInt, &value, NULL, "Number of capture channels (defaults to 2)", NULL);
        jack_driver_descriptor_add_parameter(desc, &filler, "audio-outs", 'o', JackDriverParamUInt, &value, NULL, "Number of playback channels (defaults to 2)", NULL);

        value.ui = 1U;
        jack_driver_descriptor_add_parameter(desc, &filler, "midi-ins", 'I', JackDriverParamUInt, &value, NULL, "Number of midi capture channels (defaults to 1)", NULL);
        jack_driver_descriptor_add_parameter(desc, &filler, "midi-outs", 'O', JackDriverParamUInt, &value, NULL, "Number of midi playback channels (defaults to 1)", NULL);

        value.ui = 48000U;
        jack_driver_descriptor_add_parameter(desc, &filler, "rate", 'r', JackDriverParamUInt, &value, NULL, "Sample rate", NULL);

        value.ui = 1024U;
        jack_driver_descriptor_add_parameter(desc, &filler, "period", 'p', JackDriverParamUInt, &value, NULL, "Frames per period", NULL);

        value.ui = 5U;
        jack_driver_descriptor_add_parameter(desc, &filler, "num-periods", 'n', JackDriverParamUInt, &value, NULL, "Network latency setting in no. of periods", NULL);

        value.ui = 3000U;
        jack_driver_descriptor_add_parameter(desc, &filler, "listen-port", 'l', JackDriverParamUInt, &value, NULL, "The socket port we are listening on for sync packets", NULL);

        value.ui = 1U;
        jack_driver_descriptor_add_parameter(desc, &filler, "factor", 'f', JackDriverParamUInt, &value, NULL, "Factor for sample rate reduction", NULL);

        value.ui = 0U;
        jack_driver_descriptor_add_parameter(desc, &filler, "upstream-factor", 'u', JackDriverParamUInt, &value, NULL, "Factor for sample rate reduction on the upstream", NULL);

#if HAVE_CELT
        value.ui = 0U;
        jack_driver_descriptor_add_parameter(desc, &filler, "celt", 'c', JackDriverParamUInt, &value, NULL, "Set CELT encoding and number of kbits per channel", NULL);
#endif
#if HAVE_OPUS
        value.ui = 0U;
        jack_driver_descriptor_add_parameter(desc, &filler, "opus", 'P', JackDriverParamUInt, &value, NULL, "Set Opus encoding and number of kbits per channel", NULL);
#endif
        value.ui = 0U;
        jack_driver_descriptor_add_parameter(desc, &filler, "bit-depth", 'b', JackDriverParamUInt, &value, NULL, "Sample bit-depth (0 for float, 8 for 8bit and 16 for 16bit)", NULL);

        value.i = true;
        jack_driver_descriptor_add_parameter(desc, &filler, "transport-sync", 't', JackDriverParamBool, &value, NULL, "Whether to slave the transport to the master transport", NULL);

        value.ui = true;
        jack_driver_descriptor_add_parameter(desc, &filler, "autoconf", 'a', JackDriverParamBool, &value, NULL, "Whether to use Autoconfig, or just start", NULL);

        value.ui = 1U;
        jack_driver_descriptor_add_parameter(desc, &filler, "redundancy", 'R', JackDriverParamUInt, &value, NULL, "Send packets N times", NULL);

        value.ui = false;
        jack_driver_descriptor_add_parameter(desc, &filler, "native-endian", 'e', JackDriverParamBool, &value, NULL, "Dont convert samples to network byte order", NULL);

        value.i = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "jitterval", 'J', JackDriverParamInt, &value, NULL, "Attempted jitterbuffer microseconds on master", NULL);

        value.i = false;
        jack_driver_descriptor_add_parameter(desc, &filler, "always-deadline", 'D', JackDriverParamBool, &value, NULL, "Always use deadline", NULL);

        return desc;
    }

    SERVER_EXPORT Jack::JackDriverClientInterface* driver_initialize(Jack::JackLockedEngine* engine, Jack::JackSynchro* table, const JSList* params)
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

        for (node = params; node; node = jack_slist_next(node)) {
            param = (const jack_driver_param_t*) node->data;
            switch (param->character) {
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
                    jack_error("not built with libsamplerate support");
                    return NULL;
#endif
                    break;

                case 'u':
#if HAVE_SAMPLERATE
                    resample_factor_up = param->value.ui;
#else
                    jack_error("not built with libsamplerate support");
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
                    jack_error("not built with celt support");
                    return NULL;
#endif
                    break;

                case 'P':
#if HAVE_OPUS
                    bitdepth = OPUS_MODE;
                    resample_factor = param->value.ui;
                    jack_error("OPUS: %d\n", resample_factor);
#else
                    jack_error("not built with Opus support");
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

        try {
            Jack::JackDriverClientInterface* driver = new Jack::JackWaitThreadedDriver (
                new Jack::JackNetOneDriver("system", "net_pcm", engine, table, listen_port, mtu,
                                             capture_ports_midi, playback_ports_midi, capture_ports, playback_ports,
                                             sample_rate, period_size, resample_factor,
                                             "net_pcm", handle_transport_sync, bitdepth, use_autoconfig, latency, redundancy,
                                             dont_htonl_floats, always_deadline, jitter_val));

            if (driver->Open(period_size, sample_rate, 1, 1, capture_ports, playback_ports,
                                0, "from_master_", "to_master_", 0, 0) == 0) {
                return driver;
            } else {
                delete driver;
                return NULL;
            }

        } catch (...) {
            return NULL;
        }
    }

#ifdef __cplusplus
}
#endif
}
