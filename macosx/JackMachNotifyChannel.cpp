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

#include "JackMachNotifyChannel.h"
#include "JackRPCClient.h"
#include "JackError.h"
#include "JackConstants.h"

namespace Jack
{

// Server side : server to client

int JackMachNotifyChannel::Open(const char* name)
{
    JackLog("JackMachNotifyChannel::Open name = %s\n", name);

    char buf[256];
    snprintf(buf, sizeof(buf) - 1, "%s:%s", jack_client_entry, name);

    // Connect to client notification port using client name
    if (!fClientPort.ConnectPort(buf)) {
        jack_error("Cannot connect client port");
        return -1;
    } else {
        return 0;
    }
}

void JackMachNotifyChannel::Close()
{
    fClientPort.DisconnectPort();
}

void JackMachNotifyChannel::ClientNotify(int refnum, const char* name, int notify, int sync, int value, int* result)
{
    kern_return_t res = (sync)
                        ? rpc_jack_client_sync_notify(fClientPort.GetPort(), refnum, (char*)name, notify, value, result)
                        : rpc_jack_client_async_notify(fClientPort.GetPort(), refnum, (char*)name, notify, value);
    if (res == KERN_SUCCESS) {
        *result = 0;
    } else {
		jack_error("JackMachNotifyChannel::ClientNotify: name = %s notify = %ld err = %s", name, notify, mach_error_string(res));
		*result = -1;
	}
}

} // end of namespace


