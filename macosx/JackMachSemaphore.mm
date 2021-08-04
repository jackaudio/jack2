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
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>

namespace Jack
{

void JackMachSemaphore::BuildName(const char* client_name, const char* server_name, char* res, int size)
{
    char ext_client_name[SYNC_MAX_NAME_SIZE + 1];
    JackTools::RewriteName(client_name, ext_client_name);

    // make the name as small as possible, as macos has issues with long semaphore names
    if (strcmp(server_name, "default") == 0)
        server_name = "";

    snprintf(res, std::min(size, 32), "js%d.%s%s", JackTools::GetUID(), server_name, ext_client_name);
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

    if ((fSharedMem = shm_open(fName, O_CREAT | O_RDWR, 0777)) < 0) {
        jack_error("Allocate: can't open shared memory segment; name = %s err = %s", fName, strerror(errno));
        return false;
    }

    struct stat st;
    if (fstat(fSharedMem, &st) != -1 && st.st_size == 0) {
        if (ftruncate(fSharedMem, sizeof(mach_port_t)) != 0) {
            jack_error("Allocate: can't set shared memory size in mach shared name = %s err = %s", fName, strerror(errno));
            return false;
        }
    }

    mach_port_t* const sharedSemaphoreSend =
        (mach_port_t*)mmap(NULL, sizeof(mach_port_t), PROT_READ | PROT_WRITE, MAP_SHARED, fSharedMem, 0);

    if (sharedSemaphoreSend == NULL || sharedSemaphoreSend == MAP_FAILED) {
        jack_error("Allocate: failed to mmap; name = %s err = %s", fName, strerror(errno));
        close(fSharedMem);
        fSharedMem = -1;
        shm_unlink(fName);
        return false;
    }

    fSharedSemaphoreSend = sharedSemaphoreSend;

    if ((res = semaphore_create(task, &fSemaphore, SYNC_POLICY_FIFO, value)) != KERN_SUCCESS) {
        jack_error("Allocate: can create semaphore err = %i:%s", res, mach_error_string(res));
        return false;
    }

    jack_log("JackMachSemaphore::Allocate name = %s", fName);

    *fSharedSemaphoreSend = fSemaphore;

    return true;
}

// Client side : get the published semaphore from server
bool JackMachSemaphore::ConnectInput(const char* name, const char* server_name)
{
    BuildName(name, server_name, fName, sizeof(fName));
    kern_return_t res;

    // Temporary...
    if (fSharedSemaphoreSend) {
        jack_log("Already connected name = %s", name);
        return true;
    }

    if ((fSharedMem = shm_open(fName, O_RDWR, 0)) < 0) {
        jack_error("Connect: can't open shared memory segment; name = %s err = %s", fName, strerror(errno));
        return false;
    }

    mach_port_t* const sharedSemaphoreSend =
        (mach_port_t*)mmap(NULL, sizeof(mach_port_t), PROT_READ | PROT_WRITE, MAP_SHARED, fSharedMem, 0);

    if (sharedSemaphoreSend == NULL || sharedSemaphoreSend == MAP_FAILED) {
        jack_error("Connect: failed to mmap: name = %s err = %s", fName, strerror(errno));
        close(fSharedMem);
        fSharedMem = -1;
        return false;
    }

    fSharedSemaphoreSend = sharedSemaphoreSend;
    fSemaphore = *sharedSemaphoreSend;

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

    if (!fSharedSemaphoreSend) {
        return true;
    }

    munmap(fSharedSemaphoreSend, sizeof(mach_port_t));
    fSharedSemaphoreSend = NULL;

    close(fSharedMem);
    fSharedMem = -1;
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

    if (!fSharedSemaphoreSend) {
        return;
    }

    munmap(fSharedSemaphoreSend, sizeof(mach_port_t));
    fSharedSemaphoreSend = NULL;

    close(fSharedMem);
    fSharedMem = -1;

    shm_unlink(fName);
}

} // end of namespace

