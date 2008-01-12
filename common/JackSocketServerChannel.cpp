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
    JackLog("JackSocketServerChannel::Open \n");
	fServer = server;

    // Prepare request socket
    if (fRequestListenSocket.Bind(jack_server_dir, server_name, 0) < 0) {
        JackLog("JackSocketServerChannel::Open : cannot create result listen socket\n");
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
    JackLog("JackSocketServerChannel::ClientCreate socket\n");
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
    JackLog("JackSocketServerChannel::ClientAdd\n");
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
    JackLog("JackSocketServerChannel::ClientRemove ref = %d\n", refnum);
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
    JackLog("JackSocketServerChannel::ClientKill ref = %d\n", refnum);

    if (refnum == -1) {  // Should never happen... correspond to a client that started the socket but never opened...
        jack_error("Client not opened");
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
                JackLog("JackRequest::kClientCheck\n");
                JackClientCheckRequest req;
                JackClientCheckResult res;
                if (req.Read(socket) == 0)
					res.fResult = fServer->GetEngine()->ClientCheck(req.fName, res.fName, req.fProtocol, req.fOptions, &res.fStatus);
                res.Write(socket);
                break;
            }

        case JackRequest::kClientOpen: {
                JackLog("JackRequest::ClientOpen\n");
                JackClientOpenRequest req;
                JackClientOpenResult res;
                if (req.Read(socket) == 0)
					ClientAdd(fd, req.fName, &res.fSharedEngine, &res.fSharedClient, &res.fSharedGraph, &res.fResult);
                res.Write(socket);
                break;
            }

        case JackRequest::kClientClose: {
                JackLog("JackRequest::ClientClose\n");
                JackClientCloseRequest req;
                JackResult res;
                if (req.Read(socket) == 0)
					res.fResult = fServer->GetEngine()->ClientExternalClose(req.fRefNum);
                res.Write(socket);
                ClientRemove(fd, req.fRefNum);
                break;
            }

        case JackRequest::kActivateClient: {
                JackActivateRequest req;
                JackResult res;
                JackLog("JackRequest::ActivateClient\n");
                if (req.Read(socket) == 0)
					res.fResult = fServer->GetEngine()->ClientActivate(req.fRefNum);
                res.Write(socket);
                break;
            }

        case JackRequest::kDeactivateClient: {
                JackLog("JackRequest::DeactivateClient\n");
                JackDeactivateRequest req;
                JackResult res;
                if (req.Read(socket) == 0)
					res.fResult = fServer->GetEngine()->ClientDeactivate(req.fRefNum);
                res.Write(socket);
                break;
            }

        case JackRequest::kRegisterPort: {
                JackLog("JackRequest::RegisterPort\n");
                JackPortRegisterRequest req;
                JackPortRegisterResult res;
                if (req.Read(socket) == 0)
					res.fResult = fServer->GetEngine()->PortRegister(req.fRefNum, req.fName, req.fPortType, req.fFlags, req.fBufferSize, &res.fPortIndex);
                res.Write(socket);
                break;
            }

        case JackRequest::kUnRegisterPort: {
                JackLog("JackRequest::UnRegisterPort\n");
                JackPortUnRegisterRequest req;
                JackResult res;
                if (req.Read(socket) == 0)
					res.fResult = fServer->GetEngine()->PortUnRegister(req.fRefNum, req.fPortIndex);
                res.Write(socket);
                break;
            }

        case JackRequest::kConnectNamePorts: {
                JackLog("JackRequest::ConnectPorts\n");
                JackPortConnectNameRequest req;
                JackResult res;
                if (req.Read(socket) == 0)
					res.fResult = fServer->GetEngine()->PortConnect(req.fRefNum, req.fSrc, req.fDst);
                res.Write(socket);
                break;
            }

        case JackRequest::kDisconnectNamePorts: {
                JackLog("JackRequest::DisconnectPorts\n");
                JackPortDisconnectNameRequest req;
                JackResult res;
                if (req.Read(socket) == 0)
					res.fResult = fServer->GetEngine()->PortDisconnect(req.fRefNum, req.fSrc, req.fDst);
                res.Write(socket);
                break;
            }

        case JackRequest::kConnectPorts: {
                JackLog("JackRequest::ConnectPorts\n");
                JackPortConnectRequest req;
                JackResult res;
                if (req.Read(socket) == 0)
					res.fResult = fServer->GetEngine()->PortConnect(req.fRefNum, req.fSrc, req.fDst);
                res.Write(socket);
                break;
            }

        case JackRequest::kDisconnectPorts: {
                JackLog("JackRequest::DisconnectPorts\n");
                JackPortDisconnectRequest req;
                JackResult res;
                if (req.Read(socket) == 0)
					res.fResult = fServer->GetEngine()->PortDisconnect(req.fRefNum, req.fSrc, req.fDst);
                res.Write(socket);
                break;
            }

        case JackRequest::kSetBufferSize: {
                JackLog("JackRequest::SetBufferSize\n");
                JackSetBufferSizeRequest req;
                JackResult res;
                if (req.Read(socket) == 0)
					res.fResult = fServer->SetBufferSize(req.fBufferSize);
                res.Write(socket);
                break;
            }

        case JackRequest::kSetFreeWheel: {
                JackLog("JackRequest::SetFreeWheel\n");
                JackSetFreeWheelRequest req;
                JackResult res;
                if (req.Read(socket) == 0)
					res.fResult = fServer->SetFreewheel(req.fOnOff);
                res.Write(socket);
                break;
            }

        case JackRequest::kReleaseTimebase: {
                JackLog("JackRequest::kReleaseTimebase\n");
                JackReleaseTimebaseRequest req;
                JackResult res;
                if (req.Read(socket) == 0)
					res.fResult = fServer->GetEngine()->ReleaseTimebase(req.fRefNum);
                res.Write(socket);
                break;
            }

        case JackRequest::kSetTimebaseCallback: {
                JackLog("JackRequest::kSetTimebaseCallback\n");
                JackSetTimebaseCallbackRequest req;
                JackResult res;
                if (req.Read(socket) == 0)
					res.fResult = fServer->GetEngine()->SetTimebaseCallback(req.fRefNum, req.fConditionnal);
                res.Write(socket);
                break;
            }
			
		case JackRequest::kGetInternalClientName: {
                JackLog("JackRequest::kGetInternalClientName\n");
                JackGetInternalClientNameRequest req;
                JackGetInternalClientNameResult res;
                if (req.Read(socket) == 0)
					res.fResult = fServer->GetEngine()->GetInternalClientName(req.fIntRefNum, res.fName);
                res.Write(socket);
                break;
            }
			
		case JackRequest::kInternalClientHandle: {
                JackLog("JackRequest::kInternalClientHandle\n");
                JackInternalClientHandleRequest req;
                JackInternalClientHandleResult res;
                if (req.Read(socket) == 0)
					res.fResult = fServer->GetEngine()->InternalClientHandle(req.fName, &res.fStatus, &res.fIntRefNum);
                res.Write(socket);
                break;
            }
			
		case JackRequest::kInternalClientLoad: {
                JackLog("JackRequest::kInternalClientLoad\n");
                JackInternalClientLoadRequest req;
                JackInternalClientLoadResult res;
                if (req.Read(socket) == 0)
					res.fResult = fServer->InternalClientLoad(req.fName, req.fDllName, req.fLoadInitName, req.fOptions, &res.fIntRefNum, &res.fStatus);
                res.Write(socket);
                break;
            }
			
		case JackRequest::kInternalClientUnload: {
                JackLog("JackRequest::kInternalClientUnload\n");
                JackInternalClientUnloadRequest req;
                JackInternalClientUnloadResult res;
                if (req.Read(socket) == 0)
					res.fResult = fServer->GetEngine()->InternalClientUnload(req.fIntRefNum, &res.fStatus);
                res.Write(socket);
                break;
            }

        case JackRequest::kNotification: {
                JackLog("JackRequest::Notification\n");
                JackClientNotificationRequest req;
				if (req.Read(socket) == 0)
					fServer->Notify(req.fRefNum, req.fNotify, req.fValue);
                break;
            }

        default:
            JackLog("Unknown request %ld\n", header.fType);
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

        JackLog("JackSocketServerChannel::BuildPoolTable size = %d\n", fSocketTable.size() + 1);

        // First fd is the server request socket
        fPollTable[0].fd = fRequestListenSocket.GetFd();
        fPollTable[0].events = POLLIN | POLLERR;

        // Next fd for clients
        map<int, pair<int, JackClientSocket*> >::iterator it;
        int i;

        for (i = 1, it = fSocketTable.begin(); it != fSocketTable.end(); it++, i++) {
            JackLog("fSocketTable i = %ld fd = %ld\n", i, it->first);
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
            JackLog("fPollTable i = %ld fd = %ld\n", i, fd);
            if (fPollTable[i].revents & ~POLLIN) {
                jack_error("Poll client error err = %s", strerror(errno));
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


