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

#include "JackWinNamedPipeServerChannel.h"
#include "JackRequest.h"
#include "JackServer.h"
#include "JackEngine.h"
#include "JackGlobals.h"
#include "JackClient.h"
#include <assert.h>

using namespace std;

namespace Jack
{

HANDLE	JackClientPipeThread::fMutex = NULL;  // never released....

// fRefNum = 0 is used as a "running" state for the JackWinNamedPipeServerNotifyChannel object
// fRefNum = -1 correspond to a not running client

JackClientPipeThread::JackClientPipeThread(JackWinNamedPipeClient* pipe)
        : fPipe(pipe), fServer(NULL), fRefNum(0)
{
    fThread = JackGlobals::MakeThread(this);
    if (fMutex == NULL)
        fMutex = CreateMutex(NULL, FALSE, NULL);
}

JackClientPipeThread::~JackClientPipeThread()
{
    delete fPipe;
    delete fThread;
}

int JackClientPipeThread::Open(JackServer* server)	// Open the Server/Client connection
{
    fServer = server;

    // Start listening
    if (fThread->Start() != 0){
        jack_error("Cannot start Jack server listener\n");
        return -1;
    } else {
        return 0;
    }
}

void JackClientPipeThread::Close()					// Close the Server/Client connection
{
    fThread->Kill();
}

bool JackClientPipeThread::Execute()
{
    JackLog("JackClientPipeThread::Execute\n");

	if (HandleRequest(fPipe) < 0) {
		return false;
	} else {
		return true;
	}
}

int JackClientPipeThread::HandleRequest(JackWinNamedPipeClient* pipe)
{
    // Read header
    JackRequest header;
    if (header.Read(pipe) < 0) {
        jack_error("HandleRequest: cannot read header");
		 // Lock the global mutex
		if (WaitForSingleObject(fMutex, INFINITE) == WAIT_FAILED)
			jack_error("JackClientPipeThread::HandleRequest: mutex wait error");
		KillClient(fPipe);
		// Unlock the global mutex
		ReleaseMutex(fMutex);
        return -1;
    }

    // Lock the global mutex
    if (WaitForSingleObject(fMutex, INFINITE) == WAIT_FAILED)
        jack_error("JackClientPipeThread::HandleRequest: mutex wait error");

    // Read data
    switch (header.fType) {

        case JackRequest::kClientNew: {
                JackLog("JackRequest::ClientNew\n");
                JackClientNewRequest req;
                JackClientNewResult res;
                req.Read(pipe);
                AddClient(pipe, req.fName, &res.fSharedEngine, &res.fSharedClient, &res.fSharedPorts, &res.fResult);
                res.Write(pipe);
                break;
            }

        case JackRequest::kClientClose: {
                JackLog("JackRequest::ClientClose\n");
                JackClientCloseRequest req;
                JackResult res;
                req.Read(pipe);
                res.fResult = fServer->GetEngine()->ClientClose(req.fRefNum);
                res.Write(pipe);
                RemoveClient(pipe, req.fRefNum);
                break;
            }

        case JackRequest::kActivateClient: {
                JackActivateRequest req;
                JackResult res;
                JackLog("JackRequest::ActivateClient\n");
                req.Read(pipe);
                res.fResult = fServer->Activate(req.fRefNum);
                res.Write(pipe);
                break;
            }

        case JackRequest::kDeactivateClient: {
                JackLog("JackRequest::DeactivateClient\n");
                JackDeactivateRequest req;
                JackResult res;
                req.Read(pipe);
                res.fResult = fServer->Deactivate(req.fRefNum);
                res.Write(pipe);
                break;
            }

        case JackRequest::kRegisterPort: {
                JackLog("JackRequest::RegisterPort\n");
                JackPortRegisterRequest req;
                JackPortRegisterResult res;
                req.Read(pipe);
                res.fResult = fServer->GetEngine()->PortRegister(req.fRefNum, req.fName, req.fFlags, req.fBufferSize, &res.fPortIndex);
                res.Write(pipe);
                break;
            }

        case JackRequest::kUnRegisterPort: {
                JackLog("JackRequest::UnRegisterPort\n");
                JackPortUnRegisterRequest req;
                JackResult res;
                req.Read(pipe);
                res.fResult = fServer->GetEngine()->PortUnRegister(req.fRefNum, req.fPortIndex);
                res.Write(pipe);
                break;
            }

        case JackRequest::kConnectNamePorts: {
                JackLog("JackRequest::ConnectPorts\n");
                JackPortConnectNameRequest req;
                JackResult res;
                req.Read(pipe);
                res.fResult = fServer->GetEngine()->PortConnect(req.fRefNum, req.fSrc, req.fDst);
                res.Write(pipe);
                break;
            }

        case JackRequest::kDisconnectNamePorts: {
                JackLog("JackRequest::DisconnectPorts\n");
                JackPortDisconnectNameRequest req;
                JackResult res;
                req.Read(pipe);
                res.fResult = fServer->GetEngine()->PortDisconnect(req.fRefNum, req.fSrc, req.fDst);
                res.Write(pipe);
                break;
            }

        case JackRequest::kConnectPorts: {
                JackLog("JackRequest::ConnectPorts\n");
                JackPortConnectRequest req;
                JackResult res;
                req.Read(pipe);
                res.fResult = fServer->GetEngine()->PortConnect(req.fRefNum, req.fSrc, req.fDst);
                res.Write(pipe);
                break;
            }

        case JackRequest::kDisconnectPorts: {
                JackLog("JackRequest::DisconnectPorts\n");
                JackPortDisconnectRequest req;
                JackResult res;
                req.Read(pipe);
                res.fResult = fServer->GetEngine()->PortDisconnect(req.fRefNum, req.fSrc, req.fDst);
                res.Write(pipe);
                break;
            }

        case JackRequest::kSetBufferSize: {
                JackLog("JackRequest::SetBufferSize\n");
                JackSetBufferSizeRequest req;
                JackResult res;
                req.Read(pipe);
                res.fResult = fServer->SetBufferSize(req.fBufferSize);
                res.Write(pipe);
                break;
            }

        case JackRequest::kSetFreeWheel: {
                JackLog("JackRequest::SetFreeWheel\n");
                JackSetFreeWheelRequest req;
                JackResult res;
                req.Read(pipe);
                res.fResult = fServer->SetFreewheel(req.fOnOff);
                res.Write(pipe);
                break;
            }

        case JackRequest::kReleaseTimebase: {
                JackLog("JackRequest::kReleaseTimebase\n");
                JackReleaseTimebaseRequest req;
                JackResult res;
                req.Read(pipe);
                res.fResult = fServer->GetEngine()->ReleaseTimebase(req.fRefNum);
                res.Write(pipe);
                break;
            }

        case JackRequest::kSetTimebaseCallback: {
                JackLog("JackRequest::kSetTimebaseCallback\n");
                JackSetTimebaseCallbackRequest req;
                JackResult res;
                req.Read(pipe);
                res.fResult = fServer->GetEngine()->SetTimebaseCallback(req.fRefNum, req.fConditionnal);
                res.Write(pipe);
                break;
            }

        case JackRequest::kNotification: {
                JackLog("JackRequest::Notification\n");
                JackClientNotificationRequest req;
                req.Read(pipe);
                fServer->Notify(req.fRefNum, req.fNotify, req.fValue);
                break;
            }

        default:
            JackLog("Unknown request %ld\n", header.fType);
            break;
    }

    // Unlock the global mutex
    ReleaseMutex(fMutex);
    return 0;
}

void JackClientPipeThread::AddClient(JackWinNamedPipeClient* pipe, char* name, int* shared_engine, int* shared_client, int* shared_ports, int* result)
{
    JackLog("JackWinNamedPipeServerChannel::AddClient %s\n", name);
    fRefNum = -1;
    *result = fServer->GetEngine()->ClientNew(name, &fRefNum, shared_engine, shared_client, shared_ports);
}

void JackClientPipeThread::RemoveClient(JackWinNamedPipeClient* pipe, int refnum)
{
    JackLog("JackWinNamedPipeServerChannel::RemoveClient ref = %d\n", refnum);
    fRefNum = -1;
    pipe->Close();
}

void JackClientPipeThread::KillClient(JackWinNamedPipeClient* pipe)
{
    JackLog("JackClientPipeThread::KillClient \n");
    JackLog("JackWinNamedPipeServerChannel::KillClient ref = %d\n", fRefNum);

    if (fRefNum == -1) {		// Correspond to an already removed client.
        JackLog("Kill a closed client\n");
	} else if (fRefNum == 0) {  // Correspond to a still not opened client.
        JackLog("Kill a not opened client\n");  
    } else {
        fServer->Notify(fRefNum, JackNotifyChannelInterface::kDeadClient, 0);
    }

    fRefNum = -1;
    pipe->Close();
}

JackWinNamedPipeServerChannel::JackWinNamedPipeServerChannel()
{
    fThread = JackGlobals::MakeThread(this);
}

JackWinNamedPipeServerChannel::~JackWinNamedPipeServerChannel()
{
    delete fThread;
}

int JackWinNamedPipeServerChannel::Open(JackServer* server)
{
    JackLog("JackWinNamedPipeServerChannel::Open \n");

    fServer = server;

    // Needed for internal connection from JackWinNamedPipeServerNotifyChannel object
    if (fRequestListenPipe.Bind(jack_server_dir, 0) < 0) {
        jack_error("JackWinNamedPipeServerChannel::Open : cannot create result listen pipe");
        return false;
    }

    // Start listening
    if (fThread->Start() != 0) {
        jack_error("Cannot start Jack server listener\n");
        goto error;
    }

    return 0;

error:
    fRequestListenPipe.Close();
    return -1;
}

void JackWinNamedPipeServerChannel::Close()
{
    fThread->Kill();
    fRequestListenPipe.Close();
}

bool JackWinNamedPipeServerChannel::Init()
{
    JackLog("JackWinNamedPipeServerChannel::Init \n");
    JackWinNamedPipeClient* pipe;

	// Accept first client, that is the JackWinNamedPipeServerNotifyChannel object
    if ((pipe = fRequestListenPipe.AcceptClient()) == NULL) {
        jack_error("JackWinNamedPipeServerChannel::Init : cannot connect pipe");
        return false;
    } else {
        AddClient(pipe);
        return true;
    }
}

bool JackWinNamedPipeServerChannel::Execute()
{
    JackWinNamedPipeClient* pipe;

    if (fRequestListenPipe.Bind(jack_server_dir, 0) < 0) {
        jack_error("JackWinNamedPipeServerChannel::Open : cannot create result listen pipe");
        return false;
    }

    if ((pipe = fRequestListenPipe.AcceptClient()) == NULL) {
        jack_error("JackWinNamedPipeServerChannel::Open : cannot connect pipe");
        return false;
    }

    AddClient(pipe);
    return true;
}

void JackWinNamedPipeServerChannel::AddClient(JackWinNamedPipeClient* pipe)
{
    // Remove dead clients
    std::list<JackClientPipeThread*>::iterator it = fClientList.begin();
    JackClientPipeThread* client;
    while (it != fClientList.end()) {
        client = *it;
        if (!client->IsRunning()) { // Dead client
            JackLog("Remove client from list\n");
            it = fClientList.erase(it);
            delete(client);
        } else {
            it++;
        }
    }

    client = new JackClientPipeThread(pipe);
    client->Open(fServer);
    fClientList.push_back(client);
}

} // end of namespace


