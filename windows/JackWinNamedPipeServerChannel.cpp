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

#ifdef WIN32
#pragma warning (disable : 4786)
#endif

#include "JackWinNamedPipeServerChannel.h"
#include "JackNotification.h"
#include "JackRequest.h"
#include "JackServer.h"
#include "JackEngine.h"
#include "JackGlobals.h"
#include "JackClient.h"
#include <assert.h>

using namespace std;

namespace Jack
{

HANDLE JackClientPipeThread::fMutex = NULL;  // never released....

// fRefNum = -1 correspond to already removed client

JackClientPipeThread::JackClientPipeThread(JackWinNamedPipeClient* pipe)
        : fPipe(pipe), fServer(NULL), fRefNum(0)
{
    fThread = JackGlobals::MakeThread(this);
    if (fMutex == NULL)
        fMutex = CreateMutex(NULL, FALSE, NULL);
}

JackClientPipeThread::~JackClientPipeThread()
{
    JackLog("JackClientPipeThread::~JackClientPipeThread\n");
    delete fPipe;
    delete fThread;
}

int JackClientPipeThread::Open(JackServer* server)	// Open the Server/Client connection
{
    fServer = server;

    // Start listening
    if (fThread->Start() != 0) {
        jack_error("Cannot start Jack server listener\n");
        return -1;
    } else {
        return 0;
    }
}

void JackClientPipeThread::Close()					// Close the Server/Client connection
{
    JackLog("JackClientPipeThread::Close %x %ld\n", this, fRefNum);
    /*
    	TODO : solve WIN32 thread Kill issue
    	This would hang.. since Close will be followed by a delete, 
    	all ressources will be desallocated at the end.
    */

    fThread->Kill();
    fPipe->Close();
    fRefNum = -1;
}

bool JackClientPipeThread::Execute()
{
    JackLog("JackClientPipeThread::Execute\n");
    return (HandleRequest() == 0);
}

int JackClientPipeThread::HandleRequest()
{
    // Read header
    JackRequest header;
    int res = header.Read(fPipe);
    int ret = 0;

    // Lock the global mutex
    if (WaitForSingleObject(fMutex, INFINITE) == WAIT_FAILED)
        jack_error("JackClientPipeThread::HandleRequest: mutex wait error");

    if (res < 0) {
        jack_error("HandleRequest: cannot read header");
        ClientKill();
        ret = -1;
    } else {

        // Read data
        switch (header.fType) {

            case JackRequest::kClientCheck: {
                JackLog("JackRequest::kClientCheck\n");
                JackClientCheckRequest req;
                JackClientCheckResult res;
                if (req.Read(fPipe) == 0)
                    res.fResult = fServer->GetEngine()->ClientCheck(req.fName, res.fName, req.fProtocol, req.fOptions, &res.fStatus);
                res.Write(fPipe);
                break;
            }

            case JackRequest::kClientOpen: {
                JackLog("JackRequest::ClientOpen\n");
                JackClientOpenRequest req;
                JackClientOpenResult res;
                if (req.Read(fPipe) == 0)
                    ClientAdd(req.fName, &res.fSharedEngine, &res.fSharedClient, &res.fSharedGraph, &res.fResult);
                res.Write(fPipe);
                break;
            }

            case JackRequest::kClientClose: {
                JackLog("JackRequest::ClientClose\n");
                JackClientCloseRequest req;
                JackResult res;
                if (req.Read(fPipe) == 0)
                    res.fResult = fServer->GetEngine()->ClientExternalClose(req.fRefNum);
                // No write: client is actually doing an "ServerAsyncCall", and not interested by the result
                ClientRemove();
                ret = -1;
                break;
            }

            case JackRequest::kActivateClient: {
                JackActivateRequest req;
                JackResult res;
                JackLog("JackRequest::ActivateClient\n");
                if (req.Read(fPipe) == 0)
                    res.fResult = fServer->GetEngine()->ClientActivate(req.fRefNum);
                res.Write(fPipe);
                break;
            }

            case JackRequest::kDeactivateClient: {
                JackLog("JackRequest::DeactivateClient\n");
                JackDeactivateRequest req;
                JackResult res;
                if (req.Read(fPipe) == 0)
                    res.fResult = fServer->GetEngine()->ClientDeactivate(req.fRefNum);
                res.Write(fPipe);
                break;
            }

            case JackRequest::kRegisterPort: {
                JackLog("JackRequest::RegisterPort\n");
                JackPortRegisterRequest req;
                JackPortRegisterResult res;
                if (req.Read(fPipe) == 0)
                    res.fResult = fServer->GetEngine()->PortRegister(req.fRefNum, req.fName, req.fPortType, req.fFlags, req.fBufferSize, &res.fPortIndex);
                res.Write(fPipe);
                break;
            }

            case JackRequest::kUnRegisterPort: {
                JackLog("JackRequest::UnRegisterPort\n");
                JackPortUnRegisterRequest req;
                JackResult res;
                if (req.Read(fPipe) == 0)
                    res.fResult = fServer->GetEngine()->PortUnRegister(req.fRefNum, req.fPortIndex);
                res.Write(fPipe);
                break;
            }

            case JackRequest::kConnectNamePorts: {
                JackLog("JackRequest::ConnectPorts\n");
                JackPortConnectNameRequest req;
                JackResult res;
                if (req.Read(fPipe) == 0)
                    res.fResult = fServer->GetEngine()->PortConnect(req.fRefNum, req.fSrc, req.fDst);
                res.Write(fPipe);
                break;
            }

            case JackRequest::kDisconnectNamePorts: {
                JackLog("JackRequest::DisconnectPorts\n");
                JackPortDisconnectNameRequest req;
                JackResult res;
                if (req.Read(fPipe) == 0)
                    res.fResult = fServer->GetEngine()->PortDisconnect(req.fRefNum, req.fSrc, req.fDst);
                res.Write(fPipe);
                break;
            }

            case JackRequest::kConnectPorts: {
                JackLog("JackRequest::ConnectPorts\n");
                JackPortConnectRequest req;
                JackResult res;
                if (req.Read(fPipe) == 0)
                    res.fResult = fServer->GetEngine()->PortConnect(req.fRefNum, req.fSrc, req.fDst);
                res.Write(fPipe);
                break;
            }

            case JackRequest::kDisconnectPorts: {
                JackLog("JackRequest::DisconnectPorts\n");
                JackPortDisconnectRequest req;
                JackResult res;
                if (req.Read(fPipe) == 0)
                    res.fResult = fServer->GetEngine()->PortDisconnect(req.fRefNum, req.fSrc, req.fDst);
                res.Write(fPipe);
                break;
            }

            case JackRequest::kSetBufferSize: {
                JackLog("JackRequest::SetBufferSize\n");
                JackSetBufferSizeRequest req;
                JackResult res;
                if (req.Read(fPipe) == 0)
                    res.fResult = fServer->SetBufferSize(req.fBufferSize);
                res.Write(fPipe);
                break;
            }

            case JackRequest::kSetFreeWheel: {
                JackLog("JackRequest::SetFreeWheel\n");
                JackSetFreeWheelRequest req;
                JackResult res;
                if (req.Read(fPipe) == 0)
                    res.fResult = fServer->SetFreewheel(req.fOnOff);
                res.Write(fPipe);
                break;
            }

            case JackRequest::kReleaseTimebase: {
                JackLog("JackRequest::kReleaseTimebase\n");
                JackReleaseTimebaseRequest req;
                JackResult res;
                if (req.Read(fPipe) == 0)
                    res.fResult = fServer->ReleaseTimebase(req.fRefNum);
                res.Write(fPipe);
                break;
            }

            case JackRequest::kSetTimebaseCallback: {
                JackLog("JackRequest::kSetTimebaseCallback\n");
                JackSetTimebaseCallbackRequest req;
                JackResult res;
                if (req.Read(fPipe) == 0)
                    res.fResult = fServer->SetTimebaseCallback(req.fRefNum, req.fConditionnal);
                res.Write(fPipe);
                break;
            }

            case JackRequest::kGetInternalClientName: {
                JackLog("JackRequest::kGetInternalClientName\n");
                JackGetInternalClientNameRequest req;
                JackGetInternalClientNameResult res;
                if (req.Read(fPipe) == 0)
                    res.fResult = fServer->GetEngine()->GetInternalClientName(req.fIntRefNum, res.fName);
                res.Write(fPipe);
                break;
            }

            case JackRequest::kInternalClientHandle: {
                JackLog("JackRequest::kInternalClientHandle\n");
                JackInternalClientHandleRequest req;
                JackInternalClientHandleResult res;
                if (req.Read(fPipe) == 0)
                    res.fResult = fServer->GetEngine()->InternalClientHandle(req.fName, &res.fStatus, &res.fIntRefNum);
                res.Write(fPipe);
                break;
            }

            case JackRequest::kInternalClientLoad: {
                JackLog("JackRequest::kInternalClientLoad\n");
                JackInternalClientLoadRequest req;
                JackInternalClientLoadResult res;
                if (req.Read(fPipe) == 0)
                    res.fResult = fServer->InternalClientLoad(req.fName, req.fDllName, req.fLoadInitName, req.fOptions, &res.fIntRefNum, &res.fStatus);
                res.Write(fPipe);
                break;
            }

            case JackRequest::kInternalClientUnload: {
                JackLog("JackRequest::kInternalClientUnload\n");
                JackInternalClientUnloadRequest req;
                JackInternalClientUnloadResult res;
                if (req.Read(fPipe) == 0)
                    res.fResult = fServer->GetEngine()->InternalClientUnload(req.fIntRefNum, &res.fStatus);
                res.Write(fPipe);
                break;
            }

            case JackRequest::kNotification: {
                JackLog("JackRequest::Notification\n");
                JackClientNotificationRequest req;
                if (req.Read(fPipe) == 0)
                    fServer->Notify(req.fRefNum, req.fNotify, req.fValue);
                break;
            }

            default:
                JackLog("Unknown request %ld\n", header.fType);
                break;
        }
    }

    // Unlock the global mutex
    ReleaseMutex(fMutex);
    return ret;
}

void JackClientPipeThread::ClientAdd(char* name, int* shared_engine, int* shared_client, int* shared_graph, int* result)
{
    JackLog("JackClientPipeThread::ClientAdd %s\n", name);
    fRefNum = -1;
    *result = fServer->GetEngine()->ClientExternalOpen(name, &fRefNum, shared_engine, shared_client, shared_graph);
}

void JackClientPipeThread::ClientRemove()
{
    JackLog("JackClientPipeThread::ClientRemove ref = %d\n", fRefNum);
    /* TODO : solve WIN32 thread Kill issue
    Close();
    */
    fRefNum = -1;
    fPipe->Close();
}

void JackClientPipeThread::ClientKill()
{
    JackLog("JackClientPipeThread::ClientKill ref = %d\n", fRefNum);

    if (fRefNum == -1) {		// Correspond to an already removed client.
        JackLog("Kill a closed client\n");
    } else if (fRefNum == 0) {  // Correspond to a still not opened client.
        JackLog("Kill a not opened client\n");
    } else {
        fServer->Notify(fRefNum, kDeadClient, 0);
    }

    Close();
}

JackWinNamedPipeServerChannel::JackWinNamedPipeServerChannel()
{
    fThread = JackGlobals::MakeThread(this);
}

JackWinNamedPipeServerChannel::~JackWinNamedPipeServerChannel()
{
    std::list<JackClientPipeThread*>::iterator it;

    for (it = fClientList.begin(); it !=  fClientList.end(); it++) {
        JackClientPipeThread* client = *it;
        client->Close();
        delete client;
    }

    delete fThread;
}

int JackWinNamedPipeServerChannel::Open(const char* server_name, JackServer* server)
{
    JackLog("JackWinNamedPipeServerChannel::Open \n");

    fServer = server;
    snprintf(fServerName, sizeof(fServerName), server_name);

    // Needed for internal connection from JackWinNamedPipeServerNotifyChannel object
    if (fRequestListenPipe.Bind(jack_server_dir, server_name, 0) < 0) {
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
    /* TODO : solve WIN32 thread Kill issue
    	This would hang the server... since we are quitting it, its not really problematic, 
    	all ressources will be desallocated at the end.
    	
    	fRequestListenPipe.Close();
    	fThread->Stop();
    */

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
        ClientAdd(pipe);
        return true;
    }
}

bool JackWinNamedPipeServerChannel::Execute()
{
    JackWinNamedPipeClient* pipe;

    if (fRequestListenPipe.Bind(jack_server_dir, fServerName, 0) < 0) {
        jack_error("JackWinNamedPipeServerChannel::Open : cannot create result listen pipe");
        return false;
    }

    if ((pipe = fRequestListenPipe.AcceptClient()) == NULL) {
        jack_error("JackWinNamedPipeServerChannel::Open : cannot connect pipe");
        return false;
    }

    ClientAdd(pipe);
    return true;
}

void JackWinNamedPipeServerChannel::ClientAdd(JackWinNamedPipeClient* pipe)
{
    // Remove dead (= not running anymore) clients.
    std::list<JackClientPipeThread*>::iterator it = fClientList.begin();
    JackClientPipeThread* client;

    JackLog("ClientAdd size  %ld\n", fClientList.size());

    while (it != fClientList.end()) {
        client = *it;
        JackLog("Remove dead client = %x running =  %ld\n", client, client->IsRunning());
        if (client->IsRunning()) {
            it++;
        } else {
            it = fClientList.erase(it);
            delete client;
        }
    }

    client = new JackClientPipeThread(pipe);
    client->Open(fServer);
    // Here we are sure that the client is running (because it's thread is in "running" state).
    fClientList.push_back(client);
}

} // end of namespace


