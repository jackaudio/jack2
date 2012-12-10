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

#include "JackMidiPort.h"
#include "JackTools.h"
#include "types.h"
#include "transport.h"
#ifndef WIN32
#include <netinet/in.h>
#endif
#include <cmath>

using namespace std;

#ifndef htonll
#ifdef __BIG_ENDIAN__
#define htonll(x) (x)
#define ntohll(x) (x)
#else
#define htonll(x) ((((uint64_t)htonl(x)) << 32) + htonl(x >> 32))
#define ntohll(x) ((((uint64_t)ntohl(x)) << 32) + ntohl(x >> 32))
#endif
#endif

#define MASTER_PROTOCOL 6
#define SLAVE_PROTOCOL 6

#define NET_PACKET_ERROR -2

#define OPTIMIZED_PROTOCOL

#define HEADER_SIZE (sizeof(packet_header_t))
#define PACKET_AVAILABLE_SIZE(params) ((params)->fMtu - sizeof(packet_header_t))

namespace Jack
{
    typedef struct _session_params session_params_t;
    typedef struct _packet_header packet_header_t;
    typedef struct _net_transport_data net_transport_data_t;
    typedef struct sockaddr socket_address_t;
    typedef struct in_addr address_t;
    typedef jack_default_audio_sample_t sample_t;

    enum JackNetEncoder {

        JackFloatEncoder = 0,
        JackIntEncoder = 1,
        JackCeltEncoder = 2,
        JackOpusEncoder = 3,
    };

//session params ******************************************************************************

    /**
    \brief This structure containes master/slave connection parameters, it's used to setup the whole system

    We have :
        - some info like version, type and packet id
        - names
        - network parameters (hostnames and mtu)
        - nunber of audio and midi channels
        - sample rate and buffersize
        - number of audio frames in one network packet (depends on the channel number)
        - is the NetDriver in Sync or ASync mode ?
        - is the NetDriver linked with the master's transport

    Data encoding : headers (session_params and packet_header) are encoded using HTN kind of functions but float data
    are kept in LITTLE_ENDIAN format (to avoid 2 conversions in the more common LITTLE_ENDIAN <==> LITTLE_ENDIAN connection case).
    */

    PRE_PACKED_STRUCTURE
    struct _session_params
    {
        char fPacketType[8];                //packet type ('param')
        uint32_t fProtocolVersion;          //version
        int32_t fPacketID;                 //indicates the packet type
        char fName[JACK_CLIENT_NAME_SIZE];  //slave's name
        char fMasterNetName[256];           //master hostname (network)
        char fSlaveNetName[256];            //slave hostname (network)
        uint32_t fMtu;                      //connection mtu
        uint32_t fID;                       //slave's ID
        uint32_t fTransportSync;            //is the transport synced ?
        int32_t fSendAudioChannels;         //number of master->slave channels
        int32_t fReturnAudioChannels;       //number of slave->master channels
        int32_t fSendMidiChannels;          //number of master->slave midi channels
        int32_t fReturnMidiChannels;        //number of slave->master midi channels
        uint32_t fSampleRate;               //session sample rate
        uint32_t fPeriodSize;               //period size
        uint32_t fSampleEncoder;            //samples encoder
        uint32_t fKBps;                     //KB per second for CELT encoder
        uint32_t fSlaveSyncMode;            //is the slave in sync mode ?
        uint32_t fNetworkLatency;           //network latency
    } POST_PACKED_STRUCTURE;

//net status **********************************************************************************

    /**
    \Brief This enum groups network error by type
    */

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

    /**
    \Brief This enum indicates the type of a sync packet (used in the initialization phase)
    */

    enum _sync_packet_type
    {
        INVALID = 0,        //...
        SLAVE_AVAILABLE,    //a slave is available
        SLAVE_SETUP,        //slave configuration
        START_MASTER,       //slave is ready, start master
        START_SLAVE,        //master is ready, activate slave
        KILL_MASTER         //master must stop
    };

    typedef enum _sync_packet_type sync_packet_type_t;


//packet header *******************************************************************************

