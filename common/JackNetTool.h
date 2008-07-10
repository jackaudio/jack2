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

#include "types.h"
#include "JackConstants.h"
#include "JackMidiPort.h"
#include "JackExports.h"
#include "JackError.h"
#include "JackTools.h"
#include "JackPlatformNetSocket.h"

#include <string>
#include <algorithm>
#include <cmath>

namespace Jack
{
    typedef struct _session_params session_params_t;
    typedef struct _packet_header packet_header_t;
    typedef struct sockaddr socket_address_t;
    typedef struct in_addr address_t;
    typedef jack_default_audio_sample_t sample_t;

//session params ******************************************************************************

    struct _session_params
    {
        char fPacketType[7];			//packet type ('param')
        char fProtocolVersion;			//version
        uint32_t fPacketID;			//indicates the packet type
        char fMasterNetName[256];		//master hostname (network)
        char fSlaveNetName[256];		//slave hostname (network)
        uint32_t fMtu;				//connection mtu
        uint32_t fID;				//slave's ID
        uint32_t fSendAudioChannels;		//number of master->slave channels
        uint32_t fReturnAudioChannels;		//number of slave->master channels
        uint32_t fSendMidiChannels;		//number of master->slave midi channels
        uint32_t fReturnMidiChannels;		//number of slave->master midi channels
        uint32_t fSampleRate;			//session sample rate
        uint32_t fPeriodSize;			//period size
        uint32_t fFramesPerPacket;		//complete frames per packet
        uint32_t fBitdepth;			//samples bitdepth (unused)
        char fName[JACK_CLIENT_NAME_SIZE];	//slave's name
    };


//net status **********************************************************************************

    enum  _net_status
    {
        NET_SOCKET_ERROR = 0,
        NET_CONNECT_ERROR,
        NET_ERROR,
        NET_SEND_ERROR,
        NET_RECV_ERROR,
        NET_CONNECTED,
        NET_ROLLING
    };

    typedef enum _net_status net_status_t;

//sync packet type ****************************************************************************

    enum _sync_packet_type
    {
        INVALID = 0,	    //...
        SLAVE_AVAILABLE,	//a slave is available
        SLAVE_SETUP,		//slave configuration
        START_MASTER,		//slave is ready, start master
        START_SLAVE,		//master is ready, activate slave
        KILL_MASTER		    //master must stop
    };

    typedef enum _sync_packet_type sync_packet_type_t;


//packet header *******************************************************************************

    struct _packet_header
    {
        char fPacketType[7];		//packet type ( 'headr' )
        char fDataType;			//a for audio, m for midi
        char fDataStream;		//s for send, r for return
        uint32_t fID;			//to identify the slave
        uint32_t fBitdepth;		//bitdepth of the data samples
        uint32_t fMidiDataSize;		//size of midi data (if packet is 'midi typed') in bytes
        uint32_t fNMidiPckt;		//number of midi packets of the cycle
        uint32_t fCycle;		//process cycle counter
        uint32_t fSubCycle;		//midi/audio subcycle counter
        char fIsLastPckt;		//is it the last packet of a given cycle ('y' or 'n')
        char fFree[13];             	//unused
    };

//midi data ***********************************************************************************

    class EXPORT NetMidiBuffer
    {
    private:
        int fNPorts;
        size_t fMaxBufsize;
        int fMaxPcktSize;
        //data
        char* fBuffer;
        char* fNetBuffer;
        JackMidiBuffer** fPortBuffer;
         
    public:
        NetMidiBuffer ( session_params_t* params, uint32_t nports, char* net_buffer );
        ~NetMidiBuffer();

        void Reset();
        size_t GetSize();
        //utility
        void DisplayEvents();
        //jack<->buffer
        int RenderFromJackPorts();
        int RenderToJackPorts();
        //network<->buffer
        int RenderFromNetwork ( int subcycle, size_t copy_size );
        int RenderToNetwork ( int subcycle, size_t total_size );
        
         void SetBuffer(int index, JackMidiBuffer* buffer);
    };

// audio data *********************************************************************************

    class EXPORT NetAudioBuffer
    {
    private:
        int fNPorts;
        jack_nframes_t fPeriodSize;
        jack_nframes_t fSubPeriodSize;
        size_t fSubPeriodBytesSize;
        char* fNetBuffer;
        sample_t** fPortBuffer;
    public:
        NetAudioBuffer ( session_params_t* params, uint32_t nports, char* net_buffer );
        ~NetAudioBuffer();

        size_t GetSize();
        //jack<->buffer
        void RenderFromJackPorts ( int subcycle );
        void RenderToJackPorts ( int subcycle );
        
        void SetBuffer(int index, sample_t* buffer);
    };

//utility *************************************************************************************

    //socket API management
    EXPORT int SocketAPIInit();
    EXPORT int SocketAPIEnd();
    //n<-->h functions
    EXPORT void SessionParamsHToN ( session_params_t* params );
    EXPORT void SessionParamsNToH ( session_params_t* params );
    EXPORT void PacketHeaderHToN ( packet_header_t* header );
    EXPORT void PacketHeaderNToH ( packet_header_t* header );
    //display session parameters
    EXPORT void SessionParamsDisplay ( session_params_t* params );
    //display packet header
    EXPORT void PacketHeaderDisplay ( packet_header_t* header );
    //get the packet type from a sesion parameters
    EXPORT sync_packet_type_t GetPacketType ( session_params_t* params );
    //set the packet type in a session parameters
    EXPORT int SetPacketType ( session_params_t* params, sync_packet_type_t packet_type );
    //step of network initialization
    EXPORT jack_nframes_t SetFramesPerPacket ( session_params_t* params );
    //get the midi packet number for a given cycle
    EXPORT int GetNMidiPckt ( session_params_t* params, size_t data_size );
    //set the recv timeout on a socket
    EXPORT int SetRxTimeout ( JackNetSocket* socket, session_params_t* params );
    //check if 'next' packet is really the next after 'previous'
    EXPORT bool IsNextPacket ( packet_header_t* previous, packet_header_t* next, uint subcycles );
}
