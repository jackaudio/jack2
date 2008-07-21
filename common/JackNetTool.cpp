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

#include "JackNetTool.h"

using namespace std;

namespace Jack
{
// NetMidiBuffer**********************************************************************************

    NetMidiBuffer::NetMidiBuffer ( session_params_t* params, uint32_t nports, char* net_buffer )
    {
        fNPorts = nports;
        fMaxBufsize = fNPorts * sizeof ( sample_t ) * params->fPeriodSize ;
        fMaxPcktSize = params->fMtu - sizeof ( packet_header_t );
        fBuffer = new char[fMaxBufsize];
        fPortBuffer = new JackMidiBuffer* [fNPorts];
        for ( int port_index = 0; port_index < fNPorts; port_index++ )
            fPortBuffer[port_index] = NULL;
        fNetBuffer = net_buffer;
    }

    NetMidiBuffer::~NetMidiBuffer()
    {
        delete[] fBuffer;
        delete[] fPortBuffer;
    }

    size_t NetMidiBuffer::GetSize()
    {
        return fMaxBufsize;
    }

    void NetMidiBuffer::SetBuffer(int index, JackMidiBuffer* buffer)
    {
        fPortBuffer[index] = buffer;
    }

    void NetMidiBuffer::DisplayEvents()
    {
        for ( int port_index = 0; port_index < fNPorts; port_index++ )
        {
            for ( uint event = 0; event < fPortBuffer[port_index]->event_count; event++ )
                if ( fPortBuffer[port_index]->IsValid() )
                    jack_info ( "port %d : midi event %u/%u -> time : %u, size : %u",
                                port_index + 1, event + 1, fPortBuffer[port_index]->event_count,
                                fPortBuffer[port_index]->events[event].time, fPortBuffer[port_index]->events[event].size );
        }
    }

    int NetMidiBuffer::RenderFromJackPorts()
    {
        int pos = 0;
        size_t copy_size;
        for ( int port_index = 0; port_index < fNPorts; port_index++ )
        {
            copy_size = sizeof ( JackMidiBuffer ) + fPortBuffer[port_index]->event_count * sizeof ( JackMidiEvent );
            memcpy ( fBuffer + pos, fPortBuffer[port_index], copy_size );
            pos += copy_size;
            memcpy ( fBuffer + pos, fPortBuffer[port_index] + ( fPortBuffer[port_index]->buffer_size - fPortBuffer[port_index]->write_pos ),
                     fPortBuffer[port_index]->write_pos );
            pos += fPortBuffer[port_index]->write_pos;
        }
        return pos;
    }

    int NetMidiBuffer::RenderToJackPorts()
    {
        int pos = 0;
        int copy_size;
        for ( int port_index = 0; port_index < fNPorts; port_index++ )
        {
            copy_size = sizeof ( JackMidiBuffer ) + reinterpret_cast<JackMidiBuffer*> ( fBuffer + pos )->event_count * sizeof ( JackMidiEvent );
            memcpy ( fPortBuffer[port_index], fBuffer + pos, copy_size );
            pos += copy_size;
            memcpy ( fPortBuffer[port_index] + ( fPortBuffer[port_index]->buffer_size - fPortBuffer[port_index]->write_pos ),
                     fBuffer + pos, fPortBuffer[port_index]->write_pos );
            pos += fPortBuffer[port_index]->write_pos;
        }
        return pos;
    }

    int NetMidiBuffer::RenderFromNetwork ( int subcycle, size_t copy_size )
    {
        memcpy ( fBuffer + subcycle * fMaxPcktSize, fNetBuffer, copy_size );
        return copy_size;
    }

    int NetMidiBuffer::RenderToNetwork ( int subcycle, size_t total_size )
    {
        int size = total_size - subcycle * fMaxPcktSize;
        int copy_size = ( size <= fMaxPcktSize ) ? size : fMaxPcktSize;
        memcpy ( fNetBuffer, fBuffer + subcycle * fMaxPcktSize, copy_size );
        return copy_size;
    }

// net audio buffer *********************************************************************************

    NetAudioBuffer::NetAudioBuffer ( session_params_t* params, uint32_t nports, char* net_buffer )
    {
        fNPorts = nports;
        fPeriodSize = params->fPeriodSize;
        fSubPeriodSize = params->fFramesPerPacket;
        fSubPeriodBytesSize = fSubPeriodSize * sizeof ( sample_t );
        fPortBuffer = new sample_t* [fNPorts];
        for ( int port_index = 0; port_index < fNPorts; port_index++ )
            fPortBuffer[port_index] = NULL;
        fNetBuffer = net_buffer;
    }

    NetAudioBuffer::~NetAudioBuffer()
    {
        delete[] fPortBuffer;
    }

    size_t NetAudioBuffer::GetSize()
    {
        return fNPorts * fSubPeriodBytesSize;
    }

    void NetAudioBuffer::SetBuffer(int index, sample_t* buffer)
    {
        fPortBuffer[index] = buffer;
    }

