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

#ifndef __JackNetSlaveInterface__
#define __JackNetSlaveInterface__

#include "JackNetTool.h"

#ifdef JACK_MONITOR
#include "JackFrameTimer.h"
#endif

namespace Jack
{
    /**
    \Brief This class describes the Net Interface for slaves (NetDriver and NetAdapter)
    */

    class EXPORT JackNetSlaveInterface
    {
        protected:
            session_params_t fParams;
            char* fMulticastIP;
            JackNetSocket fSocket;
            uint fNSubProcess;

            //headers
            packet_header_t fTxHeader;
            packet_header_t fRxHeader;

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

            //sub processes
            bool Init();
            net_status_t GetNetMaster();
            net_status_t SendMasterStartSync();
            int SetParams();
            int SyncRecv();
            int SyncSend();
            int DataRecv();
            int DataSend();

            //network operations
            int Recv ( size_t size, int flags );
            int Send ( size_t size, int flags );

        public:
            JackNetSlaveInterface();
            JackNetSlaveInterface ( const char* ip, int port ) : fMulticastIP ( NULL ), fSocket ( ip, port )
            {}
            ~JackNetSlaveInterface();
    };
}

#endif
