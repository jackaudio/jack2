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

#include "JackMachSemaphore.h"
#include "JackConstants.h"
#include "JackTools.h"
#include "JackError.h"
#include <stdio.h>

namespace Jack
{

void JackMachSemaphore::BuildName(const char* client_name, const char* server_name, char* res, int size)
{
    char ext_client_name[JACK_CLIENT_NAME_SIZE + 1];
    JackTools::RewriteName(client_name, ext_client_name);
    snprintf(res, size, "jack_mach_sem.%d_%s_%s", JackTools::GetUID(), server_name, ext_client_name);
}

bool JackMachSemaphore::Signal()
{
    if (!fSemaphore) {
        jack_error("JackMachSemaphore::Signal name = %s already deallocated!!", fName);
        return false;
    }

    if (fFlush) {
        return true;
    }

    kern_return_t res;
    if ((res = semaphore_signal(fSemaphore)) != KERN_SUCCESS) {
        jack_error("JackMachSemaphore::Signal name = %s err = %s", fName, mach_error_string(res));
    }
    return (res == KERN_SUCCESS);
}

bool JackMachSemaphore::SignalAll()
{
    if (!fSemaphore) {
        jack_error("JackMachSemaphore::SignalAll name = %s already deallocated!!", fName);
        return false;
    }

    if (fFlush) {
        return true;
    }

    kern_return_t res;
    // When signaled several times, do not accumulate signals...
    if ((res = semaphore_signal_all(fSemaphore)) != KERN_SUCCESS) {
        jack_error("JackMachSemaphore::SignalAll name = %s err = %s", fName, mach_error_string(res));
    }
    return (res == KERN_SUCCESS);
}

bool JackMachSemaphore::Wait()
{
    if (!fSemaphore) {
        jack_error("JackMachSemaphore::Wait name = %s already deallocated!!", fName);
        return false;
    }

    kern_return_t res;
    if ((res = semaphore_wait(fSemaphore)) != KERN_SUCCESS) {
        jack_error("JackMachSemaphore::Wait name = %s err = %s", fName, mach_error_string(res));
    }
    return (res == KERN_SUCCESS);
}

bool JackMachSemaphore::TimedWait(long usec)
{
    if (!fSemaphore) {
        jack_error("JackMachSemaphore::TimedWait name = %s already deallocated!!", fName);
        return false;
    }

    kern_return_t res;
    mach_timespec time;
    time.tv_sec = usec / 1000000;
    time.tv_nsec = (usec % 1000000) * 1000;

    if ((res = semaphore_timedwait(fSemaphore, time)) != KERN_SUCCESS) {
        jack_error("JackMachSemaphore::TimedWait name = %s usec = %ld err = %s", fName, usec, mach_error_string(res));
    }
    return (res == KERN_SUCCESS);
}

// Server side : publish the semaphore in the global namespace
bool JackMachSemaphore::Allocate(const char* name, const char* server_name, int value)
{
    BuildName(name, server_name, fName, sizeof(fName));
    mach_port_t task = mach_task_self();
    kern_return_t res;

    if (fBootPort == 0) {
        if ((res = task_get_bootstrap_port(task, &fBootPort)) != KERN_SUCCESS) {
            jack_error("Allocate: Can't find bootstrap mach port err = %s", mach_error_string(res));
            return false;
        }
    }

    if ((res = semaphore_create(task, &fSemaphore, SYNC_POLICY_FIFO, value)) != KERN_SUCCESS) {
        jack_error("Allocate: can create semaphore err = %s", mach_error_string(res));
        return false;
    }

    if ((res = bootstrap_register(fBootPort, fName, fSemaphore)) != KERN_SUCCESS) {
        jack_error("Allocate: can't check in mach semaphore name = %s err = %s", fName, mach_error_string(res));

        switch (res) {
            case BOOTSTRAP_SUCCESS :
                /* service not currently registered, "a good thing" (tm) */
                break;
            case BOOTSTRAP_NOT_PRIVILEGED :
                jack_log("bootstrap_register(): bootstrap not privileged");
                break;
            case BOOTSTRAP_SERVICE_ACTIVE :
                jack_log("bootstrap_register(): bootstrap service active");
                break;
            default :
                jack_log("bootstrap_register() err = %s", mach_error_string(res));
                break;
        }

        return false;
    }

    jack_log("JackMachSemaphore::Allocate name = %s", fName);
    return true;
}

// Client side : get the published semaphore from server
bool JackMachSemaphore::ConnectInput(const char* name, const char* server_name)
{
    BuildName(name, server_name, fName, sizeof(fName));
    kern_return_t res;

    if (fBootPort == 0) {
        if ((res = task_get_bootstrap_port(mach_task_self(), &fBootPort)) != KERN_SUCCESS) {
            jack_error("Connect: can't find bootstrap port err = %s", mach_error_string(res));
            return false;
        }
    }

    if ((res = bootstrap_look_up(fBootPort, fName, &fSemaphore)) != KERN_SUCCESS) {
        jack_error("Connect: can't find mach semaphore name = %s err = %s", fName, mach_error_string(res));
        return false;
    }

    jack_log("JackMachSemaphore::Connect name = %s ", fName);
    return true;
}

bool JackMachSemaphore::Connect(const char* name, const char* server_name)
{
    return ConnectInput(name, server_name);
}

bool JackMachSemaphore::ConnectOutput(const char* name, const char* server_name)
{
    return ConnectInput(name, server_name);
}

bool JackMachSemaphore::Disconnect()
{
    if (fSemaphore > 0) {
        jack_log("JackMachSemaphore::Disconnect name = %s", fName);
        fSemaphore = 0;
    }
    // Nothing to do
    return true;
}

// Server side : destroy the JackGlobals
void JackMachSemaphore::Destroy()
{
    kern_return_t res;

    if (fSemaphore > 0) {
        jack_log("JackMachSemaphore::Destroy name = %s", fName);
        if ((res = semaphore_destroy(mach_task_self(), fSemaphore)) != KERN_SUCCESS) {
            jack_error("JackMachSemaphore::Destroy can't destroy semaphore err = %s", mach_error_string(res));
        }
        fSemaphore = 0;
    } else {
        jack_error("JackMachSemaphore::Destroy semaphore < 0");
    }
}

} // end of namespace

