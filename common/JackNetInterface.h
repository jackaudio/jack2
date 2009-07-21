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

#ifndef __JackNetInterface__
#define __JackNetInterface__

#include "JackNetTool.h"

namespace Jack
{
    /**
    \Brief This class describes the basic Net Interface, used by both master and slave
    */

    class SERVER_EXPORT JackNetInterface
    {
        protected:
            session_params_t fParams;
            JackNetSocket fSocket;
            char fMulticastIP[32];
            uint fNSubProcess;

            //headers
            packet_header_t fTxHeader;
            packet_header_t fRxHeader;
            
            // transport
            net_transport_data_t fSendTransportData;
            net_transport_data_t fReturnTransportData;

            //network buffers
            char* fTxBuffer;
            char* fRxBuffer;
            char* fTxData;
            char* fRxData;

            //jack buffers
            NetMidiBuffer* fNetMidiCaptureBuffer;
            NetMidiBuffer* fNetMidiPlaybackBuffer;
            NetAudioBuffer* fNetAudioCaptureBuffer;
            NetAudioBuffer* fNetAudioPlaybackBuffer;

            //sizes
            int fAudioRxLen;
            int fAudioTxLen;
            int fPayloadSize;

            //utility methods
            void SetFramesPerPacket();
            int SetNetBufferSize();
            int GetNMidiPckt();
            bool IsNextPacket();

            //virtual methods : depends on the sub class master/slave
            virtual void SetParams();
            virtual bool Init() = 0;

            //transport
            virtual void EncodeTransportData() = 0;
            virtual void DecodeTransportData() = 0;

            //sync packet
            virtual void EncodeSyncPacket() = 0;
            virtual void DecodeSyncPacket() = 0;

            virtual int SyncRecv() = 0;
            virtual int SyncSend() = 0;
            virtual int DataRecv() = 0;
            virtual int DataSend() = 0;

            virtual int Send ( size_t size, int flags ) = 0;
            virtual int Recv ( size_t size, int flags ) = 0;

            JackNetInterface();
            JackNetInterface ( const char* multicast_ip, int port );
            JackNetInterface ( session_params_t& params, JackNetSocket& socket, const char* multicast_ip );

        public:
            virtual ~JackNetInterface();
    };

    /**
    \Brief This class describes the Net Interface for masters (NetMaster)
    */

    class SERVER_EXPORT JackNetMasterInterface : public JackNetInterface
    {
        protected:
            bool fRunning;
            int fCycleOffset;

            bool Init();
            int SetRxTimeout();
            void SetParams();
            
            void Exit();
            
            int SyncRecv();
            int SyncSend();
            
            int DataRecv();
            int DataSend();
            
             //sync packet
            void EncodeSyncPacket();
            void DecodeSyncPacket();

            int Send ( size_t size, int flags );
            int Recv ( size_t size, int flags );
            
            bool IsSynched();

        public:
            JackNetMasterInterface() : JackNetInterface(), fRunning(false), fCycleOffset(0)
            {}
            JackNetMasterInterface ( session_params_t& params, JackNetSocket& socket, const char* multicast_ip )
                    : JackNetInterface ( params, socket, multicast_ip )
            {}
            ~JackNetMasterInterface()
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
            bool InitConnection();
            bool InitRendering();
            
            net_status_t SendAvailableToMaster();
            net_status_t SendStartToMaster();
            
            void SetParams();
            
            int SyncRecv();
            int SyncSend();
            
            int DataRecv();
            int DataSend();
            
            //sync packet
            void EncodeSyncPacket();
            void DecodeSyncPacket();

            int Recv ( size_t size, int flags );
            int Send ( size_t size, int flags );

        public:
            JackNetSlaveInterface() : JackNetInterface()
            {
                //open Socket API with the first slave
                if ( fSlaveCounter++ == 0 )
                {
                    if ( SocketAPIInit() < 0 )
                    {
                        jack_error ( "Can't init Socket API, exiting..." );
                        throw -1;
                    }
                }
            }
            JackNetSlaveInterface ( const char* ip, int port ) : JackNetInterface ( ip, port )
            {
                //open Socket API with the first slave
                if ( fSlaveCounter++ == 0 )
                {
                    if ( SocketAPIInit() < 0 )
                    {
                        jack_error ( "Can't init Socket API, exiting..." );
                        throw -1;
                    }
                }
            }
            ~JackNetSlaveInterface()
            {
                //close Socket API with the last slave
                if ( --fSlaveCounter == 0 )
                    SocketAPIEnd();
            }
    };
}

#define DEFAULT_MULTICAST_IP "225.3.19.154"
#define DEFAULT_PORT 19000
#define DEFAULT_MTU 1500

#define SLAVE_SETUP_RETRY 5

#define MASTER_INIT_TIMEOUT 1000000     // in usec
#define SLAVE_INIT_TIMEOUT 2000000      // in usec

#define MAX_LATENCY 6

#endif
