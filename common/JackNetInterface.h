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

#ifndef __JackNetInterface__
#define __JackNetInterface__

#include "JackNetTool.h"
#include <limits.h>

namespace Jack
{

#define DEFAULT_MULTICAST_IP "225.3.19.154"
#define DEFAULT_PORT    19000
#define DEFAULT_MTU     1500

#define SLAVE_SETUP_RETRY   5

#define MANAGER_INIT_TIMEOUT    2000000         // in usec
#define MASTER_INIT_TIMEOUT     1000000 * 10    // in usec
#define SLAVE_INIT_TIMEOUT      1000000 * 10    // in usec
#define PACKET_TIMEOUT          500000          // in usec

#define NETWORK_MAX_LATENCY     20

    /**
    \Brief This class describes the basic Net Interface, used by both master and slave.
    */

    class SERVER_EXPORT JackNetInterface
    {

        protected:
        
            bool fSetTimeOut;

            void Initialize();

            session_params_t fParams;
            JackNetSocket fSocket;
            char fMulticastIP[32];

            // headers
            packet_header_t fTxHeader;
            packet_header_t fRxHeader;

            // transport
            net_transport_data_t fSendTransportData;
            net_transport_data_t fReturnTransportData;

            // network buffers
            char* fTxBuffer;
            char* fRxBuffer;
            char* fTxData;
            char* fRxData;

            // JACK buffers
            NetMidiBuffer* fNetMidiCaptureBuffer;
            NetMidiBuffer* fNetMidiPlaybackBuffer;
            NetAudioBuffer* fNetAudioCaptureBuffer;
            NetAudioBuffer* fNetAudioPlaybackBuffer;

            // utility methods
            int SetNetBufferSize();
            void FreeNetworkBuffers();

            // virtual methods : depends on the sub class master/slave
            virtual bool SetParams();
            virtual bool Init() = 0;

            // transport
            virtual void EncodeTransportData() = 0;
            virtual void DecodeTransportData() = 0;

            // sync packet
            virtual void EncodeSyncPacket() = 0;
            virtual void DecodeSyncPacket() = 0;

            virtual int SyncRecv() = 0;
            virtual int SyncSend() = 0;
            virtual int DataRecv() = 0;
            virtual int DataSend() = 0;

            virtual int Send(size_t size, int flags) = 0;
            virtual int Recv(size_t size, int flags) = 0;

            virtual void FatalRecvError() = 0;
            virtual void FatalSendError() = 0;

            int MidiSend(NetMidiBuffer* buffer, int midi_channnels, int audio_channels);
            int AudioSend(NetAudioBuffer* buffer, int audio_channels);

            int MidiRecv(packet_header_t* rx_head, NetMidiBuffer* buffer, uint& recvd_midi_pckt);
            int AudioRecv(packet_header_t* rx_head, NetAudioBuffer* buffer);

            int FinishRecv(NetAudioBuffer* buffer);
            
            void SetRcvTimeOut();

            NetAudioBuffer* AudioBufferFactory(int nports, char* buffer);

        public:

            JackNetInterface();
            JackNetInterface(const char* multicast_ip, int port);
            JackNetInterface(session_params_t& params, JackNetSocket& socket, const char* multicast_ip);

            virtual ~JackNetInterface();

    };

    /**
    \Brief This class describes the Net Interface for masters (NetMaster)
    */

    class SERVER_EXPORT JackNetMasterInterface : public JackNetInterface
    {

        protected:

            bool fRunning;
            int fCurrentCycleOffset;
            int fMaxCycleOffset;
       
            bool Init();
            bool SetParams();

            void Exit();

            int SyncRecv();
            int SyncSend();

            int DataRecv();
            int DataSend();

            // sync packet
            void EncodeSyncPacket();
            void DecodeSyncPacket();

            int Send(size_t size, int flags);
            int Recv(size_t size, int flags);

            bool IsSynched();

            void FatalRecvError();
            void FatalSendError();

        public:

            JackNetMasterInterface() : JackNetInterface(), fRunning(false), fCurrentCycleOffset(0), fMaxCycleOffset(0)
            {}
            JackNetMasterInterface(session_params_t& params, JackNetSocket& socket, const char* multicast_ip)
                    : JackNetInterface(params, socket, multicast_ip), fRunning(false), fCurrentCycleOffset(0), fMaxCycleOffset(0)
            {}

            virtual~JackNetMasterInterface()
            {}
    };

    /**
    \Brief This class describes the Net Interface for slaves (NetDriver and NetAdapter)
    */

    class SERVER_EXPORT JackNetSlaveInterface : public JackNetInterface
    {

        protected:

            static uint fSlaveCounter;

            bool Init();
            bool InitConnection(int time_out_sec);
            bool InitRendering();

            net_status_t SendAvailableToMaster(long count = LONG_MAX);  // long here (and not int...)
            net_status_t SendStartToMaster();

            bool SetParams();

            int SyncRecv();
            int SyncSend();

            int DataRecv();
            int DataSend();

            // sync packet
            void EncodeSyncPacket();
            void DecodeSyncPacket();

            int Recv(size_t size, int flags);
            int Send(size_t size, int flags);

            void FatalRecvError();
            void FatalSendError();

            void InitAPI();

        public:

            JackNetSlaveInterface() : JackNetInterface()
            {
                InitAPI();
            }

            JackNetSlaveInterface(const char* ip, int port) : JackNetInterface(ip, port)
            {
                InitAPI();
            }

            virtual ~JackNetSlaveInterface()
            {
                // close Socket API with the last slave
                if (--fSlaveCounter == 0) {
                    SocketAPIEnd();
                }
            }
    };
}

#endif
