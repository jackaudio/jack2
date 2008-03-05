/*
Copyright (C) 2004-2006 Grame

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

#include "JackWinNamedPipeClientChannel.h"
#include "JackRequest.h"
#include "JackClient.h"
#include "JackGlobals.h"

namespace Jack
{

JackWinNamedPipeClientChannel::JackWinNamedPipeClientChannel()
{
    fThread = JackGlobals::MakeThread(this);
    fClient = NULL;
}

JackWinNamedPipeClientChannel::~JackWinNamedPipeClientChannel()
{
    delete fThread;
}

int JackWinNamedPipeClientChannel::ServerCheck(const char* server_name)
{
    JackLog("JackWinNamedPipeClientChannel::ServerCheck = %s\n", server_name);

    // Connect to server
    if (fRequestPipe.Connect(jack_server_dir, server_name, 0) < 0) {
        jack_error("Cannot connect to server pipe");
        return -1;
    } else {
        return 0;
    }
}

int JackWinNamedPipeClientChannel::Open(const char* server_name, const char* name, char* name_res, JackClient* obj, jack_options_t options, jack_status_t* status)
{
    int result = 0;
    JackLog("JackWinNamedPipeClientChannel::Open name = %s\n", name);

    /*
    16/08/07: was called before doing "fRequestPipe.Connect" .... still necessary?
       if (fNotificationListenPipe.Bind(jack_client_dir, name, 0) < 0) {
           jack_error("Cannot bind pipe");
           goto error;
       }
    */

    if (fRequestPipe.Connect(jack_server_dir, server_name, 0) < 0) {
        jack_error("Cannot connect to server pipe");
        goto error;
    }

    // Check name in server
    ClientCheck(name, name_res, JACK_PROTOCOL_VERSION, (int)options, (int*)status, &result);
    if (result < 0) {
        jack_error("Client name = %s conflits with another running client", name);
        goto error;
    }

    if (fNotificationListenPipe.Bind(jack_client_dir, name_res, 0) < 0) {
        jack_error("Cannot bind pipe");
        goto error;
    }


    fClient = obj;
    return 0;

error:
    fRequestPipe.Close();
    fNotificationListenPipe.Close();
    return -1;
}

void JackWinNamedPipeClientChannel::Close()
{
    fRequestPipe.Close();
    fNotificationListenPipe.Close();
    // Here the thread will correctly stop when the pipe are closed
    fThread->Stop();
}

int JackWinNamedPipeClientChannel::Start()
{
    JackLog("JackWinNamedPipeClientChannel::Start\n");

    if (fThread->Start() != 0) {
        jack_error("Cannot start Jack client listener");
        return -1;
    } else {
        return 0;
    }
}

void JackWinNamedPipeClientChannel::Stop()
{
    JackLog("JackWinNamedPipeClientChannel::Stop\n");
    fThread->Kill();  // Unsafe on WIN32... TODO : solve WIN32 thread Kill issue
}

void JackWinNamedPipeClientChannel::ServerSyncCall(JackRequest* req, JackResult* res, int* result)
{
    if (req->Write(&fRequestPipe) < 0) {
        jack_error("Could not write request type = %ld", req->fType);
        *result = -1;
        return ;
    }

    if (res->Read(&fRequestPipe) < 0) {
        jack_error("Could not read result type = %ld", req->fType);
        *result = -1;
        return ;
    }

    *result = res->fResult;
}

void JackWinNamedPipeClientChannel::ServerAsyncCall(JackRequest* req, JackResult* res, int* result)
{
    if (req->Write(&fRequestPipe) < 0) {
        jack_error("Could not write request type = %ld", req->fType);
        *result = -1;
    } else {
        *result = 0;
    }
}

void JackWinNamedPipeClientChannel::ClientCheck(const char* name, char* name_res, int protocol, int options, int* status, int* result)
{
    JackClientCheckRequest req(name, protocol, options);
    JackClientCheckResult res;
    ServerSyncCall(&req, &res, result);
    *status = res.fStatus;
    strcpy(name_res, res.fName);
}

void JackWinNamedPipeClientChannel::ClientOpen(const char* name, int* shared_engine, int* shared_client, int* shared_graph, int* result)
{
    JackClientOpenRequest req(name);
    JackClientOpenResult res;
    ServerSyncCall(&req, &res, result);
    *shared_engine = res.fSharedEngine;
    *shared_client = res.fSharedClient;
    *shared_graph = res.fSharedGraph;
}

void JackWinNamedPipeClientChannel::ClientClose(int refnum, int* result)
{
    JackClientCloseRequest req(refnum);
    JackResult res;
    ServerAsyncCall(&req, &res, result);
}

