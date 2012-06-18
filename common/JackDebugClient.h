/*
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

#ifndef __JackDebugClient__
#define __JackDebugClient__

#define MAX_PORT_HISTORY 2048

#include "JackClient.h"
#include <list>
#include <fstream>

namespace Jack
{

/*!
\brief Follow a single port.
*/

typedef struct
{
    jack_port_id_t idport;
    char name[JACK_PORT_NAME_SIZE]; //portname
    int IsConnected;
    int IsUnregistered;
}
PortFollower;

/*!
\brief A "decorator" debug client to validate API use.
*/

class JackDebugClient : public JackClient
{
    protected:

        JackClient* fClient;
        std::ofstream* fStream;
        PortFollower fPortList[MAX_PORT_HISTORY]; // Arbitrary value... To be tuned...
        int fTotalPortNumber;   // The total number of port opened and maybe closed. Historical view.
        int fOpenPortNumber;    // The current number of opened port.
        int fIsActivated;
        int fIsDeactivated;
        int fIsClosed;
        bool fFreewheel;
        char fClientName[JACK_CLIENT_NAME_SIZE + 1];
        JackProcessCallback fProcessTimeCallback;
        void* fProcessTimeCallbackArg;

    public:

        JackDebugClient(JackClient* fTheClient);
        virtual ~JackDebugClient();

        virtual int Open(const char* server_name, const char* name, int uuid, jack_options_t options, jack_status_t* status);
        int Close();

        virtual JackGraphManager* GetGraphManager() const;
        virtual JackEngineControl* GetEngineControl() const;

        // Notifications
        int ClientNotify(int refnum, const char* name, int notify, int sync, const char* message, int value1, int value2);

        int Activate();
        int Deactivate();

        // Context
        int SetBufferSize(jack_nframes_t buffer_size);
        int SetFreeWheel(int onoff);
        int ComputeTotalLatencies();
        void ShutDown();
        jack_native_thread_t GetThreadID();

        // Port management
        int PortRegister(const char* port_name, const char* port_type, unsigned long flags, unsigned long buffer_size);
        int PortUnRegister(jack_port_id_t port);

        int PortConnect(const char* src, const char* dst);
        int PortDisconnect(const char* src, const char* dst);
        int PortDisconnect(jack_port_id_t src);

        int PortIsMine(jack_port_id_t port_index);
        int PortRename(jack_port_id_t port_index, const char* name);

        // Transport
        int ReleaseTimebase();
        int SetSyncCallback(JackSyncCallback sync_callback, void* arg);
        int SetSyncTimeout(jack_time_t timeout);
        int SetTimebaseCallback(int conditional, JackTimebaseCallback timebase_callback, void* arg);
        void TransportLocate(jack_nframes_t frame);
        jack_transport_state_t TransportQuery(jack_position_t* pos);
        jack_nframes_t GetCurrentTransportFrame();
        int TransportReposition(jack_position_t* pos);
        void TransportStart();
        void TransportStop();

        // Callbacks
        void OnShutdown(JackShutdownCallback callback, void *arg);
        void OnInfoShutdown(JackInfoShutdownCallback callback, void *arg);
        int SetProcessCallback(JackProcessCallback callback, void* arg);
        int SetXRunCallback(JackXRunCallback callback, void* arg);
        int SetInitCallback(JackThreadInitCallback callback, void* arg);
        int SetGraphOrderCallback(JackGraphOrderCallback callback, void* arg);
        int SetBufferSizeCallback(JackBufferSizeCallback callback, void* arg);
        int SetClientRegistrationCallback(JackClientRegistrationCallback callback, void* arg);
        int SetFreewheelCallback(JackFreewheelCallback callback, void* arg);
        int SetPortRegistrationCallback(JackPortRegistrationCallback callback, void* arg);
        int SetPortConnectCallback(JackPortConnectCallback callback, void *arg);
        int SetPortRenameCallback(JackPortRenameCallback callback, void *arg);
        int SetSessionCallback(JackSessionCallback callback, void *arg);
        int SetLatencyCallback(JackLatencyCallback callback, void *arg);

        // Internal clients
        char* GetInternalClientName(int ref);
        int InternalClientHandle(const char* client_name, jack_status_t* status);
        int InternalClientLoad(const char* client_name, jack_options_t options, jack_status_t* status, jack_varargs_t* va);
        void InternalClientUnload(int ref, jack_status_t* status);
        
        // RT Thread
        int SetProcessThread(JackThreadCallback fun, void *arg);

        // Session API
        jack_session_command_t* SessionNotify(const char* target, jack_session_event_type_t type, const char* path);
        int SessionReply(jack_session_event_t* ev);
        char* GetUUIDForClientName(const char* client_name);
        char* GetClientNameByUUID(const char* uuid);
        int ReserveClientName(const char* client_name, const char* uuid);
        int ClientHasSessionCallback(const char* client_name);

        JackClientControl* GetClientControl() const;
        void CheckClient(const char* function_name) const;

        static int TimeCallback(jack_nframes_t nframes, void *arg);
};


} // end of namespace

#endif
