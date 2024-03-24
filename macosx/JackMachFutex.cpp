/*
Copyright (C) 2004-2008 Grame
Copyright (C) 2016-2024 Filipe Coelho

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

#include "JackMachFutex.h"
#include "JackTools.h"
#include "JackConstants.h"
#include "JackError.h"
#include "promiscuous.h"
#include <cerrno>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>

#define UL_COMPARE_AND_WAIT        1
#define UL_COMPARE_AND_WAIT_SHARED 3
#define ULF_WAKE_ALL               0x00000100
#define ULF_NO_ERRNO               0x01000000

extern "C" {
int __ulock_wait(uint32_t operation, void* addr, uint64_t value, uint32_t timeout_us);
int __ulock_wake(uint32_t operation, void* addr, uint64_t value);
}

namespace Jack
{

JackMachFutex::JackMachFutex() : JackSynchro(), fSharedMem(-1), fFutex(NULL), fPrivate(false)
{
    const char* promiscuous = getenv("JACK_PROMISCUOUS_SERVER");
    fPromiscuous = (promiscuous != NULL);
    fPromiscuousGid = jack_group2gid(promiscuous);
}

void JackMachFutex::BuildName(const char* client_name, const char* server_name, char* res, int size)
{
    char ext_client_name[SYNC_MAX_NAME_SIZE + 1];
    JackTools::RewriteName(client_name, ext_client_name);

    // make the name as small as possible, as macos has issues with long semaphore names
    if (strcmp(server_name, "default") == 0)
        server_name = "";
    else if (strcmp(server_name, "mod-desktop") == 0)
        server_name = "mdsk.";

    if (fPromiscuous) {
        snprintf(res, std::min(size, 32), "js.%s%s", server_name, ext_client_name);
    } else {
        snprintf(res, std::min(size, 32), "js%d.%s%s", JackTools::GetUID(), server_name, ext_client_name);
    }
}

bool JackMachFutex::Signal()
{
    if (!fFutex) {
        jack_error("JackMachFutex::Signal name = %s already deallocated!!", fName);
        return false;
    }

    if (fFlush) {
        return true;
    }

    if (! __sync_bool_compare_and_swap(&fFutex->futex, 0, 1))
    {
        // already unlocked, do not wake futex
        if (! fFutex->internal) return true;
    }

    const uint32_t operation = ULF_NO_ERRNO | (fFutex->internal ? UL_COMPARE_AND_WAIT : UL_COMPARE_AND_WAIT_SHARED);
    __ulock_wake(operation, fFutex, 0);
    return true;
}

bool JackMachFutex::SignalAll()
{
    if (!fFutex) {
        jack_error("JackMachFutex::SignalAll name = %s already deallocated!!", fName);
        return false;
    }

    if (fFlush) {
        return true;
    }

    const uint32_t operation = ULF_NO_ERRNO | ULF_WAKE_ALL | (fFutex->internal ? UL_COMPARE_AND_WAIT : UL_COMPARE_AND_WAIT_SHARED);
    __ulock_wake(operation, fFutex, 0);
    return true;
}

bool JackMachFutex::Wait()
{
    if (!fFutex) {
        jack_error("JackMachFutex::Wait name = %s already deallocated!!", fName);
        return false;
    }

    if (fFutex->needsChange)
    {
        fFutex->needsChange = false;
        fFutex->internal = !fFutex->internal;
    }

    const uint32_t operation = fFutex->internal ? UL_COMPARE_AND_WAIT : UL_COMPARE_AND_WAIT_SHARED;

    for (;;)
    {
        if (__sync_bool_compare_and_swap(&fFutex->futex, 1, 0))
            return true;

        if (__ulock_wait(operation, fFutex, 0, UINT32_MAX) != 0)
            if (errno != EAGAIN && errno != EINTR)
                return false;
    }
}

bool JackMachFutex::TimedWait(long usec)
{
    if (usec == LONG_MAX)
        return Wait();

    if (!fFutex) {
        jack_error("JackMachFutex::TimedWait name = %s already deallocated!!", fName);
        return false;
     }

    if (fFutex->needsChange)
    {
        fFutex->needsChange = false;
        fFutex->internal = !fFutex->internal;
    }

    const uint32_t operation = fFutex->internal ? UL_COMPARE_AND_WAIT : UL_COMPARE_AND_WAIT_SHARED;

    for (;;)
    {
        if (__sync_bool_compare_and_swap(&fFutex->futex, 1, 0))
            return true;

        if (__ulock_wait(operation, fFutex, 0, usec) != 0)
            if (errno != EAGAIN && errno != EINTR)
                return false;
    }
}

// Server side : publish the futex in the global namespace
bool JackMachFutex::Allocate(const char* name, const char* server_name, int value, bool internal)
{
    BuildName(name, server_name, fName, sizeof(fName));
    jack_log("JackMachFutex::Allocate name = %s val = %ld", fName, value);

    // FIXME
    shm_unlink(fName);

    if ((fSharedMem = shm_open(fName, O_CREAT | O_RDWR, 0777)) < 0) {
        jack_error("Allocate: can't check in named futex name = %s err = %s", fName, strerror(errno));
        return false;
    }

    if (ftruncate(fSharedMem, sizeof(FutexData)) != 0) {
        jack_error("Allocate: can't set shared memory size in named futex name = %s err = %s", fName, strerror(errno));
        return false;
    }

    if (fPromiscuous && (jack_promiscuous_perms(fSharedMem, fName, fPromiscuousGid) < 0)) {
        close(fSharedMem);
        fSharedMem = -1;
        shm_unlink(fName);
        return false;
    }

    FutexData* futex = (FutexData*)mmap(NULL, sizeof(FutexData), PROT_READ|PROT_WRITE, MAP_SHARED, fSharedMem, 0);

    if (futex == NULL || futex == MAP_FAILED) {
        jack_error("Allocate: can't check in named futex name = %s err = %s", fName, strerror(errno));
        close(fSharedMem);
        fSharedMem = -1;
        shm_unlink(fName);
        return false;
    }

    mlock(futex, sizeof(FutexData));

    fPrivate = internal;

    futex->futex = value;
    futex->internal = internal;
    futex->wasInternal = internal;
    futex->needsChange = false;
    futex->externalCount = 0;
    fFutex = futex;
    return true;
}

// Client side : get the published futex from server
bool JackMachFutex::Connect(const char* name, const char* server_name)
{
    BuildName(name, server_name, fName, sizeof(fName));
    jack_log("JackMachFutex::Connect name = %s", fName);

    // Temporary...
    if (fFutex) {
        jack_log("Already connected name = %s", name);
        return true;
    }

    if ((fSharedMem = shm_open(fName, O_RDWR, 0)) < 0) {
        jack_error("Connect: can't connect named futex name = %s err = %s", fName, strerror(errno));
        return false;
    }

    FutexData* futex = (FutexData*)mmap(NULL, sizeof(FutexData), PROT_READ|PROT_WRITE, MAP_SHARED, fSharedMem, 0);

    if (futex == NULL || futex == MAP_FAILED) {
        jack_error("Connect: can't connect named futex name = %s err = %s", fName, strerror(errno));
        close(fSharedMem);
        fSharedMem = -1;
        return false;
    }

    mlock(futex, sizeof(FutexData));

    if (! fPrivate && futex->wasInternal)
    {
        const char* externalSync = getenv("JACK_INTERNAL_CLIENT_SYNC");

        if (externalSync != NULL && strstr(fName, externalSync) != NULL && ++futex->externalCount == 1)
        {
            jack_error("Note: client %s running as external client temporarily", fName);
            futex->needsChange = true;
        }
    }

    fFutex = futex;
    return true;
}

bool JackMachFutex::ConnectInput(const char* name, const char* server_name)
{
    return Connect(name, server_name);
}

bool JackMachFutex::ConnectOutput(const char* name, const char* server_name)
{
    return Connect(name, server_name);
}

bool JackMachFutex::Disconnect()
{
    if (!fFutex) {
        return true;
    }

    if (! fPrivate && fFutex->wasInternal)
    {
        const char* externalSync = getenv("JACK_INTERNAL_CLIENT_SYNC");

        if (externalSync != NULL && strstr(fName, externalSync) != NULL && --fFutex->externalCount == 0)
        {
            jack_error("Note: client %s now running as internal client again", fName);
            fFutex->needsChange = true;
        }
    }

    munmap(fFutex, sizeof(FutexData));
    fFutex = NULL;

    close(fSharedMem);
    fSharedMem = -1;
    return true;
}

// Server side : destroy the futex
void JackMachFutex::Destroy()
{
    if (!fFutex) {
        return;
    }

    munmap(fFutex, sizeof(FutexData));
    fFutex = NULL;

    close(fSharedMem);
    fSharedMem = -1;

    shm_unlink(fName);
}

} // end of namespace

