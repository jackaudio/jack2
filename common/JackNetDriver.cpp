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

#include "JackNetDriver.h"
#include "JackEngineControl.h"
#include "JackClientControl.h"
#include "JackGraphManager.h"
#include "JackDriverLoader.h"
#include "JackThreadedDriver.h"
#include "JackWaitThreadedDriver.h"
#include "JackException.h"
#include "JackExports.h"

#define DEFAULT_MULTICAST_IP "225.3.19.154"
#define DEFAULT_PORT 19000

namespace Jack
{
    JackNetDriver::JackNetDriver ( const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table,
                                   const char* ip, int port, int mtu, int midi_input_ports, int midi_output_ports, const char* net_name )
            : JackAudioDriver ( name, alias, engine, table ), fSocket ( ip, port )
    {
        fMulticastIP = new char[strlen ( ip ) + 1];
        strcpy ( fMulticastIP, ip );
        fParams.fMtu = mtu;
        fParams.fSendMidiChannels = midi_input_ports;
        fParams.fReturnMidiChannels = midi_output_ports;
        strcpy ( fParams.fName, net_name );
        fSocket.GetName ( fParams.fSlaveNetName );
    }

    JackNetDriver::~JackNetDriver()
    {
        fSocket.Close();
        SocketAPIEnd();
        delete fNetAudioCaptureBuffer;
        delete fNetAudioPlaybackBuffer;
        delete fNetMidiCaptureBuffer;
        delete fNetMidiPlaybackBuffer;
        delete[] fTxBuffer;
        delete[] fRxBuffer;
        delete[] fMulticastIP;
        delete[] fMidiCapturePortList;
        delete[] fMidiPlaybackPortList;
    }

//*************************************initialization***********************************************************************

    int JackNetDriver::Open ( jack_nframes_t buffer_size, jack_nframes_t samplerate, bool capturing, bool playing,
                              int inchannels, int outchannels, bool monitor,
                              const char* capture_driver_name, const char* playback_driver_name,
                              jack_nframes_t capture_latency, jack_nframes_t playback_latency )
    {
        int res = JackAudioDriver::Open ( buffer_size, samplerate, capturing, playing, inchannels, outchannels, monitor,
                                          capture_driver_name, playback_driver_name, capture_latency, playback_latency );
        fEngineControl->fPeriod = 0;
        fEngineControl->fComputation = 500 * 1000;
        fEngineControl->fConstraint = 500 * 1000;
        return res;
    }

    int JackNetDriver::Attach()
    {
        return 0;
    }

    int JackNetDriver::Detach()
    {
        return 0;
    }

    bool JackNetDriver::Init()
    {
        jack_log ( "JackNetDriver::Init()" );

        //new loading, but existing socket, restart the driver
        if ( fSocket.IsSocket() )
            Restart();

        //set the parameters to send
        strcpy ( fParams.fPacketType, "params" );
        fParams.fProtocolVersion = 'a';
        SetPacketType ( &fParams, SLAVE_AVAILABLE );
        fParams.fSendAudioChannels = fCaptureChannels;
        fParams.fReturnAudioChannels = fPlaybackChannels;

        //init loop : get a master and start, do it until connection is ok
        net_status_t status;
        do
        {
            //first, get a master, do it until a valid connection is running
            jack_info ( "Initializing Net Driver..." );
            do
            {
                status = GetNetMaster();
                if ( status == NET_SOCKET_ERROR )
                    return false;
            }
            while ( status != NET_CONNECTED );

            //then tell the master we are ready
            jack_info ( "Initializing connection with %s...", fParams.fMasterNetName );
            status = SendMasterStartSync();
            if ( status == NET_ERROR )
                return false;
        }
        while ( status != NET_ROLLING );

        //driver parametering
        if ( SetParams() )
        {
            jack_error ( "Fatal error : can't alloc net driver ports." );
            return false;
        }

        //init done, display parameters
        SessionParamsDisplay ( &fParams );

        return true;
    }

