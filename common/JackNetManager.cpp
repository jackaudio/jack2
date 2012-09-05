/*
Copyright(C) 2008-2011 Romain Moret at Grame

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

#include "JackNetManager.h"
#include "JackArgParser.h"
#include "JackServerGlobals.h"
#include "JackLockedEngine.h"

using namespace std;

namespace Jack
{
//JackNetMaster******************************************************************************************************

    JackNetMaster::JackNetMaster(JackNetSocket& socket, session_params_t& params, const char* multicast_ip)
            : JackNetMasterInterface(params, socket, multicast_ip)
    {
        jack_log("JackNetMaster::JackNetMaster");

        //settings
        fName = const_cast<char*>(fParams.fName);
        fClient = NULL;
        fSendTransportData.fState = -1;
        fReturnTransportData.fState = -1;
        fLastTransportState = -1;
        int port_index;

        //jack audio ports
        fAudioCapturePorts = new jack_port_t* [fParams.fSendAudioChannels];
        for (port_index = 0; port_index < fParams.fSendAudioChannels; port_index++) {
            fAudioCapturePorts[port_index] = NULL;
        }

        fAudioPlaybackPorts = new jack_port_t* [fParams.fReturnAudioChannels];
        for (port_index = 0; port_index < fParams.fReturnAudioChannels; port_index++) {
            fAudioPlaybackPorts[port_index] = NULL;
        }

        //jack midi ports
        fMidiCapturePorts = new jack_port_t* [fParams.fSendMidiChannels];
        for (port_index = 0; port_index < fParams.fSendMidiChannels; port_index++) {
            fMidiCapturePorts[port_index] = NULL;
        }

        fMidiPlaybackPorts = new jack_port_t* [fParams.fReturnMidiChannels];
        for (port_index = 0; port_index < fParams.fReturnMidiChannels; port_index++) {
            fMidiPlaybackPorts[port_index] = NULL;
        }

        //monitor
#ifdef JACK_MONITOR
        fPeriodUsecs = (int)(1000000.f * ((float) fParams.fPeriodSize / (float) fParams.fSampleRate));
        string plot_name;
        plot_name = string(fParams.fName);
        plot_name += string("_master");
        plot_name += string((fParams.fSlaveSyncMode) ? "_sync" : "_async");
        plot_name += string("_latency");
        fNetTimeMon = new JackGnuPlotMonitor<float>(128, 4, plot_name);
        string net_time_mon_fields[] =
        {
            string("sync send"),
            string("end of send"),
            string("sync recv"),
            string("end of cycle")
        };
        string net_time_mon_options[] =
        {
            string("set xlabel \"audio cycles\""),
            string("set ylabel \"% of audio cycle\"")
        };
        fNetTimeMon->SetPlotFile(net_time_mon_options, 2, net_time_mon_fields, 4);
#endif
    }

    JackNetMaster::~JackNetMaster()
    {
        jack_log("JackNetMaster::~JackNetMaster ID = %u", fParams.fID);

        if (fClient) {
            jack_deactivate(fClient);
            FreePorts();
            jack_client_close(fClient);
        }
        delete[] fAudioCapturePorts;
        delete[] fAudioPlaybackPorts;
        delete[] fMidiCapturePorts;
        delete[] fMidiPlaybackPorts;
#ifdef JACK_MONITOR
        fNetTimeMon->Save();
        delete fNetTimeMon;
#endif
    }
//init--------------------------------------------------------------------------------
    bool JackNetMaster::Init(bool auto_connect)
    {
        //network init
        if (!JackNetMasterInterface::Init()) {
            jack_error("JackNetMasterInterface::Init() error...");
            return false;
        }

        //set global parameters
        if (!SetParams()) {
            jack_error("SetParams error...");
            return false;
        }

        //jack client and process
        jack_status_t status;
        if ((fClient = jack_client_open(fName, JackNullOption, &status, NULL)) == NULL) {
            jack_error("Can't open a new JACK client");
            return false;
        }
        
        if (jack_set_process_callback(fClient, SetProcess, this) < 0) {
            goto fail;
        }

        if (jack_set_buffer_size_callback(fClient, SetBufferSize, this) < 0) {
            goto fail;
        }
        
        /*
        if (jack_set_port_connect_callback(fClient, SetConnectCallback, this) < 0) {
            goto fail;
        }
        */

        if (AllocPorts() != 0) {
            jack_error("Can't allocate JACK ports");
            goto fail;
        }

        //process can now run
        fRunning = true;

        //finally activate jack client
        if (jack_activate(fClient) != 0) {
            jack_error("Can't activate JACK client");
            goto fail;
        }

        if (auto_connect) {
            ConnectPorts();
        }
        jack_info("New NetMaster started");
        return true;

    fail:
        FreePorts();
        jack_client_close(fClient);
        fClient = NULL;
        return false;
    }

