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

#include "JackGenericClientChannel.h"
#include "JackClient.h"
#include "JackGlobals.h"
#include "JackError.h"

namespace Jack
{

JackGenericClientChannel::JackGenericClientChannel()
{}

JackGenericClientChannel::~JackGenericClientChannel()
{}

int JackGenericClientChannel::ServerCheck(const char* server_name)
{
    jack_log("JackGenericClientChannel::ServerCheck = %s", server_name);
  
    // Connect to server
    if (fRequest->Connect(jack_server_dir, server_name, 0) < 0) {
        jack_error("Cannot connect to server request channel");
        return -1;
    } else {
        return 0;
    }
}

void JackGenericClientChannel::ServerSyncCall(JackRequest* req, JackResult* res, int* result)
{
    // Check call context
    if (jack_tls_get(JackGlobals::fNotificationThread)) {
        jack_error("Cannot callback the server in notification thread!");
        *result = -1;
        return;
    }
    
    if (req->Write(fRequest) < 0) {
        jack_error("Could not write request type = %ld", req->fType);
        *result = -1;
        return;
    }

    if (res->Read(fRequest) < 0) {
        jack_error("Could not read result type = %ld", req->fType);
        *result = -1;
        return;
    }

    *result = res->fResult;
}

void JackGenericClientChannel::ServerAsyncCall(JackRequest* req, JackResult* res, int* result)
{
    // Check call context
    if (jack_tls_get(JackGlobals::fNotificationThread)) {
        jack_error("Cannot callback the server in notification thread!");
        *result = -1;
        return;
    }
    
    if (req->Write(fRequest) < 0) {
        jack_error("Could not write request type = %ld", req->fType);
        *result = -1;
    } else {
        *result = 0;
    }
}

void JackGenericClientChannel::ClientCheck(const char* name, int uuid, char* name_res, int protocol, int options, int* status, int* result, int open)
{
    JackClientCheckRequest req(name, protocol, options, uuid, open);
    JackClientCheckResult res;
    ServerSyncCall(&req, &res, result);
    *status = res.fStatus;
    strcpy(name_res, res.fName);
}

void JackGenericClientChannel::ClientOpen(const char* name, int pid, int uuid, int* shared_engine, int* shared_client, int* shared_graph, int* result)
{
    JackClientOpenRequest req(name, pid, uuid);
    JackClientOpenResult res;
    ServerSyncCall(&req, &res, result);
    *shared_engine = res.fSharedEngine;
    *shared_client = res.fSharedClient;
    *shared_graph = res.fSharedGraph;
}

void JackGenericClientChannel::ClientClose(int refnum, int* result)
{
    JackClientCloseRequest req(refnum);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackGenericClientChannel::ClientActivate(int refnum, int is_real_time, int* result)
{
    JackActivateRequest req(refnum, is_real_time);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackGenericClientChannel::ClientDeactivate(int refnum, int* result)
{
    JackDeactivateRequest req(refnum);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackGenericClientChannel::PortRegister(int refnum, const char* name, const char* type, unsigned int flags, unsigned int buffer_size, jack_port_id_t* port_index, int* result)
{
    JackPortRegisterRequest req(refnum, name, type, flags, buffer_size);
    JackPortRegisterResult res;
    ServerSyncCall(&req, &res, result);
    *port_index = res.fPortIndex;
}

void JackGenericClientChannel::PortUnRegister(int refnum, jack_port_id_t port_index, int* result)
{
    JackPortUnRegisterRequest req(refnum, port_index);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackGenericClientChannel::PortConnect(int refnum, const char* src, const char* dst, int* result)
{
    JackPortConnectNameRequest req(refnum, src, dst);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackGenericClientChannel::PortDisconnect(int refnum, const char* src, const char* dst, int* result)
{
    JackPortDisconnectNameRequest req(refnum, src, dst);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackGenericClientChannel::PortConnect(int refnum, jack_port_id_t src, jack_port_id_t dst, int* result)
{
    JackPortConnectRequest req(refnum, src, dst);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackGenericClientChannel::PortDisconnect(int refnum, jack_port_id_t src, jack_port_id_t dst, int* result)
{
    JackPortDisconnectRequest req(refnum, src, dst);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackGenericClientChannel::PortRename(int refnum, jack_port_id_t port, const char* name, int* result)
{
    JackPortRenameRequest req(refnum, port, name);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackGenericClientChannel::SetBufferSize(jack_nframes_t buffer_size, int* result)
{
    JackSetBufferSizeRequest req(buffer_size);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackGenericClientChannel::SetFreewheel(int onoff, int* result)
{
    JackSetFreeWheelRequest req(onoff);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackGenericClientChannel::ComputeTotalLatencies(int* result)
{
    JackComputeTotalLatenciesRequest req;
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackGenericClientChannel::SessionNotify(int refnum, const char* target, jack_session_event_type_t type, const char* path, jack_session_command_t** result)
{
    JackSessionNotifyRequest req(refnum, path, type, target);
    JackSessionNotifyResult res;
    int intresult;
    ServerSyncCall(&req, &res, &intresult);
    *result = res.GetCommands();
}

void JackGenericClientChannel::SessionReply(int refnum, int* result)
{
    JackSessionReplyRequest req(refnum);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackGenericClientChannel::GetUUIDForClientName(int refnum, const char* client_name, char* uuid_res, int* result)
{
    JackGetUUIDRequest req(client_name);
    JackUUIDResult res;
    ServerSyncCall(&req, &res, result);
    strncpy(uuid_res, res.fUUID, JACK_UUID_SIZE);
}

void JackGenericClientChannel::GetClientNameForUUID(int refnum, const char* uuid, char* name_res, int* result)
{
    JackGetClientNameRequest req(uuid);
    JackClientNameResult res;
    ServerSyncCall(&req, &res, result);
    strncpy(name_res, res.fName, JACK_CLIENT_NAME_SIZE);
}

void JackGenericClientChannel::ClientHasSessionCallback(const char* client_name, int* result)
{
    JackClientHasSessionCallbackRequest req(client_name);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackGenericClientChannel::ReserveClientName(int refnum, const char* client_name, const char* uuid, int* result)
{
    JackReserveNameRequest req(refnum, client_name, uuid);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackGenericClientChannel::ReleaseTimebase(int refnum, int* result)
{
    JackReleaseTimebaseRequest req(refnum);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackGenericClientChannel::SetTimebaseCallback(int refnum, int conditional, int* result)
{
    JackSetTimebaseCallbackRequest req(refnum, conditional);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackGenericClientChannel::GetInternalClientName(int refnum, int int_ref, char* name_res, int* result)
{
    JackGetInternalClientNameRequest req(refnum, int_ref);
    JackGetInternalClientNameResult res;
    ServerSyncCall(&req, &res, result);
    strcpy(name_res, res.fName);
}

void JackGenericClientChannel::InternalClientHandle(int refnum, const char* client_name, int* status, int* int_ref, int* result)
{
    JackInternalClientHandleRequest req(refnum, client_name);
    JackInternalClientHandleResult res;
    ServerSyncCall(&req, &res, result);
    *int_ref = res.fIntRefNum;
    *status = res.fStatus;
}

void JackGenericClientChannel::InternalClientLoad(int refnum, const char* client_name, const char* so_name, const char* objet_data, int options, int* status, int* int_ref, int uuid, int* result)
{
    JackInternalClientLoadRequest req(refnum, client_name, so_name, objet_data, options, uuid);
    JackInternalClientLoadResult res;
    ServerSyncCall(&req, &res, result);
    *int_ref = res.fIntRefNum;
    *status = res.fStatus;
}

void JackGenericClientChannel::InternalClientUnload(int refnum, int int_ref, int* status, int* result)
{
    JackInternalClientUnloadRequest req(refnum, int_ref);
    JackInternalClientUnloadResult res;
    ServerSyncCall(&req, &res, result);
    *status = res.fStatus;
}

} // end of namespace