    net_status_t JackNetDriver::GetNetMaster()
    {
        jack_log ( "JackNetDriver::GetNetMaster()" );
        //utility
        session_params_t params;
        int ms_timeout = 2000;
        int rx_bytes = 0;

        //socket
        if ( fSocket.NewSocket() == SOCKET_ERROR )
        {
            jack_error ( "Fatal error : network unreachable - %s", StrError ( NET_ERROR_CODE ) );
            return NET_SOCKET_ERROR;
        }

        //bind the socket
        if ( fSocket.Bind() == SOCKET_ERROR )
            jack_error ( "Can't bind the socket : %s", StrError ( NET_ERROR_CODE ) );

        //timeout on receive
        if ( fSocket.SetTimeOut ( ms_timeout ) == SOCKET_ERROR )
            jack_error ( "Can't set timeout : %s", StrError ( NET_ERROR_CODE ) );

        //send 'AVAILABLE' until 'SLAVE_SETUP' received
        jack_info ( "Waiting for a master..." );
        do
        {
            //send 'available'
            if ( fSocket.SendTo ( &fParams, sizeof ( session_params_t ), 0, fMulticastIP ) == SOCKET_ERROR )
                jack_error ( "Error in data send : %s", StrError ( NET_ERROR_CODE ) );
            //filter incoming packets : don't exit while receiving wrong packets
            do
            {
                rx_bytes = fSocket.CatchHost ( &params, sizeof ( session_params_t ), 0 );
                if ( ( rx_bytes == SOCKET_ERROR ) && ( fSocket.GetError() != NET_NO_DATA ) )
                {
                    jack_error ( "Can't receive : %s", StrError ( NET_ERROR_CODE ) );
                    return NET_RECV_ERROR;
                }
            }
            while ( ( rx_bytes > 0 )  && strcmp ( params.fPacketType, fParams.fPacketType ) );
        }
        while ( ( GetPacketType ( &params ) != SLAVE_SETUP ) );

        //connect the socket
        if ( fSocket.Connect() == SOCKET_ERROR )
        {
            jack_error ( "Error in connect : %s", StrError ( NET_ERROR_CODE ) );
            return NET_CONNECT_ERROR;
        }

        //everything is OK, copy parameters and return
        fParams = params;

        return NET_CONNECTED;
    }

    net_status_t JackNetDriver::SendMasterStartSync()
    {
        jack_log ( "JackNetDriver::GetNetMasterStartSync()" );
        //tell the master to start
        SetPacketType ( &fParams, START_MASTER );
        if ( fSocket.Send ( &fParams, sizeof ( session_params_t ), 0 ) == SOCKET_ERROR )
        {
            jack_error ( "Error in send : %s", StrError ( NET_ERROR_CODE ) );
            return ( fSocket.GetError() == NET_CONN_ERROR ) ? NET_ERROR : NET_SEND_ERROR;
        }
        return NET_ROLLING;
    }

    void JackNetDriver::Restart()
    {
        jack_info ( "Restarting driver..." );
        delete[] fTxBuffer;
        delete[] fRxBuffer;
        delete fNetAudioCaptureBuffer;
        delete fNetAudioPlaybackBuffer;
        delete fNetMidiCaptureBuffer;
        delete fNetMidiPlaybackBuffer;
        FreePorts();
        delete[] fMidiCapturePortList;
        delete[] fMidiPlaybackPortList;
        fTxBuffer = NULL;
        fRxBuffer = NULL;
        fNetAudioCaptureBuffer = NULL;
        fNetAudioPlaybackBuffer = NULL;
        fNetMidiCaptureBuffer = NULL;
        fNetMidiPlaybackBuffer = NULL;
        fMidiCapturePortList = NULL;
        fMidiPlaybackPortList = NULL;
    }