//jack ports--------------------------------------------------------------------------
    int JackNetMaster::AllocPorts()
    {
        int i;
        char name[24];
        jack_nframes_t port_latency = jack_get_buffer_size(fClient);
        jack_latency_range_t range;

        jack_log("JackNetMaster::AllocPorts");

        //audio
        for (i = 0; i < fParams.fSendAudioChannels; i++) {
            snprintf(name, sizeof(name), "to_slave_%d", i+1);
            if ((fAudioCapturePorts[i] = jack_port_register(fClient, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput | JackPortIsTerminal, 0)) == NULL)
                return -1;
            //port latency
            range.min = range.max = 0;
            jack_port_set_latency_range(fAudioCapturePorts[i], JackCaptureLatency, &range);
        }

        for (i = 0; i < fParams.fReturnAudioChannels; i++) {
            snprintf(name, sizeof(name), "from_slave_%d", i+1);
            if ((fAudioPlaybackPorts[i] = jack_port_register(fClient, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput | JackPortIsTerminal, 0)) == NULL)
                return -1;
            //port latency
            range.min = range.max = fParams.fNetworkLatency * port_latency + (fParams.fSlaveSyncMode) ? 0 : port_latency;
            jack_port_set_latency_range(fAudioPlaybackPorts[i], JackPlaybackLatency, &range);
        }

        //midi
        for (i = 0; i < fParams.fSendMidiChannels; i++) {
            snprintf(name, sizeof(name), "midi_to_slave_%d", i+1);
            if ((fMidiCapturePorts[i] = jack_port_register(fClient, name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput | JackPortIsTerminal, 0)) == NULL)
                return -1;
            //port latency
            range.min = range.max = 0;
            jack_port_set_latency_range(fMidiCapturePorts[i], JackCaptureLatency, &range);
        }

        for (i = 0; i < fParams.fReturnMidiChannels; i++) {
            snprintf(name, sizeof(name), "midi_from_slave_%d", i+1);
            if ((fMidiPlaybackPorts[i] = jack_port_register(fClient, name, JACK_DEFAULT_MIDI_TYPE,  JackPortIsOutput | JackPortIsTerminal, 0)) == NULL)
                return -1;
            //port latency
            range.min = range.max = fParams.fNetworkLatency * port_latency + (fParams.fSlaveSyncMode) ? 0 : port_latency;
            jack_port_set_latency_range(fMidiPlaybackPorts[i], JackPlaybackLatency, &range);
        }
        return 0;
    }

    void JackNetMaster::ConnectPorts()
    {
        const char **ports;

        ports = jack_get_ports(fClient, NULL, NULL, JackPortIsPhysical | JackPortIsOutput);
        if (ports != NULL) {
            for (int i = 0; i < fParams.fSendAudioChannels && ports[i]; i++) {
                jack_connect(fClient, ports[i], jack_port_name(fAudioCapturePorts[i]));
            }
            free(ports);
        }

        ports = jack_get_ports(fClient, NULL, NULL, JackPortIsPhysical | JackPortIsInput);
        if (ports != NULL) {
            for (int i = 0; i < fParams.fReturnAudioChannels && ports[i]; i++) {
                jack_connect(fClient, jack_port_name(fAudioPlaybackPorts[i]), ports[i]);
            }
            free(ports);
        }
    }

    void JackNetMaster::FreePorts()
    {
        jack_log("JackNetMaster::FreePorts ID = %u", fParams.fID);

        int port_index;
        for (port_index = 0; port_index < fParams.fSendAudioChannels; port_index++) {
            if (fAudioCapturePorts[port_index]) {
                jack_port_unregister(fClient, fAudioCapturePorts[port_index]);
            }
        }
        for (port_index = 0; port_index < fParams.fReturnAudioChannels; port_index++) {
            if (fAudioPlaybackPorts[port_index]) {
                jack_port_unregister(fClient, fAudioPlaybackPorts[port_index]);
            }
        }
        for (port_index = 0; port_index < fParams.fSendMidiChannels; port_index++) {
            if (fMidiCapturePorts[port_index]) {
                jack_port_unregister(fClient, fMidiCapturePorts[port_index]);
            }
        }
        for (port_index = 0; port_index < fParams.fReturnMidiChannels; port_index++) {
            if (fMidiPlaybackPorts[port_index]) {
                jack_port_unregister(fClient, fMidiPlaybackPorts[port_index]);
            }
        }
    }

