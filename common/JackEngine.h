/*
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

#ifndef __JackEngine__
#define __JackEngine__

#include "JackConstants.h"
#include "JackGraphManager.h"
#include "JackSynchro.h"
#include "JackTransportEngine.h"

namespace Jack
{

class JackClientInterface;
struct JackEngineControl;
class JackServerNotifyChannelInterface;
class JackEngineTiming;
class JackExternalClient;
class JackSyncInterface;

/*!
\brief Engine description.
*/

class JackEngine
{
    private:

        JackGraphManager* fGraphManager;
        JackEngineControl* fEngineControl;
        JackClientInterface* fClientTable[CLIENT_NUM];
        JackSynchro** fSynchroTable;
        JackServerNotifyChannelInterface* fChannel;              /*! To communicate between the RT thread and server */
        JackEngineTiming* fEngineTiming;
        JackSyncInterface* fSignal;
        jack_time_t fLastSwitchUsecs;

        int ClientCloseAux(int refnum, JackClientInterface* client, bool wait);
        void CheckXRun(jack_time_t callback_usecs);
		
        int NotifyAddClient(JackClientInterface* new_client, const char* name, int refnum);
        void NotifyRemoveClient(const char* name, int refnum);
		
        bool IsZombie(JackClientInterface* client, jack_time_t current_time);
        void RemoveZombifiedClients(jack_time_t current_time);
        void GetZombifiedClients(bool clients[CLIENT_NUM], jack_time_t current_time);
		
		void ProcessNext(jack_time_t callback_usecs);
		void ProcessCurrent(jack_time_t callback_usecs);
		
		bool ClientCheckName(const char* name);
		bool GenerateUniqueName(char* name);
		
		int AllocateRefnum();
		void ReleaseRefnum(int ref);

    public:

        JackEngine(JackGraphManager* manager, JackSynchro** table, JackEngineControl* controler);
        virtual ~JackEngine();

        int Open();
        int Close();
	
        // Client management
		int ClientCheck(const char* name, char* name_res, int protocol, int options, int* status);
        int ClientExternalOpen(const char* name, int* ref, int* shared_engine, int* shared_client, int* shared_graph_manager);
        int ClientInternalOpen(const char* name, int* ref, JackEngineControl** shared_engine, JackGraphManager** shared_manager, JackClientInterface* client, bool wait);

        int ClientExternalClose(int refnum);
        int ClientInternalClose(int refnum, bool wait);
    
        int ClientActivate(int refnum);
        int ClientDeactivate(int refnum);
		
		// Internal lient management
		int GetInternalClientName(int int_ref, char* name_res);
		int InternalClientHandle(const char* client_name, int* status, int* int_ref);
		int InternalClientUnload(int refnum, int* status);

        // Port management
        int PortRegister(int refnum, const char* name, unsigned int flags, unsigned int buffer_size, jack_port_id_t* port_index);
        int PortUnRegister(int refnum, jack_port_id_t port);

        int PortConnect(int refnum, const char* src, const char* dst);
        int PortDisconnect(int refnum, const char* src, const char* dst);

        int PortConnect(int refnum, jack_port_id_t src, jack_port_id_t dst);
        int PortDisconnect(int refnum, jack_port_id_t src, jack_port_id_t dst);

        // Transport management
        int ReleaseTimebase(int refnum);
        int SetTimebaseCallback(int refnum, int conditional);

        // Graph
        bool Process(jack_time_t callback_usecs);
        void ZombifyClient(int refnum);

        // Notifications
        void NotifyClient(int refnum, int event, int sync, int value);
        void NotifyClients(int event, int sync, int value);
        void NotifyXRun(jack_time_t callback_usecs);
        void NotifyXRun(int refnum);
        void NotifyGraphReorder();
        void NotifyBufferSize(jack_nframes_t nframes);
        void NotifyFreewheel(bool onoff);
        void NotifyPortRegistation(jack_port_id_t port_index, bool onoff);
		void NotifyActivate(int refnum);
};


} // end of namespace

#endif