    int JackNetDriver::SetParams()
    {
        fNSubProcess = fParams.fPeriodSize / fParams.fFramesPerPacket;
        JackAudioDriver::SetBufferSize(fParams.fPeriodSize);
        JackAudioDriver::SetSampleRate(fParams.fSampleRate);

        JackDriver::NotifyBufferSize(fParams.fPeriodSize);
        JackDriver::NotifySampleRate(fParams.fSampleRate);

        //allocate midi ports lists
        fMidiCapturePortList = new jack_port_id_t [fParams.fSendMidiChannels];
        fMidiPlaybackPortList = new jack_port_id_t [fParams.fReturnMidiChannels];

        //register jack ports
        if ( AllocPorts() != 0 )
        {
            jack_error ( "Can't allocate ports." );
            return -1;
        }

        //TX header init
        strcpy ( fTxHeader.fPacketType, "header" );
        fTxHeader.fDataStream = 'r';
        fTxHeader.fID = fParams.fID;
        fTxHeader.fCycle = 0;
        fTxHeader.fSubCycle = 0;
        fTxHeader.fMidiDataSize = 0;
        fTxHeader.fBitdepth = fParams.fBitdepth;

        //RX header init
        strcpy ( fRxHeader.fPacketType, "header" );
        fRxHeader.fDataStream = 's';
        fRxHeader.fID = fParams.fID;
        fRxHeader.fCycle = 0;
        fRxHeader.fSubCycle = 0;
        fRxHeader.fMidiDataSize = 0;
        fRxHeader.fBitdepth = fParams.fBitdepth;

        //network buffers
        fTxBuffer = new char[fParams.fMtu];
        fRxBuffer = new char[fParams.fMtu];

        //net audio/midi buffers
        fTxData = fTxBuffer + sizeof ( packet_header_t );
        fRxData = fRxBuffer + sizeof ( packet_header_t );

        //midi net buffers
        fNetMidiCaptureBuffer = new NetMidiBuffer ( &fParams, fParams.fSendMidiChannels, fRxData );
        fNetMidiPlaybackBuffer = new NetMidiBuffer ( &fParams, fParams.fReturnMidiChannels, fTxData );

        //audio net buffers
        fNetAudioCaptureBuffer = new NetAudioBuffer ( &fParams, fParams.fSendAudioChannels, fRxData );
        fNetAudioPlaybackBuffer = new NetAudioBuffer ( &fParams, fParams.fReturnAudioChannels, fTxData );

        //audio netbuffer length
        fAudioTxLen = sizeof ( packet_header_t ) + fNetAudioPlaybackBuffer->GetSize();
        fAudioRxLen = sizeof ( packet_header_t ) + fNetAudioCaptureBuffer->GetSize();

        return 0;
    }

