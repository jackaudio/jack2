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

int JackWinNamedPipeClientChannel::Open(const char* name, JackClient* obj)
{
    JackLog("JackWinNamedPipeClientChannel::Open name = %s\n", name);

    if (fNotificationListenPipe.Bind(jack_client_dir, name, 0) < 0) {
        jack_error("Cannot bind pipe");
        goto error;
    }

    if (fRequestPipe.Connect(jack_server_dir, 0) < 0) {
        jack_error("Cannot connect to server pipe");
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

void JackWinNamedPipeClientChannel::ClientNew(const char* name, int* shared_engine, int* shared_client, int* shared_graph, int* result)
{
    JackClientNewRequest req(name);
    JackClientNewResult res;
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

void JackWinNamedPipeClientChannel::PortRegister(int refnum, const char* name, unsigned int flags, unsigned int buffer_size, jack_port_id_t* port_index, int* result)
{
    JackPortRegisterRequest req(refnum, name, "audio", flags, buffer_size);
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

    //fClient->Init(); // To be checked

    if (event.Read(&fNotificationListenPipe) < 0) {
        fNotificationListenPipe.Close();
        jack_error("JackWinNamedPipeClientChannel read fail");
        goto error;
    }

    res.fResult = fClient->ClientNotify(event.fRefNum, event.fName, event.fNotify, event.fSync, event.fValue);

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


