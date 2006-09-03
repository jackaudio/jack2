/*
Copyright (C) 2001 Paul Davis
Copyright (C) 2004-2006 Grame  

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

#ifndef __JackGraphManager__
#define __JackGraphManager__

#include "JackShmMem.h"
#include "JackPort.h"
#include "JackConstants.h"
#include "JackConnectionManager.h"
#include "JackAtomicState.h"

namespace Jack
{

/*!
\brief Graph manager: contains the connection manager and the port array.
*/

class JackGraphManager : public JackShmMem, public JackAtomicState<JackConnectionManager>
{

    private:

        JackPort fPortArray[PORT_NUM];   
		JackClientTiming fClientTiming[CLIENT_NUM];

        jack_port_id_t AllocatePortAux(int refnum, const char* port_name, JackPortFlags flags);
        void GetConnectionsAux(JackConnectionManager* manager, const char** res, jack_port_id_t port_index);
        const char** GetPortsAux(const char* port_name_pattern, const char* type_name_pattern, unsigned long flags);
        float* GetBuffer(jack_port_id_t port_index);
        void* GetBufferAux(JackConnectionManager* manager, jack_port_id_t port_index, jack_nframes_t frames);
        jack_nframes_t GetTotalLatencyAux(jack_port_id_t port_index, jack_port_id_t src_port_index, JackConnectionManager* manager, int hop_count);

    public:

        JackGraphManager()
        {}
        virtual ~JackGraphManager()
        {}

        // Ports management
        jack_port_id_t AllocatePort(int refnum, const char* port_name, JackPortFlags flags);  
        void ReleasePort(jack_port_id_t port_index);	
        JackPort* GetPort(jack_port_id_t index); 
        jack_port_id_t GetPort(const char* name); 
        jack_nframes_t GetTotalLatency(jack_port_id_t port_index); 
        int RequestMonitor(jack_port_id_t port_index, bool onoff); 

        // Connections management
        int Connect(jack_port_id_t src_index, jack_port_id_t dst_index);  
        int Disconnect(jack_port_id_t src_index, jack_port_id_t dst_index); 
        int GetConnectionsNum(jack_port_id_t port_index); 

        int ConnectedTo(jack_port_id_t port_src, const char* port_name); 
        const char** GetConnections(jack_port_id_t port_index); 
        const char** GetPorts(const char* port_name_pattern, const char* type_name_pattern, unsigned long flags);

        int CheckPorts(const char* src, const char* dst, jack_port_id_t* src_index, jack_port_id_t* dst_index); 
        int CheckPorts(jack_port_id_t port_src, jack_port_id_t port_dst); 
        int CheckPort(jack_port_id_t port_index); 

        void DisconnectAllInput(jack_port_id_t port_index); 
        void DisconnectAllOutput(jack_port_id_t port_index); 
        int DisconnectAll(jack_port_id_t port_index); 

        // Client management
        int AllocateRefNum(); 
        void ReleaseRefNum(int refnum); 

        bool IsDirectConnection(int ref1, int ref2); 
        void DirectConnect(int ref1, int ref2); 
        void DirectDisconnect(int ref1, int ref2); 

        int RemovePort(int refnum, jack_port_id_t port_index); 
        void RemoveAllPorts(int refnum); 
        void DisconnectAllPorts(int refnum); 

        int GetInputRefNum(jack_port_id_t port_index); 
        int GetOutputRefNum(jack_port_id_t port_index); 

        // Buffer management
        void* GetBuffer(jack_port_id_t port_index, jack_nframes_t frames); 

        // Activation management
        void RunCurrentGraph();  
        bool RunNextGraph();  
        bool IsFinishedGraph();  

        int ResumeRefNum(JackClientControl* control, JackSynchro** table);  
        int SuspendRefNum(JackClientControl* control, JackSynchro** table, long usecs); 
		
		JackClientTiming* GetClientTiming(int ref);
		
        void Save(JackConnectionManager* dst);
        void Restore(JackConnectionManager* src);

};


} // end of namespace

#endif

