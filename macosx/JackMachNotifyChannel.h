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

#ifndef __JackMachNotifyChannel__
#define __JackMachNotifyChannel__

#include "JackMachPort.h"

namespace Jack
{

/*!
\brief JackNotifyChannel using Mach IPC.
*/

class JackMachNotifyChannel
{

    private:

        JackMachPort fClientPort;    /*! Mach port to communicate with the client : from server to client */

    public:

        JackMachNotifyChannel()
        {}

        int Open(const char* name);		// Open the Server/Client connection
        void Close();					// Close the Server/Client connection

        void ClientNotify(int refnum, const char* name, int notify, int sync, const char* message, int value1, int value2, int* result);
};

} // end of namespace

#endif

