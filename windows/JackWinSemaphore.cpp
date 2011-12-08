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

#include "JackWinSemaphore.h"
#include "JackConstants.h"
#include "JackTools.h"
#include "JackError.h"
#include <stdio.h>

namespace Jack
{

void JackWinSemaphore::BuildName(const char* client_name, const char* server_name, char* res, int size)
{
    char ext_client_name[JACK_CLIENT_NAME_SIZE + 1];
    JackTools::RewriteName(client_name, ext_client_name);
    _snprintf(res, size, "jack_pipe.%s_%s", server_name, ext_client_name);
}

bool JackWinSemaphore::Signal()
{
    BOOL res;
    assert(fSemaphore);

    if (fFlush) {
        return true;
    }

    if (!(res = ReleaseSemaphore(fSemaphore, 1, NULL))) {
        jack_error("JackWinSemaphore::Signal name = %s err = %ld", fName, GetLastError());
    }

    return res;
}

bool JackWinSemaphore::SignalAll()
{
    BOOL res;
    assert(fSemaphore);

    if (fFlush) {
        return true;
    }

    if (!(res = ReleaseSemaphore(fSemaphore, 1, NULL))) {
        jack_error("JackWinSemaphore::SignalAll name = %s err = %ld", fName, GetLastError());
    }

    return res;
}

bool JackWinSemaphore::Wait()
{
    DWORD res;

    if ((res = WaitForSingleObject(fSemaphore, INFINITE)) == WAIT_TIMEOUT) {
        jack_error("JackWinSemaphore::TimedWait name = %s time_out", fName);
    }

    return (res == WAIT_OBJECT_0);
}

bool JackWinSemaphore::TimedWait(long usec)
{
    DWORD res;

    if ((res = WaitForSingleObject(fSemaphore, usec / 1000)) == WAIT_TIMEOUT) {
        jack_error("JackWinSemaphore::TimedWait name = %s time_out", fName);
    }

    return (res == WAIT_OBJECT_0);
}

// Client side : get the published semaphore from server
bool JackWinSemaphore::ConnectInput(const char* name, const char* server_name)
{
    BuildName(name, server_name, fName, sizeof(fName));
    jack_log("JackWinSemaphore::Connect %s", fName);

    // Temporary...
    if (fSemaphore) {
        jack_log("Already connected name = %s", name);
        return true;
    }

    if ((fSemaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS , FALSE, fName)) == NULL) {
        jack_error("Connect: can't check in named event name = %s err = %ld", fName, GetLastError());
        return false;
    } else {
        return true;
    }
}

bool JackWinSemaphore::Connect(const char* name, const char* server_name)
{
    return ConnectInput(name, server_name);
}

bool JackWinSemaphore::ConnectOutput(const char* name, const char* server_name)
{
    return ConnectInput(name, server_name);
}

bool JackWinSemaphore::Disconnect()
{
    if (fSemaphore) {
        jack_log("JackWinSemaphore::Disconnect %s", fName);
        CloseHandle(fSemaphore);
        fSemaphore = NULL;
        return true;
    } else {
        return false;
    }
}

bool JackWinSemaphore::Allocate(const char* name, const char* server_name, int value)
{
    BuildName(name, server_name, fName, sizeof(fName));
    jack_log("JackWinSemaphore::Allocate name = %s val = %ld", fName, value);

    if ((fSemaphore = CreateSemaphore(NULL, value, 32767, fName)) == NULL) {
        jack_error("Allocate: can't check in named semaphore name = %s err = %ld", fName, GetLastError());
        return false;
    } else if (GetLastError() == ERROR_ALREADY_EXISTS) {
        jack_error("Allocate: named semaphore already exist name = %s", fName);
        // Try to open it
        fSemaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, fName);
        return (fSemaphore != NULL);
    } else {
        return true;
    }
}

void JackWinSemaphore::Destroy()
{
    if (fSemaphore != NULL) {
        jack_log("JackWinSemaphore::Destroy %s", fName);
        CloseHandle(fSemaphore);
        fSemaphore = NULL;
    } else {
        jack_error("JackWinSemaphore::Destroy synchro == NULL");
    }
}


} // end of namespace
