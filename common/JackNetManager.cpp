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

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "JackNetManager.h"

#define DEFAULT_MULTICAST_IP "225.3.19.154"
#define DEFAULT_PORT 19000

using namespace std;

namespace Jack
{
//JackNetMaster******************************************************************************************************
#ifdef JACK_MONITOR
    uint JackNetMaster::fMeasureCnt = 128;
    uint JackNetMaster::fMeasurePoints = 4;
    string JackNetMaster::fMonitorFieldNames[] =
    {
        string ( "sync send" ),
        string ( "end of send" ),
        string ( "sync recv" ),
        string ( "end of cycle" )
    };
    uint JackNetMaster::fMonitorPlotOptionsCnt = 2;
    string JackNetMaster::fMonitorPlotOptions[] =
    {
        string ( "set xlabel \"audio cycles\"" ),
        string ( "set ylabel \"% of audio cycle\"" )
    };
#endif

    JackNetMaster::JackNetMaster ( JackNetMasterManager* manager, session_params_t& params ) : fSocket()
    {
        jack_log ( "JackNetMaster::JackNetMaster" );
        //settings
        fMasterManager = manager;
        fParams = params;
        fSocket.CopyParams ( &fMasterManager->fSocket );
        fNSubProcess = fParams.fPeriodSize / fParams.fFramesPerPacket;
        fClientName = const_cast<char*> ( fParams.fName );
        fNetJumpCnt = 0;
        fJackClient = NULL;
        fRunning = false;
        fSyncState = 1;
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

        //TX header init
        strcpy ( fTxHeader.fPacketType, "header" );
        fTxHeader.fDataStream = 's';
        fTxHeader.fID = fParams.fID;
        fTxHeader.fCycle = 0;
        fTxHeader.fSubCycle = 0;
        fTxHeader.fMidiDataSize = 0;
        fTxHeader.fBitdepth = fParams.fBitdepth;

        //RX header init
        strcpy ( fRxHeader.fPacketType, "header" );
        fRxHeader.fDataStream = 'r';
        fRxHeader.fID = fParams.fID;
        fRxHeader.fCycle = 0;
        fRxHeader.fSubCycle = 0;
        fRxHeader.fMidiDataSize = 0;
        fRxHeader.fBitdepth = fParams.fBitdepth;

        //network buffers
        fTxBuffer = new char [fParams.fMtu];
        fRxBuffer = new char [fParams.fMtu];

        //net audio buffers
        fTxData = fTxBuffer + sizeof ( packet_header_t );
        fRxData = fRxBuffer + sizeof ( packet_header_t );

        //midi net buffers
        fNetMidiCaptureBuffer = new NetMidiBuffer ( &fParams, fParams.fSendMidiChannels, fTxData );
        fNetMidiPlaybackBuffer = new NetMidiBuffer ( &fParams, fParams.fReturnMidiChannels, fRxData );

        //audio net buffers
        fNetAudioCaptureBuffer = new NetAudioBuffer ( &fParams, fParams.fSendAudioChannels, fTxData );
        fNetAudioPlaybackBuffer = new NetAudioBuffer ( &fParams, fParams.fReturnAudioChannels, fRxData );

        //audio netbuffer length
        fAudioTxLen = sizeof ( packet_header_t ) + fNetAudioCaptureBuffer->GetSize();
        fAudioRxLen = sizeof ( packet_header_t ) + fNetAudioPlaybackBuffer->GetSize();

        //payload size
        fPayloadSize = fParams.fMtu - sizeof ( packet_header_t );

        //monitor
#ifdef JACK_MONITOR
        fPeriodUsecs = ( int ) ( 1000000.f * ( ( float ) fParams.fPeriodSize / ( float ) fParams.fSampleRate ) );
        string plot_name = string ( fParams.fName );
        plot_name += string ( "_master" );
        plot_name += string ( ( fParams.fSlaveSyncMode ) ? "_sync" : "_async" );
        fMonitor = new JackGnuPlotMonitor<float> ( JackNetMaster::fMeasureCnt, JackNetMaster::fMeasurePoints, plot_name );
        fMeasure = new float[JackNetMaster::fMeasurePoints];
        fMonitor->SetPlotFile ( JackNetMaster::fMonitorPlotOptions, JackNetMaster::fMonitorPlotOptionsCnt,
                                JackNetMaster::fMonitorFieldNames, JackNetMaster::fMeasurePoints );
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
        fSocket.Close();
        delete fNetAudioCaptureBuffer;
        delete fNetAudioPlaybackBuffer;
        delete fNetMidiCaptureBuffer;
        delete fNetMidiPlaybackBuffer;
        delete[] fAudioCapturePorts;
        delete[] fAudioPlaybackPorts;
        delete[] fMidiCapturePorts;
        delete[] fMidiPlaybackPorts;
        delete[] fTxBuffer;
        delete[] fRxBuffer;
#ifdef JACK_MONITOR
        fMonitor->Save();
        delete[] fMeasure;
        delete fMonitor;
#endif
    }

