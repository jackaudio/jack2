/*
Copyright (C) 2004-2008 Grame

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

#include "JackLibClient.h"
#include "JackMachClientChannel.h"
#include "JackRPCEngine.h"
#include "JackLibGlobals.h"
#include <assert.h>

using namespace Jack;

#define rpc_type kern_return_t  // for astyle

rpc_type rpc_jack_client_sync_notify(mach_port_t client_port, int refnum, client_name_t name, int notify, int value1, int value2, int* result)
{
    jack_log("rpc_jack_client_sync_notify ref = %ld name = %s notify = %ld val1 = %ld val2 = %ld", refnum, name, notify, value1, value2);
    JackClient* client = gClientTable[client_port];
    assert(client);
    *result = client->ClientNotify(refnum, name, notify, true, value1, value2);
    return KERN_SUCCESS;
}

rpc_type rpc_jack_client_async_notify(mach_port_t client_port, int refnum, client_name_t name, int notify,  int value1, int value2)
{
    jack_log("rpc_jack_client_async_notify ref = %ld name = %s notify = %ld val1 = %ld val2 = %ld", refnum, name, notify, value1, value2);
    JackClient* client = gClientTable[client_port];
    assert(client);
    client->ClientNotify(refnum, name, notify, false, value1, value2);
    return KERN_SUCCESS;
}

