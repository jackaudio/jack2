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

#include "JackNetInterface.h"
#include "thread.h"
#include "jack.h"
#include "jslist.h"
#include <list>

namespace Jack
{
    class JackNetMasterManager;

    /**
    \Brief This class describes a Net Master
    */

    class JackNetMaster : public JackNetMasterInterface
    {
            friend class JackNetMasterManager;
        private:
            static int SetProcess ( jack_nframes_t nframes, void* arg );
            static void SetTimebaseCallback ( jack_transport_state_t state, jack_nframes_t nframes, jack_position_t* pos, int new_pos, void* arg );

            //jack client
            jack_client_t* fJackClient;
            const char* fClientName;

            //jack ports
            jack_port_t** fAudioCapturePorts;
            jack_port_t** fAudioPlaybackPorts;
            jack_port_t** fMidiCapturePorts;
            jack_port_t** fMidiPlaybackPorts;

            //sync and transport
            uint32_t fLastTransportState;
            net_transport_data_t fTransportData;

            //monitoring
#ifdef JACK_MONITOR
            jack_time_t fPeriodUsecs;
            JackGnuPlotMonitor<float>* fNetTimeMon;
#endif

            bool Init();
            int AllocPorts();
            void FreePorts();
            void Exit();

            //transport
            int SetTransportData();
            int DecodeTransportData();

            //sync packet
            int SetSyncPacket();
            int DecodeSyncPacket();

            int Process();
            void TimebaseCallback ( jack_position_t* pos);

        public:
            JackNetMaster ( JackNetSocket& socket, session_params_t& params, const char* multicast_ip );
            ~JackNetMaster ();

            bool IsSlaveReadyToRoll();
    };

    typedef std::list<JackNetMaster*> master_list_t;
    typedef master_list_t::iterator master_list_it_t;

    /**
    \Brief This class describer the Network Manager
    */

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
    };
}

#endif
