/*
Copyright (C) 2008-2011 Romain Moret at Grame

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

#include "JackNetAdapter.h"
#include "JackException.h"
#include "JackServerGlobals.h"
#include "JackEngineControl.h"
#include "JackArgParser.h"
#include <assert.h>

namespace Jack
{
    JackNetAdapter::JackNetAdapter(jack_client_t* jack_client, jack_nframes_t buffer_size, jack_nframes_t sample_rate, const JSList* params)
            : JackAudioAdapterInterface(buffer_size, sample_rate), JackNetSlaveInterface(), fThread(this)
    {
        jack_log("JackNetAdapter::JackNetAdapter");

        /*
        Global parameter setting : we can't call JackNetSlaveInterface constructor with some parameters before,
        because we don't have full parametering right now, parameters will be parsed from the param list,
        and then JackNetSlaveInterface will be filled with proper values.
        */
        char multicast_ip[32];
        uint udp_port;
        GetHostName(fParams.fName, JACK_CLIENT_NAME_SIZE);
        fSocket.GetName(fParams.fSlaveNetName);
        fParams.fMtu = DEFAULT_MTU;
        // Desactivated for now...
        fParams.fTransportSync = 0;
        int send_audio = -1;
        int return_audio = -1;
        fParams.fSendMidiChannels = 0;
        fParams.fReturnMidiChannels = 0;
        fParams.fSampleRate = sample_rate;
        fParams.fPeriodSize = buffer_size;
        fParams.fSlaveSyncMode = 1;
        fParams.fNetworkLatency = 2;
        fParams.fSampleEncoder = JackFloatEncoder;
        fClient = jack_client;

        // Possibly use env variable
        const char* default_udp_port = getenv("JACK_NETJACK_PORT");
        udp_port = (default_udp_port) ? atoi(default_udp_port) : DEFAULT_PORT;

        const char* default_multicast_ip = getenv("JACK_NETJACK_MULTICAST");
        if (default_multicast_ip) {
            strcpy(multicast_ip, default_multicast_ip);
        } else {
            strcpy(multicast_ip, DEFAULT_MULTICAST_IP);
        }

        //options parsing
        const JSList* node;
        const jack_driver_param_t* param;
        for (node = params; node; node = jack_slist_next(node))
        {
            param = (const jack_driver_param_t*) node->data;

            switch (param->character) {
                case 'a' :
                    assert(strlen(param->value.str) < 32);
                    strcpy(multicast_ip, param->value.str);
                    break;
                case 'p' :
                    udp_port = param->value.ui;
                    break;
                case 'M' :
                    fParams.fMtu = param->value.i;
                    break;
                case 'C' :
                    send_audio = param->value.i;
                    break;
                case 'P' :
                    return_audio = param->value.i;
                    break;
                case 'n' :
                    strncpy(fParams.fName, param->value.str, JACK_CLIENT_NAME_SIZE);
                    break;
                case 't' :
                    fParams.fTransportSync = param->value.ui;
                    break;
            #if HAVE_CELT
                case 'c':
                    if (param->value.i > 0) {
                        fParams.fSampleEncoder = JackCeltEncoder;
                        fParams.fKBps = param->value.i;
                    }
                    break;
            #endif
            #if HAVE_OPUS
                case 'O':
                    if (param->value.i > 0) {
                        fParams.fSampleEncoder = JackOpusEncoder;
                        fParams.fKBps = param->value.i;
                    }
                    break;
            #endif
                case 'l' :
                    fParams.fNetworkLatency = param->value.i;
                    if (fParams.fNetworkLatency > NETWORK_MAX_LATENCY) {
                        jack_error("Error : network latency is limited to %d\n", NETWORK_MAX_LATENCY);
                        throw std::bad_alloc();
                    }
                    break;
                case 'q':
                    fQuality = param->value.ui;
                    break;
                case 'g':
                    fRingbufferCurSize = param->value.ui;
                    fAdaptative = false;
                    break;
             }
        }

        strcpy(fMulticastIP, multicast_ip);

        // Set the socket parameters
        fSocket.SetPort(udp_port);
        fSocket.SetAddress(fMulticastIP, udp_port);

        // If not set, takes default
        fParams.fSendAudioChannels = (send_audio == -1) ? 2 : send_audio;

        // If not set, takes default
        fParams.fReturnAudioChannels = (return_audio == -1) ? 2 : return_audio;

        // Set the audio adapter interface channel values
        SetInputs(fParams.fSendAudioChannels);
        SetOutputs(fParams.fReturnAudioChannels);

        // Soft buffers will be allocated later (once network initialization done)
        fSoftCaptureBuffer = NULL;
        fSoftPlaybackBuffer = NULL;
    }

    JackNetAdapter::~JackNetAdapter()
    {
        jack_log("JackNetAdapter::~JackNetAdapter");

        if (fSoftCaptureBuffer) {
            for (int port_index = 0; port_index < fCaptureChannels; port_index++) {
                delete[] fSoftCaptureBuffer[port_index];
            }
            delete[] fSoftCaptureBuffer;
        }
        if (fSoftPlaybackBuffer) {
            for (int port_index = 0; port_index < fPlaybackChannels; port_index++) {
                delete[] fSoftPlaybackBuffer[port_index];
            }
            delete[] fSoftPlaybackBuffer;
        }
    }