//transport---------------------------------------------------------------------------
    void JackNetMaster::EncodeTransportData()
    {
        //is there a new timebase master ?
        //TODO : check if any timebase callback has been called (and if it's conditional or not) and set correct value...
        fSendTransportData.fTimebaseMaster = NO_CHANGE;

        //update state and position
        fSendTransportData.fState = static_cast<uint>(jack_transport_query(fClient, &fSendTransportData.fPosition));

        //is it a new state ?
        fSendTransportData.fNewState = ((fSendTransportData.fState != fLastTransportState) && (fSendTransportData.fState != fReturnTransportData.fState));
        if (fSendTransportData.fNewState) {
            jack_info("Sending '%s' to '%s' frame = %ld", GetTransportState(fSendTransportData.fState), fParams.fName, fSendTransportData.fPosition.frame);
        }
        fLastTransportState = fSendTransportData.fState;
   }

    void JackNetMaster::DecodeTransportData()
    {
        //is there timebase master change ?
        if (fReturnTransportData.fTimebaseMaster != NO_CHANGE) {

            int timebase = 0;
            switch (fReturnTransportData.fTimebaseMaster)
            {
                case RELEASE_TIMEBASEMASTER :
                    timebase = jack_release_timebase(fClient);
                    if (timebase < 0) {
                        jack_error("Can't release timebase master");
                    } else {
                        jack_info("'%s' isn't the timebase master anymore", fParams.fName);
                    }
                    break;

                case TIMEBASEMASTER :
                    timebase = jack_set_timebase_callback(fClient, 0, SetTimebaseCallback, this);
                    if (timebase < 0) {
                        jack_error("Can't set a new timebase master");
                    } else {
                        jack_info("'%s' is the new timebase master", fParams.fName);
                    }
                    break;

                case CONDITIONAL_TIMEBASEMASTER :
                    timebase = jack_set_timebase_callback(fClient, 1, SetTimebaseCallback, this);
                    if (timebase != EBUSY) {
                        if (timebase < 0)
                            jack_error("Can't set a new timebase master");
                        else
                            jack_info("'%s' is the new timebase master", fParams.fName);
                    }
                    break;
            }
        }

        //is the slave in a new transport state and is this state different from master's ?
        if (fReturnTransportData.fNewState && (fReturnTransportData.fState != jack_transport_query(fClient, NULL))) {

            switch (fReturnTransportData.fState)
            {
                case JackTransportStopped :
                    jack_transport_stop(fClient);
                    jack_info("'%s' stops transport", fParams.fName);
                    break;

                case JackTransportStarting :
                    if (jack_transport_reposition(fClient, &fReturnTransportData.fPosition) == EINVAL)
                        jack_error("Can't set new position");
                    jack_transport_start(fClient);
                    jack_info("'%s' starts transport frame = %d", fParams.fName, fReturnTransportData.fPosition.frame);
                    break;

                case JackTransportNetStarting :
                    jack_info("'%s' is ready to roll...", fParams.fName);
                    break;

                case JackTransportRolling :
                    jack_info("'%s' is rolling", fParams.fName);
                    break;
            }
        }
    }

    void JackNetMaster::SetTimebaseCallback(jack_transport_state_t state, jack_nframes_t nframes, jack_position_t* pos, int new_pos, void* arg)
    {
        static_cast<JackNetMaster*>(arg)->TimebaseCallback(pos);
    }

    void JackNetMaster::TimebaseCallback(jack_position_t* pos)
    {
        pos->bar = fReturnTransportData.fPosition.bar;
        pos->beat = fReturnTransportData.fPosition.beat;
        pos->tick = fReturnTransportData.fPosition.tick;
        pos->bar_start_tick = fReturnTransportData.fPosition.bar_start_tick;
        pos->beats_per_bar = fReturnTransportData.fPosition.beats_per_bar;
        pos->beat_type = fReturnTransportData.fPosition.beat_type;
        pos->ticks_per_beat = fReturnTransportData.fPosition.ticks_per_beat;
        pos->beats_per_minute = fReturnTransportData.fPosition.beats_per_minute;
    }