    /**
    \Brief This structure is a complete header

    A header indicates :
        - it is a header
        - the type of data the packet contains (sync, midi or audio)
        - the path of the packet (send -master->slave- or return -slave->master-)
        - the unique ID of the slave
        - the sample's bitdepth (unused for now)
        - the size of the midi data contains in the packet (indicates how much midi data will be sent)
        - the number of midi packet(s) : more than one is very unusual, it depends on the midi load
        - the ID of the current cycle (used to check missing packets)
        - the ID of the packet subcycle (for audio data)
        - a flag indicating this packet is the last of the cycle (for sync robustness, it's better to process this way)
        - a flag indicating if, in async mode, the previous graph was not finished or not
        - padding to fill 64 bytes

    */

    PRE_PACKED_STRUCTURE
    struct _packet_header
    {
        char fPacketType[8];        //packet type ('headr')
        uint32_t fDataType;         //a for audio, m for midi and s for sync
        uint32_t fDataStream;       //s for send, r for return
        uint32_t fID;               //unique ID of the slave
        uint32_t fNumPacket;        //number of data packets of the cycle
        uint32_t fPacketSize;       //packet size in bytes
        uint32_t fActivePorts;      //number of active ports
        uint32_t fCycle;            //process cycle counter
        uint32_t fSubCycle;         //midi/audio subcycle counter
        uint32_t fIsLastPckt;       //is it the last packet of a given cycle ('y' or 'n')
    } POST_PACKED_STRUCTURE;

//net timebase master

    /**
    \Brief This enum describes timebase master's type
    */

    enum _net_timebase_master
    {
        NO_CHANGE = 0,
        RELEASE_TIMEBASEMASTER = 1,
        TIMEBASEMASTER = 2,
        CONDITIONAL_TIMEBASEMASTER = 3
    };

    typedef enum _net_timebase_master net_timebase_master_t;


//transport data ******************************************************************************

    /**
    \Brief This structure contains transport data to be sent over the network
    */

    PRE_PACKED_STRUCTURE
    struct _net_transport_data
    {
        uint32_t fNewState;             //is it a state change
        uint32_t fTimebaseMaster;       //is there a new timebase master
        int32_t fState;                 //current cycle state
        jack_position_t fPosition;      //current cycle position
    } POST_PACKED_STRUCTURE;

 //midi data ***********************************************************************************

    /**
    \Brief Midi buffer and operations class

    This class is a toolset to manipulate Midi buffers.
    A JackMidiBuffer has a fixed size, which is the same than an audio buffer size.
    An intermediate fixed size buffer allows to uninterleave midi data (from jack ports).
    But for a big majority of the process cycles, this buffer is filled less than 1%,
    Sending over a network 99% of useless data seems completely unappropriate.
    The idea is to count effective midi data, and then send the smallest packet we can.
    To do it, we use an intermediate buffer.
    We have two methods to convert data from jack ports to intermediate buffer,
    And two others to convert this intermediate buffer to a network buffer (header + payload data)

    */

    class SERVER_EXPORT NetMidiBuffer
    {
        private:

            int fNPorts;
            size_t fMaxBufsize;
            int fMaxPcktSize;

            char* fBuffer;
            char* fNetBuffer;
            JackMidiBuffer** fPortBuffer;

            size_t fCycleBytesSize;  // needed size in bytes ofr an entire cycle

        public:

            NetMidiBuffer(session_params_t* params, uint32_t nports, char* net_buffer);
            ~NetMidiBuffer();

            void Reset();

            // needed size in bytes for an entire cycle
            size_t GetCycleSize();
            int GetNumPackets(int data_sizen, int max_size);

            void SetBuffer(int index, JackMidiBuffer* buffer);
            JackMidiBuffer* GetBuffer(int index);

            //utility
            void DisplayEvents();

            //jack<->buffer
            int RenderFromJackPorts();
            void RenderToJackPorts();

