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

#ifndef __JackDebugClient__
#define __JackDebugClient__

#define MAX_PORT_HISTORY 2048

#include "JackClient.h"
#include <list>

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
    int IsUnregistrated;
}
PortFollower;

/*!
\brief A "decorator" debug client to validate API use.
*/

class JackDebugClient : public JackClient
{
    private:

        JackClient* fClient;
        std::ofstream* fStream;

    protected:
	
        PortFollower fPortList[MAX_PORT_HISTORY]; // Arbitrary value... To be tuned...
        int fTotalPortNumber;	// The total number of port opened and maybe closed. Historical view.
        int fOpenPortNumber;	// The current number of opened port.
        int fIsActivated;
        int fIsDeactivated;
        int fIsClosed;
        char fClientName[JACK_CLIENT_NAME_SIZE];

    public:

        JackDebugClient(JackClient *fTheClient);
        virtual ~JackDebugClient();

        virtual int Open(const char* name);
        int Close();

        virtual JackGraphManager* GetGraphManager() const;
        virtual JackEngineControl* GetEngineControl() const;

        // Notifications
        int ClientNotify(int refnum, const char* name, int notify, int sync, int value);

        int Activate();
        int Deactivate();

        // Context
        int SetBufferSize(jack_nframes_t buffer_size);
        int SetFreeWheel(int onoff);
        void ShutDown();
        pthread_t GetThreadID();

        // Port management
        int PortRegister(const char* port_name, const char* port_type, unsigned long flags, unsigned long buffer_size);
        int PortUnRegister(jack_port_id_t port);

        int PortConnect(const char* src, const char* dst);
        int PortDisconnect(const char* src, const char* dst);
        int PortConnect(jack_port_id_t src, jack_port_id_t dst);
        int PortDisconnect(jack_port_id_t src);

        int PortIsMine(jack_port_id_t port_index);

        // Transport
        int ReleaseTimebase();
        int SetSyncCallback(JackSyncCallback sync_callback, void* arg);
        int SetSyncTimeout(jack_time_t timeout);
        int SetTimebaseCallback(int conditional, JackTimebaseCallback timebase_callback, void* arg);
        int TransportLocate(jack_nframes_t frame);
        jack_transport_state_t TransportQuery(jack_position_t* pos);
        jack_nframes_t GetCurrentTransportFrame();
        int TransportReposition(jack_position_t* pos);
        void TransportStart();
        void TransportStop();

        // Callbacks
        void OnShutdown(JackShutdownCallback callback, void *arg);
        int SetProcessCallback(JackProcessCallback callback, void* arg);
        int SetXRunCallback(JackXRunCallback callback, void* arg);
        int SetInitCallback(JackThreadInitCallback callback, void* arg);
        int SetGraphOrderCallback(JackGraphOrderCallback callback, void* arg);
        int SetBufferSizeCallback(JackBufferSizeCallback callback, void* arg);
        int SetFreewheelCallback(JackFreewheelCallback callback, void* arg);
        int SetPortRegistrationCallback(JackPortRegistrationCallback callback, void* arg);
        JackClientControl* GetClientControl() const;
		
		void CheckClient() const;

};


} // end of namespace

#endif