//sync--------------------------------------------------------------------------------

    bool JackNetMaster::IsSlaveReadyToRoll()
    {
        return (fReturnTransportData.fState == JackTransportNetStarting);
    }

    int JackNetMaster::SetBufferSize(jack_nframes_t nframes, void* arg)
    {
        JackNetMaster* obj = static_cast<JackNetMaster*>(arg);
        if (nframes != obj->fParams.fPeriodSize) {
            jack_error("Cannot handle buffer size change, so JackNetMaster proxy will be removed...");
            obj->Exit();
        }
        return 0;
    }

//process-----------------------------------------------------------------------------
    int JackNetMaster::SetProcess(jack_nframes_t nframes, void* arg)
    {
        try {
            return static_cast<JackNetMaster*>(arg)->Process();
        } catch (JackNetException& e) {
            return 0;
        }
    }
    
    void JackNetMaster::SetConnectCallback(jack_port_id_t a, jack_port_id_t b, int connect, void* arg)
    {
        static_cast<JackNetMaster*>(arg)->ConnectCallback(a, b, connect);
    }
    
    void JackNetMaster::ConnectCallback(jack_port_id_t a, jack_port_id_t b, int connect)
    {
        jack_info("JackNetMaster::ConnectCallback a = %d b = %d connect = %d", a, b, connect);
        if (connect) {
            jack_connect(fClient, jack_port_name(jack_port_by_id(fClient, a)), "system:playback_1");
        }
    }

    int JackNetMaster::Process()
    {
        if (!fRunning) {
            return 0;
        }

#ifdef JACK_MONITOR
        jack_time_t begin_time = GetMicroSeconds();
        fNetTimeMon->New();
#endif

        //buffers
        for (int midi_port_index = 0; midi_port_index < fParams.fSendMidiChannels; midi_port_index++) {
            fNetMidiCaptureBuffer->SetBuffer(midi_port_index,
                                            static_cast<JackMidiBuffer*>(jack_port_get_buffer(fMidiCapturePorts[midi_port_index],
                                            fParams.fPeriodSize)));
        }
        for (int audio_port_index = 0; audio_port_index < fParams.fSendAudioChannels; audio_port_index++) {

        #ifdef OPTIMIZED_PROTOCOL
            if (fNetAudioCaptureBuffer->GetConnected(audio_port_index)) {
                // Port is connected on other side...
                fNetAudioCaptureBuffer->SetBuffer(audio_port_index,
                                                ((jack_port_connected(fAudioCapturePorts[audio_port_index]) > 0)
                                                ? static_cast<sample_t*>(jack_port_get_buffer(fAudioCapturePorts[audio_port_index], fParams.fPeriodSize))
                                                : NULL));
            } else {
                fNetAudioCaptureBuffer->SetBuffer(audio_port_index, NULL);
            }
        #else
            fNetAudioCaptureBuffer->SetBuffer(audio_port_index,
                                            static_cast<sample_t*>(jack_port_get_buffer(fAudioCapturePorts[audio_port_index],
                                            fParams.fPeriodSize)));
        #endif
            // TODO
        }

        for (int midi_port_index = 0; midi_port_index < fParams.fReturnMidiChannels; midi_port_index++) {
            fNetMidiPlaybackBuffer->SetBuffer(midi_port_index,
                                                static_cast<JackMidiBuffer*>(jack_port_get_buffer(fMidiPlaybackPorts[midi_port_index],
                                                fParams.fPeriodSize)));
        }
        for (int audio_port_index = 0; audio_port_index < fParams.fReturnAudioChannels; audio_port_index++) {

        #ifdef OPTIMIZED_PROTOCOL
            sample_t* out = (jack_port_connected(fAudioPlaybackPorts[audio_port_index]) > 0)
                ? static_cast<sample_t*>(jack_port_get_buffer(fAudioPlaybackPorts[audio_port_index], fParams.fPeriodSize))
                : NULL;
            if (out) {
                memset(out, 0, sizeof(float) * fParams.fPeriodSize);
            }
            fNetAudioPlaybackBuffer->SetBuffer(audio_port_index, out);
        #else
            sample_t* out = static_cast<sample_t*>(jack_port_get_buffer(fAudioPlaybackPorts[audio_port_index], fParams.fPeriodSize));
            if (out) {
                memset(out, 0, sizeof(float) * fParams.fPeriodSize);
            }
            fNetAudioPlaybackBuffer->SetBuffer(audio_port_index, out)));
        #endif
        }

        if (IsSynched()) {  // only send if connection is "synched"

            //encode the first packet
            EncodeSyncPacket();

            if (SyncSend() == SOCKET_ERROR) {
                return SOCKET_ERROR;
            }

    #ifdef JACK_MONITOR
            fNetTimeMon->Add((((float)(GetMicroSeconds() - begin_time)) / (float) fPeriodUsecs) * 100.f);
    #endif

            //send data
            if (DataSend() == SOCKET_ERROR) {
                return SOCKET_ERROR;
            }

    #ifdef JACK_MONITOR
            fNetTimeMon->Add((((float)(GetMicroSeconds() - begin_time)) / (float) fPeriodUsecs) * 100.f);
    #endif

        } else {
            jack_info("Connection is not synched, skip cycle...");
        }

        //receive sync
        int res = SyncRecv();
        if ((res == 0) || (res == SOCKET_ERROR)) {
            return res;
        }