//open/close--------------------------------------------------------------------------
    int JackNetAdapter::Open()
    {
        jack_info("NetAdapter started in %s mode %s Master's transport sync.",
                    (fParams.fSlaveSyncMode) ? "sync" : "async", (fParams.fTransportSync) ? "with" : "without");

        if (fThread.StartSync() < 0) {
            jack_error("Cannot start netadapter thread");
            return -1;
        }

        return 0;
    }

    int JackNetAdapter::Close()
    {
        int res = 0;
        jack_log("JackNetAdapter::Close");

#ifdef JACK_MONITOR
        fTable.Save(fHostBufferSize, fHostSampleRate, fAdaptedSampleRate, fAdaptedBufferSize);
#endif

        if (fThread.Kill() < 0) {
            jack_error("Cannot kill thread");
            res = -1;
        }

        fSocket.Close();
        return res;
   }

    int JackNetAdapter::SetBufferSize(jack_nframes_t buffer_size)
    {
        JackAudioAdapterInterface::SetHostBufferSize(buffer_size);
        return 0;
    }

//thread------------------------------------------------------------------------------
    // TODO : if failure, thread exist... need to restart ?

    bool JackNetAdapter::Init()
    {
        jack_log("JackNetAdapter::Init");

        //init network connection
        if (!JackNetSlaveInterface::Init()) {
            jack_error("JackNetSlaveInterface::Init() error...");
            return false;
        }

        //then set global parameters
        if (!SetParams()) {
            jack_error("SetParams error...");
            return false;
        }

        //set buffers
        if (fCaptureChannels > 0) {
            fSoftCaptureBuffer = new sample_t*[fCaptureChannels];
            for (int port_index = 0; port_index < fCaptureChannels; port_index++) {
                fSoftCaptureBuffer[port_index] = new sample_t[fParams.fPeriodSize];
                fNetAudioCaptureBuffer->SetBuffer(port_index, fSoftCaptureBuffer[port_index]);
            }
        }

        if (fPlaybackChannels > 0) {
            fSoftPlaybackBuffer = new sample_t*[fPlaybackChannels];
            for (int port_index = 0; port_index < fPlaybackChannels; port_index++) {
                fSoftPlaybackBuffer[port_index] = new sample_t[fParams.fPeriodSize];
                fNetAudioPlaybackBuffer->SetBuffer(port_index, fSoftPlaybackBuffer[port_index]);
            }
        }

        //set audio adapter parameters
        SetAdaptedBufferSize(fParams.fPeriodSize);
        SetAdaptedSampleRate(fParams.fSampleRate);

        // Will do "something" on OSX only...
        fThread.SetParams(GetEngineControl()->fPeriod, GetEngineControl()->fComputation, GetEngineControl()->fConstraint);

        if (fThread.AcquireSelfRealTime(GetEngineControl()->fClientPriority) < 0) {
            jack_error("AcquireSelfRealTime error");
        } else {
            set_threaded_log_function();
        }

        //init done, display parameters
        SessionParamsDisplay(&fParams);
        return true;
    }

    bool JackNetAdapter::Execute()
    {
        try {
            // Keep running even in case of error
            while (fThread.GetStatus() == JackThread::kRunning)
                if (Process() == SOCKET_ERROR) {
                    return false;
                }
            return false;
        } catch (JackNetException& e) {
            // Otherwise just restart...
            e.PrintMessage();
            jack_info("NetAdapter is restarted");
            Reset();
            fThread.DropSelfRealTime();
            fThread.SetStatus(JackThread::kIniting);
            if (Init()) {
                fThread.SetStatus(JackThread::kRunning);
                return true;
            } else {
                return false;
            }
        }
    }

