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

#ifndef __JackClient__
#define __JackClient__

#include "JackClientInterface.h"
#include "JackThread.h"
#include "JackConstants.h"
#include "JackSynchro.h"
#include "types.h"
#include "transport_types.h"
#include "varargs.h"
#include <list>

namespace Jack
{

class JackClientChannelInterface;
class JackGraphManager;
class JackServer;
class JackEngine;
class JackSynchro;
struct JackClientControl;
struct JackEngineControl;
class JackSyncInterface;

typedef void (*JackShutdownCallback)(void *arg);

/*!
\brief The base class for clients: share part of the implementation for JackInternalClient and JackLibClient.
*/

class JackClient : public JackClientInterface, public JackRunnableInterface
{

    protected:

        JackProcessCallback fProcess;
        JackGraphOrderCallback fGraphOrder;
        JackXRunCallback fXrun;
        JackShutdownCallback fShutdown;
        JackThreadInitCallback fInit;
        JackBufferSizeCallback fBufferSize;
		JackClientRegistrationCallback fClientRegistration;
        JackFreewheelCallback fFreewheel;
        JackPortRegistrationCallback fPortRegistration;
        JackTimebaseCallback fTimebase;
        JackSyncCallback fSync;
        void* fProcessArg;
        void* fGraphOrderArg;
        void* fXrunArg;
        void* fShutdownArg;
        void* fInitArg;
        void* fBufferSizeArg;
		void* fClientRegistrationArg;
        void* fFreewheelArg;
        void* fPortRegistrationArg;
        void* fTimebaseArg;
        void* fSyncArg;
        int fConditionnal;
		char fServerName[64];

        JackThread*	fThread;    /*! Thread to execute the Process function */
        JackClientChannelInterface* fChannel;
        JackSynchro** fSynchroTable;
        std::list<jack_port_id_t> fPortList;

        int StartThread();
        void SetupDriverSync(bool freewheel);
        bool IsActive();

        bool CallProcessCallback();
        void CallSyncCallback();
        void CallTimebaseCallback();
        int RequestNewPos(jack_position_t* pos);

        virtual int ClientNotifyImp(int refnum, const char* name, int notify, int sync, int value);

    public:

        JackClient();
        JackClient(JackSynchro** table);
        virtual ~JackClient();

        virtual int Open(const char* server_name, const char* name, jack_options_t options, jack_status_t* status) = 0;
        virtual int Close();

        virtual JackGraphManager* GetGraphManager() const = 0;
        virtual JackEngineControl* GetEngineControl() const = 0;

        // Notifications
        virtual int ClientNotify(int refnum, const char* name, int notify, int sync, int value);

        virtual int Activate();
        virtual int Deactivate();

        // Context
        virtual int SetBufferSize(jack_nframes_t buffer_size);
        virtual int SetFreeWheel(int onoff);
        virtual void ShutDown();
        virtual pthread_t GetThreadID();

        // Port management
        virtual int PortRegister(const char* port_name, const char* port_type, unsigned long flags, unsigned long buffer_size);
        virtual int PortUnRegister(jack_port_id_t port);

        virtual int PortConnect(const char* src, const char* dst);
        virtual int PortDisconnect(const char* src, const char* dst);
        virtual int PortConnect(jack_port_id_t src, jack_port_id_t dst);
        virtual int PortDisconnect(jack_port_id_t src);

        virtual int PortIsMine(jack_port_id_t port_index);

        // Transport
        virtual int ReleaseTimebase();
        virtual int SetSyncCallback(JackSyncCallback sync_callback, void* arg);
        virtual int SetSyncTimeout(jack_time_t timeout);
        virtual int SetTimebaseCallback(int conditional, JackTimebaseCallback timebase_callback, void* arg);
        virtual int TransportLocate(jack_nframes_t frame);
        virtual jack_transport_state_t TransportQuery(jack_position_t* pos);
        virtual jack_nframes_t GetCurrentTransportFrame();
        virtual int TransportReposition(jack_position_t* pos);
        virtual void TransportStart();
        virtual void TransportStop();

        // Callbacks
        virtual void OnShutdown(JackShutdownCallback callback, void *arg);
        virtual int SetProcessCallback(JackProcessCallback callback, void* arg);
        virtual int SetXRunCallback(JackXRunCallback callback, void* arg);
        virtual int SetInitCallback(JackThreadInitCallback callback, void* arg);
        virtual int SetGraphOrderCallback(JackGraphOrderCallback callback, void* arg);
        virtual int SetBufferSizeCallback(JackBufferSizeCallback callback, void* arg);
		virtual int SetClientRegistrationCallback(JackClientRegistrationCallback callback, void* arg);
        virtual int SetFreewheelCallback(JackFreewheelCallback callback, void* arg);
        virtual int SetPortRegistrationCallback(JackPortRegistrationCallback callback, void* arg);
		
		// Internal clients
		virtual char* GetInternalClientName(int ref);
		virtual int InternalClientHandle(const char* client_name, jack_status_t* status);
		virtual int InternalClientLoad(const char* client_name, jack_options_t options, jack_status_t* status, jack_varargs_t* va);
		virtual void InternalClientUnload(int ref, jack_status_t* status);

        // JackRunnableInterface interface
        bool Init();
        bool Execute();
};

// Each "side" server and client will implement this to get the shared graph manager, engine control and inter-process synchro table.
extern JackGraphManager* GetGraphManager();
extern JackEngineControl* GetEngineControl();
extern JackSynchro** GetSynchroTable();

} // end of namespace

#endif
