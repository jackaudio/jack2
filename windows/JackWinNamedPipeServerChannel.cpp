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


#include "JackWinNamedPipeServerChannel.h"
#include "JackNotification.h"
#include "JackRequest.h"
#include "JackServer.h"
#include "JackLockedEngine.h"
#include "JackGlobals.h"
#include "JackClient.h"
#include "JackNotification.h"
#include "JackException.h"
#include <assert.h>

using namespace std;

namespace Jack
{

HANDLE JackClientPipeThread::fMutex = NULL;  // Never released....

// fRefNum = -1 correspond to already removed client

JackClientPipeThread::JackClientPipeThread(JackWinNamedPipeClient* pipe)
    :fPipe(pipe), fDecoder(NULL), fServer(NULL), fThread(this), fRefNum(0)
{
    // First one allocated the static fMutex
    if (fMutex == NULL) {
        fMutex = CreateMutex(NULL, FALSE, NULL);
    }
}

JackClientPipeThread::~JackClientPipeThread()
{
    jack_log("JackClientPipeThread::~JackClientPipeThread");
    delete fPipe;
}

int JackClientPipeThread::Open(JackServer* server)      // Open the Server/Client connection
{
    // Start listening
    if (fThread.Start() != 0) {
        jack_error("Cannot start Jack server listener\n");
        return -1;
    } else {
        fDecoder = new JackRequestDecoder(server, this);
        fServer = server;
        return 0;
    }
}

void JackClientPipeThread::Close()                                      // Close the Server/Client connection
{
    jack_log("JackClientPipeThread::Close 0 %x %ld", this, fRefNum);

    //fThread.Kill();
    fPipe->Close();
    fRefNum = -1;

    delete fDecoder;
    fDecoder = NULL;
}

bool JackClientPipeThread::Execute()
{
    try {

        jack_log("JackClientPipeThread::Execute %x", this);
        JackRequest header;
        int res = header.Read(fPipe);
        bool ret = true;

        // Lock the global mutex
        if (WaitForSingleObject(fMutex, INFINITE) == WAIT_FAILED) {
            jack_error("JackClientPipeThread::Execute : mutex wait error");
        }

        // Decode header
        if (res < 0) {
            jack_log("JackClientPipeThread::Execute : cannot decode header");
            ClientKill();
            ret = false;
        // Decode request
        } else if (fDecoder->HandleRequest(fPipe, header.fType) < 0) {
            ret = false;
        }

        // Unlock the global mutex
        if (!ReleaseMutex(fMutex)) {
            jack_error("JackClientPipeThread::Execute : mutex release error");
        }
        return ret;

    } catch (JackQuitException& e) {
        jack_log("JackClientPipeThread::Execute : JackQuitException");
        return false;
    }
}

void JackClientPipeThread::ClientAdd(detail::JackChannelTransactionInterface* socket, JackClientOpenRequest* req, JackClientOpenResult *res)
{
    jack_log("JackClientPipeThread::ClientAdd %x %s", this, req->fName);
    fRefNum = -1;
    res->fResult = fServer->GetEngine()->ClientExternalOpen(req->fName, req->fPID, req->fUUID, &fRefNum, &res->fSharedEngine, &res->fSharedClient, &res->fSharedGraph);
}

void JackClientPipeThread::ClientRemove(detail::JackChannelTransactionInterface* socket_aux, int refnum)
{
    jack_log("JackClientPipeThread::ClientRemove ref = %d", refnum);
    Close();
}

void JackClientPipeThread::ClientKill()
{
    jack_log("JackClientPipeThread::ClientKill ref = %d", fRefNum);

    if (fRefNum == -1) {        // Correspond to an already removed client.
        jack_log("Kill a closed client %x", this);
    } else if (fRefNum == 0) {  // Correspond to a still not opened client.
        jack_log("Kill a not opened client %x", this);
    } else {
        fServer->ClientKill(fRefNum);
    }

    Close();
}

JackWinNamedPipeServerChannel::JackWinNamedPipeServerChannel():fThread(this)
{}

JackWinNamedPipeServerChannel::~JackWinNamedPipeServerChannel()
{
    std::list<JackClientPipeThread*>::iterator it;

    for (it = fClientList.begin(); it !=  fClientList.end(); it++) {
        JackClientPipeThread* client = *it;
        client->Close();
        delete client;
    }
}

int JackWinNamedPipeServerChannel::Open(const char* server_name, JackServer* server)
{
    jack_log("JackWinNamedPipeServerChannel::Open");
    snprintf(fServerName, sizeof(fServerName), server_name);

    // Needed for internal connection from JackWinNamedPipeServerNotifyChannel object
    if (ClientListen()) {
        fServer = server;
        return 0;
    } else {
        jack_error("JackWinNamedPipeServerChannel::Open : cannot create result listen pipe");
        return -1;
    }
}

void JackWinNamedPipeServerChannel::Close()
{
    /* TODO : solve WIN32 thread Kill issue
    This would hang the server... since we are quitting it, its not really problematic,
    all ressources will be deallocated at the end.

    fRequestListenPipe.Close();
    fThread.Stop();
    */

    fRequestListenPipe.Close();
}

int JackWinNamedPipeServerChannel::Start()
{
    if (fThread.Start() != 0) {
        jack_error("Cannot start Jack server listener");
        return -1;
    } else {
        return 0;
    }
}

void JackWinNamedPipeServerChannel::Stop()
{
    fThread.Kill();
}

bool JackWinNamedPipeServerChannel::Init()
{
    jack_log("JackWinNamedPipeServerChannel::Init");
    // Accept first client, that is the JackWinNamedPipeServerNotifyChannel object
    return ClientAccept();
}

bool JackWinNamedPipeServerChannel::ClientListen()
{
    if (fRequestListenPipe.Bind(jack_server_dir, fServerName, 0) < 0) {
        jack_error("JackWinNamedPipeServerChannel::ClientListen : cannot create result listen pipe");
        return false;
    } else {
        return true;
    }
}

bool JackWinNamedPipeServerChannel::ClientAccept()
{
     JackWinNamedPipeClient* pipe;

     if ((pipe = fRequestListenPipe.AcceptClient()) == NULL) {
        jack_error("JackWinNamedPipeServerChannel::ClientAccept : cannot connect pipe");
        return false;
    } else {
        ClientAdd(pipe);
        return true;
    }
}

bool JackWinNamedPipeServerChannel::Execute()
{
    if (!ClientListen()) {
       return false;
    }

    return ClientAccept();
}

void JackWinNamedPipeServerChannel::ClientAdd(JackWinNamedPipeClient* pipe)
{
    // Remove dead (= not running anymore) clients.
    std::list<JackClientPipeThread*>::iterator it = fClientList.begin();
    JackClientPipeThread* client;

    jack_log("JackWinNamedPipeServerChannel::ClientAdd size %ld", fClientList.size());

    while (it != fClientList.end()) {
        client = *it;
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


