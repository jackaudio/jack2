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

#ifndef __JackChannel__
#define __JackChannel__

#include "types.h"
#include "JackError.h"
#include "JackTransportEngine.h"

namespace Jack
{

class JackClientInterface;
class JackClient;
class JackServer;
struct JackEngineControl;
class JackGraphManager;

/*!
\brief Inter process channel for server/client bidirectionnal communication : request and (receiving) notifications.
*/

class JackClientChannelInterface
{

    public:

        JackClientChannelInterface()
        {}
        virtual ~JackClientChannelInterface()
        {}

        // Open the Server/Client connection
        virtual int Open(const char* name, JackClient* obj)
        {
            return 0;
        }

        // Close the Server/Client connection
        virtual void Close()
        {}

        // Start listening for messages from the server
        virtual int Start()
        {
            return 0;
        }

        // Stop listening for messages from the server
        virtual void Stop()
        {}

        virtual void ClientOpen(const char* name, int* shared_engine, int* shared_client, int* shared_graph, int* result)
        {}
        virtual void ClientOpen(const char* name, int* ref, JackEngineControl** shared_engine, JackGraphManager** shared_manager, JackClientInterface* client, int* result)
        {}
        virtual void ClientClose(int refnum, int* result)
        {}

        virtual void ClientActivate(int refnum, int* result)
        {}
        virtual void ClientDeactivate(int refnum, int* result)
        {}

        virtual void PortRegister(int refnum, const char* name, unsigned int flags, unsigned int buffer_size, jack_port_id_t* port_index, int* result)
        {}
        virtual void PortUnRegister(int refnum, jack_port_id_t port_index, int* result)
        {}

        virtual void PortConnect(int refnum, const char* src, const char* dst, int* result)
        {}
        virtual void PortDisconnect(int refnum, const char* src, const char* dst, int* result)
        {}
        virtual void PortConnect(int refnum, jack_port_id_t src, jack_port_id_t dst, int* result)
        {}
        virtual void PortDisconnect(int refnum, jack_port_id_t src, jack_port_id_t dst, int* result)
        {}

        virtual void SetBufferSize(jack_nframes_t buffer_size, int* result)
        {}
        virtual void SetFreewheel(int onoff, int* result)
        {}

        virtual void ReleaseTimebase(int refnum, int* result)
        {}

        virtual void SetTimebaseCallback(int refnum, int conditional, int* result)
        {}

};

/*!
\brief Inter process channel for server to client notifications.
*/

class JackNotifyChannelInterface
{

    public:

        JackNotifyChannelInterface()
        {}
        virtual ~JackNotifyChannelInterface()
        {}

        // Open the Server/Client connection
        virtual int Open(const char* name)
        {
            return 0;
        }
        // Close the Server/Client connection
        virtual void Close()
        {}

        /*
        The "sync" parameter allows to choose between "synchronous" and "asynchronous" notification
        */
        virtual void ClientNotify(int refnum, const char* name, int notify, int sync, int value, int* result)
        {}

};

/*!
\brief Entry point channel for client/server communication.
*/

class JackServerChannelInterface
{

    public:

        JackServerChannelInterface()
        {}
        virtual ~JackServerChannelInterface()
        {}

        // Open the Server/Client connection
        virtual int Open(JackServer* server)
        {
            return 0;
        }
        // Close the Server/Client connection
        virtual void Close()
        {}
};

/*!
\brief Channel for server RT thread to request server thread communication.
*/

class JackServerNotifyChannelInterface
{

    public:

        JackServerNotifyChannelInterface()
        {}
        virtual ~JackServerNotifyChannelInterface()
        {}

        // Open the Server RT/Server connection
        virtual int Open()
        {
            return 0;
        }
        // Close the Server RT/Server connection
        virtual void Close()
        {}

        virtual void ClientNotify(int refnum, int notify, int value)
        {}

};


} // end of namespace

#endif

