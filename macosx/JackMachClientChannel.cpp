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

#include "JackMachClientChannel.h"
#include "JackRPCEngine.h"
#include "JackRPCClientServer.c"
#include "JackError.h"
#include "JackLibClient.h"
#include "JackLibGlobals.h"
#include "JackMachThread.h"
#include "JackConstants.h"

namespace Jack
{

JackMachClientChannel::JackMachClientChannel()
{
    fThread = new JackMachThread(this);
}

JackMachClientChannel::~JackMachClientChannel()
{
    delete fThread;
}

// Server <===> client

int JackMachClientChannel::ServerCheck(const char* server_name)
{
	JackLog("JackMachClientChannel::ServerCheck = %s\n", server_name);
	
    // Connect to server
    if (!fServerPort.ConnectPort(jack_server_entry)) {
        jack_error("Cannot connect to server Mach port");
        return -1;
    } else {
		return 0;
	}
}

int JackMachClientChannel::Open(const char* name, char* name_res, JackClient* client, jack_options_t options, jack_status_t* status)
{
    JackLog("JackMachClientChannel::Open name = %s\n", name);

    // Connect to server
    if (!fServerPort.ConnectPort(jack_server_entry)) {
        jack_error("Cannot connect to server Mach port");
        return -1;
    }
	
	// Check name in server
	int result = 0;
	ClientCheck(name, name_res, JACK_PROTOCOL_VERSION, (int)options, (int*)status, &result);
    if (result < 0) {
		int status1 = *status;
        if (status1 & JackVersionError)
			jack_error("JACK protocol mismatch %d", JACK_PROTOCOL_VERSION);
        else
			jack_error("Client name = %s conflits with another running client", name);
		return -1;
    }

    // Prepare local port using client name
    char buf[JACK_CLIENT_NAME_SIZE];
    snprintf(buf, sizeof(buf) - 1, "%s:%s", jack_client_entry, name_res);

    if (!fClientPort.AllocatePort(buf, 16)) {
        jack_error("Cannot allocate client Mach port");
        return -1;
    }

    JackLibGlobals::fGlobals->fClientTable[fClientPort.GetPort()] = client;
    return 0;
}

void JackMachClientChannel::Close()
{
    JackLog("JackMachClientChannel::Close\n");
    JackLibGlobals::fGlobals->fClientTable.erase(fClientPort.GetPort());
    fServerPort.DisconnectPort();
    fClientPort.DestroyPort();

    // TO CHECK
    kern_return_t res;
    if ((res = mach_port_destroy(mach_task_self(), fPrivatePort)) != KERN_SUCCESS) {
        jack_error("JackMachClientChannel::Close err = %s", mach_error_string(res));
    }
}

int JackMachClientChannel::Start()
{
    JackLog("JackMachClientChannel::Start\n");
    if (fThread->Start() != 0) {
        jack_error("Cannot start Jack client listener");
        return -1;
    } else {
        return 0;
    }
}

void JackMachClientChannel::Stop()
{
    JackLog("JackMachClientChannel::Stop\n");
    fThread->Kill();
}

void JackMachClientChannel::ClientCheck(const char* name, char* name_res, int protocol, int options, int* status, int* result)
{
	kern_return_t res = rpc_jack_client_check(fServerPort.GetPort(), (char*)name, name_res, protocol, options, status, result);
    if (res != KERN_SUCCESS) {
        *result = -1;
        jack_error("JackMachClientChannel::ClientCheck err = %s", mach_error_string(res));
    }
}

void JackMachClientChannel::ClientOpen(const char* name, int* shared_engine, int* shared_client, int* shared_graph, int* result)
{
    kern_return_t res = rpc_jack_client_open(fServerPort.GetPort(), (char*)name, &fPrivatePort, shared_engine, shared_client, shared_graph, result);
    if (res != KERN_SUCCESS) {
        *result = -1;
        jack_error("JackMachClientChannel::ClientOpen err = %s", mach_error_string(res));
    }
}

void JackMachClientChannel::ClientClose(int refnum, int* result)
{
    kern_return_t res = rpc_jack_client_close(fPrivatePort, refnum, result);
    if (res != KERN_SUCCESS) {
        *result = -1;
        jack_error("JackMachClientChannel::ClientClose err = %s", mach_error_string(res));
    }
}

void JackMachClientChannel::ClientActivate(int refnum, int* result)
{
    kern_return_t res = rpc_jack_client_activate(fPrivatePort, refnum, result);
    if (res != KERN_SUCCESS) {
        *result = -1;
        jack_error("JackMachClientChannel::ClientActivate err = %s", mach_error_string(res));
    }
}

void JackMachClientChannel::ClientDeactivate(int refnum, int* result)
{
    kern_return_t res = rpc_jack_client_deactivate(fPrivatePort, refnum, result);
    if (res != KERN_SUCCESS) {
        *result = -1;
        jack_error("JackMachClientChannel::ClientDeactivate err = %s", mach_error_string(res));
    }
}

void JackMachClientChannel::PortRegister(int refnum, const char* name, unsigned int flags, unsigned int buffer_size, jack_port_id_t* port_index, int* result)
{
    kern_return_t res = rpc_jack_port_register(fPrivatePort, refnum, (char*)name, flags, buffer_size, port_index, result);
    if (res != KERN_SUCCESS) {
        *result = -1;
        jack_error("JackMachClientChannel::PortRegister err = %s", mach_error_string(res));
    }
}

void JackMachClientChannel::PortUnRegister(int refnum, jack_port_id_t port_index, int* result)
{
    kern_return_t res = rpc_jack_port_unregister(fPrivatePort, refnum, port_index, result);
    if (res != KERN_SUCCESS) {
        *result = -1;
        jack_error("JackMachClientChannel::PortUnRegister err = %s", mach_error_string(res));
    }
}

void JackMachClientChannel::PortConnect(int refnum, const char* src, const char* dst, int* result)
{
    kern_return_t res = rpc_jack_port_connect_name(fPrivatePort, refnum, (char*)src, (char*)dst, result);
    if (res != KERN_SUCCESS) {
        jack_error("JackMachClientChannel::PortConnect err = %s", mach_error_string(res));
    }
}

void JackMachClientChannel::PortDisconnect(int refnum, const char* src, const char* dst, int* result)
{
    kern_return_t res = rpc_jack_port_disconnect_name(fPrivatePort, refnum, (char*)src, (char*)dst, result);
    if (res != KERN_SUCCESS) {
        *result = -1;
        jack_error("JackMachClientChannel::PortDisconnect err = %s", mach_error_string(res));
    }
}

void JackMachClientChannel::PortConnect(int refnum, jack_port_id_t src, jack_port_id_t dst, int* result)
{
    kern_return_t res = rpc_jack_port_connect(fPrivatePort, refnum, src, dst, result);
    if (res != KERN_SUCCESS) {
        *result = -1;
        jack_error("JackMachClientChannel::PortConnect err = %s", mach_error_string(res));
    }
}

void JackMachClientChannel::PortDisconnect(int refnum, jack_port_id_t src, jack_port_id_t dst, int* result)
{
    kern_return_t res = rpc_jack_port_disconnect(fPrivatePort, refnum, src, dst, result);
    if (res != KERN_SUCCESS) {
        *result = -1;
        jack_error("JackMachClientChannel::PortDisconnect err = %s", mach_error_string(res));
    }
}

void JackMachClientChannel::SetBufferSize(jack_nframes_t buffer_size, int* result)
{
    kern_return_t res = rpc_jack_set_buffer_size(fPrivatePort, buffer_size, result);
    if (res != KERN_SUCCESS) {
        *result = -1;
        jack_error("JackMachClientChannel::SetBufferSize err = %s", mach_error_string(res));
    }
}

void JackMachClientChannel::SetFreewheel(int onoff, int* result)
{
    kern_return_t res = rpc_jack_set_freewheel(fPrivatePort, onoff, result);
    if (res != KERN_SUCCESS) {
        *result = -1;
        jack_error("JackMachClientChannel::SetFreewheel err = %s", mach_error_string(res));
    }
}

void JackMachClientChannel::ReleaseTimebase(int refnum, int* result)
{
    kern_return_t res = rpc_jack_release_timebase(fPrivatePort, refnum, result);
    if (res != KERN_SUCCESS) {
        *result = -1;
        jack_error("JackMachClientChannel::ReleaseTimebase err = %s", mach_error_string(res));
    }
}

void JackMachClientChannel::SetTimebaseCallback(int refnum, int conditional, int* result)
{
    kern_return_t res = rpc_jack_set_timebase_callback(fPrivatePort, refnum, conditional, result);
    if (res != KERN_SUCCESS) {
        *result = -1;
        jack_error("JackMachClientChannel::SetTimebaseCallback err = %s", mach_error_string(res));
    }
}

void JackMachClientChannel::GetInternalClientName(int refnum, int int_ref, char* name_res, int* result)
{
    kern_return_t res = rpc_jack_get_internal_clientname(fPrivatePort, refnum, int_ref, name_res, result);
    if (res != KERN_SUCCESS) {
        *result = -1;
        jack_error("JackMachClientChannel::GetInternalClientName err = %s", mach_error_string(res));
    }
}

void JackMachClientChannel::InternalClientHandle(int refnum, const char* client_name, int* status, int* int_ref, int* result)
{
    kern_return_t res = rpc_jack_internal_clienthandle(fPrivatePort, refnum, (char*)client_name, status, int_ref, result);
    if (res != KERN_SUCCESS) {
        *result = -1;
        jack_error("JackMachClientChannel::InternalClientHandle err = %s", mach_error_string(res));
    }
}

void JackMachClientChannel::InternalClientLoad(int refnum, const char* client_name, const char* so_name, const char* objet_data, int options, int* status, int* int_ref, int* result)
{
    kern_return_t res = rpc_jack_internal_clientload(fPrivatePort, refnum, (char*)client_name, (char*)so_name, (char*)objet_data, options, status, int_ref, result);
    if (res != KERN_SUCCESS) {
        *result = -1;
        jack_error("JackMachClientChannel::InternalClientLoad err = %s", mach_error_string(res));
    }
}

void JackMachClientChannel::InternalClientUnload(int refnum, int int_ref, int* status, int* result)
{
    kern_return_t res = rpc_jack_internal_clientunload(fPrivatePort, refnum, int_ref, status, result);
    if (res != KERN_SUCCESS) {
        *result = -1;
        jack_error("JackMachClientChannel::InternalClientUnload err = %s", mach_error_string(res));
    }
}

bool JackMachClientChannel::Execute()
{
    kern_return_t res;
    if ((res = mach_msg_server(JackRPCClient_server, 1024, fClientPort.GetPort(), 0)) != KERN_SUCCESS) {
        jack_error("JackMachClientChannel::Execute err = %s", mach_error_string(res));
        //fClient->ShutDown();
        return false;
    } else {
        return true;
    }
}

} // end of namespace


