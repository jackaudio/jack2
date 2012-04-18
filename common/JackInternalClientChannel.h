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

#ifndef __JackInternalClientChannel__
#define __JackInternalClientChannel__

#include "JackChannel.h"

namespace Jack
{

/*!
\brief JackClientChannel for server internal clients.
*/

class JackInternalClientChannel : public detail::JackClientChannelInterface
{

    private:

        JackServer* fServer;
        JackLockedEngine* fEngine;

    public:

        JackInternalClientChannel(JackServer* server): fServer(server), fEngine(server->GetEngine())
        {}
        virtual ~JackInternalClientChannel()
        {}

        void ClientCheck(const char* name, int uuid, char* name_res, int protocol, int options, int* status, int* result, int open)
        {
            *result = fEngine->ClientCheck(name, uuid, name_res, protocol, options, status);
        }
        void ClientOpen(const char* name, int* ref, JackEngineControl** shared_engine, JackGraphManager** shared_manager, JackClientInterface* client, int* result)
        {
            *result = fEngine->ClientInternalOpen(name, ref, shared_engine, shared_manager, client, true);
        }
        void ClientClose(int refnum, int* result)
        {
            *result = fEngine->ClientInternalClose(refnum, true);
        }

        void ClientActivate(int refnum, int is_real_time, int* result)
        {
            *result = fEngine->ClientActivate(refnum, is_real_time);
        }
        void ClientDeactivate(int refnum, int* result)
        {
            *result = fEngine->ClientDeactivate(refnum);
        }

        void PortRegister(int refnum, const char* name, const char* type, unsigned int flags, unsigned int buffer_size, unsigned int* port_index, int* result)
        {
            *result = fEngine->PortRegister(refnum, name, type, flags, buffer_size, port_index);
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
        void PortRename(int refnum, jack_port_id_t port, const char* name, int* result)
        {
            *result = fEngine->PortRename(refnum, port, name);
        }

        void SetBufferSize(jack_nframes_t buffer_size, int* result)
        {
            *result = fServer->SetBufferSize(buffer_size);
        }
        void SetFreewheel(int onoff, int* result)
        {
            *result = fServer->SetFreewheel(onoff);
        }
        void ComputeTotalLatencies(int* result)
        {
            *result = fEngine->ComputeTotalLatencies();
        }

        void ReleaseTimebase(int refnum, int* result)
        {
            *result = fServer->ReleaseTimebase(refnum);
        }

        void SetTimebaseCallback(int refnum, int conditional, int* result)
        {
            *result = fServer->SetTimebaseCallback(refnum, conditional);
        }

        void GetInternalClientName(int refnum, int int_ref, char* name_res, int* result)
        {
            *result = fEngine->GetInternalClientName(int_ref, name_res);
        }

        void InternalClientHandle(int refnum, const char* client_name, int* status, int* int_ref, int* result)
        {
            *result = fEngine->InternalClientHandle(client_name, status, int_ref);
        }

        void InternalClientLoad(int refnum, const char* client_name, const char* so_name, const char* objet_data, int options, int* status, int* int_ref, int uuid, int* result)
        {
            *result = fServer->InternalClientLoad1(client_name, so_name, objet_data, options, int_ref, uuid, status);
        }

        void InternalClientUnload(int refnum, int int_ref, int* status, int* result)
        {
            *result = fEngine->InternalClientUnload(int_ref, status);
        }

        void SessionNotify(int refnum, const char *target, jack_session_event_type_t type, const char *path, jack_session_command_t** result)
        {
            JackSessionNotifyResult* res;
            fEngine->SessionNotify(refnum, target, type, path, NULL, &res);
            if (res == NULL) {
                *result = NULL;
                return;
            }

            *result = res->GetCommands();
            delete(res);
        }

        void SessionReply(int refnum, int* result)
        {
            *result = fEngine->SessionReply(refnum);
        }

        void GetUUIDForClientName(int refnum, const char* client_name, char* uuid_res, int* result)
        {
            *result = fEngine->GetUUIDForClientName(client_name, uuid_res);
        }

        void GetClientNameForUUID(int refnum, const char* uuid, char* name_res, int* result)
        {
            *result = fEngine->GetClientNameForUUID(uuid, name_res);
        }

        void ReserveClientName(int refnum, const char* client_name, const char *uuid, int* result)
        {
            *result = fEngine->ReserveClientName(client_name, uuid);
        }

        void ClientHasSessionCallback(const char* client_name, int* result)
        {
            *result = fEngine->ClientHasSessionCallback(client_name);
        }


};

} // end of namespace

#endif