    int JackNetDriver::AllocPorts()
    {
        jack_log ( "JackNetDriver::AllocPorts fBufferSize = %ld fSampleRate = %ld", fEngineControl->fBufferSize, fEngineControl->fSampleRate );
        JackPort* port;
        jack_port_id_t port_id;
        char name[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
        char alias[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
        unsigned long port_flags;
        int audio_port_index;
        uint midi_port_index;

        //audio
        port_flags = JackPortIsOutput | JackPortIsPhysical | JackPortIsTerminal;
        for ( audio_port_index = 0; audio_port_index < fCaptureChannels; audio_port_index++ )
        {
            snprintf ( alias, sizeof ( alias ) - 1, "%s:%s:out%d", fAliasName, fCaptureDriverName, audio_port_index + 1 );
            snprintf ( name, sizeof ( name ) - 1, "%s:capture_%d", fClientControl.fName, audio_port_index + 1 );
            if ( ( port_id = fGraphManager->AllocatePort ( fClientControl.fRefNum, name, JACK_DEFAULT_AUDIO_TYPE,
                             static_cast<JackPortFlags> ( port_flags ), fEngineControl->fBufferSize ) ) == NO_PORT )
            {
                jack_error ( "driver: cannot register port for %s", name );
                return -1;
            }
            port = fGraphManager->GetPort ( port_id );
            port->SetAlias ( alias );
            port->SetLatency ( fEngineControl->fBufferSize + fCaptureLatency );
            fCapturePortList[audio_port_index] = port_id;
            jack_log ( "JackNetDriver::AllocPorts() fCapturePortList[%d] audio_port_index = %ld", audio_port_index, port_id );
        }
        port_flags = JackPortIsInput | JackPortIsPhysical | JackPortIsTerminal;
        for ( audio_port_index = 0; audio_port_index < fPlaybackChannels; audio_port_index++ )
        {
            snprintf ( alias, sizeof ( alias ) - 1, "%s:%s:in%d", fAliasName, fPlaybackDriverName, audio_port_index + 1 );
            snprintf ( name, sizeof ( name ) - 1, "%s:playback_%d",fClientControl.fName, audio_port_index + 1 );
            if ( ( port_id = fGraphManager->AllocatePort ( fClientControl.fRefNum, name, JACK_DEFAULT_AUDIO_TYPE,
                             static_cast<JackPortFlags> ( port_flags ), fEngineControl->fBufferSize ) ) == NO_PORT )
            {
                jack_error ( "driver: cannot register port for %s", name );
                return -1;
            }
            port = fGraphManager->GetPort ( port_id );
            port->SetAlias ( alias );
            port->SetLatency ( fEngineControl->fBufferSize + ( ( fEngineControl->fSyncMode ) ? 0 : fEngineControl->fBufferSize ) + fPlaybackLatency );
            fPlaybackPortList[audio_port_index] = port_id;
            jack_log ( "JackNetDriver::AllocPorts() fPlaybackPortList[%d] audio_port_index = %ld", audio_port_index, port_id );
        }
        //midi
        port_flags = JackPortIsOutput | JackPortIsPhysical | JackPortIsTerminal;
        for ( midi_port_index = 0; midi_port_index < fParams.fSendMidiChannels; midi_port_index++ )
        {
            snprintf ( alias, sizeof ( alias ) - 1, "%s:%s:out%d", fAliasName, fCaptureDriverName, midi_port_index + 1 );
            snprintf ( name, sizeof ( name ) - 1, "%s:midi_capture_%d", fClientControl.fName, midi_port_index + 1 );
            if ( ( port_id = fGraphManager->AllocatePort ( fClientControl.fRefNum, name, JACK_DEFAULT_MIDI_TYPE,
                             static_cast<JackPortFlags> ( port_flags ), fEngineControl->fBufferSize ) ) == NO_PORT )
            {
                jack_error ( "driver: cannot register port for %s", name );
                return -1;
            }
            fMidiCapturePortList[midi_port_index] = port_id;
            jack_log ( "JackNetDriver::AllocPorts() fMidiCapturePortList[%d] midi_port_index = %ld", midi_port_index, port_id );
        }

        port_flags = JackPortIsInput | JackPortIsPhysical | JackPortIsTerminal;
        for ( midi_port_index = 0; midi_port_index < fParams.fReturnMidiChannels; midi_port_index++ )
        {
            snprintf ( alias, sizeof ( alias ) - 1, "%s:%s:in%d", fAliasName, fPlaybackDriverName, midi_port_index + 1 );
            snprintf ( name, sizeof ( name ) - 1, "%s:midi_playback_%d", fClientControl.fName, midi_port_index + 1 );
            if ( ( port_id = fGraphManager->AllocatePort ( fClientControl.fRefNum, name, JACK_DEFAULT_MIDI_TYPE,
                             static_cast<JackPortFlags> ( port_flags ), fEngineControl->fBufferSize ) ) == NO_PORT )
            {
                jack_error ( "driver: cannot register port for %s", name );
                return -1;
            }
            fMidiPlaybackPortList[midi_port_index] = port_id;
            jack_log ( "JackNetDriver::AllocPorts() fMidiPlaybackPortList[%d] midi_port_index = %ld", midi_port_index, port_id );
        }

        return 0;
    }

    int JackNetDriver::FreePorts()
    {
        jack_log ( "JackNetDriver::FreePorts" );
        int audio_port_index;
        uint midi_port_index;
        for ( audio_port_index = 0; audio_port_index < fCaptureChannels; audio_port_index++ )
            fGraphManager->ReleasePort ( fClientControl.fRefNum, fCapturePortList[audio_port_index] );
        for ( audio_port_index = 0; audio_port_index < fPlaybackChannels; audio_port_index++ )
            fGraphManager->ReleasePort ( fClientControl.fRefNum, fPlaybackPortList[audio_port_index] );
        for ( midi_port_index = 0; midi_port_index < fParams.fSendMidiChannels; midi_port_index++ )
            fGraphManager->ReleasePort ( fClientControl.fRefNum, fMidiCapturePortList[midi_port_index] );
        for ( midi_port_index = 0; midi_port_index < fParams.fReturnMidiChannels; midi_port_index++ )
            fGraphManager->ReleasePort ( fClientControl.fRefNum, fMidiPlaybackPortList[midi_port_index] );
        return 0;
    }

    JackMidiBuffer* JackNetDriver::GetMidiInputBuffer ( int port_index )
    {
        return static_cast<JackMidiBuffer*> ( fGraphManager->GetBuffer ( fMidiCapturePortList[port_index], fEngineControl->fBufferSize ) );
    }

    JackMidiBuffer* JackNetDriver::GetMidiOutputBuffer ( int port_index )
    {
        return static_cast<JackMidiBuffer*> ( fGraphManager->GetBuffer ( fMidiPlaybackPortList[port_index], fEngineControl->fBufferSize ) );
    }

    int JackNetDriver::Recv ( size_t size, int flags )
    {
        int rx_bytes;
        if ( ( rx_bytes = fSocket.Recv ( fRxBuffer, size, flags ) ) == SOCKET_ERROR )
        {
            net_error_t error = fSocket.GetError();
            if ( error == NET_NO_DATA )
            {
                jack_error ( "No incoming data, is the master still running ?" );
                return 0;
            }
            else if ( error == NET_CONN_ERROR )
            {
                throw JackDriverException ( "Connection lost." );
            }
            else
            {
                jack_error ( "Error in receive : %s", StrError ( NET_ERROR_CODE ) );
                return 0;
            }
        }
        return rx_bytes;
    }

    int JackNetDriver::Send ( size_t size, int flags )
    {
        int tx_bytes;
        if ( ( tx_bytes = fSocket.Send ( fTxBuffer, size, flags ) ) == SOCKET_ERROR )
        {
            net_error_t error = fSocket.GetError();
            if ( error == NET_CONN_ERROR )
            {
                throw JackDriverException ( "Connection lost." );
            }
            else
                jack_error ( "Error in send : %s", StrError ( NET_ERROR_CODE ) );
        }
        return tx_bytes;
    }

//*************************************process************************************************************************

    int JackNetDriver::Read()
    {
        int rx_bytes;
        uint recvd_midi_pckt = 0;
        packet_header_t* rx_head = reinterpret_cast<packet_header_t*> ( fRxBuffer );
        fRxHeader.fIsLastPckt = 'n';
        uint midi_port_index;
        int audio_port_index;

        //buffers
        for ( midi_port_index = 0; midi_port_index < fParams.fSendMidiChannels; midi_port_index++ )
            fNetMidiCaptureBuffer->SetBuffer(midi_port_index, GetMidiInputBuffer ( midi_port_index ));
        for ( audio_port_index = 0; audio_port_index < fCaptureChannels; audio_port_index++ )
            fNetAudioCaptureBuffer->SetBuffer(audio_port_index, GetInputBuffer ( audio_port_index ));

        //receive sync (launch the cycle)
        do
        {
            rx_bytes = Recv ( sizeof ( packet_header_t ), 0 );
            if ( ( rx_bytes == 0 ) || ( rx_bytes == SOCKET_ERROR ) )
                return rx_bytes;
        }
        while ( !rx_bytes && ( rx_head->fDataType != 's' ) );

        JackDriver::CycleTakeBeginTime();

        //audio, midi or sync if driver is late
        if ( fParams.fSendMidiChannels || fParams.fSendAudioChannels )
        {
            do
            {
                rx_bytes = Recv ( fParams.fMtu, MSG_PEEK );
                if ( rx_bytes < 1 )
                    return rx_bytes;
                if ( rx_bytes && ( rx_head->fDataStream == 's' ) && ( rx_head->fID == fParams.fID ) )
                {
                    switch ( rx_head->fDataType )
                    {
                    case 'm':	//midi
                        rx_bytes = Recv ( rx_bytes, 0 );
                        fRxHeader.fIsLastPckt = rx_head->fIsLastPckt;
                        fNetMidiCaptureBuffer->RenderFromNetwork ( rx_head->fSubCycle, rx_bytes - sizeof ( packet_header_t ) );
                        if ( ++recvd_midi_pckt == rx_head->fNMidiPckt )
                            fNetMidiCaptureBuffer->RenderToJackPorts();
                        break;
                    case 'a':	//audio
                        rx_bytes = Recv ( fAudioRxLen, 0 );
                        if ( !IsNextPacket ( &fRxHeader, rx_head, fNSubProcess ) )
                            jack_error ( "Packet(s) missing..." );
                        fRxHeader.fCycle = rx_head->fCycle;
                        fRxHeader.fSubCycle = rx_head->fSubCycle;
                        fRxHeader.fIsLastPckt = rx_head->fIsLastPckt;
                        fNetAudioCaptureBuffer->RenderToJackPorts ( rx_head->fSubCycle );
                        break;
                    case 's':	//sync
                        jack_info ( "NetDriver : driver overloaded, skipping receive." );
                        fRxHeader.fCycle = rx_head->fCycle;
                        return 0;
                    }
                }
            }
            while ( fRxHeader.fIsLastPckt != 'y' );
        }
        fRxHeader.fCycle = rx_head->fCycle;
        return 0;
    }

    int JackNetDriver::Write()
    {
        int tx_bytes, copy_size;
        fTxHeader.fCycle = fRxHeader.fCycle;
        fTxHeader.fSubCycle = 0;
        fTxHeader.fIsLastPckt = 'n';
        uint midi_port_index;
        int audio_port_index;

        //buffers
        for ( midi_port_index = 0; midi_port_index < fParams.fReturnMidiChannels; midi_port_index++ )
            fNetMidiPlaybackBuffer->SetBuffer(midi_port_index, GetMidiOutputBuffer ( midi_port_index ));
        for ( audio_port_index = 0; audio_port_index < fPlaybackChannels; audio_port_index++ )
            fNetAudioPlaybackBuffer->SetBuffer(audio_port_index, GetOutputBuffer ( audio_port_index ));

        //midi
        if ( fParams.fReturnMidiChannels )
        {
            fTxHeader.fDataType = 'm';
            fTxHeader.fMidiDataSize = fNetMidiPlaybackBuffer->RenderFromJackPorts();
            fTxHeader.fNMidiPckt = GetNMidiPckt ( &fParams, fTxHeader.fMidiDataSize );
            for ( uint subproc = 0; subproc < fTxHeader.fNMidiPckt; subproc++ )
            {
                fTxHeader.fSubCycle = subproc;
                if ( ( subproc == ( fTxHeader.fNMidiPckt - 1 ) ) && !fParams.fReturnAudioChannels )
                    fTxHeader.fIsLastPckt = 'y';
                memcpy ( fTxBuffer, &fTxHeader, sizeof ( packet_header_t ) );
                copy_size = fNetMidiPlaybackBuffer->RenderToNetwork ( subproc, fTxHeader.fMidiDataSize );
                tx_bytes = Send ( sizeof ( packet_header_t ) + copy_size, 0 );
            }
        }

        //audio
        if ( fParams.fReturnAudioChannels )
        {
            fTxHeader.fDataType = 'a';
            for ( uint subproc = 0; subproc < fNSubProcess; subproc++ )
            {
                fTxHeader.fSubCycle = subproc;
                if ( subproc == ( fNSubProcess - 1 ) )
                    fTxHeader.fIsLastPckt = 'y';
                fNetAudioPlaybackBuffer->RenderFromJackPorts ( subproc );
                memcpy ( fTxBuffer, &fTxHeader, sizeof ( packet_header_t ) );
                tx_bytes = Send ( fAudioTxLen, 0 );
            }
        }
        return 0;
    }

//*************************************loader*******************************************************

#ifdef __cplusplus
    extern "C"
    {
#endif
        EXPORT jack_driver_desc_t* driver_get_descriptor ()
        {
            jack_driver_desc_t* desc = ( jack_driver_desc_t* ) calloc ( 1, sizeof ( jack_driver_desc_t ) );
            strcpy ( desc->name, "net" );
            desc->nparams = 8;
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
            desc->params[i].value.i = 19000;
            strcpy ( desc->params[i].short_desc, "UDP port" );
            strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

            i++;
            strcpy ( desc->params[i].name, "mtu" );
            desc->params[i].character = 'M';
            desc->params[i].type = JackDriverParamInt;
            desc->params[i].value.i = 1500;
            strcpy ( desc->params[i].short_desc, "MTU to the master" );
            strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

            i++;
            strcpy ( desc->params[i].name, "input_ports" );
            desc->params[i].character = 'C';
            desc->params[i].type = JackDriverParamInt;
            desc->params[i].value.i = 2;
            strcpy ( desc->params[i].short_desc, "Number of audio input ports" );
            strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

            i++;
            strcpy ( desc->params[i].name, "output_ports" );
            desc->params[i].character = 'P';
            desc->params[i].type = JackDriverParamInt;
            desc->params[i].value.i = 2;
            strcpy ( desc->params[i].short_desc, "Number of audio output ports" );
            strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

            i++;
            strcpy ( desc->params[i].name, "midi_in_ports" );
            desc->params[i].character = 'i';
            desc->params[i].type = JackDriverParamInt;
            desc->params[i].value.i = 0;
            strcpy ( desc->params[i].short_desc, "Number of midi input ports" );
            strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

            i++;
            strcpy ( desc->params[i].name, "midi_out_ports" );
            desc->params[i].character = 'o';
            desc->params[i].type = JackDriverParamUInt;
            desc->params[i].value.i = 0;
            strcpy ( desc->params[i].short_desc, "Number of midi output ports" );
            strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

            i++;
            strcpy ( desc->params[i].name, "client_name" );
            desc->params[i].character = 'n';
            desc->params[i].type = JackDriverParamString;
            strcpy ( desc->params[i].value.str, "'hostname'" );
            strcpy ( desc->params[i].short_desc, "Name of the jack client" );
            strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

            return desc;
        }

        EXPORT Jack::JackDriverClientInterface* driver_initialize ( Jack::JackLockedEngine* engine, Jack::JackSynchro* table, const JSList* params )
        {
            if ( SocketAPIInit() < 0 )
            {
                jack_error ( "Can't init Socket API, exiting..." );
                return NULL;
            }
            const char* multicast_ip = DEFAULT_MULTICAST_IP;
            char name[JACK_CLIENT_NAME_SIZE];
            GetHostName ( name, JACK_CLIENT_NAME_SIZE );
            int udp_port = DEFAULT_PORT;
            int mtu = 1500;
            jack_nframes_t period_size = 128;
            jack_nframes_t sample_rate = 48000;
            int audio_capture_ports = 2;
            int audio_playback_ports = 2;
            int midi_input_ports = 0;
            int midi_output_ports = 0;
            bool monitor = false;

            const JSList* node;
            const jack_driver_param_t* param;
            for ( node = params; node; node = jack_slist_next ( node ) )
            {
                param = ( const jack_driver_param_t* ) node->data;
                switch ( param->character )
                {
                case 'a' :
                    multicast_ip = strdup ( param->value.str );
                    break;
                case 'p':
                    udp_port = param->value.ui;
                    break;
                case 'M':
                    mtu = param->value.i;
                    break;
                case 'C':
                    audio_capture_ports = param->value.i;
                    break;
                case 'P':
                    audio_playback_ports = param->value.i;
                    break;
                case 'i':
                    midi_input_ports = param->value.i;
                    break;
                case 'o':
                    midi_output_ports = param->value.i;
                    break;
                case 'n' :
                    strncpy ( name, param->value.str, JACK_CLIENT_NAME_SIZE );
                }
            }

            Jack::JackDriverClientInterface* driver = new Jack::JackWaitThreadedDriver (
                new Jack::JackNetDriver ( "system", "net_pcm", engine, table, multicast_ip, udp_port, mtu,
                                          midi_input_ports, midi_output_ports, name ) );
            if ( driver->Open ( period_size, sample_rate, 1, 1, audio_capture_ports, audio_playback_ports,
                                monitor, "from_master_", "to_master_", 0, 0 ) == 0 )
                return driver;

            delete driver;
            return NULL;
        }

#ifdef __cplusplus
    }
#endif
}