            //network<->buffer
            void RenderFromNetwork(int sub_cycle, size_t copy_size);
            int RenderToNetwork(int sub_cycle, size_t total_size);

    };

// audio data *********************************************************************************

    class SERVER_EXPORT NetAudioBuffer
    {

        protected:

            int fNPorts;
            int fLastSubCycle;

            char* fNetBuffer;
            sample_t** fPortBuffer;
            bool* fConnectedPorts;

            jack_nframes_t fPeriodSize;
            jack_nframes_t fSubPeriodSize;
            size_t fSubPeriodBytesSize;

            float fCycleDuration;       // in sec
            size_t fCycleBytesSize;     // needed size in bytes for an entire cycle

            int CheckPacket(int cycle, int sub_cycle);
            void NextCycle();
            void Cleanup();

        public:

            NetAudioBuffer(session_params_t* params, uint32_t nports, char* net_buffer);
            virtual ~NetAudioBuffer();

            bool GetConnected(int port_index) { return fConnectedPorts[port_index]; }
            void SetConnected(int port_index, bool state) { fConnectedPorts[port_index] = state; }

            // needed syze in bytes ofr an entire cycle
            virtual size_t GetCycleSize() = 0;

            // cycle duration in sec
            virtual float GetCycleDuration() = 0;

            virtual int GetNumPackets(int active_ports) = 0;

            virtual void SetBuffer(int index, sample_t* buffer);
            virtual sample_t* GetBuffer(int index);

            //jack<->buffer
            virtual int RenderFromJackPorts();
            virtual void RenderToJackPorts();

            //network<->buffer
            virtual int RenderFromNetwork(int cycle, int sub_cycle, uint32_t port_num) = 0;
            virtual int RenderToNetwork(int sub_cycle, uint32_t port_num) = 0;

            virtual void RenderFromNetwork(char* net_buffer, int active_port, int sub_cycle, size_t copy_size) {}
            virtual void RenderToNetwork(char* net_buffer, int active_port, int sub_cycle, size_t copy_size) {}

            virtual int ActivePortsToNetwork(char* net_buffer);
            virtual void ActivePortsFromNetwork(char* net_buffer, uint32_t port_num);

    };

    class SERVER_EXPORT NetFloatAudioBuffer : public NetAudioBuffer
    {

        private:

            int fPacketSize;

            void UpdateParams(int active_ports);

        public:

            NetFloatAudioBuffer(session_params_t* params, uint32_t nports, char* net_buffer);
            virtual ~NetFloatAudioBuffer();

            // needed size in bytes for an entire cycle
            size_t GetCycleSize();

            // cycle duration in sec
            float GetCycleDuration();
            int GetNumPackets(int active_ports);

            //jack<->buffer
            int RenderFromNetwork(int cycle, int sub_cycle, uint32_t port_num);
            int RenderToNetwork(int sub_cycle, uint32_t port_num);

            void RenderFromNetwork(char* net_buffer, int active_port, int sub_cycle);
            void RenderToNetwork(char* net_buffer, int active_port, int sub_cycle);

    };

#if HAVE_CELT

#include <celt/celt.h>

    class SERVER_EXPORT NetCeltAudioBuffer : public NetAudioBuffer
    {
        private:

            CELTMode** fCeltMode;
            CELTEncoder** fCeltEncoder;
            CELTDecoder** fCeltDecoder;

            int fCompressedSizeByte;
            int fNumPackets;

            size_t fLastSubPeriodBytesSize;

            unsigned char** fCompressedBuffer;

            void FreeCelt();

        public:

            NetCeltAudioBuffer(session_params_t* params, uint32_t nports, char* net_buffer, int kbps);
            virtual ~NetCeltAudioBuffer();

            // needed size in bytes for an entire cycle
            size_t GetCycleSize();

             // cycle duration in sec
            float GetCycleDuration();
            int GetNumPackets(int active_ports);

            //jack<->buffer
            int RenderFromJackPorts();
            void RenderToJackPorts();

