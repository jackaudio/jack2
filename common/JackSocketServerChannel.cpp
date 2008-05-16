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

#include "JackSocketServerChannel.h"
#include "JackRequest.h"
#include "JackServer.h"
#include "JackEngine.h"
#include "JackGlobals.h"
#include "JackClient.h"
#include "JackNotification.h"
#include <assert.h>

using namespace std;

namespace Jack
{

JackSocketServerChannel::JackSocketServerChannel()
{
    fThread = JackGlobals::MakeThread(this);
    fPollTable = NULL;
    fRebuild = true;
}

JackSocketServerChannel::~JackSocketServerChannel()
{
    delete fThread;
    delete[] fPollTable;
}

int JackSocketServerChannel::Open(const char* server_name, JackServer* server)
{
    jack_log("JackSocketServerChannel::Open ");
    fServer = server;

    // Prepare request socket
    if (fRequestListenSocket.Bind(jack_server_dir, server_name, 0) < 0) {
        jack_log("JackSocketServerChannel::Open : cannot create result listen socket");
        return -1;
    }

    // Prepare for poll
    BuildPoolTable();

    // Start listening
    if (fThread->Start() != 0) {
        jack_error("Cannot start Jack server listener");
        goto error;
    }

    return 0;

error:
    fRequestListenSocket.Close();
    return -1;
}

void JackSocketServerChannel::Close()
{
    fThread->Kill();
    fRequestListenSocket.Close();
}

void JackSocketServerChannel::ClientCreate()
{
    jack_log("JackSocketServerChannel::ClientCreate socket");
    JackClientSocket* socket = fRequestListenSocket.Accept();
    if (socket) {
        fSocketTable[socket->GetFd()] = make_pair( -1, socket);
        fRebuild = true;
    } else {
        jack_error("Client socket cannot be created");
    }
}

void JackSocketServerChannel::ClientAdd(int fd, char* name, int* shared_engine, int* shared_client, int* shared_graph, int* result)
{
    jack_log("JackSocketServerChannel::ClientAdd");
    int refnum = -1;
    *result = fServer->GetEngine()->ClientExternalOpen(name, &refnum, shared_engine, shared_client, shared_graph);
    if (*result == 0) {
        fSocketTable[fd].first = refnum;
        fRebuild = true;
    } else {
        jack_error("Cannot create new client");
    }
}

void JackSocketServerChannel::ClientRemove(int fd, int refnum)
{
    pair<int, JackClientSocket*> elem = fSocketTable[fd];
    JackClientSocket* socket = elem.second;
    assert(socket);
    jack_log("JackSocketServerChannel::ClientRemove ref = %d", refnum);
    fSocketTable.erase(fd);
    socket->Close();
    delete socket;
    fRebuild = true;
}

void JackSocketServerChannel::ClientKill(int fd)
{
    pair<int, JackClientSocket*> elem = fSocketTable[fd];
    JackClientSocket* socket = elem.second;
    int refnum = elem.first;

    assert(socket);
    jack_log("JackSocketServerChannel::ClientKill ref = %d", refnum);

    if (refnum == -1) {  // Should never happen... correspond to a client that started the socket but never opened...
        jack_log("Client was not opened : probably correspond to server_check");
    } else {
        fServer->Notify(refnum, kDeadClient, 0);
    }

    fSocketTable.erase(fd);
    socket->Close();
    delete socket;
    fRebuild = true;
}

int JackSocketServerChannel::HandleRequest(int fd)
{
    pair<int, JackClientSocket*> elem = fSocketTable[fd];
    JackClientSocket* socket = elem.second;
    assert(socket);

    // Read header
    JackRequest header;
    if (header.Read(socket) < 0) {
        jack_error("HandleRequest: cannot read header");
        return -1;
    }

    // Read data
    switch (header.fType) {

        case JackRequest::kClientCheck: {
            jack_log("JackRequest::kClientCheck");
            JackClientCheckRequest req;
            JackClientCheckResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->GetEngine()->ClientCheck(req.fName, res.fName, req.fProtocol, req.fOptions, &res.fStatus);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::kClientCheck write error name = %s", req.fName);
            break;
        }

        case JackRequest::kClientOpen: {
            jack_log("JackRequest::ClientOpen");
            JackClientOpenRequest req;
            JackClientOpenResult res;
            if (req.Read(socket) == 0)
                ClientAdd(fd, req.fName, &res.fSharedEngine, &res.fSharedClient, &res.fSharedGraph, &res.fResult);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::ClientOpen write error name = %s", req.fName);
            break;
        }

        case JackRequest::kClientClose: {
            jack_log("JackRequest::ClientClose");
            JackClientCloseRequest req;
            JackResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->GetEngine()->ClientExternalClose(req.fRefNum);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::ClientClose write error ref = %d", req.fRefNum);
            ClientRemove(fd, req.fRefNum);
            break;
        }

        case JackRequest::kActivateClient: {
            JackActivateRequest req;
            JackResult res;
            jack_log("JackRequest::ActivateClient");
            if (req.Read(socket) == 0)
                res.fResult = fServer->GetEngine()->ClientActivate(req.fRefNum, ref.fState);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::ActivateClient write error ref = %d", req.fRefNum);
            break;
        }

        case JackRequest::kDeactivateClient: {
            jack_log("JackRequest::DeactivateClient");
            JackDeactivateRequest req;
            JackResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->GetEngine()->ClientDeactivate(req.fRefNum);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::DeactivateClient write error ref = %d", req.fRefNum);
            break;
        }

        case JackRequest::kRegisterPort: {
            jack_log("JackRequest::RegisterPort");
            JackPortRegisterRequest req;
            JackPortRegisterResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->GetEngine()->PortRegister(req.fRefNum, req.fName, req.fPortType, req.fFlags, req.fBufferSize, &res.fPortIndex);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::RegisterPort write error ref = %d", req.fRefNum);
            break;
        }

        case JackRequest::kUnRegisterPort: {
            jack_log("JackRequest::UnRegisterPort");
            JackPortUnRegisterRequest req;
            JackResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->GetEngine()->PortUnRegister(req.fRefNum, req.fPortIndex);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::UnRegisterPort write error ref = %d", req.fRefNum);
            break;
        }

        case JackRequest::kConnectNamePorts: {
            jack_log("JackRequest::ConnectPorts");
            JackPortConnectNameRequest req;
            JackResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->GetEngine()->PortConnect(req.fRefNum, req.fSrc, req.fDst);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::ConnectNamePorts write error ref = %d", req.fRefNum);
            break;
        }

        case JackRequest::kDisconnectNamePorts: {
            jack_log("JackRequest::DisconnectPorts");
            JackPortDisconnectNameRequest req;
            JackResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->GetEngine()->PortDisconnect(req.fRefNum, req.fSrc, req.fDst);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::DisconnectNamePorts write error ref = %d", req.fRefNum);
            break;
        }

        case JackRequest::kConnectPorts: {
            jack_log("JackRequest::ConnectPorts");
            JackPortConnectRequest req;
            JackResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->GetEngine()->PortConnect(req.fRefNum, req.fSrc, req.fDst);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::ConnectPorts write error ref = %d", req.fRefNum);
            break;
        }

        case JackRequest::kDisconnectPorts: {
            jack_log("JackRequest::DisconnectPorts");
            JackPortDisconnectRequest req;
            JackResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->GetEngine()->PortDisconnect(req.fRefNum, req.fSrc, req.fDst);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::DisconnectPorts write error ref = %d", req.fRefNum);
            break;
        }

        case JackRequest::kSetBufferSize: {
            jack_log("JackRequest::SetBufferSize");
            JackSetBufferSizeRequest req;
            JackResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->SetBufferSize(req.fBufferSize);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::SetBufferSize write error");
            break;
        }

        case JackRequest::kSetFreeWheel: {
            jack_log("JackRequest::SetFreeWheel");
            JackSetFreeWheelRequest req;
            JackResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->SetFreewheel(req.fOnOff);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::SetFreeWheel write error");
            break;
        }

        case JackRequest::kReleaseTimebase: {
            jack_log("JackRequest::kReleaseTimebase");
            JackReleaseTimebaseRequest req;
            JackResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->ReleaseTimebase(req.fRefNum);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::kReleaseTimebase write error ref = %d", req.fRefNum);
            break;
        }

        case JackRequest::kSetTimebaseCallback: {
            jack_log("JackRequest::kSetTimebaseCallback");
            JackSetTimebaseCallbackRequest req;
            JackResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->SetTimebaseCallback(req.fRefNum, req.fConditionnal);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::SetTimebaseCallback write error ref = %d", req.fRefNum);
            break;
        }

        case JackRequest::kGetInternalClientName: {
            jack_log("JackRequest::kGetInternalClientName");
            JackGetInternalClientNameRequest req;
            JackGetInternalClientNameResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->GetEngine()->GetInternalClientName(req.fIntRefNum, res.fName);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::GetInternalClientName write error ref = %d", req.fRefNum);
            break;
        }

        case JackRequest::kInternalClientHandle: {
            jack_log("JackRequest::kInternalClientHandle");
            JackInternalClientHandleRequest req;
            JackInternalClientHandleResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->GetEngine()->InternalClientHandle(req.fName, &res.fStatus, &res.fIntRefNum);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::InternalClientHandle write error ref = %d", req.fRefNum);
            break;
        }

        case JackRequest::kInternalClientLoad: {
            jack_log("JackRequest::kInternalClientLoad");
            JackInternalClientLoadRequest req;
            JackInternalClientLoadResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->InternalClientLoad(req.fName, req.fDllName, req.fLoadInitName, req.fOptions, &res.fIntRefNum, &res.fStatus);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::InternalClientLoad write error name = %s", req.fName);
            break;
        }

        case JackRequest::kInternalClientUnload: {
            jack_log("JackRequest::kInternalClientUnload");
            JackInternalClientUnloadRequest req;
            JackInternalClientUnloadResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->GetEngine()->InternalClientUnload(req.fIntRefNum, &res.fStatus);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::InternalClientUnload write error ref = %d", req.fRefNum);
            break;
        }

        case JackRequest::kNotification: {
            jack_log("JackRequest::Notification");
            JackClientNotificationRequest req;
            if (req.Read(socket) == 0)
                fServer->Notify(req.fRefNum, req.fNotify, req.fValue);
            break;
        }

        default:
            jack_log("Unknown request %ld", header.fType);
            break;
    }

    return 0;
}

void JackSocketServerChannel::BuildPoolTable()
{
    if (fRebuild) {
        fRebuild = false;
        delete[] fPollTable;
        fPollTable = new pollfd[fSocketTable.size() + 1];

        jack_log("JackSocketServerChannel::BuildPoolTable size = %d", fSocketTable.size() + 1);

        // First fd is the server request socket
        fPollTable[0].fd = fRequestListenSocket.GetFd();
        fPollTable[0].events = POLLIN | POLLERR;

        // Next fd for clients
        map<int, pair<int, JackClientSocket*> >::iterator it;
        int i;

        for (i = 1, it = fSocketTable.begin(); it != fSocketTable.end(); it++, i++) {
            jack_log("fSocketTable i = %ld fd = %ld", i, it->first);
            fPollTable[i].fd = it->first;
            fPollTable[i].events = POLLIN | POLLPRI | POLLERR | POLLHUP | POLLNVAL;
        }
    }
}

bool JackSocketServerChannel::Execute()
{
    // Global poll
    if ((poll(fPollTable, fSocketTable.size() + 1, 10000) < 0) && (errno != EINTR)) {
        jack_error("Engine poll failed err = %s request thread quits...", strerror(errno));
        return false;
    } else {

        // Poll all clients
        for (unsigned int i = 1; i < fSocketTable.size() + 1; i++) {
            int fd = fPollTable[i].fd;
            jack_log("fPollTable i = %ld fd = %ld", i, fd);
            if (fPollTable[i].revents & ~POLLIN) {
                jack_log("Poll client error err = %s", strerror(errno));
                ClientKill(fd);
            } else if (fPollTable[i].revents & POLLIN) {
                if (HandleRequest(fd) < 0) {
                    jack_error("Could not handle external client request");
                    //ClientRemove(fd); TO CHECK
                }
            }
        }

        // Check the server request socket */
        if (fPollTable[0].revents & POLLERR) {
            jack_error("Error on server request socket err = %s", strerror(errno));
            //return false; TO CHECK
        }

        if (fPollTable[0].revents & POLLIN) {
            ClientCreate();
        }
    }

    BuildPoolTable();
    return true;
}

} // end of namespace