#ifdef JACK_MONITOR
        fNetTimeMon->Add((((float)(GetMicroSeconds() - begin_time)) / (float) fPeriodUsecs) * 100.f);
#endif

        //decode sync
        DecodeSyncPacket();

        //receive data
        res = DataRecv();
        if ((res == 0) || (res == SOCKET_ERROR)) {
            return res;
        } else if (res == NET_PACKET_ERROR) {
            // Well not a real XRun...
            JackServerGlobals::fInstance->GetEngine()->NotifyXRun(GetMicroSeconds(), 0);
        }

#ifdef JACK_MONITOR
        fNetTimeMon->AddLast((((float)(GetMicroSeconds() - begin_time)) / (float) fPeriodUsecs) * 100.f);
#endif
        return 0;
    }

//JackNetMasterManager***********************************************************************************************

    JackNetMasterManager::JackNetMasterManager(jack_client_t* client, const JSList* params) : fSocket()
    {
        jack_log("JackNetMasterManager::JackNetMasterManager");

        fClient = client;
        fName = jack_get_client_name(fClient);
        fGlobalID = 0;
        fRunning = true;
        fAutoConnect = false;

        const JSList* node;
        const jack_driver_param_t* param;
     
        jack_on_shutdown(fClient, SetShutDown, this);
    
        // Possibly use env variable
        const char* default_udp_port = getenv("JACK_NETJACK_PORT");
        fSocket.SetPort((default_udp_port) ? atoi(default_udp_port) : DEFAULT_PORT);

        const char* default_multicast_ip = getenv("JACK_NETJACK_MULTICAST");
        if (default_multicast_ip) {
            strcpy(fMulticastIP, default_multicast_ip);
        } else {
            strcpy(fMulticastIP, DEFAULT_MULTICAST_IP);
        }

        for (node = params; node; node = jack_slist_next(node)) {

            param = (const jack_driver_param_t*) node->data;
            switch (param->character)
            {
                case 'a' :
                    if (strlen(param->value.str) < 32) {
                        strcpy(fMulticastIP, param->value.str);
                    } else {
                        jack_error("Can't use multicast address %s, using default %s", param->value.ui, DEFAULT_MULTICAST_IP);
                    }
                    break;

                case 'p':
                    fSocket.SetPort(param->value.ui);
                    break;

                case 'c':
                    fAutoConnect = param->value.i;
                    break;
            }
        }

        //set sync callback
        jack_set_sync_callback(fClient, SetSyncCallback, this);

        //activate the client (for sync callback)
        if (jack_activate(fClient) != 0) {
            jack_error("Can't activate the NetManager client, transport disabled");
        }

        //launch the manager thread
        if (jack_client_create_thread(fClient, &fThread, 0, 0, NetManagerThread, this)) {
            jack_error("Can't create the NetManager control thread");
        }
    }

    JackNetMasterManager::~JackNetMasterManager()
    {
        jack_log("JackNetMasterManager::~JackNetMasterManager");
        ShutDown();
    }

    int JackNetMasterManager::CountIO(int flags)
    {
        const char **ports;
        int count = 0;
        jack_port_t* port;

        ports = jack_get_ports(fClient, NULL, NULL, flags);
        if (ports != NULL) {
            while (ports[count]
                    && (port = jack_port_by_name(fClient, ports[count]))
                    && (strcmp(jack_port_type(port), JACK_DEFAULT_AUDIO_TYPE) == 0)) {
                count++;
            }
            free(ports);
        }
        return count;
    }
    
    void JackNetMasterManager::SetShutDown(void* arg)
    {
        static_cast<JackNetMasterManager*>(arg)->ShutDown();
    }
    
    void JackNetMasterManager::ShutDown()
    {
        jack_log("JackNetMasterManager::ShutDown");
        if (fRunning) {
            jack_client_kill_thread(fClient, fThread);
            fRunning = false;
        }
        master_list_t::iterator it;
        for (it = fMasterList.begin(); it != fMasterList.end(); it++) {
            delete(*it);
        }
        fMasterList.clear();
        fSocket.Close();
        SocketAPIEnd();
    }

    int JackNetMasterManager::SetSyncCallback(jack_transport_state_t state, jack_position_t* pos, void* arg)
    {
        return static_cast<JackNetMasterManager*>(arg)->SyncCallback(state, pos);
    }

    int JackNetMasterManager::SyncCallback(jack_transport_state_t state, jack_position_t* pos)
    {
        //check if each slave is ready to roll
        int ret = 1;
        master_list_it_t it;
        for (it = fMasterList.begin(); it != fMasterList.end(); it++) {
            if (!(*it)->IsSlaveReadyToRoll()) {
                ret = 0;
            }
        }
        jack_log("JackNetMasterManager::SyncCallback returns '%s'", (ret) ? "true" : "false");
        return ret;
    }

    void* JackNetMasterManager::NetManagerThread(void* arg)
    {
        JackNetMasterManager* master_manager = static_cast<JackNetMasterManager*>(arg);
        jack_info("Starting Jack NetManager");
        jack_info("Listening on '%s:%d'", master_manager->fMulticastIP, master_manager->fSocket.GetPort());
        master_manager->Run();
        return NULL;
    }

    void JackNetMasterManager::Run()
    {
        jack_log("JackNetMasterManager::Run");
        //utility variables
        int attempt = 0;

        //data
        session_params_t host_params;
        int rx_bytes = 0;
        JackNetMaster* net_master;

        //init socket API (win32)
        if (SocketAPIInit() < 0) {
            jack_error("Can't init Socket API, exiting...");
            return;
        }

        //socket
        if (fSocket.NewSocket() == SOCKET_ERROR) {
            jack_error("Can't create NetManager input socket : %s", StrError(NET_ERROR_CODE));
            return;
        }

        //bind the socket to the local port
        if (fSocket.Bind() == SOCKET_ERROR) {
            jack_error("Can't bind NetManager socket : %s", StrError(NET_ERROR_CODE));
            fSocket.Close();
            return;
        }

        //join multicast group
        if (fSocket.JoinMCastGroup(fMulticastIP) == SOCKET_ERROR) {
            jack_error("Can't join multicast group : %s", StrError(NET_ERROR_CODE));
        }

        //local loop
        if (fSocket.SetLocalLoop() == SOCKET_ERROR) {
            jack_error("Can't set local loop : %s", StrError(NET_ERROR_CODE));
        }

        //set a timeout on the multicast receive (the thread can now be cancelled)
        if (fSocket.SetTimeOut(MANAGER_INIT_TIMEOUT) == SOCKET_ERROR) {
            jack_error("Can't set timeout : %s", StrError(NET_ERROR_CODE));
        }

        //main loop, wait for data, deal with it and wait again
        do
        {
            session_params_t net_params;
            rx_bytes = fSocket.CatchHost(&net_params, sizeof(session_params_t), 0);
            SessionParamsNToH(&net_params, &host_params);
            
            if ((rx_bytes == SOCKET_ERROR) && (fSocket.GetError() != NET_NO_DATA)) {
                jack_error("Error in receive : %s", StrError(NET_ERROR_CODE));
                if (++attempt == 10) {
                    jack_error("Can't receive on the socket, exiting net manager");
                    return;
                }
            }

            if (rx_bytes == sizeof(session_params_t)) {
                switch (GetPacketType(&host_params))
                {
                    case SLAVE_AVAILABLE:
                        if ((net_master = InitMaster(host_params))) {
                            SessionParamsDisplay(&net_master->fParams);
                        } else {
                            jack_error("Can't init new NetMaster...");
                        }
                        jack_info("Waiting for a slave...");
                        break;
                    case KILL_MASTER:
                        if (KillMaster(&host_params)) {
                            jack_info("Waiting for a slave...");
                        }
                        break;
                    default:
                        break;
                }
            }
        }
        while (fRunning);
    }

    JackNetMaster* JackNetMasterManager::InitMaster(session_params_t& params)
    {
        jack_log("JackNetMasterManager::InitMaster, Slave : %s", params.fName);

        //check MASTER <<==> SLAVE network protocol coherency
        if (params.fProtocolVersion != MASTER_PROTOCOL) {
            jack_error("Error : slave %s is running with a different protocol %d != %d", params.fName,  params.fProtocolVersion, MASTER_PROTOCOL);
            return NULL;
        }

        //settings
        fSocket.GetName(params.fMasterNetName);
        params.fID = ++fGlobalID;
        params.fSampleRate = jack_get_sample_rate(fClient);
        params.fPeriodSize = jack_get_buffer_size(fClient);

        if (params.fSendAudioChannels == -1) {
            params.fSendAudioChannels = CountIO(JackPortIsPhysical | JackPortIsOutput);
            jack_info("Takes physical %d inputs for client", params.fSendAudioChannels);
        }

        if (params.fReturnAudioChannels == -1) {
            params.fReturnAudioChannels = CountIO(JackPortIsPhysical | JackPortIsInput);
            jack_info("Takes physical %d outputs for client", params.fReturnAudioChannels);
        }

        //create a new master and add it to the list
        JackNetMaster* master = new JackNetMaster(fSocket, params, fMulticastIP);
        if (master->Init(fAutoConnect)) {
            fMasterList.push_back(master);
            return master;
        }
        delete master;
        return NULL;
    }

    master_list_it_t JackNetMasterManager::FindMaster(uint32_t id)
    {
        jack_log("JackNetMasterManager::FindMaster ID = %u", id);

        master_list_it_t it;
        for (it = fMasterList.begin(); it != fMasterList.end(); it++) {
            if ((*it)->fParams.fID == id) {
                return it;
            }
        }
        return it;
    }

    int JackNetMasterManager::KillMaster(session_params_t* params)
    {
        jack_log("JackNetMasterManager::KillMaster ID = %u", params->fID);

        master_list_it_t master = FindMaster(params->fID);
        if (master != fMasterList.end()) {
            fMasterList.erase(master);
            delete *master;
            return 1;
        }
        return 0;
    }
}//namespace

