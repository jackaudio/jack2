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

#include "JackMachServerNotifyChannel.h"
#include "JackRPCEngineUser.c"
#include "JackConstants.h"
#include "JackError.h"
#include <stdio.h>

namespace Jack
{

int JackMachServerNotifyChannel::Open(const char* server_name)
{
    jack_log("JackMachServerChannel::Open");
    char jack_server_entry_name[512];
    snprintf(jack_server_entry_name, sizeof(jack_server_entry_name), "%s_%s", jack_server_entry, server_name);

    if (!fClientPort.ConnectPort(jack_server_entry_name)) {
        jack_error("Cannot connect to server port");
        return -1;
    } else {
        return 0;
    }
}

void JackMachServerNotifyChannel::Close()
{
    //fClientPort.DisconnectPort(); pas nécessaire car le JackMachServerChannel a déja disparu?
}

void JackMachServerNotifyChannel::Notify(int refnum, int notify, int value)
{
    kern_return_t res = rpc_jack_client_rt_notify(fClientPort.GetPort(), refnum, notify, value, 0);
    if (res != KERN_SUCCESS) {
        jack_error("Could not write request ref = %ld notify = %ld err = %s", refnum, notify, mach_error_string(res));
    }
}

} // end of namespace


