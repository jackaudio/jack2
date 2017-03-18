/*
Copyright (C) 2004-2008 Grame
Copyright (C) 2016 Filipe Coelho

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

#include "JackLinuxFutex.h"
#include "JackTools.h"
#include "JackConstants.h"
#include "JackError.h"
#include "promiscuous.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <syscall.h>
#include <linux/futex.h>

namespace Jack
{

JackLinuxFutex::JackLinuxFutex() : JackSynchro(), fSharedMem(-1), fFutex(NULL), fPrivate(false)
{
    const char* promiscuous = getenv("JACK_PROMISCUOUS_SERVER");
    fPromiscuous = (promiscuous != NULL);
    fPromiscuousGid = jack_group2gid(promiscuous);
}

void JackLinuxFutex::BuildName(const char* client_name, const char* server_name, char* res, int size)
{
    char ext_client_name[SYNC_MAX_NAME_SIZE + 1];
    JackTools::RewriteName(client_name, ext_client_name);
    if (fPromiscuous) {
        snprintf(res, size, "jack_sem.%s_%s", server_name, ext_client_name);
    } else {
        snprintf(res, size, "jack_sem.%d_%s_%s", JackTools::GetUID(), server_name, ext_client_name);
    }
}

bool JackLinuxFutex::Signal()
{
    if (!fFutex) {
        jack_error("JackLinuxFutex::Signal name = %s already deallocated!!", fName);
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

    ::syscall(__NR_futex, fFutex, fFutex->internal ? FUTEX_WAKE_PRIVATE : FUTEX_WAKE, 1, NULL, NULL, 0);
    return true;
}

bool JackLinuxFutex::SignalAll()
{
    return Signal();
}

bool JackLinuxFutex::Wait()
{
    if (!fFutex) {
        jack_error("JackLinuxFutex::Wait name = %s already deallocated!!", fName);
        return false;
    }

    if (fFutex->needsChange)
    {
        fFutex->needsChange = false;
        fFutex->internal = !fFutex->internal;
    }

    for (;;)
    {
        if (__sync_bool_compare_and_swap(&fFutex->futex, 1, 0))
            return true;

        if (::syscall(__NR_futex, fFutex, fFutex->internal ? FUTEX_WAIT_PRIVATE : FUTEX_WAIT, 0, NULL, NULL, 0) != 0 && errno != EWOULDBLOCK)
            return false;
    }
}

bool JackLinuxFutex::TimedWait(long usec)
{
    if (!fFutex) {
        jack_error("JackLinuxFutex::TimedWait name = %s already deallocated!!", fName);
        return false;
     }

    if (fFutex->needsChange)
    {
        fFutex->needsChange = false;
        fFutex->internal = !fFutex->internal;
    }

    const uint secs  =  usec / 1000000;
    const int  nsecs = (usec % 1000000) * 1000;

    const timespec timeout = { static_cast<time_t>(secs), nsecs };

    for (;;)
    {
        if (__sync_bool_compare_and_swap(&fFutex->futex, 1, 0))
            return true;

        if (::syscall(__NR_futex, fFutex, fFutex->internal ? FUTEX_WAIT_PRIVATE : FUTEX_WAIT, 0, &timeout, NULL, 0) != 0 && errno != EWOULDBLOCK)
            return false;
    }
}

// Server side : publish the futex in the global namespace
bool JackLinuxFutex::Allocate(const char* name, const char* server_name, int value, bool internal)
{
    BuildName(name, server_name, fName, sizeof(fName));
    jack_log("JackLinuxFutex::Allocate name = %s val = %ld", fName, value);

    if ((fSharedMem = shm_open(fName, O_CREAT | O_RDWR, 0777)) < 0) {
        jack_error("Allocate: can't check in named futex name = %s err = %s", fName, strerror(errno));
        return false;
    }

    ftruncate(fSharedMem, sizeof(FutexData));

    if (fPromiscuous && (jack_promiscuous_perms(fSharedMem, fName, fPromiscuousGid) < 0)) {
        close(fSharedMem);
        fSharedMem = -1;
        shm_unlink(fName);
        return false;
    }

    if ((fFutex = (FutexData*)mmap(NULL, sizeof(FutexData), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_LOCKED, fSharedMem, 0)) == NULL) {
        jack_error("Allocate: can't check in named futex name = %s err = %s", fName, strerror(errno));
        close(fSharedMem);
        fSharedMem = -1;
        shm_unlink(fName);
        return false;
    }

    fPrivate = internal;

    fFutex->futex = value;
    fFutex->internal = internal;
    fFutex->wasInternal = internal;
    fFutex->needsChange = false;
    fFutex->externalCount = 0;
    return true;
}

// Client side : get the published futex from server
bool JackLinuxFutex::Connect(const char* name, const char* server_name)
{
    BuildName(name, server_name, fName, sizeof(fName));
    jack_log("JackLinuxFutex::Connect name = %s", fName);

    // Temporary...
    if (fFutex) {
        jack_log("Already connected name = %s", name);
        return true;
    }

    if ((fSharedMem = shm_open(fName, O_RDWR, 0)) < 0) {
        jack_error("Connect: can't connect named futex name = %s err = %s", fName, strerror(errno));
        return false;
    }

    if ((fFutex = (FutexData*)mmap(NULL, sizeof(FutexData), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_LOCKED, fSharedMem, 0)) == NULL) {
        jack_error("Connect: can't connect named futex name = %s err = %s", fName, strerror(errno));
        close(fSharedMem);
        fSharedMem = -1;
        return false;
    }

    if (! fPrivate && fFutex->wasInternal)
    {
        const char* externalSync = getenv("JACK_INTERNAL_CLIENT_SYNC");

        if (externalSync != NULL && strstr(fName, externalSync) != NULL && ++fFutex->externalCount == 1)
        {
            jack_error("Note: client %s running as external client temporarily", fName);
            fFutex->needsChange = true;
        }
    }

    return true;
}

bool JackLinuxFutex::ConnectInput(const char* name, const char* server_name)
{
    return Connect(name, server_name);
}

bool JackLinuxFutex::ConnectOutput(const char* name, const char* server_name)
{
    return Connect(name, server_name);
}

bool JackLinuxFutex::Disconnect()
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
void JackLinuxFutex::Destroy()
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