//transport---------------------------------------------------------------------------
    void JackNetAdapter::DecodeTransportData()
    {
        //TODO : we need here to get the actual timebase master to eventually release it from its duty (see JackNetDriver)

        //is there a new transport state ?
        if (fSendTransportData.fNewState &&(fSendTransportData.fState != jack_transport_query(fClient, NULL))) {
            switch (fSendTransportData.fState)
            {
                case JackTransportStopped :
                    jack_transport_stop(fClient);
                    jack_info("NetMaster : transport stops");
                    break;

                case JackTransportStarting :
                    jack_transport_reposition(fClient, &fSendTransportData.fPosition);
                    jack_transport_start(fClient);
                    jack_info("NetMaster : transport starts");
                    break;

                case JackTransportRolling :
                    // TODO, we need to :
                    // - find a way to call TransportEngine->SetNetworkSync()
                    // - turn the transport state to JackTransportRolling
                    jack_info("NetMaster : transport rolls");
                    break;
            }
        }
    }

    void JackNetAdapter::EncodeTransportData()
    {
        //is there a timebase master change ?
        int refnum = -1;
        bool conditional = 0;
        //TODO : get the actual timebase master
        if (refnum != fLastTimebaseMaster) {
            //timebase master has released its function
            if (refnum == -1) {
                fReturnTransportData.fTimebaseMaster = RELEASE_TIMEBASEMASTER;
                jack_info("Sending a timebase master release request.");
            } else {
                //there is a new timebase master
                fReturnTransportData.fTimebaseMaster = (conditional) ? CONDITIONAL_TIMEBASEMASTER : TIMEBASEMASTER;
                jack_info("Sending a %s timebase master request.", (conditional) ? "conditional" : "non-conditional");
            }
            fLastTimebaseMaster = refnum;
        } else {
            fReturnTransportData.fTimebaseMaster = NO_CHANGE;
        }

        //update transport state and position
        fReturnTransportData.fState = jack_transport_query(fClient, &fReturnTransportData.fPosition);

        //is it a new state (that the master need to know...) ?
        fReturnTransportData.fNewState = ((fReturnTransportData.fState != fLastTransportState) &&
                                           (fReturnTransportData.fState != fSendTransportData.fState));
        if (fReturnTransportData.fNewState) {
            jack_info("Sending transport state '%s'.", GetTransportState(fReturnTransportData.fState));
        }
        fLastTransportState = fReturnTransportData.fState;
    }

//read/write operations---------------------------------------------------------------
    int JackNetAdapter::Read()
    {
        //don't return -1 in case of sync recv failure
        //we need the process to continue for network error detection
        if (SyncRecv() == SOCKET_ERROR) {
            return 0;
        }

        DecodeSyncPacket();
        return DataRecv();
    }

    int JackNetAdapter::Write()
    {
        EncodeSyncPacket();

        if (SyncSend() == SOCKET_ERROR) {
            return SOCKET_ERROR;
        }

        return DataSend();
    }

//process-----------------------------------------------------------------------------
    int JackNetAdapter::Process()
    {
        //read data from the network
        //in case of fatal network error, stop the process
        if (Read() == SOCKET_ERROR) {
            return SOCKET_ERROR;
        }

        PushAndPull(fSoftCaptureBuffer, fSoftPlaybackBuffer, fAdaptedBufferSize);

        //then write data to network
        //in case of failure, stop process
        if (Write() == SOCKET_ERROR) {
            return SOCKET_ERROR;
        }

        return 0;
    }

} // namespace Jack

