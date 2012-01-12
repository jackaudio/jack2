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
#include "JackRequestDecoder.h"

#include <poll.h>
#include <map>

namespace Jack
{

class JackServer;

/*!
\brief JackServerChannel using sockets.
*/

class JackSocketServerChannel : public JackRunnableInterface, public JackClientHandlerInterface
{

    private:

        JackServerSocket fRequestListenSocket;  // Socket to create request socket for the client
        JackThread fThread;                     // Thread to execute the event loop
        JackRequestDecoder* fDecoder;
        JackServer* fServer;

        pollfd* fPollTable;
        bool fRebuild;
        std::map<int, std::pair<int, JackClientSocket*> > fSocketTable;

        void BuildPoolTable();

        void ClientCreate();
        void ClientKill(int fd);
  
        void ClientAdd(detail::JackChannelTransactionInterface* socket, JackClientOpenRequest* req, JackClientOpenResult *res);
        void ClientRemove(detail::JackChannelTransactionInterface* socket, int refnum);

        int GetFd(JackClientSocket* socket);

    public:

        JackSocketServerChannel();
        ~JackSocketServerChannel();

        int Open(const char* server_name, JackServer* server);  // Open the Server/Client connection
        void Close();                                           // Close the Server/Client connection

        int Start();
        void Stop();

        // JackRunnableInterface interface
        bool Init();
        bool Execute();
};

} // end of namespace

#endif

