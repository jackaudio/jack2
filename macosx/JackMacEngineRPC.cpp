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

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "JackServer.h"
#include "JackLockedEngine.h"
#include "JackRPCEngine.h"
#include "JackMachServerChannel.h"
#include <assert.h>

using namespace Jack;

//-------------------
// Client management
//-------------------

#define rpc_type kern_return_t // for astyle

rpc_type server_rpc_jack_client_check(mach_port_t private_port, client_name_t name, client_name_t name_res, int protocol, int options, int* status, int* result)
{
    jack_log("rpc_jack_client_check");
    JackMachServerChannel* channel = JackMachServerChannel::fPortTable[private_port];
    assert(channel);
    channel->ClientCheck((char*)name, (char*)name_res, protocol, options, status, result);
    return KERN_SUCCESS;
}

rpc_type server_rpc_jack_client_open(mach_port_t server_port, client_name_t name, int pid, mach_port_t* private_port, int* shared_engine, int* shared_client, int* shared_graph, int* result)
{
    jack_log("rpc_jack_client_new %s", name);
    JackMachServerChannel* channel = JackMachServerChannel::fPortTable[server_port];
    assert(channel);
    channel->ClientOpen((char*)name, pid, private_port, shared_engine, shared_client, shared_graph, result);
    return KERN_SUCCESS;
}

rpc_type server_rpc_jack_client_close(mach_port_t private_port, int refnum, int* result)
{
    jack_log("rpc_jack_client_close");
    JackMachServerChannel* channel = JackMachServerChannel::fPortTable[private_port];
    assert(channel);
    channel->ClientClose(private_port, refnum);
    *result = 0;
    return KERN_SUCCESS;
}

rpc_type server_rpc_jack_client_activate(mach_port_t private_port, int refnum, int state, int* result)
{
    jack_log("rpc_jack_client_activate");
    JackMachServerChannel* channel = JackMachServerChannel::fPortTable[private_port];
    assert(channel);
    *result = channel->GetEngine()->ClientActivate(refnum, state);
    return KERN_SUCCESS;
}

rpc_type server_rpc_jack_client_deactivate(mach_port_t private_port, int refnum, int* result)
{
    jack_log("rpc_jack_client_deactivate");
    JackMachServerChannel* channel = JackMachServerChannel::fPortTable[private_port];
    assert(channel);
    *result = channel->GetEngine()->ClientDeactivate(refnum);
    return KERN_SUCCESS;
}

//-----------------
// Port management
//-----------------

rpc_type server_rpc_jack_port_register(mach_port_t private_port, int refnum, client_port_name_t name, client_port_type_t type, unsigned int flags, unsigned int buffer_size, unsigned int* port_index, int* result)
{
    jack_log("rpc_jack_port_register %ld %s", refnum, name);
    JackMachServerChannel* channel = JackMachServerChannel::fPortTable[private_port];
    assert(channel);
    *result = channel->GetEngine()->PortRegister(refnum, name, type, flags, buffer_size, port_index);
    return KERN_SUCCESS;
}

rpc_type server_rpc_jack_port_unregister(mach_port_t private_port, int refnum, int port, int* result)
{
    jack_log("rpc_jack_port_unregister %ld %ld ", refnum, port);
    JackMachServerChannel* channel = JackMachServerChannel::fPortTable[private_port];
    assert(channel);
    *result = channel->GetEngine()->PortUnRegister(refnum, port);
    return KERN_SUCCESS;
}

rpc_type server_rpc_jack_port_connect_name(mach_port_t private_port, int refnum, client_port_name_t src, client_port_name_t dst, int* result)
{
    jack_log("rpc_jack_port_connect_name");
    JackMachServerChannel* channel = JackMachServerChannel::fPortTable[private_port];
    assert(channel);
    *result = channel->GetEngine()->PortConnect(refnum, src, dst);
    return KERN_SUCCESS;
}

rpc_type server_rpc_jack_port_disconnect_name(mach_port_t private_port, int refnum, client_port_name_t src, client_port_name_t dst, int* result)
{
    jack_log("rpc_jack_port_disconnect_name");
    JackMachServerChannel* channel = JackMachServerChannel::fPortTable[private_port];
    assert(channel);
    *result = channel->GetEngine()->PortDisconnect(refnum, src, dst);
    return KERN_SUCCESS;
}

rpc_type server_rpc_jack_port_connect(mach_port_t private_port, int refnum, int src, int dst, int* result)
{
    jack_log("rpc_jack_port_connect");
    JackMachServerChannel* channel = JackMachServerChannel::fPortTable[private_port];
    assert(channel);
    *result = channel->GetEngine()->PortConnect(refnum, src, dst);
    return KERN_SUCCESS;
}

