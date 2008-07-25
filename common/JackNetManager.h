/*
Copyright (C) 2008 Grame

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

#ifndef __JACKNETMANAGER_H__
#define __JACKNETMANAGER_H__

#include "JackNetTool.h"
#include "thread.h"
#include "jack.h"
#include "jslist.h"
#include <list>

namespace Jack
{
    class JackNetMasterManager;

    class JackNetMaster
    {
            friend class JackNetMasterManager;
        private:
            static int SetProcess ( jack_nframes_t nframes, void* arg );

            JackNetMasterManager* fMasterManager;
            session_params_t fParams;
            JackNetSocket fSocket;
            uint fNSubProcess;
            int fNetJumpCnt;
            bool fRunning;

            //jack client
            jack_client_t* fJackClient;
            const char* fClientName;

            //jack ports
            jack_port_t** fAudioCapturePorts;
            jack_port_t** fAudioPlaybackPorts;
            jack_port_t** fMidiCapturePorts;
            jack_port_t** fMidiPlaybackPorts;

            //sync and transport
            int fSyncState;
            net_transport_data_t fTransportData;

            //network headers
            packet_header_t fTxHeader;
            packet_header_t fRxHeader;

            //network buffers
            char* fTxBuffer;
            char* fRxBuffer;
            char* fTxData;
            char* fRxData;

            //jack buffers
            NetAudioBuffer* fNetAudioCaptureBuffer;
            NetAudioBuffer* fNetAudioPlaybackBuffer;
            NetMidiBuffer* fNetMidiCaptureBuffer;
            NetMidiBuffer* fNetMidiPlaybackBuffer;

            //sizes
            int fAudioTxLen;
            int fAudioRxLen;
            int fPayloadSize;

            //monitoring
#ifdef JACK_MONITOR
            static uint fMeasureCnt;
            static uint fMeasurePoints;
            static uint fMonitorPlotOptionsCnt;
            static std::string fMonitorPlotOptions[];
            static std::string fMonitorFieldNames[];
            jack_time_t fPeriodUsecs;
            float* fMeasure;
            int fMeasureId;
            JackGnuPlotMonitor<float>* fMonitor;
#endif

            bool Init();
            void FreePorts();
            void Exit();

            int SetSyncPacket();

            int Send ( char* buffer, size_t size, int flags );
            int Recv ( size_t size, int flags );
            int Process();
        public:
            JackNetMaster ( JackNetMasterManager* manager, session_params_t& params );
            ~JackNetMaster ();
    };

    typedef std::list<JackNetMaster*> master_list_t;
    typedef master_list_t::iterator master_list_it_t;

    class JackNetMasterManager
    {
            friend class JackNetMaster;
        private:
            static int SetSyncCallback ( jack_transport_state_t state, jack_position_t* pos, void* arg );
            static void* NetManagerThread ( void* arg );

            jack_client_t* fManagerClient;
            const char* fManagerName;
            const char* fMulticastIP;
            JackNetSocket fSocket;
            pthread_t fManagerThread;
            master_list_t fMasterList;
            uint32_t fGlobalID;
            bool fRunning;

            void Run();
            JackNetMaster* MasterInit ( session_params_t& params );
            master_list_it_t FindMaster ( uint32_t client_id );
            int KillMaster ( session_params_t* params );
            void SetSlaveName ( session_params_t& params );

            int SyncCallback ( jack_transport_state_t state, jack_position_t* pos );
        public:
            JackNetMasterManager ( jack_client_t* jack_client, const JSList* params );
            ~JackNetMasterManager();

            void Exit();
    };
}

#endif
