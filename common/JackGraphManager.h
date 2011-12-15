/*
Copyright (C) 2001 Paul Davis
Copyright (C) 2004-2008 Grame

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#ifndef __JackGraphManager__
#define __JackGraphManager__

#include "JackShmMem.h"
#include "JackPort.h"
#include "JackConstants.h"
#include "JackConnectionManager.h"
#include "JackAtomicState.h"
#include "JackPlatformPlug.h"
#include "JackSystemDeps.h"

namespace Jack
{

/*!
\brief Graph manager: contains the connection manager and the port array.
*/

PRE_PACKED_STRUCTURE
class SERVER_EXPORT JackGraphManager : public JackShmMem, public JackAtomicState<JackConnectionManager>
{

    private:

        unsigned int fPortMax;
        JackClientTiming fClientTiming[CLIENT_NUM];
        JackPort fPortArray[0];    // The actual size depends of port_max, it will be dynamically computed and allocated using "placement" new

        void AssertPort(jack_port_id_t port_index);
        jack_port_id_t AllocatePortAux(int refnum, const char* port_name, const char* port_type, JackPortFlags flags);
        void GetConnectionsAux(JackConnectionManager* manager, const char** res, jack_port_id_t port_index);
        void GetPortsAux(const char** matching_ports, const char* port_name_pattern, const char* type_name_pattern, unsigned long flags);
        jack_default_audio_sample_t* GetBuffer(jack_port_id_t port_index);
        void* GetBufferAux(JackConnectionManager* manager, jack_port_id_t port_index, jack_nframes_t frames);
        jack_nframes_t ComputeTotalLatencyAux(jack_port_id_t port_index, jack_port_id_t src_port_index, JackConnectionManager* manager, int hop_count);
        void RecalculateLatencyAux(jack_port_id_t port_index, jack_latency_callback_mode_t mode);

    public:

        JackGraphManager(int port_max);
        ~JackGraphManager()
        {}

        void SetBufferSize(jack_nframes_t buffer_size);

        // Ports management
        jack_port_id_t AllocatePort(int refnum, const char* port_name, const char* port_type, JackPortFlags flags, jack_nframes_t buffer_size);
        int ReleasePort(int refnum, jack_port_id_t port_index);
        void GetInputPorts(int refnum, jack_int_t* res);
        void GetOutputPorts(int refnum, jack_int_t* res);
        void RemoveAllPorts(int refnum);
        void DisconnectAllPorts(int refnum);

        JackPort* GetPort(jack_port_id_t index);
        jack_port_id_t GetPort(const char* name);

        int ComputeTotalLatency(jack_port_id_t port_index);
        int ComputeTotalLatencies();
        void RecalculateLatency(jack_port_id_t port_index, jack_latency_callback_mode_t mode);

        int RequestMonitor(jack_port_id_t port_index, bool onoff);

        // Connections management
        int Connect(jack_port_id_t src_index, jack_port_id_t dst_index);
        int Disconnect(jack_port_id_t src_index, jack_port_id_t dst_index);
        int IsConnected(jack_port_id_t port_src, jack_port_id_t port_dst);

        // RT, client
        int GetConnectionsNum(jack_port_id_t port_index)
        {
            JackConnectionManager* manager = ReadCurrentState();
            return manager->Connections(port_index);
        }

        const char** GetConnections(jack_port_id_t port_index);
        void GetConnections(jack_port_id_t port_index, jack_int_t* connections);  // TODO
        const char** GetPorts(const char* port_name_pattern, const char* type_name_pattern, unsigned long flags);

        int GetTwoPorts(const char* src, const char* dst, jack_port_id_t* src_index, jack_port_id_t* dst_index);
        int CheckPorts(jack_port_id_t port_src, jack_port_id_t port_dst);

        void DisconnectAllInput(jack_port_id_t port_index);
        void DisconnectAllOutput(jack_port_id_t port_index);
        int DisconnectAll(jack_port_id_t port_index);

        bool IsDirectConnection(int ref1, int ref2);
        void DirectConnect(int ref1, int ref2);
        void DirectDisconnect(int ref1, int ref2);

        void Activate(int refnum);
        void Deactivate(int refnum);

        int GetInputRefNum(jack_port_id_t port_index);
        int GetOutputRefNum(jack_port_id_t port_index);

        // Buffer management
        void* GetBuffer(jack_port_id_t port_index, jack_nframes_t frames);

        // Activation management
        void RunCurrentGraph();
        bool RunNextGraph();
        bool IsFinishedGraph();

        void InitRefNum(int refnum);
        int ResumeRefNum(JackClientControl* control, JackSynchro* table);
        int SuspendRefNum(JackClientControl* control, JackSynchro* table, long usecs);
        void TopologicalSort(std::vector<jack_int_t>& sorted);

        JackClientTiming* GetClientTiming(int refnum)
        {
            return &fClientTiming[refnum];
        }

        void Save(JackConnectionManager* dst);
        void Restore(JackConnectionManager* src);

        static JackGraphManager* Allocate(int port_max);
        static void Destroy(JackGraphManager* manager);

} POST_PACKED_STRUCTURE;


} // end of namespace

#endif

