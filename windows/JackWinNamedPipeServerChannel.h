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

#ifndef __JackWinNamedPipeServerChannel__
#define __JackWinNamedPipeServerChannel__

#include "JackWinNamedPipe.h"
#include "JackPlatformPlug.h"
#include "JackConstants.h"
#include "JackRequestDecoder.h"
#include <list>

namespace Jack
{

class JackServer;

class JackClientPipeThread : public JackRunnableInterface, public JackClientHandlerInterface
{

    private:

        JackWinNamedPipeClient* fPipe;
        JackRequestDecoder* fDecoder;
        JackServer*	fServer;
        JackThread fThread;
        int fRefNum;

        void ClientAdd(detail::JackChannelTransactionInterface* socket, JackClientOpenRequest* req, JackClientOpenResult *res);
        void ClientRemove(detail::JackChannelTransactionInterface* socket, int refnum);

        void ClientKill();

        static HANDLE fMutex;

    public:

        JackClientPipeThread(JackWinNamedPipeClient* pipe);
        virtual ~JackClientPipeThread();

        int Open(JackServer* server);   // Open the Server/Client connection
        void Close();                   // Close the Server/Client connection

        // JackRunnableInterface interface
        bool Execute();

        // To be used for find out if the object can be deleted
        bool IsRunning()
        {
            return (fRefNum >= 0);
        }

};

/*!
\brief JackServerChannel using pipe.
*/

class JackWinNamedPipeServerChannel : public JackRunnableInterface
{

    private:

        JackWinNamedPipeServer fRequestListenPipe;	// Pipe to create request socket for the client
        JackServer*	fServer;
        JackThread fThread;                         // Thread to execute the event loop
        char fServerName[JACK_SERVER_CONTROL_NAME_SIZE];

        std::list<JackClientPipeThread*> fClientList;

        void ClientAdd(JackWinNamedPipeClient* pipe);

        bool ClientListen();
        bool ClientAccept();

    public:

        JackWinNamedPipeServerChannel();
        ~JackWinNamedPipeServerChannel();

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

