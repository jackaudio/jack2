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

#ifndef __JackGenericClientChannel__
#define __JackGenericClientChannel__

#include "JackChannel.h"

namespace Jack
{

struct JackRequest;
struct JackResult;

/*!
\brief Generic JackClientChannel class.
*/

class JackGenericClientChannel : public detail::JackClientChannelInterface
{

    protected:

        detail::JackClientRequestInterface* fRequest;

        void ServerSyncCall(JackRequest* req, JackResult* res, int* result);
        void ServerAsyncCall(JackRequest* req, JackResult* res, int* result);

    public:

        JackGenericClientChannel();
        virtual ~JackGenericClientChannel();

        virtual int Open(const char* server_name, const char* name, int uuid, char* name_res, JackClient* obj, jack_options_t options, jack_status_t* status) { return -1; }
        virtual void Close() {}

        virtual int Start() { return -1; }
        virtual void Stop() {}

        int ServerCheck(const char* server_name);

        void ClientCheck(const char* name, int uuid, char* name_res, int protocol, int options, int* status, int* result, int open);
        void ClientOpen(const char* name, int pid, int uuid, int* shared_engine, int* shared_client, int* shared_graph, int* result);
        void ClientOpen(const char* name, int* ref, int uuid, JackEngineControl** shared_engine, JackGraphManager** shared_manager, JackClientInterface* client, int* result)
        {}
        void ClientClose(int refnum, int* result);

        void ClientActivate(int refnum, int is_real_time, int* result);
        void ClientDeactivate(int refnum, int* result);

        void PortRegister(int refnum, const char* name, const char* type, unsigned int flags, unsigned int buffer_size, jack_port_id_t* port_index, int* result);
        void PortUnRegister(int refnum, jack_port_id_t port_index, int* result);

        void PortConnect(int refnum, const char* src, const char* dst, int* result);
        void PortDisconnect(int refnum, const char* src, const char* dst, int* result);

        void PortConnect(int refnum, jack_port_id_t src, jack_port_id_t dst, int* result);
        void PortDisconnect(int refnum, jack_port_id_t src, jack_port_id_t dst, int* result);

        void PortRename(int refnum, jack_port_id_t port, const char* name, int* result);

        void SetBufferSize(jack_nframes_t buffer_size, int* result);
        void SetFreewheel(int onoff, int* result);

        void ComputeTotalLatencies(int* result);

        void ReleaseTimebase(int refnum, int* result);
        void SetTimebaseCallback(int refnum, int conditional, int* result);

        void GetInternalClientName(int refnum, int int_ref, char* name_res, int* result);
        void InternalClientHandle(int refnum, const char* client_name, int* status, int* int_ref, int* result);
        void InternalClientLoad(int refnum, const char* client_name, const char* so_name, const char* objet_data, int options, int* status, int* int_ref, int uuid, int* result);
        void InternalClientUnload(int refnum, int int_ref, int* status, int* result);

        // Session API
        void SessionNotify(int refnum, const char* target, jack_session_event_type_t type, const char* path, jack_session_command_t** result);
        void SessionReply(int refnum, int* result);
        void GetUUIDForClientName(int refnum, const char* client_name, char* uuid_res, int* result);
        void GetClientNameForUUID(int refnum, const char* uuid, char* name_res, int* result);
        void ReserveClientName(int refnum, const char* client_name, const char *uuid, int* result);
        void ClientHasSessionCallback(const char* client_name, int* result);
};

} // end of namespace

#endif