void JackWinNamedPipeClientChannel::ClientActivate(int refnum, int* result)
{
    JackActivateRequest req(refnum);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackWinNamedPipeClientChannel::ClientDeactivate(int refnum, int* result)
{
    JackDeactivateRequest req(refnum);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackWinNamedPipeClientChannel::PortRegister(int refnum, const char* name, const char* type, unsigned int flags, unsigned int buffer_size, unsigned int* port_index, int* result)
{
    JackPortRegisterRequest req(refnum, name, type, flags, buffer_size);
    JackPortRegisterResult res;
    ServerSyncCall(&req, &res, result);
    *port_index = res.fPortIndex;
}

void JackWinNamedPipeClientChannel::PortUnRegister(int refnum, jack_port_id_t port_index, int* result)
{
    JackPortUnRegisterRequest req(refnum, port_index);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackWinNamedPipeClientChannel::PortConnect(int refnum, const char* src, const char* dst, int* result)
{
    JackPortConnectNameRequest req(refnum, src, dst);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackWinNamedPipeClientChannel::PortDisconnect(int refnum, const char* src, const char* dst, int* result)
{
    JackPortDisconnectNameRequest req(refnum, src, dst);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackWinNamedPipeClientChannel::PortConnect(int refnum, jack_port_id_t src, jack_port_id_t dst, int* result)
{
    JackPortConnectRequest req(refnum, src, dst);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackWinNamedPipeClientChannel::PortDisconnect(int refnum, jack_port_id_t src, jack_port_id_t dst, int* result)
{
    JackPortDisconnectRequest req(refnum, src, dst);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackWinNamedPipeClientChannel::SetBufferSize(jack_nframes_t buffer_size, int* result)
{
    JackSetBufferSizeRequest req(buffer_size);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackWinNamedPipeClientChannel::SetFreewheel(int onoff, int* result)
{
    JackSetFreeWheelRequest req(onoff);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackWinNamedPipeClientChannel::ReleaseTimebase(int refnum, int* result)
{
    JackReleaseTimebaseRequest req(refnum);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackWinNamedPipeClientChannel::SetTimebaseCallback(int refnum, int conditional, int* result)
{
    JackSetTimebaseCallbackRequest req(refnum, conditional);
    JackResult res;
    ServerSyncCall(&req, &res, result);
}

void JackWinNamedPipeClientChannel::GetInternalClientName(int refnum, int int_ref, char* name_res, int* result)
{
    JackGetInternalClientNameRequest req(refnum, int_ref);
    JackGetInternalClientNameResult res;
    ServerSyncCall(&req, &res, result);
    strcpy(name_res, res.fName);
}

void JackWinNamedPipeClientChannel::InternalClientHandle(int refnum, const char* client_name, int* status, int* int_ref, int* result)
{
    JackInternalClientHandleRequest req(refnum, client_name);
    JackInternalClientHandleResult res;
    ServerSyncCall(&req, &res, result);
    *int_ref = res.fIntRefNum;
    *status = res.fStatus;
}

void JackWinNamedPipeClientChannel::InternalClientLoad(int refnum, const char* client_name, const char* so_name, const char* objet_data, int options, int* status, int* int_ref, int* result)
{
    JackInternalClientLoadRequest req(refnum, client_name, so_name, objet_data, options);
    JackInternalClientLoadResult res;
    ServerSyncCall(&req, &res, result);
    *int_ref = res.fIntRefNum;
    *status = res.fStatus;
}

void JackWinNamedPipeClientChannel::InternalClientUnload(int refnum, int int_ref, int* status, int* result)
{
    JackInternalClientUnloadRequest req(refnum, int_ref);
    JackInternalClientUnloadResult res;
    ServerSyncCall(&req, &res, result);
    *status = res.fStatus;
}

bool JackWinNamedPipeClientChannel::Init()
{
    JackLog("JackWinNamedPipeClientChannel::Init \n");

    if (!fNotificationListenPipe.Accept()) {
        jack_error("JackWinNamedPipeClientChannel: cannot establish notification pipe");
        return false;
    } else {
        return true;
    }
}

bool JackWinNamedPipeClientChannel::Execute()
{
    JackClientNotification event;
    JackResult res;

    if (event.Read(&fNotificationListenPipe) < 0) {
        fNotificationListenPipe.Close();
        jack_error("JackWinNamedPipeClientChannel read fail");
        goto error;
    }

    res.fResult = fClient->ClientNotify(event.fRefNum, event.fName, event.fNotify, event.fSync, event.fValue1, event.fValue2);

    if (event.fSync) {
        if (res.Write(&fNotificationListenPipe) < 0) {
            fNotificationListenPipe.Close();
            jack_error("JackWinNamedPipeClientChannel write fail");
            goto error;
        }
    }
    return true;

error:

    //fClient->ShutDown(); needed ??
    return false;
}

} // end of namespace