            //network<->buffer
            int RenderFromNetwork(int cycle, int sub_cycle, uint32_t port_num);
            int RenderToNetwork(int sub_cycle, uint32_t  port_num);
    };

#endif

#if HAVE_OPUS

#include <opus/opus.h>
#include <opus/opus_custom.h>

    class SERVER_EXPORT NetOpusAudioBuffer : public NetAudioBuffer
    {
        private:

            OpusCustomMode** fOpusMode;
            OpusCustomEncoder** fOpusEncoder;
            OpusCustomDecoder** fOpusDecoder;

            unsigned short *fCompressedSizesByte;
            int fCompressedMaxSizeByte;
            int fNumPackets;

            size_t fLastSubPeriodBytesSize;

            unsigned char** fCompressedBuffer;
            void FreeOpus();

        public:

            NetOpusAudioBuffer(session_params_t* params, uint32_t nports, char* net_buffer, int kbps);
            virtual ~NetOpusAudioBuffer();

            // needed size in bytes for an entire cycle
            size_t GetCycleSize();

             // cycle duration in sec
            float GetCycleDuration();
            int GetNumPackets(int active_ports);

            //jack<->buffer
            int RenderFromJackPorts();
            void RenderToJackPorts();

            //network<->buffer
            int RenderFromNetwork(int cycle, int sub_cycle, uint32_t port_num);
            int RenderToNetwork(int sub_cycle, uint32_t  port_num);
    };

#endif

    class SERVER_EXPORT NetIntAudioBuffer : public NetAudioBuffer
    {
        private:

            int fCompressedSizeByte;
            int fNumPackets;

            size_t fLastSubPeriodBytesSize;

            short** fIntBuffer;

        public:

            NetIntAudioBuffer(session_params_t* params, uint32_t nports, char* net_buffer);
            virtual ~NetIntAudioBuffer();

            // needed size in bytes for an entire cycle
            size_t GetCycleSize();

             // cycle duration in sec
            float GetCycleDuration();
            int GetNumPackets(int active_ports);

            //jack<->buffer
            int RenderFromJackPorts();
            void RenderToJackPorts();

            //network<->buffer
            int RenderFromNetwork(int cycle, int sub_cycle, uint32_t port_num);
            int RenderToNetwork(int sub_cycle, uint32_t port_num);
    };

    //utility *************************************************************************************

    //socket API management
    SERVER_EXPORT int SocketAPIInit();
    SERVER_EXPORT int SocketAPIEnd();
    //n<-->h functions
    SERVER_EXPORT void SessionParamsHToN(session_params_t* src_params, session_params_t* dst_params);
    SERVER_EXPORT void SessionParamsNToH(session_params_t* src_params, session_params_t* dst_params);
    SERVER_EXPORT void PacketHeaderHToN(packet_header_t* src_header, packet_header_t* dst_header);
    SERVER_EXPORT void PacketHeaderNToH(packet_header_t* src_header, packet_header_t* dst_header);
    SERVER_EXPORT void MidiBufferHToN(JackMidiBuffer* src_buffer, JackMidiBuffer* dst_buffer);
    SERVER_EXPORT void MidiBufferNToH(JackMidiBuffer* src_buffer, JackMidiBuffer* dst_buffer);
    SERVER_EXPORT void TransportDataHToN(net_transport_data_t* src_params, net_transport_data_t* dst_params);
    SERVER_EXPORT void TransportDataNToH(net_transport_data_t* src_params, net_transport_data_t* dst_params);
    //display session parameters
    SERVER_EXPORT void SessionParamsDisplay(session_params_t* params);
    //display packet header
    SERVER_EXPORT void PacketHeaderDisplay(packet_header_t* header);
    //get the packet type from a sesion parameters
    SERVER_EXPORT sync_packet_type_t GetPacketType(session_params_t* params);
    //set the packet type in a session parameters
    SERVER_EXPORT int SetPacketType(session_params_t* params, sync_packet_type_t packet_type);
    //transport utility
    SERVER_EXPORT const char* GetTransportState(int transport_state);
    SERVER_EXPORT void NetTransportDataDisplay(net_transport_data_t* data);
}
