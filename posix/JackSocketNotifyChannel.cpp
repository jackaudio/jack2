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

#include "JackRequest.h"
#include "JackSocketNotifyChannel.h"
#include "JackError.h"
#include "JackConstants.h"

namespace Jack
{

// Server to client
int JackSocketNotifyChannel::Open(const char* name)
{
    jack_log("JackSocketNotifyChannel::Open name = %s", name);

    // Connect to client listen socket
    if (fNotifySocket.Connect(jack_client_dir, name, 0) < 0) {
        jack_error("Cannot connect client socket");
        return -1;
    }
    
    // Use a time out for notifications
    fNotifySocket.SetReadTimeOut(SOCKET_TIME_OUT);
    return 0;
}

void JackSocketNotifyChannel::Close()
{
    jack_log("JackSocketNotifyChannel::Close");
    fNotifySocket.Close();
}

void JackSocketNotifyChannel::ClientNotify(int refnum, const char* name, int notify, int sync, const char* message, int value1, int value2, int* result)
{
    JackClientNotification event(name, refnum, notify, sync, message, value1, value2);
    JackResult res;

    // Send notification
    if (event.Write(&fNotifySocket) < 0) {
        jack_error("Could not write notification");
        *result = -1;
        return;
    }

    // Read the result in "synchronous" mode only
    if (sync) {
        // Get result : use a time out
        if (res.Read(&fNotifySocket) < 0) {
            jack_error("Could not read notification result");
            *result = -1;
        } else {
            *result = res.fResult;
        }
    } else {
        *result = 0;
    }
}

} // end of namespace


