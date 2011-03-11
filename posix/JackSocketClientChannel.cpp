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

#include "JackSocketClientChannel.h"
#include "JackRequest.h"
#include "JackClient.h"
#include "JackGlobals.h"

namespace Jack
{

JackSocketClientChannel::JackSocketClientChannel():
    fThread(this)
{
    fNotificationSocket = NULL;
    fClient = NULL;
}

JackSocketClientChannel::~JackSocketClientChannel()
{
    delete fNotificationSocket;
}

int JackSocketClientChannel::ServerCheck(const char* server_name)
{
    jack_log("JackSocketClientChannel::ServerCheck = %s", server_name);

    // Connect to server
    if (fRequestSocket.Connect(jack_server_dir, server_name, 0) < 0) {
        jack_error("Cannot connect to server socket");
        fRequestSocket.Close();
        return -1;
    } else {
        return 0;
    }
}

int JackSocketClientChannel::Open(const char* server_name, const char* name, int uuid, char* name_res, JackClient* obj, jack_options_t options, jack_status_t* status)
{
    int result = 0;
    jack_log("JackSocketClientChannel::Open name = %s", name);

    if (fRequestSocket.Connect(jack_server_dir, server_name, 0) < 0) {
        jack_error("Cannot connect to server socket");
        goto error;
    }

    // Check name in server
    ClientCheck(name, uuid, name_res, JACK_PROTOCOL_VERSION, (int)options, (int*)status, &result);
    if (result < 0) {
        int status1 = *status;
        if (status1 & JackVersionError)
            jack_error("JACK protocol mismatch %d", JACK_PROTOCOL_VERSION);
        else
            jack_error("Client name = %s conflits with another running client", name);
        goto error;
    }

    if (fNotificationListenSocket.Bind(jack_client_dir, name_res, 0) < 0) {
        jack_error("Cannot bind socket");
        goto error;
    }

    fClient = obj;
    return 0;

error:
    fRequestSocket.Close();
    fNotificationListenSocket.Close();
    return -1;
}

void JackSocketClientChannel::Close()
{
    fRequestSocket.Close();
    fNotificationListenSocket.Close();
    if (fNotificationSocket)
        fNotificationSocket->Close();
}

int JackSocketClientChannel::Start()
{
    jack_log("JackSocketClientChannel::Start");
    /*
     To be sure notification thread is started before ClientOpen is called.
    */
    if (fThread.StartSync() != 0) {
        jack_error("Cannot start Jack client listener");
        return -1;
    } else {
        return 0;
    }
}

void JackSocketClientChannel::Stop()
{
    jack_log("JackSocketClientChannel::Stop");
    fThread.Kill();
}

void JackSocketClientChannel::ServerSyncCall(JackRequest* req, JackResult* res, int* result)
{
    if (req->Write(&fRequestSocket) < 0) {
        jack_error("Could not write request type = %ld", req->fType);
        *result = -1;
        return;
    }

    if (res->Read(&fRequestSocket) < 0) {
        jack_error("Could not read result type = %ld", req->fType);
        *result = -1;
        return;
    }

    *result = res->fResult;
}

void JackSocketClientChannel::ServerAsyncCall(JackRequest* req, JackResult* res, int* result)
{
    if (req->Write(&fRequestSocket) < 0) {
        jack_error("Could not write request type = %ld", req->fType);
        *result = -1;
    } else {
        *result = 0;
    }
}

void JackSocketClientChannel::ClientCheck(const char* name, int uuid, char* name_res, int protocol, int options, int* status, int* result)
{
    JackClientCheckRequest req(name, protocol, options, uuid);
    JackClientCheckResult res;
    ServerSyncCall(&req, &res, result);
    *status = res.fStatus;
    strcpy(name_res, res.fName);
}

void JackSocketClientChannel::ClientOpen(const char* name, int pid, int uuid, int* shared_engine, int* shared_client, int* shared_graph, int* result)
{
    JackClientOpenRequest req(name, pid, uuid);
    JackClientOpenResult res;
    ServerSyncCall(&req, &res, result);
    *shared_engine = res.fSharedEngine;
    *shared_client = res.fSharedClient;
    *shared_graph = res.fSharedGraph;
}

void JackSocketClientChannel::ClientClose(int refnum, int* result)
{
    JackClientCloseRequest req(refnum);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackSocketClientChannel::ClientActivate(int refnum, int is_real_time, int* result)
{
    JackActivateRequest req(refnum, is_real_time);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackSocketClientChannel::ClientDeactivate(int refnum, int* result)
{
    JackDeactivateRequest req(refnum);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackSocketClientChannel::PortRegister(int refnum, const char* name, const char* type, unsigned int flags, unsigned int buffer_size, jack_port_id_t* port_index, int* result)
{
    JackPortRegisterRequest req(refnum, name, type, flags, buffer_size);
    JackPortRegisterResult res;
    ServerSyncCall(&req, &res, result);
    *port_index = res.fPortIndex;
}

void JackSocketClientChannel::PortUnRegister(int refnum, jack_port_id_t port_index, int* result)
{
    JackPortUnRegisterRequest req(refnum, port_index);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackSocketClientChannel::PortConnect(int refnum, const char* src, const char* dst, int* result)
{
    JackPortConnectNameRequest req(refnum, src, dst);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackSocketClientChannel::PortDisconnect(int refnum, const char* src, const char* dst, int* result)
{
    JackPortDisconnectNameRequest req(refnum, src, dst);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackSocketClientChannel::PortConnect(int refnum, jack_port_id_t src, jack_port_id_t dst, int* result)
{
    JackPortConnectRequest req(refnum, src, dst);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackSocketClientChannel::PortDisconnect(int refnum, jack_port_id_t src, jack_port_id_t dst, int* result)
{
    JackPortDisconnectRequest req(refnum, src, dst);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackSocketClientChannel::PortRename(int refnum, jack_port_id_t port, const char* name, int* result)
{
    JackPortRenameRequest req(refnum, port, name);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackSocketClientChannel::SetBufferSize(jack_nframes_t buffer_size, int* result)
{
    JackSetBufferSizeRequest req(buffer_size);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackSocketClientChannel::SetFreewheel(int onoff, int* result)
{
    JackSetFreeWheelRequest req(onoff);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackSocketClientChannel::ComputeTotalLatencies(int* result)
{
    JackComputeTotalLatenciesRequest req;
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackSocketClientChannel::SessionNotify(int refnum, const char* target, jack_session_event_type_t type, const char* path, jack_session_command_t** result)
{
    JackSessionNotifyRequest req(refnum, path, type, target);
    JackSessionNotifyResult res;
    int intresult;
    ServerSyncCall(&req, &res, &intresult);

    jack_session_command_t* session_command = (jack_session_command_t *)malloc(sizeof(jack_session_command_t) * (res.fCommandList.size() + 1));
    int i = 0;

    for (std::list<JackSessionCommand>::iterator ci=res.fCommandList.begin(); ci!=res.fCommandList.end(); ci++) {
        session_command[i].uuid = strdup( ci->fUUID );
        session_command[i].client_name = strdup( ci->fClientName );
        session_command[i].command = strdup( ci->fCommand );
        session_command[i].flags = ci->fFlags;
        i += 1;
    }

    session_command[i].uuid = NULL;
    session_command[i].client_name = NULL;
    session_command[i].command = NULL;
    session_command[i].flags = (jack_session_flags_t)0;

    *result = session_command;
}

void JackSocketClientChannel::SessionReply(int refnum, int* result)
{
    JackSessionReplyRequest req(refnum);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackSocketClientChannel::GetUUIDForClientName(int refnum, const char* client_name, char* uuid_res, int* result)
{
    JackGetUUIDRequest req(client_name);
    JackUUIDResult res;
    ServerSyncCall(&req, &res, result);
    strncpy(uuid_res, res.fUUID, JACK_UUID_SIZE);
}

void JackSocketClientChannel::GetClientNameForUUID(int refnum, const char* uuid, char* name_res, int* result)
{
    JackGetClientNameRequest req(uuid);
    JackClientNameResult res;
    ServerSyncCall(&req, &res, result);
    strncpy(name_res, res.fName, JACK_CLIENT_NAME_SIZE);
}

void JackSocketClientChannel::ClientHasSessionCallback(const char* client_name, int* result)
{
    JackClientHasSessionCallbackRequest req(client_name);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackSocketClientChannel::ReserveClientName(int refnum, const char* client_name, const char* uuid, int* result)
{
    JackReserveNameRequest req(refnum, client_name, uuid);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackSocketClientChannel::ReleaseTimebase(int refnum, int* result)
{
    JackReleaseTimebaseRequest req(refnum);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackSocketClientChannel::SetTimebaseCallback(int refnum, int conditional, int* result)
{
    JackSetTimebaseCallbackRequest req(refnum, conditional);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackSocketClientChannel::GetInternalClientName(int refnum, int int_ref, char* name_res, int* result)
{
    JackGetInternalClientNameRequest req(refnum, int_ref);
    JackGetInternalClientNameResult res;
    ServerSyncCall(&req, &res, result);
    strcpy(name_res, res.fName);
}

void JackSocketClientChannel::InternalClientHandle(int refnum, const char* client_name, int* status, int* int_ref, int* result)
{
    JackInternalClientHandleRequest req(refnum, client_name);
    JackInternalClientHandleResult res;
    ServerSyncCall(&req, &res, result);
    *int_ref = res.fIntRefNum;
    *status = res.fStatus;
}

void JackSocketClientChannel::InternalClientLoad(int refnum, const char* client_name, const char* so_name, const char* objet_data, int options, int* status, int* int_ref, int uuid, int* result)
{
    JackInternalClientLoadRequest req(refnum, client_name, so_name, objet_data, options, uuid);
    JackInternalClientLoadResult res;
    ServerSyncCall(&req, &res, result);
    *int_ref = res.fIntRefNum;
    *status = res.fStatus;
}

void JackSocketClientChannel::InternalClientUnload(int refnum, int int_ref, int* status, int* result)
{
    JackInternalClientUnloadRequest req(refnum, int_ref);
    JackInternalClientUnloadResult res;
    ServerSyncCall(&req, &res, result);
    *status = res.fStatus;
}

bool JackSocketClientChannel::Init()
{
    jack_log("JackSocketClientChannel::Init");
    fNotificationSocket = fNotificationListenSocket.Accept();
    // No more needed
    fNotificationListenSocket.Close();

    if (!fNotificationSocket) {
        jack_error("JackSocketClientChannel: cannot establish notication socket");
        return false;
    } else {
        return true;
    }
}

bool JackSocketClientChannel::Execute()
{
    JackClientNotification event;
    JackResult res;

    if (event.Read(fNotificationSocket) < 0) {
        fNotificationSocket->Close();
        jack_error("JackSocketClientChannel read fail");
        goto error;
    }

    res.fResult = fClient->ClientNotify(event.fRefNum, event.fName, event.fNotify, event.fSync, event.fMessage, event.fValue1, event.fValue2);

    if (event.fSync) {
        if (res.Write(fNotificationSocket) < 0) {
            fNotificationSocket->Close();
            jack_error("JackSocketClientChannel write fail");
            goto error;
        }
    }
    return true;

error:
    fClient->ShutDown();
    return false;
}

} // end of namespace