    void NetAudioBuffer::RenderFromJackPorts ( int subcycle )
    {
        for ( int port_index = 0; port_index < fNPorts; port_index++ )
            memcpy ( fNetBuffer + port_index * fSubPeriodBytesSize, fPortBuffer[port_index] + subcycle * fSubPeriodSize, fSubPeriodBytesSize );
    }

    void NetAudioBuffer::RenderToJackPorts ( int subcycle )
    {
        for ( int port_index = 0; port_index < fNPorts; port_index++ )
            memcpy ( fPortBuffer[port_index] + subcycle * fSubPeriodSize, fNetBuffer + port_index * fSubPeriodBytesSize, fSubPeriodBytesSize );
    }

// SessionParams ************************************************************************************

    EXPORT void SessionParamsHToN ( session_params_t* params )
    {
        params->fPacketID = htonl ( params->fPacketID );
        params->fMtu = htonl ( params->fMtu );
        params->fID = htonl ( params->fID );
        params->fTransportSync = htonl ( params->fTransportSync );
        params->fSendAudioChannels = htonl ( params->fSendAudioChannels );
        params->fReturnAudioChannels = htonl ( params->fReturnAudioChannels );
        params->fSendMidiChannels = htonl ( params->fSendMidiChannels );
        params->fReturnMidiChannels = htonl ( params->fReturnMidiChannels );
        params->fSampleRate = htonl ( params->fSampleRate );
        params->fPeriodSize = htonl ( params->fPeriodSize );
        params->fFramesPerPacket = htonl ( params->fFramesPerPacket );
        params->fBitdepth = htonl ( params->fBitdepth );
    }

    EXPORT void SessionParamsNToH ( session_params_t* params )
    {
        params->fPacketID = ntohl ( params->fPacketID );
        params->fMtu = ntohl ( params->fMtu );
        params->fID = ntohl ( params->fID );
        params->fTransportSync = ntohl ( params->fTransportSync );
        params->fSendAudioChannels = ntohl ( params->fSendAudioChannels );
        params->fReturnAudioChannels = ntohl ( params->fReturnAudioChannels );
        params->fSendMidiChannels = ntohl ( params->fSendMidiChannels );
        params->fReturnMidiChannels = ntohl ( params->fReturnMidiChannels );
        params->fSampleRate = ntohl ( params->fSampleRate );
        params->fPeriodSize = ntohl ( params->fPeriodSize );
        params->fFramesPerPacket = ntohl ( params->fFramesPerPacket );
        params->fBitdepth = ntohl ( params->fBitdepth );
    }

    EXPORT void SessionParamsDisplay ( session_params_t* params )
    {
    	char bitdepth[16];
    	( params->fBitdepth ) ? sprintf ( bitdepth, "%u", params->fBitdepth ) : sprintf ( bitdepth, "%s", "float" );
        jack_info ( "**************** Network parameters ****************" );
        jack_info ( "Name : %s", params->fName );
        jack_info ( "Protocol revision : %c", params->fProtocolVersion );
        jack_info ( "MTU : %u", params->fMtu );
        jack_info ( "Master name : %s", params->fMasterNetName );
        jack_info ( "Slave name : %s", params->fSlaveNetName );
        jack_info ( "ID : %u", params->fID );
        jack_info ( "Transport Sync : %s", (params->fTransportSync) ? "yes" : "no" );
        jack_info ( "Send channels (audio - midi) : %d - %d", params->fSendAudioChannels, params->fSendMidiChannels );
        jack_info ( "Return channels (audio - midi) : %d - %d", params->fReturnAudioChannels, params->fReturnMidiChannels );
        jack_info ( "Sample rate : %u frames per second", params->fSampleRate );
        jack_info ( "Period size : %u frames per period", params->fPeriodSize );
        jack_info ( "Frames per packet : %u", params->fFramesPerPacket );
        jack_info ( "Packet per period : %u", params->fPeriodSize / params->fFramesPerPacket );
        jack_info ( "Bitdepth : %s", bitdepth );
        jack_info ( "****************************************************" );
    }

    EXPORT sync_packet_type_t GetPacketType ( session_params_t* params )
    {
        switch ( params->fPacketID )
        {
        case 0:
            return SLAVE_AVAILABLE;
        case 1:
            return SLAVE_SETUP;
        case 2:
            return START_MASTER;
        case 3:
            return START_SLAVE;
        case 4:
            return KILL_MASTER;
        }
        return INVALID;
    }

    EXPORT int SetPacketType ( session_params_t* params, sync_packet_type_t packet_type )
    {
        switch ( packet_type )
        {
        case INVALID:
            return -1;
        case SLAVE_AVAILABLE:
            params->fPacketID = 0;
            break;
        case SLAVE_SETUP:
            params->fPacketID = 1;
            break;
        case START_MASTER:
            params->fPacketID = 2;
            break;
        case START_SLAVE:
            params->fPacketID = 3;
            break;
        case KILL_MASTER:
            params->fPacketID = 4;
        }
        return 0;
    }

// Packet header **********************************************************************************

    EXPORT void PacketHeaderHToN ( packet_header_t* header )
    {
        header->fID = htonl ( header->fID );
        header->fMidiDataSize = htonl ( header->fMidiDataSize );
        header->fBitdepth = htonl ( header->fBitdepth );
        header->fNMidiPckt = htonl ( header->fNMidiPckt );
        header->fCycle = ntohl ( header->fCycle );
        header->fSubCycle = htonl ( header->fSubCycle );
    }