//loader------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C"
{
#endif

#include "driver_interface.h"
#include "JackAudioAdapter.h"

    using namespace Jack;

    SERVER_EXPORT jack_driver_desc_t* jack_get_descriptor()
    {
        jack_driver_desc_t * desc;
        jack_driver_desc_filler_t filler;
        jack_driver_param_value_t value;

        desc = jack_driver_descriptor_construct("netadapter", JackDriverNone, "netjack net <==> audio backend adapter", &filler);

        strcpy(value.str, DEFAULT_MULTICAST_IP);
        jack_driver_descriptor_add_parameter(desc, &filler, "multicast-ip", 'a', JackDriverParamString, &value, NULL, "Multicast address, or explicit IP of the master", NULL);

        value.i = DEFAULT_PORT;
        jack_driver_descriptor_add_parameter(desc, &filler, "udp-net-port", 'p', JackDriverParamInt, &value, NULL, "UDP port", NULL);

        value.i = DEFAULT_MTU;
        jack_driver_descriptor_add_parameter(desc, &filler, "mtu", 'M', JackDriverParamInt, &value, NULL, "MTU to the master", NULL);

        value.i = 2;
        jack_driver_descriptor_add_parameter(desc, &filler, "input-ports", 'C', JackDriverParamInt, &value, NULL, "Number of audio input ports", NULL);
        jack_driver_descriptor_add_parameter(desc, &filler, "output-ports", 'P', JackDriverParamInt, &value, NULL, "Number of audio output ports", NULL);

    #if HAVE_CELT
        value.i = -1;
        jack_driver_descriptor_add_parameter(desc, &filler, "celt", 'c', JackDriverParamInt, &value, NULL, "Set CELT encoding and number of kBits per channel", NULL);
    #endif

    #if HAVE_OPUS
        value.i = -1;
        jack_driver_descriptor_add_parameter(desc, &filler, "opus", 'O', JackDriverParamInt, &value, NULL, "Set Opus encoding and number of kBits per channel", NULL);
    #endif

        strcpy(value.str, "'hostname'");
        jack_driver_descriptor_add_parameter(desc, &filler, "client-name", 'n', JackDriverParamString, &value, NULL, "Name of the jack client", NULL);

        value.ui = 0U;
        jack_driver_descriptor_add_parameter(desc, &filler, "transport-sync", 't', JackDriverParamUInt, &value, NULL, "Sync transport with master's", NULL);

        value.ui = 5U;
        jack_driver_descriptor_add_parameter(desc, &filler, "latency", 'l', JackDriverParamUInt, &value, NULL, "Network latency", NULL);

        value.i = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "quality", 'q', JackDriverParamInt, &value, NULL, "Resample algorithm quality (0 - 4)", NULL);

        value.i = 32768;
        jack_driver_descriptor_add_parameter(desc, &filler, "ring-buffer", 'g', JackDriverParamInt, &value, NULL, "Fixed ringbuffer size", "Fixed ringbuffer size (if not set => automatic adaptative)");

        value.i = false;
        jack_driver_descriptor_add_parameter(desc, &filler, "auto-connect", 'c', JackDriverParamBool, &value, NULL, "Auto connect netmaster to system ports", "");

        return desc;
    }

    SERVER_EXPORT int jack_internal_initialize(jack_client_t* client, const JSList* params)
    {
        jack_log("Loading netadapter");

        Jack::JackAudioAdapter* adapter;
        jack_nframes_t buffer_size = jack_get_buffer_size(client);
        jack_nframes_t sample_rate = jack_get_sample_rate(client);

        try {

            adapter = new Jack::JackAudioAdapter(client, new Jack::JackNetAdapter(client, buffer_size, sample_rate, params), params);
            assert(adapter);

            if (adapter->Open() == 0) {
                return 0;
            } else {
                delete adapter;
                return 1;
            }

        } catch (...) {
            jack_info("netadapter allocation error");
            return 1;
        }
    }

    SERVER_EXPORT int jack_initialize(jack_client_t* jack_client, const char* load_init)
    {
        JSList* params = NULL;
        bool parse_params = true;
        int res = 1;
        jack_driver_desc_t* desc = jack_get_descriptor();

        Jack::JackArgParser parser(load_init);
        if (parser.GetArgc() > 0) {
            parse_params = parser.ParseParams(desc, &params);
        }

        if (parse_params) {
            res = jack_internal_initialize(jack_client, params);
            parser.FreeParams(params);
        }
        return res;
    }

    SERVER_EXPORT void jack_finish(void* arg)
    {
        Jack::JackAudioAdapter* adapter = static_cast<Jack::JackAudioAdapter*>(arg);

        if (adapter) {
            jack_log("Unloading netadapter");
            adapter->Close();
            delete adapter;
        }
    }

#ifdef __cplusplus
}
#endif
