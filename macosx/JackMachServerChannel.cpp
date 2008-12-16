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

#include "JackTools.h"
#include "JackMachServerChannel.h"
#include "JackRPCEngineServer.c"
#include "JackError.h"
#include "JackServer.h"
#include "JackLockedEngine.h"
#include "JackNotification.h"

using namespace std;

namespace Jack
{

map<mach_port_t, JackMachServerChannel*> JackMachServerChannel::fPortTable;

JackMachServerChannel::JackMachServerChannel():fThread(this)
{}

JackMachServerChannel::~JackMachServerChannel()
{}

int JackMachServerChannel::Open(const char* server_name, JackServer* server)
{
    jack_log("JackMachServerChannel::Open");
    char jack_server_entry_name[512];
    snprintf(jack_server_entry_name, sizeof(jack_server_entry_name), "%s.%d_%s", jack_server_entry, JackTools::GetUID(), server_name);

    if (!fServerPort.AllocatePort(jack_server_entry_name, 16)) { // 16 is the max possible value
        jack_error("Cannot check in Jack server");
        return -1;
    }

    if (fThread.Start() != 0) {
        jack_error("Cannot start Jack server listener");
        return -1;
    }

    fServer = server;
    fPortTable[fServerPort.GetPort()] = this;
    return 0;
}

void JackMachServerChannel::Close()
{
    jack_log("JackMachServerChannel::Close");
    fThread.Kill();
    fServerPort.DestroyPort();
}

JackLockedEngine* JackMachServerChannel::GetEngine()
{
    return fServer->GetEngine();
}

JackServer* JackMachServerChannel::GetServer()
{
    return fServer;
}

void JackMachServerChannel::ClientCheck(char* name, char* name_res, int protocol, int options, int* status, int* result)
{
    *result = GetEngine()->ClientCheck(name, name_res, protocol, options, status);
}

void JackMachServerChannel::ClientOpen(char* name, int pid, mach_port_t* private_port, int* shared_engine, int* shared_client, int* shared_graph, int* result)
{
    int refnum = -1;
    *result = GetEngine()->ClientExternalOpen(name, pid, &refnum, shared_engine, shared_client, shared_graph);

    if (*result == 0) {
        mach_port_t port = fServerPort.AddPort();
        if (port != 0) {
            fClientTable[port] = refnum;
            fPortTable[port] = this;
            *private_port = port;
        } else {
            jack_error("Cannot create private client mach port");
            *result = -1;
        }
    } else {
        jack_error("Cannot create new client");
    }
}

void JackMachServerChannel::ClientClose(mach_port_t private_port, int refnum)
{
    GetEngine()->ClientExternalClose(refnum);
    fClientTable.erase(private_port);

    // Hum, hum....
    kern_return_t res;
    if ((res = mach_port_destroy(mach_task_self(), private_port)) != KERN_SUCCESS) {
        jack_error("server_rpc_jack_client_close mach_port_destroy %s", mach_error_string(res));
    }
}

void JackMachServerChannel::ClientKill(mach_port_t private_port)
{
    jack_log("JackMachServerChannel::ClientKill");
    int refnum = fClientTable[private_port];
    assert(refnum > 0);
    fServer->ClientKill(refnum);
    fClientTable.erase(private_port);

    // Hum, hum....
    kern_return_t res;
    if ((res = mach_port_destroy(mach_task_self(), private_port)) != KERN_SUCCESS) {
        jack_error("server_rpc_jack_client_close mach_port_destroy %s", mach_error_string(res));
    }
}

boolean_t JackMachServerChannel::MessageHandler(mach_msg_header_t* Request, mach_msg_header_t* Reply)
{
    if (Request->msgh_id == MACH_NOTIFY_NO_SENDERS) {
        jack_log("MACH_NOTIFY_NO_SENDERS %ld", Request->msgh_local_port);
        JackMachServerChannel* channel = JackMachServerChannel::fPortTable[Request->msgh_local_port];
        assert(channel);
        channel->ClientKill(Request->msgh_local_port);
    } else {
        JackRPCEngine_server(Request, Reply);
    }
    return true;
}

bool JackMachServerChannel::Execute()
{
    kern_return_t res;
    if ((res = mach_msg_server(MessageHandler, 1024, fServerPort.GetPortSet(), 0)) != KERN_SUCCESS) {
        jack_log("JackMachServerChannel::Execute: err = %s", mach_error_string(res));
    }
    //return (res == KERN_SUCCESS);  mach_msg_server can fail if the client reply port is not valid anymore (crashed client)
    return true;
}

} // end of namespace