rpc_type server_rpc_jack_port_disconnect(mach_port_t private_port, int refnum, int src, int dst, int* result)
{
    jack_log("rpc_jack_port_disconnect");
    JackMachServerChannel* channel = JackMachServerChannel::fPortTable[private_port];
    assert(channel);
    *result = channel->GetEngine()->PortDisconnect(refnum, src, dst);
    return KERN_SUCCESS;
}

//------------------------
// Buffer size, freewheel
//------------------------

rpc_type server_rpc_jack_set_buffer_size(mach_port_t private_port, int buffer_size, int* result)
{
    jack_log("server_rpc_jack_set_buffer_size");
    JackMachServerChannel* channel = JackMachServerChannel::fPortTable[private_port];
    assert(channel);
    *result = channel->GetServer()->SetBufferSize(buffer_size);
    return KERN_SUCCESS;
}

rpc_type server_rpc_jack_set_freewheel(mach_port_t private_port, int onoff, int* result)
{
    jack_log("server_rpc_jack_set_freewheel");
    JackMachServerChannel* channel = JackMachServerChannel::fPortTable[private_port];
    assert(channel);
    *result = channel->GetServer()->SetFreewheel(onoff);
    return KERN_SUCCESS;
}

//----------------------
// Transport management
//----------------------

rpc_type server_rpc_jack_release_timebase(mach_port_t private_port, int refnum, int* result)
{
    jack_log("server_rpc_jack_release_timebase");
    JackMachServerChannel* channel = JackMachServerChannel::fPortTable[private_port];
    assert(channel);
    *result = channel->GetServer()->ReleaseTimebase(refnum);
    return KERN_SUCCESS;
}

rpc_type server_rpc_jack_set_timebase_callback(mach_port_t private_port, int refnum, int conditional, int* result)
{
    jack_log("server_rpc_jack_set_timebase_callback");
    JackMachServerChannel* channel = JackMachServerChannel::fPortTable[private_port];
    assert(channel);
    *result = channel->GetServer()->SetTimebaseCallback(refnum, conditional);
    return KERN_SUCCESS;
}

//------------------
// Internal clients
//------------------

rpc_type server_rpc_jack_get_internal_clientname(mach_port_t private_port, int refnum, int int_ref, client_name_t name_res, int* result)
{
    jack_log("server_rpc_jack_get_internal_clientname");
    JackMachServerChannel* channel = JackMachServerChannel::fPortTable[private_port];
    assert(channel);
    *result = channel->GetServer()->GetEngine()->GetInternalClientName(int_ref, (char*)name_res);
    return KERN_SUCCESS;
}

rpc_type server_rpc_jack_internal_clienthandle(mach_port_t private_port, int refnum, client_name_t client_name, int* status, int* int_ref, int* result)
{
    jack_log("server_rpc_jack_internal_clienthandle");
    JackMachServerChannel* channel = JackMachServerChannel::fPortTable[private_port];
    assert(channel);
    *result = channel->GetServer()->GetEngine()->InternalClientHandle(client_name, status, int_ref);
    return KERN_SUCCESS;
}

rpc_type server_rpc_jack_internal_clientload(mach_port_t private_port, int refnum, client_name_t client_name, so_name_t so_name, objet_data_t objet_data, int options, int* status, int* int_ref, int* result)
{
    jack_log("server_rpc_jack_internal_clientload");
    JackMachServerChannel* channel = JackMachServerChannel::fPortTable[private_port];
    assert(channel);
    *result = channel->GetServer()->InternalClientLoad(client_name, so_name, objet_data, options, int_ref, status);
    return KERN_SUCCESS;
}

rpc_type server_rpc_jack_internal_clientunload(mach_port_t private_port, int refnum, int int_ref, int* status, int* result)
{
    jack_log("server_rpc_jack_internal_clientunload");
    JackMachServerChannel* channel = JackMachServerChannel::fPortTable[private_port];
    assert(channel);
    *result = channel->GetServer()->GetEngine()->InternalClientUnload(int_ref, status);
    return KERN_SUCCESS;
}

//-----------------
// RT notification
//-----------------

rpc_type server_rpc_jack_client_rt_notify(mach_port_t server_port, int refnum, int notify, int value)
{
    jack_log("rpc_jack_client_rt_notify ref = %ld notify = %ld value = %ld", refnum, notify, value);
    JackMachServerChannel* channel = JackMachServerChannel::fPortTable[server_port];
    assert(channel);
    assert(channel->GetServer());
    channel->GetServer()->Notify(refnum, notify, value);
    return KERN_SUCCESS;
}
