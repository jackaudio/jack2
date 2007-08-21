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

#ifndef __JackInternalClientChannel__
#define __JackInternalClientChannel__

#include "JackChannel.h"

namespace Jack
{

/*!
\brief JackClientChannel for server internal clients.
*/

class JackInternalClientChannel : public JackClientChannelInterface
{

    private:

        JackServer* fServer;
        JackEngine* fEngine;

    public:

        JackInternalClientChannel(JackServer* server): fServer(server), fEngine(server->GetEngine())
        {}
        virtual ~JackInternalClientChannel()
        {}
		
		// Open the Server/Client connection
        virtual int Open(const char* name, char* name_res, JackClient* obj, jack_options_t options, jack_status_t* status)
        {
            return 0;
        }

		void ClientCheck(const char* name, char* name_res, int options, int* status, int* result)
		{
            *result = fEngine->ClientCheck(name, name_res, options, status);
        }
        void ClientOpen(const char* name, int* ref, JackEngineControl** shared_engine, JackGraphManager** shared_manager, JackClientInterface* client, int* result)
        {
            *result = fEngine->ClientInternalOpen(name, ref, shared_engine, shared_manager, client);
        }
        void ClientClose(int refnum, int* result)
        {
            *result = fEngine->ClientInternalClose(refnum);
        }
		
        void ClientActivate(int refnum, int* result)
        {
    		*result = fEngine->ClientActivate(refnum);
        }
        void ClientDeactivate(int refnum, int* result)
        {
     		*result = fEngine->ClientDeactivate(refnum);
        }

        void PortRegister(int refnum, const char* name, unsigned int flags, unsigned int buffer_size, jack_port_id_t* port_index, int* result)
        {
            *result = fEngine->PortRegister(refnum, name, flags, buffer_size, port_index);
        }
        void PortUnRegister(int refnum, jack_port_id_t port_index, int* result)
        {
            *result = fEngine->PortUnRegister(refnum, port_index);
        }

        void PortConnect(int refnum, const char* src, const char* dst, int* result)
        {
            *result = fEngine->PortConnect(refnum, src, dst);
        }
        void PortDisconnect(int refnum, const char* src, const char* dst, int* result)
        {
            *result = fEngine->PortDisconnect(refnum, src, dst);
        }

        void PortConnect(int refnum, jack_port_id_t src, jack_port_id_t dst, int* result)
        {
            *result = fEngine->PortConnect(refnum, src, dst);
        }
        void PortDisconnect(int refnum, jack_port_id_t src, jack_port_id_t dst, int* result)
        {
            *result = fEngine->PortDisconnect(refnum, src, dst);
        }

        void SetBufferSize(jack_nframes_t buffer_size, int* result)
        {
            *result = fServer->SetBufferSize(buffer_size);
        }
        void SetFreewheel(int onoff, int* result)
        {
            *result = fServer->SetFreewheel(onoff);
        }

        // A FINIR
        void ReleaseTimebase(int refnum, int* result)
        {
            *result = fEngine->ReleaseTimebase(refnum);
        }

        void SetTimebaseCallback(int refnum, int conditional, int* result)
        {
            *result = fEngine->SetTimebaseCallback(refnum, conditional);
        }

};

} // end of namespace

#endif

