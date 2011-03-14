/*
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

#include "JackNetManager.h"
#include "JackArgParser.h"
#include "JackTime.h"

using namespace std;

namespace Jack
{
//JackNetMaster******************************************************************************************************

    JackNetMaster::JackNetMaster ( JackNetSocket& socket, session_params_t& params, const char* multicast_ip)
            : JackNetMasterInterface ( params, socket, multicast_ip )
    {
        jack_log ( "JackNetMaster::JackNetMaster" );

        //settings
        fClientName = const_cast<char*> ( fParams.fName );
        fJackClient = NULL;
        fSendTransportData.fState = -1;
        fReturnTransportData.fState = -1;
        fLastTransportState = -1;
        uint port_index;

        //jack audio ports
        fAudioCapturePorts = new jack_port_t* [fParams.fSendAudioChannels];
        for ( port_index = 0; port_index < fParams.fSendAudioChannels; port_index++ )
            fAudioCapturePorts[port_index] = NULL;
        fAudioPlaybackPorts = new jack_port_t* [fParams.fReturnAudioChannels];
        for ( port_index = 0; port_index < fParams.fReturnAudioChannels; port_index++ )
            fAudioPlaybackPorts[port_index] = NULL;
        //jack midi ports
        fMidiCapturePorts = new jack_port_t* [fParams.fSendMidiChannels];
        for ( port_index = 0; port_index < fParams.fSendMidiChannels; port_index++ )
            fMidiCapturePorts[port_index] = NULL;
        fMidiPlaybackPorts = new jack_port_t* [fParams.fReturnMidiChannels];
        for ( port_index = 0; port_index < fParams.fReturnMidiChannels; port_index++ )
            fMidiPlaybackPorts[port_index] = NULL;

        //monitor
#ifdef JACK_MONITOR
        fPeriodUsecs = ( int ) ( 1000000.f * ( ( float ) fParams.fPeriodSize / ( float ) fParams.fSampleRate ) );
        string plot_name;
        plot_name = string ( fParams.fName );
        plot_name += string ( "_master" );
        plot_name += string ( ( fParams.fSlaveSyncMode ) ? "_sync" : "_async" );
        switch ( fParams.fNetworkMode )
        {
            case 's' :
                plot_name += string ( "_slow" );
                break;
            case 'n' :
                plot_name += string ( "_normal" );
                break;
            case 'f' :
                plot_name += string ( "_fast" );
                break;
        }
        fNetTimeMon = new JackGnuPlotMonitor<float> ( 128, 4, plot_name );
        string net_time_mon_fields[] =
        {
            string ( "sync send" ),
            string ( "end of send" ),
            string ( "sync recv" ),
            string ( "end of cycle" )
        };
        string net_time_mon_options[] =
        {
            string ( "set xlabel \"audio cycles\"" ),
            string ( "set ylabel \"% of audio cycle\"" )
        };
        fNetTimeMon->SetPlotFile ( net_time_mon_options, 2, net_time_mon_fields, 4 );
#endif
    }

    JackNetMaster::~JackNetMaster()
    {
        jack_log ( "JackNetMaster::~JackNetMaster, ID %u.", fParams.fID );

        if ( fJackClient )
        {
            jack_deactivate ( fJackClient );
            FreePorts();
            jack_client_close ( fJackClient );
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
        if ( !JackNetMasterInterface::Init() )
            return false;

        //set global parameters
        SetParams();

        //jack client and process
        jack_status_t status;
        if ( ( fJackClient = jack_client_open ( fClientName, JackNullOption, &status, NULL ) ) == NULL )
        {
            jack_error ( "Can't open a new jack client." );
            return false;
        }

        if (jack_set_process_callback(fJackClient, SetProcess, this ) < 0)
             goto fail;

        if (jack_set_buffer_size_callback(fJackClient, SetBufferSize, this) < 0)
             goto fail;

        if ( AllocPorts() != 0 )
        {
            jack_error ( "Can't allocate jack ports." );
            goto fail;
        }

        //process can now run
        fRunning = true;

        //finally activate jack client
        if ( jack_activate ( fJackClient ) != 0 )
        {
            jack_error ( "Can't activate jack client." );
            goto fail;
        }

        if (auto_connect)
            ConnectPorts();
        jack_info ( "New NetMaster started." );
        return true;

    fail:
        FreePorts();
        jack_client_close ( fJackClient );
        fJackClient = NULL;
        return false;
    }

//jack ports--------------------------------------------------------------------------
    int JackNetMaster::AllocPorts()
    {
        uint i;
        char name[24];
        jack_nframes_t port_latency = jack_get_buffer_size ( fJackClient );
        jack_latency_range_t range;

        jack_log ( "JackNetMaster::AllocPorts" );

        //audio
        for ( i = 0; i < fParams.fSendAudioChannels; i++ )
        {
            sprintf ( name, "to_slave_%d", i+1 );
            if ( ( fAudioCapturePorts[i] = jack_port_register ( fJackClient, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput | JackPortIsTerminal, 0 ) ) == NULL )
                return -1;
            //port latency
            range.min = range.max = 0;
            jack_port_set_latency_range(fAudioCapturePorts[i], JackCaptureLatency, &range);
        }

        for ( i = 0; i < fParams.fReturnAudioChannels; i++ )
        {
            sprintf ( name, "from_slave_%d", i+1 );
            if ( ( fAudioPlaybackPorts[i] = jack_port_register ( fJackClient, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput | JackPortIsTerminal, 0 ) ) == NULL )
                return -1;
            //port latency
            switch ( fParams.fNetworkMode )
            {
                case 'f' :
                    range.min = range.max = (fParams.fSlaveSyncMode) ? 0 : port_latency;
                    jack_port_set_latency_range(fAudioPlaybackPorts[i], JackPlaybackLatency, &range);
                    break;
                case 'n' :
                    range.min = range.max = port_latency + (fParams.fSlaveSyncMode) ? 0 : port_latency;
                    jack_port_set_latency_range(fAudioPlaybackPorts[i], JackPlaybackLatency, &range);
                    break;
                case 's' :
                    range.min = range.max = 2 * port_latency + (fParams.fSlaveSyncMode) ? 0 : port_latency;
                    jack_port_set_latency_range(fAudioPlaybackPorts[i], JackPlaybackLatency, &range);
                    break;
            }
        }


        //midi
        for ( i = 0; i < fParams.fSendMidiChannels; i++ )
        {
            sprintf ( name, "midi_to_slave_%d", i+1 );
            if ( ( fMidiCapturePorts[i] = jack_port_register ( fJackClient, name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput | JackPortIsTerminal, 0 ) ) == NULL )
                return -1;
            //port latency
            range.min = range.max = 0;
            jack_port_set_latency_range(fMidiCapturePorts[i], JackCaptureLatency, &range);
        }
        for ( i = 0; i < fParams.fReturnMidiChannels; i++ )
        {
            sprintf ( name, "midi_from_slave_%d", i+1 );
            if ( ( fMidiPlaybackPorts[i] = jack_port_register ( fJackClient, name, JACK_DEFAULT_MIDI_TYPE,  JackPortIsOutput | JackPortIsTerminal, 0 ) ) == NULL )
                return -1;
            //port latency
            switch ( fParams.fNetworkMode )
            {
                case 'f' :
                    range.min = range.max = (fParams.fSlaveSyncMode) ? 0 : port_latency;
                    jack_port_set_latency_range(fMidiPlaybackPorts[i], JackPlaybackLatency, &range);
                    break;
                case 'n' :
                    range.min = range.max = port_latency + (fParams.fSlaveSyncMode) ? 0 : port_latency;
                    jack_port_set_latency_range(fMidiPlaybackPorts[i], JackPlaybackLatency, &range);
                    break;
                case 's' :
                    range.min = range.max = 2 * port_latency + (fParams.fSlaveSyncMode) ? 0 : port_latency;
                    jack_port_set_latency_range(fMidiPlaybackPorts[i], JackPlaybackLatency, &range);
                    break;
            }
        }
        return 0;
    }

    void JackNetMaster::ConnectPorts()
    {
        const char **ports;

        ports = jack_get_ports(fJackClient, NULL, NULL, JackPortIsPhysical | JackPortIsOutput);
        if (ports != NULL) {
            for (unsigned int i = 0; i < fParams.fSendAudioChannels && ports[i]; i++) {
                jack_connect(fJackClient, ports[i], jack_port_name(fAudioCapturePorts[i]));
            }
            free(ports);
        }

        ports = jack_get_ports(fJackClient, NULL, NULL, JackPortIsPhysical | JackPortIsInput);
        if (ports != NULL) {
            for (unsigned int i = 0; i < fParams.fReturnAudioChannels && ports[i]; i++) {
                jack_connect(fJackClient, jack_port_name(fAudioPlaybackPorts[i]), ports[i]);
            }
            free(ports);
        }
    }

    void JackNetMaster::FreePorts()
    {
        jack_log ( "JackNetMaster::FreePorts, ID %u", fParams.fID );

        uint port_index;
        for ( port_index = 0; port_index < fParams.fSendAudioChannels; port_index++ )
            if ( fAudioCapturePorts[port_index] )
                jack_port_unregister ( fJackClient, fAudioCapturePorts[port_index] );
        for ( port_index = 0; port_index < fParams.fReturnAudioChannels; port_index++ )
            if ( fAudioPlaybackPorts[port_index] )
                jack_port_unregister ( fJackClient, fAudioPlaybackPorts[port_index] );
        for ( port_index = 0; port_index < fParams.fSendMidiChannels; port_index++ )
            if ( fMidiCapturePorts[port_index] )
                jack_port_unregister ( fJackClient, fMidiCapturePorts[port_index] );
        for ( port_index = 0; port_index < fParams.fReturnMidiChannels; port_index++ )
            if ( fMidiPlaybackPorts[port_index] )
                jack_port_unregister ( fJackClient, fMidiPlaybackPorts[port_index] );
    }

//transport---------------------------------------------------------------------------
    void JackNetMaster::EncodeTransportData()
    {
        //is there a new timebase master ?
        //TODO : check if any timebase callback has been called (and if it's conditional or not) and set correct value...
        fSendTransportData.fTimebaseMaster = NO_CHANGE;

        //update state and position
        fSendTransportData.fState = static_cast<uint> ( jack_transport_query ( fJackClient, &fSendTransportData.fPosition ) );

        //is it a new state ?
        fSendTransportData.fNewState = ( ( fSendTransportData.fState != fLastTransportState ) &&
                                         ( fSendTransportData.fState != fReturnTransportData.fState ) );
        if ( fSendTransportData.fNewState )
            jack_info ( "Sending '%s' to '%s' frame = %ld", GetTransportState ( fSendTransportData.fState ), fParams.fName, fSendTransportData.fPosition.frame );
        fLastTransportState = fSendTransportData.fState;
   }

    void JackNetMaster::DecodeTransportData()
    {
        //is there timebase master change ?
        if ( fReturnTransportData.fTimebaseMaster != NO_CHANGE )
        {
            int timebase = 0;
            switch ( fReturnTransportData.fTimebaseMaster )
            {
                case RELEASE_TIMEBASEMASTER :
                    timebase = jack_release_timebase ( fJackClient );
                    if ( timebase < 0 )
                        jack_error ( "Can't release timebase master." );
                    else
                        jack_info ( "'%s' isn't the timebase master anymore.", fParams.fName );
                    break;

                case TIMEBASEMASTER :
                    timebase = jack_set_timebase_callback ( fJackClient, 0, SetTimebaseCallback, this );
                    if ( timebase < 0 )
                        jack_error ( "Can't set a new timebase master." );
                    else
                        jack_info ( "'%s' is the new timebase master.", fParams.fName );
                    break;

                case CONDITIONAL_TIMEBASEMASTER :
                    timebase = jack_set_timebase_callback ( fJackClient, 1, SetTimebaseCallback, this );
                    if ( timebase != EBUSY )
                    {
                        if ( timebase < 0 )
                            jack_error ( "Can't set a new timebase master." );
                        else
                            jack_info ( "'%s' is the new timebase master.", fParams.fName );
                    }
                    break;
            }
        }

        //is the slave in a new transport state and is this state different from master's ?
        if ( fReturnTransportData.fNewState && ( fReturnTransportData.fState != jack_transport_query ( fJackClient, NULL ) ) )
        {
            switch ( fReturnTransportData.fState )
            {
                case JackTransportStopped :
                    jack_transport_stop ( fJackClient );
                    jack_info ( "'%s' stops transport.", fParams.fName );
                    break;

                case JackTransportStarting :
                    if ( jack_transport_reposition ( fJackClient, &fReturnTransportData.fPosition ) == EINVAL )
                        jack_error ( "Can't set new position." );
                    jack_transport_start ( fJackClient );
                    jack_info ( "'%s' starts transport frame = %d", fParams.fName, fReturnTransportData.fPosition.frame);
                    break;

                case JackTransportNetStarting :
                    jack_info ( "'%s' is ready to roll..", fParams.fName );
                    break;

                case JackTransportRolling :
                    jack_info ( "'%s' is rolling.", fParams.fName );
                    break;
            }
        }
    }

    void JackNetMaster::SetTimebaseCallback ( jack_transport_state_t state, jack_nframes_t nframes, jack_position_t* pos, int new_pos, void* arg )
    {
        static_cast<JackNetMaster*> ( arg )->TimebaseCallback ( pos );
    }

    void JackNetMaster::TimebaseCallback ( jack_position_t* pos )
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
        return ( fReturnTransportData.fState == JackTransportNetStarting );
    }

    int JackNetMaster::SetBufferSize(jack_nframes_t nframes, void* arg)
    {
        JackNetMaster* obj = static_cast<JackNetMaster*>(arg);
        if (nframes != obj->fParams.fPeriodSize) {
            jack_error("Cannot handle bufer size change, so JackNetMaster proxy will be removed...");
            obj->Exit();
        }
        return 0;
    }

//process-----------------------------------------------------------------------------
    int JackNetMaster::SetProcess ( jack_nframes_t nframes, void* arg )
    {
        return static_cast<JackNetMaster*> ( arg )->Process();
    }

    int JackNetMaster::Process()
    {
        if ( !fRunning )
            return 0;

        uint port_index;
        int res = 0;

#ifdef JACK_MONITOR
        jack_time_t begin_time = GetMicroSeconds();
        fNetTimeMon->New();
#endif

        //buffers
        for ( port_index = 0; port_index < fParams.fSendMidiChannels; port_index++ )
            fNetMidiCaptureBuffer->SetBuffer ( port_index, static_cast<JackMidiBuffer*> ( jack_port_get_buffer ( fMidiCapturePorts[port_index],
                                               fParams.fPeriodSize ) ) );
        for ( port_index = 0; port_index < fParams.fSendAudioChannels; port_index++ )
            fNetAudioCaptureBuffer->SetBuffer ( port_index, static_cast<sample_t*> ( jack_port_get_buffer ( fAudioCapturePorts[port_index],
                                                fParams.fPeriodSize ) ) );
        for ( port_index = 0; port_index < fParams.fReturnMidiChannels; port_index++ )
            fNetMidiPlaybackBuffer->SetBuffer ( port_index, static_cast<JackMidiBuffer*> ( jack_port_get_buffer ( fMidiPlaybackPorts[port_index],
                                                fParams.fPeriodSize ) ) );
        for ( port_index = 0; port_index < fParams.fReturnAudioChannels; port_index++ )
            fNetAudioPlaybackBuffer->SetBuffer ( port_index, static_cast<sample_t*> ( jack_port_get_buffer ( fAudioPlaybackPorts[port_index],
                                                 fParams.fPeriodSize ) ) );

        if (IsSynched()) {  // only send if connection is "synched"

            //encode the first packet
            EncodeSyncPacket();

            //send sync
            if ( SyncSend() == SOCKET_ERROR )
                return SOCKET_ERROR;

    #ifdef JACK_MONITOR
            fNetTimeMon->Add ( ( ( ( float ) (GetMicroSeconds() - begin_time ) ) / ( float ) fPeriodUsecs ) * 100.f );
    #endif

            //send data
            if ( DataSend() == SOCKET_ERROR )
                return SOCKET_ERROR;

    #ifdef JACK_MONITOR
            fNetTimeMon->Add ( ( ( ( float ) (GetMicroSeconds() - begin_time ) ) / ( float ) fPeriodUsecs ) * 100.f );
    #endif

        } else {
            jack_error("Connection is not synched, skip cycle...");
        }

        //receive sync
        res = SyncRecv();
        if ( ( res == 0 ) || ( res == SOCKET_ERROR ) )
            return res;

#ifdef JACK_MONITOR
        fNetTimeMon->Add ( ( ( ( float ) (GetMicroSeconds() - begin_time ) ) / ( float ) fPeriodUsecs ) * 100.f );
#endif

        //decode sync
        DecodeSyncPacket();

        //receive data
        res = DataRecv();
        if ( ( res == 0 ) || ( res == SOCKET_ERROR ) )
            return res;

#ifdef JACK_MONITOR
        fNetTimeMon->AddLast ( ( ( ( float ) (GetMicroSeconds() - begin_time ) ) / ( float ) fPeriodUsecs ) * 100.f );
#endif
        return 0;
    }

//JackNetMasterManager***********************************************************************************************

    JackNetMasterManager::JackNetMasterManager ( jack_client_t* client, const JSList* params ) : fSocket()
    {
        jack_log ( "JackNetMasterManager::JackNetMasterManager" );

        fManagerClient = client;
        fManagerName = jack_get_client_name ( fManagerClient );
        strcpy(fMulticastIP, DEFAULT_MULTICAST_IP);
        fSocket.SetPort ( DEFAULT_PORT );
        fGlobalID = 0;
        fRunning = true;
        fAutoConnect = false;

        const JSList* node;
        const jack_driver_param_t* param;
        for ( node = params; node; node = jack_slist_next ( node ) )
        {
            param = ( const jack_driver_param_t* ) node->data;
            switch ( param->character )
            {
                case 'a' :
                    if (strlen (param->value.str) < 32)
                        strcpy(fMulticastIP, param->value.str);
                    else
                        jack_error("Can't use multicast address %s, using default %s", param->value.ui, DEFAULT_MULTICAST_IP);
                    break;

                case 'p':
                    fSocket.SetPort ( param->value.ui );
                    break;

                case 'c':
                    fAutoConnect = param->value.i;
                    break;
            }
        }

        //set sync callback
        jack_set_sync_callback ( fManagerClient, SetSyncCallback, this );

        //activate the client (for sync callback)
        if ( jack_activate ( fManagerClient ) != 0 )
            jack_error ( "Can't activate the network manager client, transport disabled." );

        //launch the manager thread
        if ( jack_client_create_thread ( fManagerClient, &fManagerThread, 0, 0, NetManagerThread, this ) )
            jack_error ( "Can't create the network manager control thread." );
    }

    JackNetMasterManager::~JackNetMasterManager()
    {
        jack_log ( "JackNetMasterManager::~JackNetMasterManager" );
        jack_info ( "Exiting net manager..." );
        fRunning = false;
        jack_client_kill_thread ( fManagerClient, fManagerThread );
        master_list_t::iterator it;
        for ( it = fMasterList.begin(); it != fMasterList.end(); it++ )
            delete ( *it );
        fSocket.Close();
        SocketAPIEnd();
    }

    int JackNetMasterManager::SetSyncCallback ( jack_transport_state_t state, jack_position_t* pos, void* arg )
    {
        return static_cast<JackNetMasterManager*> ( arg )->SyncCallback ( state, pos );
    }

    int JackNetMasterManager::SyncCallback ( jack_transport_state_t state, jack_position_t* pos )
    {
        //check if each slave is ready to roll
        int ret = 1;
        master_list_it_t it;
        for ( it = fMasterList.begin(); it != fMasterList.end(); it++ )
            if ( ! ( *it )->IsSlaveReadyToRoll() )
                ret = 0;
        jack_log ( "JackNetMasterManager::SyncCallback returns '%s'", ( ret ) ? "true" : "false" );
        return ret;
    }

    void* JackNetMasterManager::NetManagerThread ( void* arg )
    {
        JackNetMasterManager* master_manager = static_cast<JackNetMasterManager*> ( arg );
        jack_info ( "Starting Jack Network Manager." );
        jack_info ( "Listening on '%s:%d'", master_manager->fMulticastIP, master_manager->fSocket.GetPort() );
        master_manager->Run();
        return NULL;
    }

    void JackNetMasterManager::Run()
    {
        jack_log ( "JackNetMasterManager::Run" );
        //utility variables
        int attempt = 0;

        //data
        session_params_t host_params;
        int rx_bytes = 0;
        JackNetMaster* net_master;

        //init socket API (win32)
        if ( SocketAPIInit() < 0 )
        {
            jack_error ( "Can't init Socket API, exiting..." );
            return;
        }

        //socket
        if ( fSocket.NewSocket() == SOCKET_ERROR )
        {
            jack_error ( "Can't create the network management input socket : %s", StrError ( NET_ERROR_CODE ) );
            return;
        }

        //bind the socket to the local port
        if ( fSocket.Bind() == SOCKET_ERROR )
        {
            jack_error ( "Can't bind the network manager socket : %s", StrError ( NET_ERROR_CODE ) );
            fSocket.Close();
            return;
        }

        //join multicast group
        if ( fSocket.JoinMCastGroup ( fMulticastIP ) == SOCKET_ERROR )
            jack_error ( "Can't join multicast group : %s", StrError ( NET_ERROR_CODE ) );

        //local loop
        if ( fSocket.SetLocalLoop() == SOCKET_ERROR )
            jack_error ( "Can't set local loop : %s", StrError ( NET_ERROR_CODE ) );

        //set a timeout on the multicast receive (the thread can now be cancelled)
        if ( fSocket.SetTimeOut ( 2000000 ) == SOCKET_ERROR )
            jack_error ( "Can't set timeout : %s", StrError ( NET_ERROR_CODE ) );

        jack_info ( "Waiting for a slave..." );

        //main loop, wait for data, deal with it and wait again
        do
        {
            session_params_t net_params;
            rx_bytes = fSocket.CatchHost ( &net_params, sizeof ( session_params_t ), 0 );
            SessionParamsNToH(&net_params, &host_params);
            if ( ( rx_bytes == SOCKET_ERROR ) && ( fSocket.GetError() != NET_NO_DATA ) )
            {
                jack_error ( "Error in receive : %s", StrError ( NET_ERROR_CODE ) );
                if ( ++attempt == 10 )
                {
                    jack_error ( "Can't receive on the socket, exiting net manager." );
                    return;
                }
            }
            if ( rx_bytes == sizeof ( session_params_t ) )
            {
                switch ( GetPacketType ( &host_params ) )
                {
                    case SLAVE_AVAILABLE:
                        if ( ( net_master = InitMaster ( host_params ) ) )
                            SessionParamsDisplay ( &net_master->fParams );
                        else
                            jack_error ( "Can't init new net master..." );
                        jack_info ( "Waiting for a slave..." );
                        break;
                    case KILL_MASTER:
                        if ( KillMaster ( &host_params ) )
                            jack_info ( "Waiting for a slave..." );
                        break;
                    default:
                        break;
                }
            }
        }
        while ( fRunning );
    }

    JackNetMaster* JackNetMasterManager::InitMaster ( session_params_t& params )
    {
        jack_log ( "JackNetMasterManager::InitMaster, Slave : %s", params.fName );

        //check MASTER <<==> SLAVE network protocol coherency
        if (params.fProtocolVersion != MASTER_PROTOCOL) {
            jack_error ( "Error : slave is running with a different protocol %s", params.fName );
            return NULL;
        }

        //settings
        fSocket.GetName ( params.fMasterNetName );
        params.fID = ++fGlobalID;
        params.fSampleRate = jack_get_sample_rate ( fManagerClient );
        params.fPeriodSize = jack_get_buffer_size ( fManagerClient );
        params.fBitdepth = 0;
        SetSlaveName ( params );

        //create a new master and add it to the list
        JackNetMaster* master = new JackNetMaster(fSocket, params, fMulticastIP);
        if ( master->Init(fAutoConnect) )
        {
            fMasterList.push_back ( master );
            return master;
        }
        delete master;
        return NULL;
    }

    void JackNetMasterManager::SetSlaveName ( session_params_t& params )
    {
        jack_log ( "JackNetMasterManager::SetSlaveName" );

        master_list_it_t it;
        for ( it = fMasterList.begin(); it != fMasterList.end(); it++ )
            if ( strcmp ( ( *it )->fParams.fName, params.fName ) == 0 )
                sprintf ( params.fName, "%s-%u", params.fName, params.fID );
    }

    master_list_it_t JackNetMasterManager::FindMaster ( uint32_t id )
    {
        jack_log ( "JackNetMasterManager::FindMaster, ID %u.", id );

        master_list_it_t it;
        for ( it = fMasterList.begin(); it != fMasterList.end(); it++ )
            if ( ( *it )->fParams.fID == id )
                return it;
        return it;
    }

    int JackNetMasterManager::KillMaster ( session_params_t* params )
    {
        jack_log ( "JackNetMasterManager::KillMaster, ID %u.", params->fID );

        master_list_it_t master = FindMaster ( params->fID );
        if ( master != fMasterList.end() )
        {
            fMasterList.erase ( master );
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
        jack_driver_desc_t *desc;
        desc = ( jack_driver_desc_t* ) calloc ( 1, sizeof ( jack_driver_desc_t ) );

        strcpy ( desc->name, "netmanager" );                        // size MUST be less then JACK_DRIVER_NAME_MAX + 1
        strcpy ( desc->desc, "netjack multi-cast master component" );  // size MUST be less then JACK_DRIVER_PARAM_DESC + 1

        desc->nparams = 3;
        desc->params = ( jack_driver_param_desc_t* ) calloc ( desc->nparams, sizeof ( jack_driver_param_desc_t ) );

        int i = 0;
        strcpy ( desc->params[i].name, "multicast_ip" );
        desc->params[i].character = 'a';
        desc->params[i].type = JackDriverParamString;
        strcpy ( desc->params[i].value.str, DEFAULT_MULTICAST_IP );
        strcpy ( desc->params[i].short_desc, "Multicast Address" );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

        i++;
        strcpy ( desc->params[i].name, "udp_net_port" );
        desc->params[i].character = 'p';
        desc->params[i].type = JackDriverParamInt;
        desc->params[i].value.i = DEFAULT_PORT;
        strcpy ( desc->params[i].short_desc, "UDP port" );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

        i++;
        strcpy ( desc->params[i].name, "auto_connect" );
        desc->params[i].character = 'c';
        desc->params[i].type = JackDriverParamBool;
        desc->params[i].value.i = false;
        strcpy ( desc->params[i].short_desc, "Auto connect netmaster to system ports" );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

        return desc;
    }

    SERVER_EXPORT int jack_internal_initialize ( jack_client_t* jack_client, const JSList* params )
    {
        if ( master_manager )
        {
            jack_error ( "Master Manager already loaded" );
            return 1;
        }
        else
        {
            jack_log ( "Loading Master Manager" );
            master_manager = new Jack::JackNetMasterManager ( jack_client, params );
            return ( master_manager ) ? 0 : 1;
        }
    }

    SERVER_EXPORT int jack_initialize ( jack_client_t* jack_client, const char* load_init )
    {
        JSList* params = NULL;
        bool parse_params = true;
        int res = 1;
        jack_driver_desc_t* desc = jack_get_descriptor();

        Jack::JackArgParser parser ( load_init );
        if ( parser.GetArgc() > 0 )
            parse_params = parser.ParseParams ( desc, &params );

        if (parse_params) {
            res = jack_internal_initialize ( jack_client, params );
            parser.FreeParams ( params );
        }
        return res;
    }

    SERVER_EXPORT void jack_finish ( void* arg )
    {
        if ( master_manager )
        {
            jack_log ( "Unloading Master Manager" );
            delete master_manager;
            master_manager = NULL;
        }
    }
#ifdef __cplusplus
}
#endif
