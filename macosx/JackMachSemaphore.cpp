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

#include "JackMachSemaphore.h"
#include "JackError.h"
#include <stdio.h>
#include <assert.h>

namespace Jack
{

mach_port_t JackMachSemaphore::fBootPort = 0;

void JackMachSemaphore::BuildName(const char* name, char* res)
{
    sprintf(res, "jack_mach_sem.%s", name);
}

bool JackMachSemaphore::Signal()
{
    kern_return_t res;
    assert(fSemaphore > 0);

    if (fFlush)
        return true;

    if ((res = semaphore_signal(fSemaphore)) != KERN_SUCCESS) {
        jack_error("JackMachSemaphore::Signal name = %s err = %s", fName, mach_error_string(res));
    }
    return (res == KERN_SUCCESS);
}

bool JackMachSemaphore::SignalAll()
{
    kern_return_t res;
    assert(fSemaphore > 0);

    if (fFlush)
        return true;
    // When signaled several times, do not accumulate signals...
    if ((res = semaphore_signal_all(fSemaphore)) != KERN_SUCCESS) {
        jack_error("JackMachSemaphore::SignalAll name = %s err = %s", fName, mach_error_string(res));
    }
    return (res == KERN_SUCCESS);
}

bool JackMachSemaphore::Wait()
{
    kern_return_t res;
    assert(fSemaphore > 0);
    if ((res = semaphore_wait(fSemaphore)) != KERN_SUCCESS) {
        jack_error("JackMachSemaphore::Wait name = %s err = %s", fName, mach_error_string(res));
    }
    return (res == KERN_SUCCESS);
}

bool JackMachSemaphore::TimedWait(long usec)
{
    kern_return_t res;
    mach_timespec time;
    time.tv_sec = usec / 1000000;
    time.tv_nsec = (usec % 1000000) * 1000;
    assert(fSemaphore > 0);
    if ((res = semaphore_timedwait(fSemaphore, time)) != KERN_SUCCESS) {
        jack_error("JackMachSemaphore::TimedWait name = %s usec = %ld err = %s", fName, usec, mach_error_string(res));
    }
    return (res == KERN_SUCCESS);
}

// Server side : publish the semaphore in the global namespace
bool JackMachSemaphore::Allocate(const char* name, int value)
{
    BuildName(name, fName);
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
                JackLog("bootstrap_register(): bootstrap not privileged\n");
                break;
            case BOOTSTRAP_SERVICE_ACTIVE :
                JackLog("bootstrap_register(): bootstrap service active\n");
                break;
            default :
                JackLog("bootstrap_register() err = %s\n", mach_error_string(res));
                break;
        }

        return false;
    }

    JackLog("JackMachSemaphore::Allocate name = %s\n", fName);
    return true;
}

// Client side : get the published semaphore from server
bool JackMachSemaphore::ConnectInput(const char* name)
{
    BuildName(name, fName);
    kern_return_t res;

    // Temporary...  A REVOIR
    /*
    if (fSemaphore > 0) {
    	JackLog("Already connected name = %s\n", name);
    	return true;
    }
    */

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

    JackLog("JackMachSemaphore::Connect name = %s \n", fName);
    return true;
}

bool JackMachSemaphore::Connect(const char* name)
{
    return ConnectInput(name);
}

bool JackMachSemaphore::ConnectOutput(const char* name)
{
    return ConnectInput(name);
}

bool JackMachSemaphore::Disconnect()
{
	if (fSemaphore > 0) {
		JackLog("JackMachSemaphore::Disconnect name = %s\n", fName);
	}
    // Nothing to do
    return true;
}

// Server side : destroy the JackGlobals
void JackMachSemaphore::Destroy()
{
    kern_return_t res;

    if (fSemaphore > 0) {
        JackLog("JackMachSemaphore::Destroy\n");
        if ((res = semaphore_destroy(mach_task_self(), fSemaphore)) != KERN_SUCCESS) {
            jack_error("JackMachSemaphore::Destroy can't destroy semaphore err = %s", mach_error_string(res));
        }
        fSemaphore = 0;
    } else {
        jack_error("JackMachSemaphore::Destroy semaphore < 0");
    }
}

} // end of namespace

