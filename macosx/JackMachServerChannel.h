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

#ifndef __JackMachServerChannel__
#define __JackMachServerChannel__

#include "JackChannel.h"
#include "JackThread.h"
#include "JackMachPort.h"
#include <map>

namespace Jack
{

class JackServer;
class JackEngineInterface;

/*!
\brief JackServerChannel using Mach IPC.
*/

class JackMachServerChannel : public JackServerChannelInterface, public JackRunnableInterface
{

    private:

        JackMachPortSet fServerPort;    /*! Mach port to communicate with the server : from client to server */
        JackThread* fThread;			/*! Thread to execute the event loop */
        JackServer* fServer;
        std::map<mach_port_t, int> fClientTable;

        static boolean_t MessageHandler(mach_msg_header_t* Request, mach_msg_header_t* Reply);

    public:

        JackMachServerChannel();
        virtual ~JackMachServerChannel();

        int Open(const char* server_name, JackServer* server);	// Open the Server/Client connection
        void Close();                   // Close the Server/Client connection

        JackEngineInterface* GetEngine();
        JackServer* GetServer();

        void ClientCheck(char* name, char* name_res, int protocol, int options, int* status, int* result);
        void ClientOpen(char* name, int pid, mach_port_t* private_port, int* shared_engine, int* shared_client, int* shared_graph, int* result);
        void ClientClose(mach_port_t private_port, int refnum);
        void ClientKill(mach_port_t private_port);

        bool Execute();

        static std::map<mach_port_t, JackMachServerChannel*> fPortTable;
};

} // end of namespace

#endif