    bool JackNetMaster::Init()
    {
        jack_log ( "JackNetMaster::Init, ID %u.", fParams.fID );
        session_params_t params;
        int msec_timeout = 1000;
        uint attempt = 0;
        int rx_bytes = 0;

        //socket
        if ( fSocket.NewSocket() == SOCKET_ERROR )
        {
            jack_error ( "Can't create socket : %s", StrError ( NET_ERROR_CODE ) );
            return false;
        }

        //timeout on receive (for init)
        if ( fSocket.SetTimeOut ( msec_timeout ) < 0 )
            jack_error ( "Can't set timeout : %s", StrError ( NET_ERROR_CODE ) );

        //connect
        if ( fSocket.Connect() == SOCKET_ERROR )
        {
            jack_error ( "Can't connect : %s", StrError ( NET_ERROR_CODE ) );
            return false;
        }

        //send 'SLAVE_SETUP' until 'START_MASTER' received
        jack_info ( "Sending parameters to %s ...", fParams.fSlaveNetName );
        do
        {
            SetPacketType ( &fParams, SLAVE_SETUP );
            if ( fSocket.Send ( &fParams, sizeof ( session_params_t ), 0 ) == SOCKET_ERROR )
                jack_error ( "Error in send : ", StrError ( NET_ERROR_CODE ) );
            if ( ( ( rx_bytes = fSocket.Recv ( &params, sizeof ( session_params_t ), 0 ) ) == SOCKET_ERROR ) && ( fSocket.GetError() != NET_NO_DATA ) )
            {
                jack_error ( "Problem with network." );
                return false;
            }
        }
        while ( ( GetPacketType ( &params ) != START_MASTER ) && ( ++attempt < 5 ) );
        if ( attempt == 5 )
        {
            jack_error ( "Slave doesn't respond, exiting." );
            return false;
        }

        //set the new timeout for the socket
        if ( SetRxTimeout ( &fSocket, &fParams ) == SOCKET_ERROR )
        {
            jack_error ( "Can't set rx timeout : %s", StrError ( NET_ERROR_CODE ) );
            return false;
        }

        //jack client and process
        jack_status_t status;
        jack_options_t options = JackNullOption;
        if ( ( fJackClient = jack_client_open ( fClientName, options, &status, NULL ) ) == NULL )
        {
            jack_error ( "Can't open a new jack client." );
            return false;
        }

        jack_set_process_callback ( fJackClient, SetProcess, this );

        //port registering
        uint i;
        char name[24];
        jack_nframes_t port_latency = jack_get_buffer_size ( fJackClient );
        unsigned long port_flags;
        //audio
        port_flags = JackPortIsInput | JackPortIsPhysical | JackPortIsTerminal;
        for ( i = 0; i < fParams.fSendAudioChannels; i++ )
        {
            sprintf ( name, "to_slave_%d", i+1 );
            if ( ( fAudioCapturePorts[i] = jack_port_register ( fJackClient, name, JACK_DEFAULT_AUDIO_TYPE, port_flags, 0 ) ) == NULL )
                goto fail;
            jack_port_set_latency ( fAudioCapturePorts[i], 0 );
        }
        port_flags = JackPortIsOutput | JackPortIsPhysical | JackPortIsTerminal;
        for ( i = 0; i < fParams.fReturnAudioChannels; i++ )
        {
            sprintf ( name, "from_slave_%d", i+1 );
            if ( ( fAudioPlaybackPorts[i] = jack_port_register ( fJackClient, name, JACK_DEFAULT_AUDIO_TYPE, port_flags, 0 ) ) == NULL )
                goto fail;
            jack_port_set_latency ( fAudioPlaybackPorts[i], port_latency + ( fParams.fSlaveSyncMode ) ? 0 : port_latency );
        }
        //midi
        port_flags = JackPortIsInput | JackPortIsPhysical | JackPortIsTerminal;
        for ( i = 0; i < fParams.fSendMidiChannels; i++ )
        {
            sprintf ( name, "midi_to_slave_%d", i+1 );
            if ( ( fMidiCapturePorts[i] = jack_port_register ( fJackClient, name, JACK_DEFAULT_MIDI_TYPE, port_flags, 0 ) ) == NULL )
                goto fail;
            jack_port_set_latency ( fMidiCapturePorts[i], 0 );
        }
        port_flags = JackPortIsOutput | JackPortIsPhysical | JackPortIsTerminal;
        for ( i = 0; i < fParams.fReturnMidiChannels; i++ )
        {
            sprintf ( name, "midi_from_slave_%d", i+1 );
            if ( ( fMidiPlaybackPorts[i] = jack_port_register ( fJackClient, name, JACK_DEFAULT_MIDI_TYPE, port_flags, 0 ) ) == NULL )
                goto fail;
            jack_port_set_latency ( fMidiPlaybackPorts[i], port_latency + ( fParams.fSlaveSyncMode ) ? 0 : port_latency );
        }

        fRunning = true;

        //finally activate jack client
        if ( jack_activate ( fJackClient ) != 0 )
        {
            jack_error ( "Can't activate jack client." );
            goto fail;
        }

        jack_info ( "NetJack new master started." );

        return true;

    fail:
        FreePorts();
        jack_client_close ( fJackClient );
        fJackClient = NULL;
        return false;
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

    void JackNetMaster::Exit()
    {
        jack_log ( "JackNetMaster::Exit, ID %u", fParams.fID );
        //stop process
        fRunning = false;
        //send a 'multicast euthanasia request' - new socket is required on macosx
        jack_info ( "Exiting '%s'", fParams.fName );
        SetPacketType ( &fParams, KILL_MASTER );
        JackNetSocket mcast_socket ( fMasterManager->fMulticastIP, fSocket.GetPort() );
        if ( mcast_socket.NewSocket() == SOCKET_ERROR )
            jack_error ( "Can't create socket : %s", StrError ( NET_ERROR_CODE ) );
        if ( mcast_socket.SendTo ( &fParams, sizeof ( session_params_t ), 0, fMasterManager->fMulticastIP ) == SOCKET_ERROR )
            jack_error ( "Can't send suicide request : %s", StrError ( NET_ERROR_CODE ) );
        mcast_socket.Close();
    }

    int JackNetMaster::SetSyncPacket()
    {
        if ( fParams.fTransportSync )
        {
            //set the TransportData

            //copy to TxBuffer
            memcpy ( fTxData, &fTransportData, sizeof ( net_transport_data_t ) );
        }
        return 0;
    }

    int JackNetMaster::Send ( char* buffer, size_t size, int flags )
    {
        int tx_bytes;
        if ( ( tx_bytes = fSocket.Send ( buffer, size, flags ) ) == SOCKET_ERROR )
        {
            net_error_t error = fSocket.GetError();
            if ( error == NET_CONN_ERROR )
            {
                //fatal connection issue, exit
                jack_error ( "'%s' : %s, please check network connection with '%s'.",
                             fParams.fName, StrError ( NET_ERROR_CODE ), fParams.fSlaveNetName );
                Exit();
            }
            else
                jack_error ( "Error in send : %s", StrError ( NET_ERROR_CODE ) );
        }
        return tx_bytes;
    }

    int JackNetMaster::Recv ( size_t size, int flags )
    {
        int rx_bytes;
        if ( ( rx_bytes = fSocket.Recv ( fRxBuffer, size, flags ) ) == SOCKET_ERROR )
        {
            net_error_t error = fSocket.GetError();
            if ( error == NET_NO_DATA )
            {
                //too much receive failure, react
                if ( ++fNetJumpCnt < 100 )
                    return 0;
                else
                {
                    jack_error ( "No data from %s...", fParams.fName );
                    fNetJumpCnt = 0;
                }
            }
            else if ( error == NET_CONN_ERROR )
            {
                //fatal connection issue, exit
                jack_error ( "'%s' : %s, network connection with '%s' broken, exiting.",
                             fParams.fName, StrError ( NET_ERROR_CODE ), fParams.fSlaveNetName );
                //ask to the manager to properly remove the master
                Exit();
            }
            else
                jack_error ( "Error in receive : %s", StrError ( NET_ERROR_CODE ) );
        }
        return rx_bytes;
    }

    int JackNetMaster::SetProcess ( jack_nframes_t nframes, void* arg )
    {
        JackNetMaster* master = static_cast<JackNetMaster*> ( arg );
        return master->Process();
    }

    int JackNetMaster::Process()
    {
        if ( !fRunning )
            return 0;

        int tx_bytes, rx_bytes, copy_size;
        size_t midi_recvd_pckt = 0;
        fTxHeader.fCycle++;
        fTxHeader.fSubCycle = 0;
        fTxHeader.fIsLastPckt = 'n';
        packet_header_t* rx_head = reinterpret_cast<packet_header_t*> ( fRxBuffer );

#ifdef JACK_MONITOR
        jack_time_t begin_time = jack_get_time();
        fMeasureId = 0;
#endif

        //buffers
        uint port_index;
        for ( port_index = 0; port_index < fParams.fSendMidiChannels; port_index++ )
            fNetMidiCaptureBuffer->SetBuffer ( port_index,
                                               static_cast<JackMidiBuffer*> ( jack_port_get_buffer ( fMidiCapturePorts[port_index],
                                                                              fParams.fPeriodSize ) ) );
        for ( port_index = 0; port_index < fParams.fSendAudioChannels; port_index++ )
            fNetAudioCaptureBuffer->SetBuffer ( port_index,
                                                static_cast<sample_t*> ( jack_port_get_buffer ( fAudioCapturePorts[port_index],
                                                                         fParams.fPeriodSize ) ) );
        for ( port_index = 0; port_index < fParams.fReturnMidiChannels; port_index++ )
            fNetMidiPlaybackBuffer->SetBuffer ( port_index,
                                                static_cast<JackMidiBuffer*> ( jack_port_get_buffer ( fMidiPlaybackPorts[port_index],
                                                                               fParams.fPeriodSize ) ) );
        for ( port_index = 0; port_index < fParams.fReturnAudioChannels; port_index++ )
            fNetAudioPlaybackBuffer->SetBuffer ( port_index,
                                                 static_cast<sample_t*> ( jack_port_get_buffer ( fAudioPlaybackPorts[port_index],
                                                                          fParams.fPeriodSize ) ) );

        //send ------------------------------------------------------------------------------------------------------------------
        //sync
        fTxHeader.fDataType = 's';
        if ( !fParams.fSendMidiChannels && !fParams.fSendAudioChannels )
            fTxHeader.fIsLastPckt = 'y';
        memset ( fTxData, 0, fPayloadSize );
        SetSyncPacket();
        tx_bytes = Send ( fTxBuffer, fParams.fMtu, 0 );
        if ( tx_bytes == SOCKET_ERROR )
            return tx_bytes;

#ifdef JACK_MONITOR
        fMeasure[fMeasureId++] = ( ( ( float ) ( jack_get_time() - begin_time ) ) / ( float ) fPeriodUsecs ) * 100.f;
#endif

        //midi
        if ( fParams.fSendMidiChannels )
        {
            fTxHeader.fDataType = 'm';
            fTxHeader.fMidiDataSize = fNetMidiCaptureBuffer->RenderFromJackPorts();
            fTxHeader.fNMidiPckt = GetNMidiPckt ( &fParams, fTxHeader.fMidiDataSize );
            for ( uint subproc = 0; subproc < fTxHeader.fNMidiPckt; subproc++ )
            {
                fTxHeader.fSubCycle = subproc;
                if ( ( subproc == ( fTxHeader.fNMidiPckt - 1 ) ) && !fParams.fSendAudioChannels )
                    fTxHeader.fIsLastPckt = 'y';
                memcpy ( fTxBuffer, &fTxHeader, sizeof ( packet_header_t ) );
                copy_size = fNetMidiCaptureBuffer->RenderToNetwork ( subproc, fTxHeader.fMidiDataSize );
                tx_bytes = Send ( fTxBuffer, sizeof ( packet_header_t ) + copy_size, 0 );
                if ( tx_bytes == SOCKET_ERROR )
                    return tx_bytes;
            }
        }

        //audio
        if ( fParams.fSendAudioChannels )
        {
            fTxHeader.fDataType = 'a';
            for ( uint subproc = 0; subproc < fNSubProcess; subproc++ )
            {
                fTxHeader.fSubCycle = subproc;
                if ( subproc == ( fNSubProcess - 1 ) )
                    fTxHeader.fIsLastPckt = 'y';
                memcpy ( fTxBuffer, &fTxHeader, sizeof ( packet_header_t ) );
                fNetAudioCaptureBuffer->RenderFromJackPorts ( subproc );
                tx_bytes = Send ( fTxBuffer, fAudioTxLen, 0 );
                if ( tx_bytes == SOCKET_ERROR )
                    return tx_bytes;
            }
        }

#ifdef JACK_MONITOR
        fMeasure[fMeasureId++] = ( ( ( float ) ( jack_get_time() - begin_time ) ) / ( float ) fPeriodUsecs ) * 100.f;
#endif

        //receive --------------------------------------------------------------------------------------------------------------------
        //sync
        do
        {
            rx_bytes = Recv ( fParams.fMtu, 0 );
            if ( rx_bytes == SOCKET_ERROR )
                return rx_bytes;
        }
        while ( !rx_bytes && ( rx_head->fDataType != 's' ) );

#ifdef JACK_MONITOR
        fMeasure[fMeasureId++] = ( ( ( float ) ( jack_get_time() - begin_time ) ) / ( float ) fPeriodUsecs ) * 100.f;
#endif

        if ( fParams.fReturnMidiChannels || fParams.fReturnAudioChannels )
        {
            do
            {
                if ( ( rx_bytes = Recv ( fParams.fMtu, MSG_PEEK ) ) == SOCKET_ERROR )
                    return rx_bytes;
                if ( rx_bytes && ( rx_head->fDataStream == 'r' ) && ( rx_head->fID == fParams.fID ) )
                {
                    switch ( rx_head->fDataType )
                    {
                        case 'm':   //midi
                            rx_bytes = Recv ( rx_bytes, 0 );
                            fRxHeader.fCycle = rx_head->fCycle;
                            fRxHeader.fIsLastPckt = rx_head->fIsLastPckt;
                            fNetMidiPlaybackBuffer->RenderFromNetwork ( rx_head->fSubCycle, rx_bytes - sizeof ( packet_header_t ) );
                            if ( ++midi_recvd_pckt == rx_head->fNMidiPckt )
                                fNetMidiPlaybackBuffer->RenderToJackPorts();
                            fNetJumpCnt = 0;
                            break;
                        case 'a':   //audio
                            rx_bytes = Recv ( fAudioRxLen, 0 );
                            if ( !IsNextPacket ( &fRxHeader, rx_head, fNSubProcess ) )
                                jack_error ( "Packet(s) missing from '%s'...", fParams.fName );
                            fRxHeader.fCycle = rx_head->fCycle;
                            fRxHeader.fSubCycle = rx_head->fSubCycle;
                            fRxHeader.fIsLastPckt = rx_head->fIsLastPckt;
                            fNetAudioPlaybackBuffer->RenderToJackPorts ( rx_head->fSubCycle );
                            fNetJumpCnt = 0;
                            break;
                        case 's':   //sync
                            rx_bytes = Recv ( rx_bytes, 0 );
                            jack_error ( "NetMaster receive sync packets instead of data." );
                            return 0;
                    }
                }
            }
            while ( fRxHeader.fIsLastPckt != 'y' );
        }

#ifdef JACK_MONITOR
        fMeasure[fMeasureId++] = ( ( ( float ) ( jack_get_time() - begin_time ) ) / ( float ) fPeriodUsecs ) * 100.f;
        fMonitor->Write ( fMeasure );
        if ( fTxHeader.fCycle - fRxHeader.fCycle )
            jack_log ( "NetMonitor::%s %d", ( fParams.fSlaveSyncMode ) ? "SyncCycleOffset" : "AsyncCycleOffset", fTxHeader.fCycle - fRxHeader.fCycle );
#endif
        return 0;
    }

//JackNetMasterManager***********************************************************************************************

    JackNetMasterManager::JackNetMasterManager ( jack_client_t* client, const JSList* params ) : fSocket()
    {
        jack_log ( "JackNetMasterManager::JackNetMasterManager" );
        fManagerClient = client;
        fManagerName = jack_get_client_name ( fManagerClient );
        fMulticastIP = DEFAULT_MULTICAST_IP;
        fSocket.SetPort ( DEFAULT_PORT );
        fGlobalID = 0;
        fRunning = true;

        const JSList* node;
        const jack_driver_param_t* param;
        for ( node = params; node; node = jack_slist_next ( node ) )
        {
            param = ( const jack_driver_param_t* ) node->data;
            switch ( param->character )
            {
                case 'a' :
                    fMulticastIP = strdup ( param->value.str );
                    break;
                case 'p':
                    fSocket.SetPort ( param->value.ui );
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
        Exit();
        master_list_t::iterator it;
        for ( it = fMasterList.begin(); it != fMasterList.end(); it++ )
            delete ( *it );
        fSocket.Close();
        SocketAPIEnd();
    }

    int JackNetMasterManager::SetSyncCallback ( jack_transport_state_t state, jack_position_t* pos, void* arg )
    {
        JackNetMasterManager* master_manager = static_cast<JackNetMasterManager*> ( arg );
        return master_manager->SyncCallback ( state, pos );
    }

    int JackNetMasterManager::SyncCallback ( jack_transport_state_t state, jack_position_t* pos )
    {
        //check sync state for every master in the list
        int ret = 1;
        master_list_it_t it;
        for ( it = fMasterList.begin(); it != fMasterList.end(); it++ )
            if ( ( *it )->fSyncState == 0 )
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
        int msec_timeout = 2000;
        int attempt = 0;

        //data
        session_params_t params;
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
        if ( fSocket.Bind () == SOCKET_ERROR )
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
        if ( fSocket.SetTimeOut ( msec_timeout ) == SOCKET_ERROR )
            jack_error ( "Can't set timeout : %s", StrError ( NET_ERROR_CODE ) );

        jack_info ( "Waiting for a slave..." );

        //main loop, wait for data, deal with it and wait again
        do
        {
            rx_bytes = fSocket.CatchHost ( &params, sizeof ( session_params_t ), 0 );
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
                switch ( GetPacketType ( &params ) )
                {
                    case SLAVE_AVAILABLE:
                        if ( ( net_master = MasterInit ( params ) ) )
                            SessionParamsDisplay ( &net_master->fParams );
                        else
                            jack_error ( "Can't init new net master..." );
                        jack_info ( "Waiting for a slave..." );
                        break;
                    case KILL_MASTER:
                        if ( KillMaster ( &params ) )
                            jack_info ( "Waiting for a slave..." );
                        break;
                    default:
                        break;
                }
            }
        }
        while ( fRunning );
    }

    void JackNetMasterManager::Exit()
    {
        jack_log ( "JackNetMasterManager::Exit" );
        fRunning = false;
        jack_client_stop_thread ( fManagerClient, fManagerThread );
        jack_info ( "Exiting net manager..." );
    }

    JackNetMaster* JackNetMasterManager::MasterInit ( session_params_t& params )
    {
        jack_log ( "JackNetMasterManager::MasterInit, Slave : %s", params.fName );
        //settings
        fSocket.GetName ( params.fMasterNetName );
        params.fID = ++fGlobalID;
        params.fSampleRate = jack_get_sample_rate ( fManagerClient );
        params.fPeriodSize = jack_get_buffer_size ( fManagerClient );
        params.fBitdepth = 0;
        SetFramesPerPacket ( &params );
        SetSlaveName ( params );

        //create a new master and add it to the list
        JackNetMaster* master = new JackNetMaster ( this, params );
        if ( master->Init() )
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

    EXPORT jack_driver_desc_t* jack_get_descriptor()
    {
        jack_driver_desc_t *desc;
        desc = ( jack_driver_desc_t* ) calloc ( 1, sizeof ( jack_driver_desc_t ) );

        strcpy ( desc->name, "netmanager" );
        desc->nparams = 2;
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

        return desc;
    }

    EXPORT int jack_internal_initialize ( jack_client_t* jack_client, const JSList* params )
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

    EXPORT int jack_initialize ( jack_client_t* jack_client, const char* load_init )
    {
        JSList* params = NULL;
        jack_driver_desc_t* desc = jack_get_descriptor();
        Jack::JackArgParser parser ( load_init );

        if ( parser.GetArgc() > 0 )
        {
            if ( parser.ParseParams ( desc, &params ) < 0 )
                jack_error ( "Internal client JackArgParser::ParseParams error." );
        }

        return jack_internal_initialize ( jack_client, params );
    }

    EXPORT void jack_finish ( void* arg )
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
