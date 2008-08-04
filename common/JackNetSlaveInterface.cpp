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

#include "JackNetSlaveInterface.h"
#include "JackEngineControl.h"
#include "JackClientControl.h"
#include "JackGraphManager.h"
#include "JackException.h"

#define DEFAULT_MULTICAST_IP "225.3.19.154"
#define DEFAULT_PORT 19000

using namespace std;

namespace Jack
{
    JackNetSlaveInterface::JackNetSlaveInterface()
    {
        fMulticastIP = NULL;
    }

    JackNetSlaveInterface::~JackNetSlaveInterface()
    {
        SocketAPIEnd();
        delete[] fTxBuffer;
        delete[] fRxBuffer;
        delete[] fMulticastIP;
        delete fNetAudioCaptureBuffer;
        delete fNetAudioPlaybackBuffer;
        delete fNetMidiCaptureBuffer;
        delete fNetMidiPlaybackBuffer;
    }

//*************************************initialization***********************************************************************


    bool JackNetSlaveInterface::Init()
    {
        jack_log ( "JackNetSlaveInterface::NetInit()" );

        //set the parameters to send
        strcpy ( fParams.fPacketType, "params" );
        fParams.fProtocolVersion = 'a';
        SetPacketType ( &fParams, SLAVE_AVAILABLE );

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

        return true;
    }

