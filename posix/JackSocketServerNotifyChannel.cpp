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

#include "JackSocketServerNotifyChannel.h"
#include "JackError.h"
#include "JackRequest.h"
#include "JackConstants.h"
#include "JackNotification.h"
#include "JackServerGlobals.h"

namespace Jack
{

int JackSocketServerNotifyChannel::Open(const char* server_name)
{
    if (fRequestSocket.Connect(jack_server_dir, server_name, 0) < 0) {
        jack_error("Cannot connect to server socket");
        return -1;
    } else {
        fRequestSocket.SetNonBlocking(true);
        JackServerGlobals::fRTNotificationSocket = fRequestSocket.GetFd();
        return 0;
    }
}

void JackSocketServerNotifyChannel::Close()
{
    fRequestSocket.Close();
}

/*
The requirement is that the Notification from RT thread can be delivered... not sure using a socket is adequate here...
Can the write operation block?
A non blocking write operation shoud be used : check if write can succeed, and ignore the notification otherwise
(since its mainly used for XRun, ignoring a notification is OK, successive XRun will come...)
*/
void JackSocketServerNotifyChannel::Notify(int refnum, int notify, int value)
{
    JackClientNotificationRequest req(refnum, notify, value);
    if (req.Write(&fRequestSocket) < 0) {
        jack_error("Could not write notification ref = %d notify = %d", refnum, notify);
    }
}

void JackSocketServerNotifyChannel::NotifyQuit()
{
    Notify(-1, kQUIT, 0);
}

} // end of namespace