    EXPORT void PacketHeaderNToH ( packet_header_t* header )
    {
        header->fID = ntohl ( header->fID );
        header->fMidiDataSize = ntohl ( header->fMidiDataSize );
        header->fBitdepth = ntohl ( header->fBitdepth );
        header->fNMidiPckt = ntohl ( header->fNMidiPckt );
        header->fCycle = ntohl ( header->fCycle );
        header->fSubCycle = ntohl ( header->fSubCycle );
    }

    EXPORT void PacketHeaderDisplay ( packet_header_t* header )
    {
    	char bitdepth[16];
    	( header->fBitdepth ) ? sprintf ( bitdepth, "%u", header->fBitdepth ) : sprintf ( bitdepth, "%s", "float" );
        jack_info ( "********************Header********************" );
        jack_info ( "Data type : %c", header->fDataType );
        jack_info ( "Data stream : %c", header->fDataStream );
        jack_info ( "ID : %u", header->fID );
        jack_info ( "Cycle : %u", header->fCycle );
        jack_info ( "SubCycle : %u", header->fSubCycle );
        jack_info ( "Midi packets : %u", header->fNMidiPckt );
        jack_info ( "Midi data size : %u", header->fMidiDataSize );
        jack_info ( "Last packet : '%c'", header->fIsLastPckt );
        jack_info ( "Bitdepth : %s", bitdepth );
        jack_info ( "**********************************************" );
    }

// Utility *******************************************************************************************************

    EXPORT int SocketAPIInit()
    {
#ifdef WIN32
        WORD wVersionRequested = MAKEWORD ( 2, 2 );
        WSADATA wsaData;

        if ( WSAStartup(wVersionRequested, &wsaData) != 0 )
        {
            jack_error ( "WSAStartup error : %s", strerror ( NET_ERROR_CODE ) );
            return -1;
        }

        if ( LOBYTE ( wsaData.wVersion ) != 2 || HIBYTE ( wsaData.wVersion ) != 2 )
        {
            jack_error ( "Could not find a useable version of Winsock.dll\n" );
            WSACleanup();
            return -1;
        }
#endif
        return 0;
    }

    EXPORT int SocketAPIEnd()
    {
#ifdef WIN32
        return WSACleanup();
#endif
        return 0;
    }

    EXPORT jack_nframes_t SetFramesPerPacket ( session_params_t* params )
    {
        if ( !params->fSendAudioChannels && !params->fReturnAudioChannels )
            return ( params->fFramesPerPacket = params->fPeriodSize );
        size_t period = ( int ) powf ( 2.f, ( int ) ( log ( ( params->fMtu - sizeof ( packet_header_t ) )
                                       / ( max ( params->fReturnAudioChannels, params->fSendAudioChannels ) * sizeof ( sample_t ) ) ) / log ( 2 ) ) );
        ( period > params->fPeriodSize ) ? params->fFramesPerPacket = params->fPeriodSize : params->fFramesPerPacket = period;
        return params->fFramesPerPacket;
    }

    EXPORT int GetNMidiPckt ( session_params_t* params, size_t data_size )
    {
        //even if there is no midi data, jack need an empty buffer to know there is no event to read
        //99% of the cases : all data in one packet
        if ( data_size <= ( params->fMtu - sizeof ( packet_header_t ) ) )
            return 1;
        //else, get the number of needed packets (simply slice the biiig buffer)
        int npckt = data_size / ( params->fMtu - sizeof ( packet_header_t ) );
        if ( data_size % ( params->fMtu - sizeof ( packet_header_t ) ) )
            return ++npckt;
        return npckt;
    }

    EXPORT int SetRxTimeout ( JackNetSocket* socket, session_params_t* params )
    {
        //time in ms : 1,25 * 'one packet time'
        float time = 1250 * ( static_cast<float> ( params->fFramesPerPacket ) / static_cast<float> ( params->fSampleRate ) );
        int ms = ( int ) time;
        return ( socket->SetTimeOut ( ms ) == SOCKET_ERROR ) ? SOCKET_ERROR : ms;
    }

// Packet *******************************************************************************************************

    EXPORT bool IsNextPacket ( packet_header_t* previous, packet_header_t* next, uint subcycles )
    {
        //ignore first cycle
        if ( previous->fCycle <= 1 )
            return true;
        //same PcktID (cycle), next SubPcktID (subcycle)
        if ( ( previous->fSubCycle < ( subcycles - 1 ) ) && ( next->fCycle == previous->fCycle ) && ( next->fSubCycle == ( previous->fSubCycle + 1 ) ) )
            return true;
        //next PcktID (cycle), SubPcktID reset to 1 (first subcyle)
        if ( ( next->fCycle == ( previous->fCycle + 1 ) ) && ( previous->fSubCycle == ( subcycles - 1 ) ) && ( next->fSubCycle == 0 ) )
            return true;
        //else, next is'nt next, return false
        return false;
    }
}
