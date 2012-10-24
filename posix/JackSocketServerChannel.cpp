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
#include "JackLockedEngine.h"
#include "JackGlobals.h"
#include "JackServerGlobals.h"
#include "JackClient.h"
#include "JackTools.h"
#include "JackNotification.h"
#include "JackException.h"

#include <assert.h>
#include <signal.h>

using namespace std;

namespace Jack
{

JackSocketServerChannel::JackSocketServerChannel():
    fThread(this), fDecoder(NULL)
{
    fPollTable = NULL;
    fRebuild = true;
}

JackSocketServerChannel::~JackSocketServerChannel()
{
    delete[] fPollTable;
}

int JackSocketServerChannel::Open(const char* server_name, JackServer* server)
{
    jack_log("JackSocketServerChannel::Open");
  
    // Prepare request socket
    if (fRequestListenSocket.Bind(jack_server_dir, server_name, 0) < 0) {
        jack_log("JackSocketServerChannel::Open : cannot create result listen socket");
        return -1;
    }

    // Prepare for poll
    BuildPoolTable();
    
    fDecoder = new JackRequestDecoder(server, this);
    fServer = server;
    return 0;
}

void JackSocketServerChannel::Close()
{
   fRequestListenSocket.Close();

    // Close remaining client sockets
    std::map<int, std::pair<int, JackClientSocket*> >::iterator it;

    for (it = fSocketTable.begin(); it != fSocketTable.end(); it++) {
        pair<int, JackClientSocket*> elem = (*it).second;
        JackClientSocket* socket = elem.second;
        assert(socket);
        socket->Close();
        delete socket;
    }

    delete fDecoder;
    fDecoder = NULL;
}

int JackSocketServerChannel::Start()
{
    if (fThread.Start() != 0) {
        jack_error("Cannot start Jack server listener");
        return -1;
    } else {
        return 0;
    }
}

void JackSocketServerChannel::Stop()
{
    fThread.Stop();
}

void JackSocketServerChannel::ClientCreate()
{
    jack_log("JackSocketServerChannel::ClientCreate socket");
    JackClientSocket* socket = fRequestListenSocket.Accept();
    if (socket) {
        fSocketTable[socket->GetFd()] = make_pair(-1, socket);
        fRebuild = true;
    } else {
        jack_error("Client socket cannot be created");
    }
}

int JackSocketServerChannel::GetFd(JackClientSocket* socket_aux)
{
    std::map<int, std::pair<int, JackClientSocket*> >::iterator it;

    for (it = fSocketTable.begin(); it != fSocketTable.end(); it++) {
        pair<int, JackClientSocket*> elem = (*it).second;
        JackClientSocket* socket = elem.second;
        if (socket_aux == socket) {
            return (*it).first;
        }
    }
    return -1;
}

void JackSocketServerChannel::ClientAdd(detail::JackChannelTransactionInterface* socket_aux, JackClientOpenRequest* req, JackClientOpenResult *res)
{
    int refnum = -1;
    res->fResult = fServer->GetEngine()->ClientExternalOpen(req->fName, req->fPID, req->fUUID, &refnum, &res->fSharedEngine, &res->fSharedClient, &res->fSharedGraph);
    if (res->fResult == 0) {
        JackClientSocket* socket = dynamic_cast<JackClientSocket*>(socket_aux);
        assert(socket);
        int fd = GetFd(socket);
        assert(fd >= 0);
        fSocketTable[fd].first = refnum;
        fRebuild = true;
        jack_log("JackSocketServerChannel::ClientAdd ref = %d fd = %d", refnum, fd);
    #ifdef __APPLE__
        int on = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, (const char*)&on, sizeof(on)) < 0) {
            jack_log("JackSocketServerChannel::ClientAdd : setsockopt SO_NOSIGPIPE fd = %ld err = %s", fd, strerror(errno));
        }
    #endif
    } else {
        jack_error("Cannot create new client");
    }
}

void JackSocketServerChannel::ClientRemove(detail::JackChannelTransactionInterface* socket_aux, int refnum)
{
    JackClientSocket* socket = dynamic_cast<JackClientSocket*>(socket_aux);
    assert(socket);
    int fd = GetFd(socket);
    assert(fd >= 0);

    jack_log("JackSocketServerChannel::ClientRemove ref = %d fd = %d", refnum, fd);
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
    
    if (refnum == -1) {  // Should never happen... correspond to a client that started the socket but never opened...
        jack_log("Client was not opened : probably correspond to server_check");
    } else {
        fServer->ClientKill(refnum);
    }

    jack_log("JackSocketServerChannel::ClientKill ref = %d fd = %d", refnum, fd);
    fSocketTable.erase(fd);
    socket->Close();
    delete socket;
    fRebuild = true;
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
            jack_log("JackSocketServerChannel::BuildPoolTable fSocketTable i = %ld fd = %ld", i, it->first);
            fPollTable[i].fd = it->first;
            fPollTable[i].events = POLLIN | POLLPRI | POLLERR | POLLHUP | POLLNVAL;
        }
    }
}

bool JackSocketServerChannel::Init()
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &set, 0);
    return true;
}

bool JackSocketServerChannel::Execute()
{
    try {

        // Global poll
        if ((poll(fPollTable, fSocketTable.size() + 1, 10000) < 0) && (errno != EINTR)) {
            jack_error("JackSocketServerChannel::Execute : engine poll failed err = %s request thread quits...", strerror(errno));
            return false;
        } else {

            // Poll all clients
            for (unsigned int i = 1; i < fSocketTable.size() + 1; i++) {
                int fd = fPollTable[i].fd;
                jack_log("JackSocketServerChannel::Execute : fPollTable i = %ld fd = %ld", i, fd);
                if (fPollTable[i].revents & ~POLLIN) {
                    jack_log("JackSocketServerChannel::Execute : poll client error err = %s", strerror(errno));
                    ClientKill(fd);
                } else if (fPollTable[i].revents & POLLIN) {
                    JackClientSocket* socket = fSocketTable[fd].second;
                    // Decode header
                    JackRequest header;
                    if (header.Read(socket) < 0) {
                        jack_log("JackSocketServerChannel::Execute : cannot decode header");
                        ClientKill(fd);
                    // Decode request
                    } else {
                        // Result is not needed here
                        fDecoder->HandleRequest(socket, header.fType);
                    }
                }
            }

            // Check the server request socket */
            if (fPollTable[0].revents & POLLERR) {
                jack_error("Error on server request socket err = %s", strerror(errno));
            }

            if (fPollTable[0].revents & POLLIN) {
                ClientCreate();
            }
        }

        BuildPoolTable();
        return true;

    } catch (JackQuitException& e) {
        jack_log("JackSocketServerChannel::Execute : JackQuitException");
        return false;
    }
}

} // end of namespace


