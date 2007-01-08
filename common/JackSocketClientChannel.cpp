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

#include "JackSocketClientChannel.h"
#include "JackRequest.h"
#include "JackClient.h"
#include "JackGlobals.h"

namespace Jack
{

JackSocketClientChannel::JackSocketClientChannel()
{
    fThread = JackGlobals::MakeThread(this);
    fNotificationSocket = NULL;
    fClient = NULL;
}

JackSocketClientChannel::~JackSocketClientChannel()
{
    delete fThread;
    delete fNotificationSocket;
}

int JackSocketClientChannel::Open(const char* name, JackClient* obj)
{
    JackLog("JackSocketClientChannel::Open name = %s\n", name);

    if (fRequestSocket.Connect(jack_server_dir, 0) < 0) {
        jack_error("Cannot connect to server socket");
        goto error;
    }

    if (fNotificationListenSocket.Bind(jack_client_dir, name, 0) < 0) {
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
    JackLog("JackSocketClientChannel::Start\n");
    if (fThread->Start() != 0) {
        jack_error("Cannot start Jack client listener");
        return -1;
    } else {
        return 0;
    }
}

void JackSocketClientChannel::Stop()
{
    JackLog("JackSocketClientChannel::Stop\n");
    fThread->Kill();
}

/*
void JackSocketClientChannel::ServerSyncCall(JackRequest* req, JackResult* res, int* result)
{
    if (req->Write(&fRequestSocket) < 0) {
        jack_error("Could not write request type = %ld", req->fType);
        *result = -1;
        return ;
    }

    if (res->Read(&fRequestSocket) < 0) {
        jack_error("Could not read result type = %ld", req->fType);
        *result = -1;
        return ;
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
*/

void JackSocketClientChannel::ClientNew(const char* name, int* shared_engine, int* shared_client, int* shared_graph, int* result)
{
    JackClientNewRequest req(name);
    JackClientNewResult res;
    //ServerSyncCall(&req, &res, result);
	
	if (req.Write(&fRequestSocket) < 0) {
        jack_error("Could not write request type = %ld", req.fHeader.fType);
        *result = -1;
        return ;
    }

    if (res.Read(&fRequestSocket) < 0) {
        jack_error("Could not read result type = %ld", req.fHeader.fType);
        *result = -1;
        return ;
    }

    *result = res.fHeader.fResult;
    *shared_engine = res.fSharedEngine;
    *shared_client = res.fSharedClient;
    *shared_graph = res.fSharedGraph;
}

void JackSocketClientChannel::ClientClose(int refnum, int* result)
{
    JackClientCloseRequest req(refnum);
    JackResult res;
    //ServerAsyncCall(&req, &res, result);
	if (req.Write(&fRequestSocket) < 0) {
        jack_error("Could not write request type = %ld", req.fHeader.fType);
        *result = -1;
    } else {
        *result = 0;
    }
}

void JackSocketClientChannel::ClientActivate(int refnum, int* result)
{
    JackActivateRequest req(refnum);
    JackResult res;
    //ServerSyncCall(&req, &res, result);
	if (req.Write(&fRequestSocket) < 0) {
        jack_error("Could not write request type = %ld", req.fHeader.fType);
        *result = -1;
        return ;
    }

    if (res.Read(&fRequestSocket) < 0) {
        jack_error("Could not read result type = %ld", req.fHeader.fType);
        *result = -1;
        return ;
    }

    *result = res.fResult;
}

void JackSocketClientChannel::ClientDeactivate(int refnum, int* result)
{
    JackDeactivateRequest req(refnum);
    JackResult res;
    //ServerSyncCall(&req, &res, result);
	if (req.Write(&fRequestSocket) < 0) {
        jack_error("Could not write request type = %ld", req.fHeader.fType);
        *result = -1;
        return ;
    }

    if (res.Read(&fRequestSocket) < 0) {
        jack_error("Could not read result type = %ld", req.fHeader.fType);
        *result = -1;
        return ;
    }

	*result = res.fResult;
}

void JackSocketClientChannel::PortRegister(int refnum, const char* name, unsigned int flags, unsigned int buffer_size, jack_port_id_t* port_index, int* result)
{
    JackPortRegisterRequest req(refnum, name, "audio", flags, buffer_size);
    JackPortRegisterResult res;
    //ServerSyncCall(&req, &res, result);
	if (req.Write(&fRequestSocket) < 0) {
        jack_error("Could not write request type = %ld", req.fHeader.fType);
        *result = -1;
        return ;
    }

    if (res.Read(&fRequestSocket) < 0) {
        jack_error("Could not read result type = %ld", req.fHeader.fType);
        *result = -1;
        return ;
    }

    *result = res.fHeader.fResult;
    *port_index = res.fPortIndex;
}

void JackSocketClientChannel::PortUnRegister(int refnum, jack_port_id_t port_index, int* result)
{
    JackPortUnRegisterRequest req(refnum, port_index);
    JackResult res;
    //ServerSyncCall(&req, &res, result);
	if (req.Write(&fRequestSocket) < 0) {
        jack_error("Could not write request type = %ld", req.fHeader.fType);
        *result = -1;
        return ;
    }

    if (res.Read(&fRequestSocket) < 0) {
        jack_error("Could not read result type = %ld", req.fHeader.fType);
        *result = -1;
        return ;
    }

    *result = res.fResult;
}

void JackSocketClientChannel::PortConnect(int refnum, const char* src, const char* dst, int* result)
{
    JackPortConnectNameRequest req(refnum, src, dst);
    JackResult res;
    //ServerSyncCall(&req, &res, result);
	if (req.Write(&fRequestSocket) < 0) {
        jack_error("Could not write request type = %ld", req.fHeader.fType);
        *result = -1;
        return ;
    }

    if (res.Read(&fRequestSocket) < 0) {
        jack_error("Could not read result type = %ld", req.fHeader.fType);
        *result = -1;
        return ;
    }

    *result = res.fResult;
}

void JackSocketClientChannel::PortDisconnect(int refnum, const char* src, const char* dst, int* result)
{
    JackPortDisconnectNameRequest req(refnum, src, dst);
    JackResult res;
    //ServerSyncCall(&req, &res, result);
	if (req.Write(&fRequestSocket) < 0) {
        jack_error("Could not write request type = %ld", req.fHeader.fType);
        *result = -1;
        return ;
    }

    if (res.Read(&fRequestSocket) < 0) {
        jack_error("Could not read result type = %ld", req.fHeader.fType);
        *result = -1;
        return ;
    }

    *result = res.fResult;
}

void JackSocketClientChannel::PortConnect(int refnum, jack_port_id_t src, jack_port_id_t dst, int* result)
{
    JackPortConnectRequest req(refnum, src, dst);
    JackResult res;
    //ServerSyncCall(&req, &res, result);
	if (req.Write(&fRequestSocket) < 0) {
        jack_error("Could not write request type = %ld", req.fHeader.fType);
        *result = -1;
        return ;
    }

    if (res.Read(&fRequestSocket) < 0) {
        jack_error("Could not read result type = %ld", req.fHeader.fType);
        *result = -1;
        return ;
    }

    *result = res.fResult;
}

void JackSocketClientChannel::PortDisconnect(int refnum, jack_port_id_t src, jack_port_id_t dst, int* result)
{
    JackPortDisconnectRequest req(refnum, src, dst);
    JackResult res;
    //ServerSyncCall(&req, &res, result);
	if (req.Write(&fRequestSocket) < 0) {
        jack_error("Could not write request type = %ld", req.fHeader.fType);
        *result = -1;
        return ;
    }

    if (res.Read(&fRequestSocket) < 0) {
        jack_error("Could not read result type = %ld", req.fHeader.fType);
        *result = -1;
        return ;
    }

    *result = res.fResult;
}

void JackSocketClientChannel::SetBufferSize(jack_nframes_t buffer_size, int* result)
{
    JackSetBufferSizeRequest req(buffer_size);
    JackResult res;
    //ServerSyncCall(&req, &res, result);
	if (req.Write(&fRequestSocket) < 0) {
        jack_error("Could not write request type = %ld", req.fHeader.fType);
        *result = -1;
        return ;
    }

    if (res.Read(&fRequestSocket) < 0) {
        jack_error("Could not read result type = %ld", req.fHeader.fType);
        *result = -1;
        return ;
    }

    *result = res.fResult;
}

void JackSocketClientChannel::SetFreewheel(int onoff, int* result)
{
    JackSetFreeWheelRequest req(onoff);
    JackResult res;
    //ServerSyncCall(&req, &res, result);
	if (req.Write(&fRequestSocket) < 0) {
        jack_error("Could not write request type = %ld", req.fHeader.fType);
        *result = -1;
        return ;
    }

    if (res.Read(&fRequestSocket) < 0) {
        jack_error("Could not read result type = %ld", req.fHeader.fType);
        *result = -1;
        return ;
    }

    *result = res.fResult;
}

void JackSocketClientChannel::ReleaseTimebase(int refnum, int* result)
{
    JackReleaseTimebaseRequest req(refnum);
    JackResult res;
    //ServerSyncCall(&req, &res, result);
	if (req.Write(&fRequestSocket) < 0) {
        jack_error("Could not write request type = %ld", req.fHeader.fType);
        *result = -1;
        return ;
    }

    if (res.Read(&fRequestSocket) < 0) {
        jack_error("Could not read result type = %ld", req.fHeader.fType);
        *result = -1;
        return ;
    }

    *result = res.fResult;
}

void JackSocketClientChannel::SetTimebaseCallback(int refnum, int conditional, int* result)
{
    JackSetTimebaseCallbackRequest req(refnum, conditional);
    JackResult res;
    //ServerSyncCall(&req, &res, result);
	if (req.Write(&fRequestSocket) < 0) {
        jack_error("Could not write request type = %ld", req.fHeader.fType);
        *result = -1;
        return ;
    }

    if (res.Read(&fRequestSocket) < 0) {
        jack_error("Could not read result type = %ld", req.fHeader.fType);
        *result = -1;
        return ;
    }

    *result = res.fResult;
}

bool JackSocketClientChannel::Init()
{
    JackLog("JackSocketClientChannel::Init \n");
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

    res.fResult = fClient->ClientNotify(event.fRefNum, event.fName, event.fNotify, event.fSync, event.fValue);

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


