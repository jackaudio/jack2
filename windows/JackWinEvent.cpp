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


#include "JackWinEvent.h"
#include "JackTools.h"
#include "JackError.h"
#include <assert.h>

// http://www.codeproject.com/win32/Win32_Event_Handling.asp
// http://www.codeproject.com/threads/Synchronization.asp

namespace Jack
{

void JackWinEvent::BuildName(const char* name, const char* server_name, char* res, int size)
{
    snprintf(res, size, "jack_pipe.%s_%s", server_name, name);
}

bool JackWinEvent::Signal()
{
    BOOL res;
    assert(fEvent);

    if (fFlush)
        return true;

    if (!(res = SetEvent(fEvent))) {
        jack_error("JackWinEvent::Signal name = %s err = %ld", fName, GetLastError());
    }

    return res;
}

bool JackWinEvent::SignalAll()
{
    BOOL res;
    assert(fEvent);

    if (fFlush)
        return true;

    if (!(res = SetEvent(fEvent))) {
        jack_error("JackWinEvent::SignalAll name = %s err = %ld", fName, GetLastError());
    }

    return res;
}

bool JackWinEvent::Wait()
{
    DWORD res;

    if ((res = WaitForSingleObject(fEvent, INFINITE)) == WAIT_TIMEOUT) {
        jack_error("JackWinEvent::TimedWait name = %s time_out", fName);
    }

    return (res == WAIT_OBJECT_0);
}

bool JackWinEvent::TimedWait(long usec)
{
    DWORD res;

    if ((res = WaitForSingleObject(fEvent, usec / 1000)) == WAIT_TIMEOUT) {
        jack_error("JackWinEvent::TimedWait name = %s time_out", fName);
    }

    return (res == WAIT_OBJECT_0);
}

// Client side : get the published semaphore from server
bool JackWinEvent::ConnectInput(const char* name, const char* server_name)
{
    BuildName(name, server_name, fName, sizeof(fName));
    jack_log("JackWinEvent::Connect %s", fName);

    // Temporary...
    if (fEvent) {
        jack_log("Already connected name = %s", name);
        return true;
    }

    if ((fEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, fName)) == NULL) {
        jack_error("Connect: can't check in named event name = %s err = %ld", fName, GetLastError());
        return false;
    } else {
        return true;
    }
}

bool JackWinEvent::Connect(const char* name, const char* server_name)
{
    return ConnectInput(name, server_name);
}

bool JackWinEvent::ConnectOutput(const char* name, const char* server_name)
{
    return ConnectInput(name, server_name);
}

bool JackWinEvent::Disconnect()
{
    if (fEvent) {
        jack_log("JackWinEvent::Disconnect %s", fName);
        CloseHandle(fEvent);
        fEvent = NULL;
        return true;
    } else {
        return false;
    }
}

bool JackWinEvent::Allocate(const char* name, const char* server_name, int value)
{
    BuildName(name, server_name, fName, sizeof(fName));
    jack_log("JackWinEvent::Allocate name = %s val = %ld", fName, value);

    /* create an auto reset event */
    if ((fEvent = CreateEvent(NULL, FALSE, FALSE, fName)) == NULL) {
        jack_error("Allocate: can't check in named event name = %s err = %ld", fName, GetLastError());
        return false;
    } else if (GetLastError() == ERROR_ALREADY_EXISTS) {
        jack_error("Allocate: named event already exist name = %s", fName);
        CloseHandle(fEvent);
        fEvent = NULL;
        return false;
    } else {
        return true;
    }
}

void JackWinEvent::Destroy()
{
    if (fEvent != NULL) {
        jack_log("JackWinEvent::Destroy %s", fName);
        CloseHandle(fEvent);
        fEvent = NULL;
    } else {
        jack_error("JackWinEvent::Destroy synchro == NULL");
    }
}


} // end of namespace

