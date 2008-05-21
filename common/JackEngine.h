/*
Copyright (C) 2004-2008 Grame

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
        JackSyncInterface* fSignal;
        jack_time_t fLastSwitchUsecs;

        int ClientCloseAux(int refnum, JackClientInterface* client, bool wait);
        void CheckXRun(jack_time_t callback_usecs);

        int NotifyAddClient(JackClientInterface* new_client, const char* name, int refnum);
        void NotifyRemoveClient(const char* name, int refnum);

        void ProcessNext(jack_time_t callback_usecs);
        void ProcessCurrent(jack_time_t callback_usecs);

        bool ClientCheckName(const char* name);
        bool GenerateUniqueName(char* name);

        int AllocateRefnum();
        void ReleaseRefnum(int ref);

        void NotifyClient(int refnum, int event, int sync, int value1, int value2);
        void NotifyClients(int event, int sync, int value1, int value2);

    public:

        JackEngine()
        {}
        JackEngine(JackGraphManager* manager, JackSynchro** table, JackEngineControl* controler);
        virtual ~JackEngine();

        virtual int Open();
        virtual int Close();

        // Client management
        virtual int ClientCheck(const char* name, char* name_res, int protocol, int options, int* status);
        virtual int ClientExternalOpen(const char* name, int pid, int* ref, int* shared_engine, int* shared_client, int* shared_graph_manager);
        virtual int ClientInternalOpen(const char* name, int* ref, JackEngineControl** shared_engine, JackGraphManager** shared_manager, JackClientInterface* client, bool wait);

        virtual int ClientExternalClose(int refnum);
        virtual int ClientInternalClose(int refnum, bool wait);

        virtual int ClientActivate(int refnum, bool state);
        virtual int ClientDeactivate(int refnum);
    
        virtual int GetClientPID(const char* name);
    
        // Internal client management
        virtual int GetInternalClientName(int int_ref, char* name_res);
        virtual int InternalClientHandle(const char* client_name, int* status, int* int_ref);
        virtual int InternalClientUnload(int refnum, int* status);

        // Port management
        virtual int PortRegister(int refnum, const char* name, const char *type, unsigned int flags, unsigned int buffer_size, unsigned int* port);
        virtual int PortUnRegister(int refnum, jack_port_id_t port);

        virtual int PortConnect(int refnum, const char* src, const char* dst);
        virtual int PortDisconnect(int refnum, const char* src, const char* dst);

        virtual int PortConnect(int refnum, jack_port_id_t src, jack_port_id_t dst);
        virtual int PortDisconnect(int refnum, jack_port_id_t src, jack_port_id_t dst);

        // Graph
        virtual bool Process(jack_time_t callback_usecs);

        // Notifications
        virtual void NotifyXRun(jack_time_t callback_usecs, float delayed_usecs);
        virtual void NotifyXRun(int refnum);
        virtual void NotifyGraphReorder();
        virtual void NotifyBufferSize(jack_nframes_t nframes);
        virtual void NotifyFreewheel(bool onoff);
        virtual void NotifyPortRegistation(jack_port_id_t port_index, bool onoff);
        virtual void NotifyPortConnect(jack_port_id_t src, jack_port_id_t dst, bool onoff);
        virtual void NotifyActivate(int refnum);
};


} // end of namespace

#endif

