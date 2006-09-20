/*
Copyright (C) 2004-2005 Grame  

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

#include "JackWinEvent.h"
#include "JackError.h"

// http://www.codeproject.com/win32/Win32_Event_Handling.asp
// http://www.codeproject.com/threads/Synchronization.asp

namespace Jack
{

void JackWinEvent::BuildName(const char* name, char* res)
{
    sprintf(res, "jack_pipe.%s", name);
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

    if ((res = WaitForSingleObject(fEvent, INFINITE)) != WAIT_OBJECT_0) {
        jack_error("JackWinEvent::Wait name = %s err = %ld", fName, GetLastError());
    }

    return (res == WAIT_OBJECT_0);
}

bool JackWinEvent::TimedWait(long usec)
{
    DWORD res;

    if ((res = WaitForSingleObject(fEvent, usec / 1000)) != WAIT_OBJECT_0) {
        jack_error("JackWinEvent::Wait name = %s err = %ld", fName, GetLastError());
    }

    return (res == WAIT_OBJECT_0);
}

// Client side : get the published semaphore from server
bool JackWinEvent::ConnectInput(const char* name)
{
    BuildName(name, fName);
    JackLog("JackWinEvent::Connect %s\n", fName);

    // Temporary...
    if (fEvent) {
        JackLog("Already connected name = %s\n", name);
        return true;
    }

    if ((fEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, fName)) == NULL) {
        jack_error("Connect: can't check in named event name = %s err = %ld", fName, GetLastError());
        return false;
    } else {
        return true;
    }
}

bool JackWinEvent::Connect(const char* name)
{
    return ConnectInput(name);
}

bool JackWinEvent::ConnectOutput(const char* name)
{
    return ConnectInput(name);
}

bool JackWinEvent::Disconnect()
{
    if (fEvent) {
        JackLog("JackWinEvent::Disconnect %s\n", fName);
		SetEvent(fEvent); // to "unlock" threads pending on the event
        CloseHandle(fEvent);
        fEvent = NULL;
        return true;
    } else {
        return false;
    }
}

bool JackWinEvent::Allocate(const char* name, int value)
{
    BuildName(name, fName);
    JackLog("JackWinEvent::Allocate name = %s val = %ld\n", fName, value);

    /* create an auto reset event */
    if ((fEvent = CreateEvent(NULL, FALSE, FALSE, fName)) == NULL) {
        jack_error("Allocate: can't check in named event name = %s err = %ld", fName, GetLastError());
        return false;
    } else {
        return true;
    }
}

void JackWinEvent::Destroy()
{
    if (fEvent != NULL) {
		JackLog("JackWinEvent::Destroy %s\n", fName);
        CloseHandle(fEvent);
        fEvent = NULL;
    } else {
        jack_error("JackWinEvent::Destroy synchro == NULL");
    }
}


} // end of namespace

