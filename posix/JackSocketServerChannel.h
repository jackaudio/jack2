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

#ifndef __JackSocketServerChannel__
#define __JackSocketServerChannel__

#include "JackSocket.h"
#include "JackPlatformPlug.h"
#include <poll.h>
#include <map>

namespace Jack
{

class JackServer;

/*!
\brief JackServerChannel using sockets.
*/

class JackSocketServerChannel : public JackRunnableInterface
{

    private:

        JackServerSocket fRequestListenSocket;  // Socket to create request socket for the client
        JackThread fThread;                     // Thread to execute the event loop
        JackServer* fServer;
        pollfd* fPollTable;
        bool fRebuild;
        std::map<int, std::pair<int, JackClientSocket*> > fSocketTable;

        bool HandleRequest(int fd);
        void BuildPoolTable();

        void ClientCreate();
        void ClientAdd(int fd, char* name, int pid, int uuid, int* shared_engine, int* shared_client, int* shared_graph, int* result);
        void ClientRemove(int fd, int refnum);
        void ClientKill(int fd);

    public:

        JackSocketServerChannel();
        ~JackSocketServerChannel();

        int Open(const char* server_name, JackServer* server);  // Open the Server/Client connection
        void Close();                                           // Close the Server/Client connection

        int Start();

        // JackRunnableInterface interface
        bool Init();
        bool Execute();
};

} // end of namespace

#endif

