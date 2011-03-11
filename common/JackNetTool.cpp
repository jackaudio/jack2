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

    void NetMidiBuffer::SetBuffer ( int index, JackMidiBuffer* buffer )
    {
        fPortBuffer[index] = buffer;
    }

    JackMidiBuffer* NetMidiBuffer::GetBuffer ( int index )
    {
        return fPortBuffer[index];
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
            char* write_pos = fBuffer + pos;

            copy_size = sizeof ( JackMidiBuffer ) + fPortBuffer[port_index]->event_count * sizeof ( JackMidiEvent );
            memcpy ( fBuffer + pos, fPortBuffer[port_index], copy_size );
            pos += copy_size;
            memcpy ( fBuffer + pos, fPortBuffer[port_index] + ( fPortBuffer[port_index]->buffer_size - fPortBuffer[port_index]->write_pos ),
                     fPortBuffer[port_index]->write_pos );
            pos += fPortBuffer[port_index]->write_pos;

            JackMidiBuffer* midi_buffer = reinterpret_cast<JackMidiBuffer*>(write_pos);
            MidiBufferHToN(midi_buffer, midi_buffer);
        }
        return pos;
    }

    int NetMidiBuffer::RenderToJackPorts()
    {
        int pos = 0;
        int copy_size;
        for ( int port_index = 0; port_index < fNPorts; port_index++ )
        {
            JackMidiBuffer* midi_buffer = reinterpret_cast<JackMidiBuffer*>(fBuffer + pos);
            MidiBufferNToH(midi_buffer, midi_buffer);

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

    void NetAudioBuffer::SetBuffer ( int index, sample_t* buffer )
    {
        fPortBuffer[index] = buffer;
    }

    sample_t* NetAudioBuffer::GetBuffer ( int index )
    {
        return fPortBuffer[index];
    }

#ifdef __BIG_ENDIAN__

    static inline jack_default_audio_sample_t SwapFloat(jack_default_audio_sample_t f)
    {
          union
          {
            jack_default_audio_sample_t f;
            unsigned char b[4];
          } dat1, dat2;

          dat1.f = f;
          dat2.b[0] = dat1.b[3];
          dat2.b[1] = dat1.b[2];
          dat2.b[2] = dat1.b[1];
          dat2.b[3] = dat1.b[0];
          return dat2.f;
    }

    void NetAudioBuffer::RenderFromJackPorts ( int subcycle )
    {
        for ( int port_index = 0; port_index < fNPorts; port_index++ ) {
            jack_default_audio_sample_t* src = (jack_default_audio_sample_t*)(fPortBuffer[port_index] + subcycle * fSubPeriodSize);
            jack_default_audio_sample_t* dst = (jack_default_audio_sample_t*)(fNetBuffer + port_index * fSubPeriodBytesSize);
            for (unsigned int sample = 0; sample < fSubPeriodBytesSize / sizeof(jack_default_audio_sample_t); sample++) {
                dst[sample] = SwapFloat(src[sample]);
            }
        }
    }

    void NetAudioBuffer::RenderToJackPorts ( int subcycle )
    {
        for ( int port_index = 0; port_index < fNPorts; port_index++ ) {
            jack_default_audio_sample_t* src = (jack_default_audio_sample_t*)(fNetBuffer + port_index * fSubPeriodBytesSize);
            jack_default_audio_sample_t* dst = (jack_default_audio_sample_t*)(fPortBuffer[port_index] + subcycle * fSubPeriodSize);
            for (unsigned int sample = 0; sample < fSubPeriodBytesSize / sizeof(jack_default_audio_sample_t); sample++) {
                dst[sample] = SwapFloat(src[sample]);
            }
        }
    }

#else

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

#endif

// SessionParams ************************************************************************************

    SERVER_EXPORT void SessionParamsHToN ( session_params_t* src_params, session_params_t* dst_params )
    {
        memcpy(dst_params, src_params, sizeof(session_params_t));
        dst_params->fPacketID = htonl ( src_params->fPacketID );
        dst_params->fMtu = htonl ( src_params->fMtu );
        dst_params->fID = htonl ( src_params->fID );
        dst_params->fTransportSync = htonl ( src_params->fTransportSync );
        dst_params->fSendAudioChannels = htonl ( src_params->fSendAudioChannels );
        dst_params->fReturnAudioChannels = htonl ( src_params->fReturnAudioChannels );
        dst_params->fSendMidiChannels = htonl ( src_params->fSendMidiChannels );
        dst_params->fReturnMidiChannels = htonl ( src_params->fReturnMidiChannels );
        dst_params->fSampleRate = htonl ( src_params->fSampleRate );
        dst_params->fPeriodSize = htonl ( src_params->fPeriodSize );
        dst_params->fFramesPerPacket = htonl ( src_params->fFramesPerPacket );
        dst_params->fBitdepth = htonl ( src_params->fBitdepth );
        dst_params->fSlaveSyncMode = htonl ( src_params->fSlaveSyncMode );
    }

    SERVER_EXPORT void SessionParamsNToH (  session_params_t* src_params, session_params_t* dst_params )
    {
        memcpy(dst_params, src_params, sizeof(session_params_t));
        dst_params->fPacketID = ntohl ( src_params->fPacketID );
        dst_params->fMtu = ntohl ( src_params->fMtu );
        dst_params->fID = ntohl ( src_params->fID );
        dst_params->fTransportSync = ntohl ( src_params->fTransportSync );
        dst_params->fSendAudioChannels = ntohl ( src_params->fSendAudioChannels );
        dst_params->fReturnAudioChannels = ntohl ( src_params->fReturnAudioChannels );
        dst_params->fSendMidiChannels = ntohl ( src_params->fSendMidiChannels );
        dst_params->fReturnMidiChannels = ntohl ( src_params->fReturnMidiChannels );
        dst_params->fSampleRate = ntohl ( src_params->fSampleRate );
        dst_params->fPeriodSize = ntohl ( src_params->fPeriodSize );
        dst_params->fFramesPerPacket = ntohl ( src_params->fFramesPerPacket );
        dst_params->fBitdepth = ntohl ( src_params->fBitdepth );
        dst_params->fSlaveSyncMode = ntohl ( src_params->fSlaveSyncMode );
    }

    SERVER_EXPORT void SessionParamsDisplay ( session_params_t* params )
    {
        char bitdepth[16];
        ( params->fBitdepth ) ? sprintf ( bitdepth, "%u", params->fBitdepth ) : sprintf ( bitdepth, "%s", "float" );
        char mode[8];
        switch ( params->fNetworkMode )
        {
            case 's' :
                strcpy ( mode, "slow" );
                break;
            case 'n' :
                strcpy ( mode, "normal" );
                break;
            case 'f' :
                strcpy ( mode, "fast" );
                break;
        }
        jack_info ( "**************** Network parameters ****************" );
        jack_info ( "Name : %s", params->fName );
        jack_info ( "Protocol revision : %d", params->fProtocolVersion );
        jack_info ( "MTU : %u", params->fMtu );
        jack_info ( "Master name : %s", params->fMasterNetName );
        jack_info ( "Slave name : %s", params->fSlaveNetName );
        jack_info ( "ID : %u", params->fID );
        jack_info ( "Transport Sync : %s", ( params->fTransportSync ) ? "yes" : "no" );
        jack_info ( "Send channels (audio - midi) : %d - %d", params->fSendAudioChannels, params->fSendMidiChannels );
        jack_info ( "Return channels (audio - midi) : %d - %d", params->fReturnAudioChannels, params->fReturnMidiChannels );
        jack_info ( "Sample rate : %u frames per second", params->fSampleRate );
        jack_info ( "Period size : %u frames per period", params->fPeriodSize );
        jack_info ( "Frames per packet : %u", params->fFramesPerPacket );
        jack_info ( "Packet per period : %u", (params->fFramesPerPacket != 0) ? params->fPeriodSize / params->fFramesPerPacket : 0);
        jack_info ( "Bitdepth : %s", bitdepth );
        jack_info ( "Slave mode : %s", ( params->fSlaveSyncMode ) ? "sync" : "async" );
        jack_info ( "Network mode : %s", mode );
        jack_info ( "****************************************************" );
    }

    SERVER_EXPORT sync_packet_type_t GetPacketType ( session_params_t* params )
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

    SERVER_EXPORT int SetPacketType ( session_params_t* params, sync_packet_type_t packet_type )
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

    SERVER_EXPORT void PacketHeaderHToN ( packet_header_t* src_header, packet_header_t* dst_header )
    {
        memcpy(dst_header, src_header, sizeof(packet_header_t));
        dst_header->fID = htonl ( src_header->fID );
        dst_header->fMidiDataSize = htonl ( src_header->fMidiDataSize );
        dst_header->fBitdepth = htonl ( src_header->fBitdepth );
        dst_header->fNMidiPckt = htonl ( src_header->fNMidiPckt );
        dst_header->fPacketSize = htonl ( src_header->fPacketSize );
        dst_header->fCycle = htonl ( src_header->fCycle );
        dst_header->fSubCycle = htonl ( src_header->fSubCycle );
        dst_header->fIsLastPckt = htonl ( src_header->fIsLastPckt );
    }

    SERVER_EXPORT void PacketHeaderNToH ( packet_header_t* src_header, packet_header_t* dst_header )
    {
        memcpy(dst_header, src_header, sizeof(packet_header_t));
        dst_header->fID = ntohl ( src_header->fID );
        dst_header->fMidiDataSize = ntohl ( src_header->fMidiDataSize );
        dst_header->fBitdepth = ntohl ( src_header->fBitdepth );
        dst_header->fNMidiPckt = ntohl ( src_header->fNMidiPckt );
        dst_header->fPacketSize = ntohl ( src_header->fPacketSize );
        dst_header->fCycle = ntohl ( src_header->fCycle );
        dst_header->fSubCycle = ntohl ( src_header->fSubCycle );
        dst_header->fIsLastPckt = ntohl ( src_header->fIsLastPckt );
    }

    SERVER_EXPORT void PacketHeaderDisplay ( packet_header_t* header )
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
        jack_info ( "Last packet : '%s'", ( header->fIsLastPckt ) ? "yes" : "no" );
        jack_info ( "Bitdepth : %s", bitdepth );
        jack_info ( "**********************************************" );
    }

    SERVER_EXPORT void NetTransportDataDisplay ( net_transport_data_t* data )
    {
        jack_info ( "********************Network Transport********************" );
        jack_info ( "Transport new state : %u", data->fNewState );
        jack_info ( "Transport timebase master : %u", data->fTimebaseMaster );
        jack_info ( "Transport cycle state : %u", data->fState );
        jack_info ( "**********************************************" );
    }

    SERVER_EXPORT void MidiBufferHToN ( JackMidiBuffer* src_buffer, JackMidiBuffer* dst_buffer )
    {
        dst_buffer->magic = htonl(src_buffer->magic);
        dst_buffer->buffer_size = htonl(src_buffer->buffer_size);
        dst_buffer->nframes = htonl(src_buffer->nframes);
        dst_buffer->write_pos = htonl(src_buffer->write_pos);
        dst_buffer->event_count = htonl(src_buffer->event_count);
        dst_buffer->lost_events = htonl(src_buffer->lost_events);
        dst_buffer->mix_index = htonl(src_buffer->mix_index);
    }

    SERVER_EXPORT void MidiBufferNToH ( JackMidiBuffer* src_buffer, JackMidiBuffer* dst_buffer )
    {
        dst_buffer->magic = ntohl(src_buffer->magic);
        dst_buffer->buffer_size = ntohl(src_buffer->buffer_size);
        dst_buffer->nframes = ntohl(src_buffer->nframes);
        dst_buffer->write_pos = ntohl(src_buffer->write_pos);
        dst_buffer->event_count = ntohl(src_buffer->event_count);
        dst_buffer->lost_events = ntohl(src_buffer->lost_events);
        dst_buffer->mix_index = ntohl(src_buffer->mix_index);
    }

    SERVER_EXPORT void TransportDataHToN ( net_transport_data_t* src_params, net_transport_data_t* dst_params )
    {
        dst_params->fNewState = htonl(src_params->fNewState);
        dst_params->fTimebaseMaster = htonl(src_params->fTimebaseMaster);
        dst_params->fState = htonl(src_params->fState);
        dst_params->fPosition.unique_1 = htonll(src_params->fPosition.unique_1);
        dst_params->fPosition.usecs = htonl(src_params->fPosition.usecs);
        dst_params->fPosition.frame_rate = htonl(src_params->fPosition.frame_rate);
        dst_params->fPosition.frame = htonl(src_params->fPosition.frame);
        dst_params->fPosition.valid = (jack_position_bits_t)htonl((uint32_t)src_params->fPosition.valid);
        dst_params->fPosition.bar = htonl(src_params->fPosition.bar);
        dst_params->fPosition.beat = htonl(src_params->fPosition.beat);
        dst_params->fPosition.tick = htonl(src_params->fPosition.tick);
        dst_params->fPosition.bar_start_tick = htonll((uint64_t)src_params->fPosition.bar_start_tick);
        dst_params->fPosition.beats_per_bar = htonl((uint32_t)src_params->fPosition.beats_per_bar);
        dst_params->fPosition.beat_type = htonl((uint32_t)src_params->fPosition.beat_type);
        dst_params->fPosition.ticks_per_beat = htonll((uint64_t)src_params->fPosition.ticks_per_beat);
        dst_params->fPosition.beats_per_minute = htonll((uint64_t)src_params->fPosition.beats_per_minute);
        dst_params->fPosition.frame_time = htonll((uint64_t)src_params->fPosition.frame_time);
        dst_params->fPosition.next_time = htonll((uint64_t)src_params->fPosition.next_time);
        dst_params->fPosition.bbt_offset = htonl(src_params->fPosition.bbt_offset);
        dst_params->fPosition.audio_frames_per_video_frame = htonl((uint32_t)src_params->fPosition.audio_frames_per_video_frame);
        dst_params->fPosition.video_offset = htonl(src_params->fPosition.video_offset);
        dst_params->fPosition.unique_2 = htonll(src_params->fPosition.unique_2);
    }

    SERVER_EXPORT void TransportDataNToH ( net_transport_data_t* src_params, net_transport_data_t* dst_params )
    {
        dst_params->fNewState = ntohl(src_params->fNewState);
        dst_params->fTimebaseMaster =  ntohl(src_params->fTimebaseMaster);
        dst_params->fState = ntohl(src_params->fState);
        dst_params->fPosition.unique_1 = ntohll(src_params->fPosition.unique_1);
        dst_params->fPosition.usecs = ntohl(src_params->fPosition.usecs);
        dst_params->fPosition.frame_rate = ntohl(src_params->fPosition.frame_rate);
        dst_params->fPosition.frame = ntohl(src_params->fPosition.frame);
        dst_params->fPosition.valid = (jack_position_bits_t)ntohl((uint32_t)src_params->fPosition.valid);
        dst_params->fPosition.bar = ntohl(src_params->fPosition.bar);
        dst_params->fPosition.beat = ntohl(src_params->fPosition.beat);
        dst_params->fPosition.tick = ntohl(src_params->fPosition.tick);
        dst_params->fPosition.bar_start_tick = ntohll((uint64_t)src_params->fPosition.bar_start_tick);
        dst_params->fPosition.beats_per_bar = ntohl((uint32_t)src_params->fPosition.beats_per_bar);
        dst_params->fPosition.beat_type = ntohl((uint32_t)src_params->fPosition.beat_type);
        dst_params->fPosition.ticks_per_beat = ntohll((uint64_t)src_params->fPosition.ticks_per_beat);
        dst_params->fPosition.beats_per_minute = ntohll((uint64_t)src_params->fPosition.beats_per_minute);
        dst_params->fPosition.frame_time = ntohll((uint64_t)src_params->fPosition.frame_time);
        dst_params->fPosition.next_time = ntohll((uint64_t)src_params->fPosition.next_time);
        dst_params->fPosition.bbt_offset = ntohl(src_params->fPosition.bbt_offset);
        dst_params->fPosition.audio_frames_per_video_frame = ntohl((uint32_t)src_params->fPosition.audio_frames_per_video_frame);
        dst_params->fPosition.video_offset = ntohl(src_params->fPosition.video_offset);
        dst_params->fPosition.unique_2 = ntohll(src_params->fPosition.unique_2);
    }

// Utility *******************************************************************************************************

    SERVER_EXPORT int SocketAPIInit()
    {
#ifdef WIN32
        WORD wVersionRequested = MAKEWORD ( 2, 2 );
        WSADATA wsaData;

        if ( WSAStartup ( wVersionRequested, &wsaData ) != 0 )
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

    SERVER_EXPORT int SocketAPIEnd()
    {
#ifdef WIN32
        return WSACleanup();
#endif
        return 0;
    }

    SERVER_EXPORT const char* GetTransportState ( int transport_state )
    {
        switch ( transport_state )
        {
            case JackTransportRolling:
                return "rolling";
            case JackTransportStarting:
                return "starting";
            case JackTransportStopped:
                return "stopped";
            case JackTransportNetStarting:
                return "netstarting";
        }
        return NULL;
    }
}