static Jack::JackNetMasterManager* master_manager = NULL;

#ifdef __cplusplus
extern "C"
{
#endif

    SERVER_EXPORT jack_driver_desc_t* jack_get_descriptor()
    {
        jack_driver_desc_t * desc;
        jack_driver_desc_filler_t filler;
        jack_driver_param_value_t value;

        desc = jack_driver_descriptor_construct("netmanager", JackDriverNone, "netjack multi-cast master component", &filler);

        strcpy(value.str, DEFAULT_MULTICAST_IP);
        jack_driver_descriptor_add_parameter(desc, &filler, "multicast-ip", 'a', JackDriverParamString, &value, NULL, "Multicast address", NULL);

        value.i = DEFAULT_PORT;
        jack_driver_descriptor_add_parameter(desc, &filler, "udp-net-port", 'p', JackDriverParamInt, &value, NULL, "UDP port", NULL);

        value.i = false;
        jack_driver_descriptor_add_parameter(desc, &filler, "auto-connect", 'c', JackDriverParamBool, &value, NULL, "Auto connect netmaster to system ports", NULL);

        return desc;
    }

    SERVER_EXPORT int jack_internal_initialize(jack_client_t* jack_client, const JSList* params)
    {
        if (master_manager) {
            jack_error("Master Manager already loaded");
            return 1;
        } else {
            jack_log("Loading Master Manager");
            master_manager = new Jack::JackNetMasterManager(jack_client, params);
            return (master_manager) ? 0 : 1;
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
        if (master_manager) {
            jack_log("Unloading Master Manager");
            delete master_manager;
            master_manager = NULL;
        }
    }

#ifdef __cplusplus
}
#endif
