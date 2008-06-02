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

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "JackMachPort.h"
#include "JackError.h"

namespace Jack
{

// Server side : port is published to be accessible from other processes (clients)

bool JackMachPort::AllocatePort(const char* name, int queue)
{
    mach_port_t task = mach_task_self();
    kern_return_t res;

    if ((res = task_get_bootstrap_port(task, &fBootPort)) != KERN_SUCCESS) {
        jack_error("AllocatePort: Can't find bootstrap mach port err = %s", mach_error_string(res));
        return false;
    }

    if ((res = mach_port_allocate(task, MACH_PORT_RIGHT_RECEIVE, &fServerPort)) != KERN_SUCCESS) {
        jack_error("AllocatePort: can't allocate mach port err = %s", mach_error_string(res));
        return false;
    }

    if ((res = mach_port_insert_right(task, fServerPort, fServerPort, MACH_MSG_TYPE_MAKE_SEND)) != KERN_SUCCESS) {
        jack_error("AllocatePort: error inserting mach rights err = %s", mach_error_string(res));
        return false;
    }

    if ((res = bootstrap_register(fBootPort, (char*)name, fServerPort)) != KERN_SUCCESS) {
        jack_error("Allocate: can't check in mach port name = %s err = %s", name, mach_error_string(res));
        return false;
    }

    mach_port_limits_t qlimits;
    mach_msg_type_number_t info_cnt = MACH_PORT_LIMITS_INFO_COUNT;
    if ((res = mach_port_get_attributes(task, fServerPort, MACH_PORT_LIMITS_INFO, (mach_port_info_t) & qlimits, &info_cnt)) != KERN_SUCCESS) {
        jack_error("Allocate: mach_port_get_attributes error err = %s", name, mach_error_string(res));
    }

    jack_log("AllocatePort: queue limit %ld", qlimits.mpl_qlimit);

    if (queue > 0) {
        qlimits.mpl_qlimit = queue;
        if ((res = mach_port_set_attributes(task, fServerPort, MACH_PORT_LIMITS_INFO, (mach_port_info_t) & qlimits, MACH_PORT_LIMITS_INFO_COUNT)) != KERN_SUCCESS) {
            jack_error("Allocate: mach_port_set_attributes error name = %s err = %s", name, mach_error_string(res));
        }
    }

    return true;
}

// Server side : port is published to be accessible from other processes (clients)

bool JackMachPort::AllocatePort(const char* name)
{
    return AllocatePort(name, -1);
}

// Client side : get the published port from server

bool JackMachPort::ConnectPort(const char* name)
{
    kern_return_t res;

    jack_log("JackMachPort::ConnectPort %s", name);

    if ((res = task_get_bootstrap_port(mach_task_self(), &fBootPort)) != KERN_SUCCESS) {
        jack_error("ConnectPort: can't find bootstrap port err = %s", mach_error_string(res));
        return false;
    }

    if ((res = bootstrap_look_up(fBootPort, (char*)name, &fServerPort)) != KERN_SUCCESS) {
        jack_error("ConnectPort: can't find mach server port name = %s err = %s", name, mach_error_string(res));
        return false;
    }

    return true;
}

bool JackMachPort::DisconnectPort()
{
    jack_log("JackMacRPC::DisconnectPort");
    kern_return_t res;
    mach_port_t task = mach_task_self();

    if (fBootPort != 0) {
        if ((res = mach_port_deallocate(task, fBootPort)) != KERN_SUCCESS) {
            jack_error("JackMacRPC::DisconnectPort mach_port_deallocate fBootPort err = %s", mach_error_string(res));
        }
    }

    if (fServerPort != 0) {
        if ((res = mach_port_deallocate(task, fServerPort)) != KERN_SUCCESS) {
            jack_error("JackMacRPC::DisconnectPort mach_port_deallocate fServerPort err = %s", mach_error_string(res));
        }
    }
    
    return true;
}

bool JackMachPort::DestroyPort()
{
    jack_log("JackMacRPC::DisconnectPort");
    kern_return_t res;
    mach_port_t task = mach_task_self();

    if (fBootPort != 0) {
        if ((res = mach_port_deallocate(task, fBootPort)) != KERN_SUCCESS) {
            jack_error("JackMacRPC::DisconnectPort mach_port_deallocate fBootPort err = %s", mach_error_string(res));
        }
    }

    if (fServerPort != 0) {
        if ((res = mach_port_destroy(task, fServerPort)) != KERN_SUCCESS) {
            jack_error("JackMacRPC::DisconnectPort mach_port_destroy fServerPort err = %s", mach_error_string(res));
        }
    }

    return true;
}

mach_port_t JackMachPort::GetPort()
{
    return fServerPort;
}

bool JackMachPortSet::AllocatePort(const char* name, int queue)
{
    kern_return_t res;
    mach_port_t task = mach_task_self();

    jack_log("JackMachPortSet::AllocatePort");

    if ((res = task_get_bootstrap_port(task, &fBootPort)) != KERN_SUCCESS) {
        jack_error("AllocatePort: Can't find bootstrap mach port err = %s", mach_error_string(res));
        return false;
    }

    if ((res = mach_port_allocate(task, MACH_PORT_RIGHT_RECEIVE, &fServerPort)) != KERN_SUCCESS) {
        jack_error("AllocatePort: can't allocate mach port err = %s", mach_error_string(res));
        return false;
    }

    if ((res = mach_port_insert_right(task, fServerPort, fServerPort, MACH_MSG_TYPE_MAKE_SEND)) != KERN_SUCCESS) {
        jack_error("AllocatePort: error inserting mach rights err = %s", mach_error_string(res));
        return false;
    }

    if ((res = mach_port_allocate(task, MACH_PORT_RIGHT_PORT_SET, &fPortSet)) != KERN_SUCCESS) {
        jack_error("AllocatePort: can't allocate mach port err = %s", mach_error_string(res));
        return false;
    }

    if ((res = mach_port_move_member(task, fServerPort, fPortSet)) != KERN_SUCCESS) {
        jack_error("AllocatePort: error in mach_port_move_member err = %s", mach_error_string(res));
        return false;
    }

    if ((res = bootstrap_register(fBootPort, (char*)name, fServerPort)) != KERN_SUCCESS) {
        jack_error("Allocate: can't check in mach port name = %s err = %s", name, mach_error_string(res));
        return false;
    }

    mach_port_limits_t qlimits;
    mach_msg_type_number_t info_cnt = MACH_PORT_LIMITS_INFO_COUNT;
    if ((res = mach_port_get_attributes(task, fServerPort, MACH_PORT_LIMITS_INFO, (mach_port_info_t) & qlimits, &info_cnt)) != KERN_SUCCESS) {
        jack_error("Allocate: mach_port_get_attributes error name = %s err = %s", name, mach_error_string(res));
    }

    jack_log("AllocatePort: queue limit = %ld", qlimits.mpl_qlimit);

    if (queue > 0) {
        qlimits.mpl_qlimit = queue;

        if ((res = mach_port_set_attributes(task, fServerPort, MACH_PORT_LIMITS_INFO, (mach_port_info_t) & qlimits, MACH_PORT_LIMITS_INFO_COUNT)) != KERN_SUCCESS) {
            jack_error("Allocate: mach_port_set_attributes error name = %s err = %s", name, mach_error_string(res));
        }
    }

    return true;
}

// Server side : port is published to be accessible from other processes (clients)

bool JackMachPortSet::AllocatePort(const char* name)
{
    return AllocatePort(name, -1);
}

bool JackMachPortSet::DisconnectPort()
{
    kern_return_t res;
    mach_port_t task = mach_task_self();

    jack_log("JackMachPortSet::DisconnectPort");

    if (fBootPort != 0) {
        if ((res = mach_port_deallocate(task, fBootPort)) != KERN_SUCCESS) {
            jack_error("JackMachPortSet::DisconnectPort mach_port_deallocate fBootPort err = %s", mach_error_string(res));
        }
    }

    if (fServerPort != 0) {
        if ((res = mach_port_deallocate(task, fServerPort)) != KERN_SUCCESS) {
            jack_error("JackMachPortSet::DisconnectPort mach_port_deallocate fServerPort err = %s", mach_error_string(res));
        }
    }

    return true;
}

bool JackMachPortSet::DestroyPort()
{
    kern_return_t res;
    mach_port_t task = mach_task_self();

    jack_log("JackMachPortSet::DisconnectPort");

    if (fBootPort != 0) {
        if ((res = mach_port_deallocate(task, fBootPort)) != KERN_SUCCESS) {
            jack_error("JackMachPortSet::DisconnectPort mach_port_deallocate err = %s", mach_error_string(res));
        }
    }

    if (fServerPort != 0) {
        if ((res = mach_port_destroy(task, fServerPort)) != KERN_SUCCESS) {
            jack_error("JackMachPortSet::DisconnectPort mach_port_destroy fServerPort err = %s", mach_error_string(res));
        }
    }

    return true;
}

mach_port_t JackMachPortSet::GetPortSet()
{
    return fPortSet;
}

mach_port_t JackMachPortSet::AddPort()
{
    kern_return_t res;
    mach_port_t task = mach_task_self();
    mach_port_t old_port, result = 0;

    jack_log("JackMachPortSet::AddPort");

    if ((res = mach_port_allocate(task, MACH_PORT_RIGHT_RECEIVE, &result)) != KERN_SUCCESS) {
        jack_error("AddPort: can't allocate mach port err = %s", mach_error_string(res));
        goto error;
    }

    if ((res = mach_port_request_notification(task, result, MACH_NOTIFY_NO_SENDERS,
               1, result, MACH_MSG_TYPE_MAKE_SEND_ONCE, &old_port)) != KERN_SUCCESS) {
        jack_error("AddPort: error in mach_port_request_notification err = %s", mach_error_string(res));
        goto error;
    }

    if ((res = mach_port_move_member(task, result, fPortSet)) != KERN_SUCCESS) {
        jack_error("AddPort: error in mach_port_move_member err = %s", mach_error_string(res));
        goto error;
    }

    return result;

error:
    if (result) {
        if ((res = mach_port_destroy(task, result)) != KERN_SUCCESS) {
            jack_error("JackMacRPC::DisconnectPort mach_port_destroy err = %s", mach_error_string(res));
        }
    }
    return 0;
}


} // end of namespace