    net_status_t JackNetSlaveInterface::GetNetMaster()
    {
        jack_log ( "JackNetSlaveInterface::GetNetMaster()" );
        //utility
        session_params_t params;
        int us_timeout = 2000000;
        int rx_bytes = 0;
        unsigned char loop = 0;

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
        if ( fSocket.SetTimeOut ( us_timeout ) == SOCKET_ERROR )
            jack_error ( "Can't set timeout : %s", StrError ( NET_ERROR_CODE ) );

        //disable local loop
        if ( fSocket.SetOption ( IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof ( loop ) ) == SOCKET_ERROR )
            jack_error ( "Can't disable multicast loop : %s", StrError ( NET_ERROR_CODE ) );

        //send 'AVAILABLE' until 'SLAVE_SETUP' received
        jack_info ( "Waiting for a master..." );
        do
        {
            //send 'available'
            if ( fSocket.SendTo ( &fParams, sizeof ( session_params_t ), 0, fMulticastIP ) == SOCKET_ERROR )
                jack_error ( "Error in data send : %s", StrError ( NET_ERROR_CODE ) );
            //filter incoming packets : don't exit while no error is detected
            rx_bytes = fSocket.CatchHost ( &params, sizeof ( session_params_t ), 0 );
            if ( ( rx_bytes == SOCKET_ERROR ) && ( fSocket.GetError() != NET_NO_DATA ) )
            {
                jack_error ( "Can't receive : %s", StrError ( NET_ERROR_CODE ) );
                return NET_RECV_ERROR;
            }
        }
        while ( strcmp ( params.fPacketType, fParams.fPacketType ) && ( GetPacketType ( &params ) != SLAVE_SETUP ) );

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

    net_status_t JackNetSlaveInterface::SendMasterStartSync()
    {
        jack_log ( "JackNetSlaveInterface::GetNetMasterStartSync()" );
        //tell the master to start
        SetPacketType ( &fParams, START_MASTER );
        if ( fSocket.Send ( &fParams, sizeof ( session_params_t ), 0 ) == SOCKET_ERROR )
        {
            jack_error ( "Error in send : %s", StrError ( NET_ERROR_CODE ) );
            return ( fSocket.GetError() == NET_CONN_ERROR ) ? NET_ERROR : NET_SEND_ERROR;
        }
        return NET_ROLLING;
    }

    int JackNetSlaveInterface::SetParams()
    {
        fNSubProcess = fParams.fPeriodSize / fParams.fFramesPerPacket;

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

        //payload size
        fPayloadSize = fParams.fMtu - sizeof ( packet_header_t );

        return 0;
    }

//***********************************network operations*************************************************************

    int JackNetSlaveInterface::Recv ( size_t size, int flags )
    {
        int rx_bytes = fSocket.Recv ( fRxBuffer, size, flags );
        //handle errors
        if ( rx_bytes == SOCKET_ERROR )
        {
            net_error_t error = fSocket.GetError();
            //no data isn't really an error in realtime processing, so just return 0
            if ( error == NET_NO_DATA )
                jack_error ( "No data, is the master still running ?" );
            //if a network error occurs, this exception will restart the driver
            else if ( error == NET_CONN_ERROR )
            {
                jack_error ( "Connection lost." );
                throw JackDriverException();
            }
            else
                jack_error ( "Fatal error in receive : %s", StrError ( NET_ERROR_CODE ) );
        }
        return rx_bytes;
    }

    int JackNetSlaveInterface::Send ( size_t size, int flags )
    {
        int tx_bytes = fSocket.Send ( fTxBuffer, size, flags );
        //handle errors
        if ( tx_bytes == SOCKET_ERROR )
        {
            net_error_t error = fSocket.GetError();
            //if a network error occurs, this exception will restart the driver
            if ( error == NET_CONN_ERROR )
            {
                jack_error ( "Connection lost." );
                throw JackDriverException();
            }
            else
                jack_error ( "Fatal error in send : %s", StrError ( NET_ERROR_CODE ) );
        }
        return tx_bytes;
    }

//**************************************processes***************************************************

    int JackNetSlaveInterface::SyncRecv()
    {
        int rx_bytes;
        packet_header_t* rx_head = reinterpret_cast<packet_header_t*> ( fRxBuffer );
        fRxHeader.fIsLastPckt = 'n';
        //receive sync (launch the cycle)
        do
        {
            rx_bytes = Recv ( fParams.fMtu, 0 );
            //connection issue, send will detect it, so don't skip the cycle (return 0)
            if ( rx_bytes == SOCKET_ERROR )
                return rx_bytes;
        }
        while ( !rx_bytes && ( rx_head->fDataType != 's' ) );
        return rx_bytes;
    }

    int JackNetSlaveInterface::DataRecv()
    {
        uint recvd_midi_pckt = 0;
        int rx_bytes;
        packet_header_t* rx_head = reinterpret_cast<packet_header_t*> ( fRxBuffer );

        //audio, midi or sync if driver is late
        if ( fParams.fSendMidiChannels || fParams.fSendAudioChannels )
        {
            do
            {
                rx_bytes = Recv ( fParams.fMtu, MSG_PEEK );
                //error here, problem with recv, just skip the cycle (return -1)
                if ( rx_bytes == SOCKET_ERROR )
                    return rx_bytes;
                if ( rx_bytes && ( rx_head->fDataStream == 's' ) && ( rx_head->fID == fParams.fID ) )
                {
                    switch ( rx_head->fDataType )
                    {
                        case 'm':   //midi
                            rx_bytes = Recv ( rx_head->fPacketSize, 0 );
                            fRxHeader.fCycle = rx_head->fCycle;
                            fRxHeader.fIsLastPckt = rx_head->fIsLastPckt;
                            fNetMidiCaptureBuffer->RenderFromNetwork ( rx_head->fSubCycle, rx_bytes - sizeof ( packet_header_t ) );
                            if ( ++recvd_midi_pckt == rx_head->fNMidiPckt )
                                fNetMidiCaptureBuffer->RenderToJackPorts();
                            break;
                        case 'a':   //audio
                            rx_bytes = Recv ( rx_head->fPacketSize, 0 );
                            if ( !IsNextPacket ( &fRxHeader, rx_head, fNSubProcess ) )
                                jack_error ( "Packet(s) missing..." );
                            fRxHeader.fCycle = rx_head->fCycle;
                            fRxHeader.fSubCycle = rx_head->fSubCycle;
                            fRxHeader.fIsLastPckt = rx_head->fIsLastPckt;
                            fNetAudioCaptureBuffer->RenderToJackPorts ( rx_head->fSubCycle );
                            break;
                        case 's':   //sync
                            jack_info ( "NetSlave : overloaded, skipping receive." );
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

    int JackNetSlaveInterface::SyncSend()
    {
        //tx header
        if ( fParams.fSlaveSyncMode )
            fTxHeader.fCycle = fRxHeader.fCycle;
        else
            fTxHeader.fCycle++;
        fTxHeader.fSubCycle = 0;

        //sync
        fTxHeader.fDataType = 's';
        fTxHeader.fIsLastPckt = ( !fParams.fSendMidiChannels && !fParams.fSendAudioChannels ) ?  'y' : 'n';
        fTxHeader.fPacketSize = fParams.fMtu;
        memcpy ( fTxBuffer, &fTxHeader, sizeof ( packet_header_t ) );
        return Send ( fTxHeader.fPacketSize, 0 );
    }

    int JackNetSlaveInterface::DataSend()
    {
        int tx_bytes;
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
                fTxHeader.fPacketSize = fNetMidiPlaybackBuffer->RenderToNetwork ( subproc, fTxHeader.fMidiDataSize );
                fTxHeader.fPacketSize += sizeof ( packet_header_t );
                memcpy ( fTxBuffer, &fTxHeader, sizeof ( packet_header_t ) );
                tx_bytes = Send ( fTxHeader.fPacketSize, 0 );
                if ( tx_bytes == SOCKET_ERROR )
                    return tx_bytes;
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
                fTxHeader.fPacketSize = fAudioTxLen;
                memcpy ( fTxBuffer, &fTxHeader, sizeof ( packet_header_t ) );
                fNetAudioPlaybackBuffer->RenderFromJackPorts ( subproc );
                tx_bytes = Send ( fTxHeader.fPacketSize, 0 );
                if ( tx_bytes == SOCKET_ERROR )
                    return tx_bytes;
            }
        }
        return 0;
    }
}
