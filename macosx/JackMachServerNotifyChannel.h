/*
Copyright (C) 2004-2006 Grame  

This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef __JackMachServerNotifyChannel__
#define __JackMachServerNotifyChannel__

#include "JackChannel.h"
#include "JackMachPort.h"

namespace Jack
{

/*!
\brief JackServerNotifyChannel using Mach IPC.
*/

class JackMachServerNotifyChannel : public JackServerNotifyChannelInterface
{

    private:

        JackMachPort fClientPort;    /*! Mach port to communicate with the server : from client to server */

    public:

        JackMachServerNotifyChannel()
        {}
        virtual ~JackMachServerNotifyChannel()
        {}

        int Open();         // Open the Server/Client connection
        void Close();       // Close the Server/Client connection

        void ClientNotify(int refnum, int notify, int value);
};

} // end of namespace

#endif

