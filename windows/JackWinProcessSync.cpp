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

#include "JackWinProcessSync.h"
#include "JackError.h"

namespace Jack
{

void JackWinProcessSync::Signal()
{
    SetEvent(fEvent);
}

void JackWinProcessSync::LockedSignal()
{
    WaitForSingleObject(fMutex, INFINITE);
    SetEvent(fEvent);
    ReleaseMutex(fMutex);
}

void JackWinProcessSync::SignalAll()
{
    SetEvent(fEvent);
}

void JackWinProcessSync::LockedSignalAll()
{
    WaitForSingleObject(fMutex, INFINITE);
    SetEvent(fEvent);
    ReleaseMutex(fMutex);
}

void JackWinProcessSync::Wait()
{
    ReleaseMutex(fMutex);
	HANDLE handles[] = { fMutex, fEvent };
	DWORD res = WaitForMultipleObjects(2, handles, true, INFINITE);
	if ((res != WAIT_OBJECT_0) && (res != WAIT_TIMEOUT))
        jack_error("Wait error err = %d", GetLastError());
    ResetEvent(fEvent);
}

void JackWinProcessSync::LockedWait()
{
    /*
    Does it make sense on Windows, use non-locked version for now...
    WaitForSingleObject(fMutex, INFINITE);
    ReleaseMutex(fMutex);
	HANDLE handles[] = { fMutex, fEvent };
	DWORD res = WaitForMultipleObjects(2, handles, true, INFINITE);
	if ((res != WAIT_OBJECT_0) && (res != WAIT_TIMEOUT))
        jack_error("LockedWait error err = %d", GetLastError());
    ResetEvent(fEvent);
    */
    Wait();
}

bool JackWinProcessSync::TimedWait(long usec)
{
  	ReleaseMutex(fMutex);
	HANDLE handles[] = { fMutex, fEvent };
	DWORD res = WaitForMultipleObjects(2, handles, true, usec / 1000);
	if ((res != WAIT_OBJECT_0) && (res != WAIT_TIMEOUT))
        jack_error("Wait error err = %d", GetLastError());
    ResetEvent(fEvent);
}

bool JackWinProcessSync::LockedTimedWait(long usec)
{
    /*
    Does it make sense on Windows, use non-locked version for now...
    WaitForSingleObject(fMutex, INFINITE);
    ReleaseMutex(fMutex);
    HANDLE handles[] = { fMutex, fEvent };
	DWORD res = WaitForMultipleObjects(2, handles, true, usec / 1000);
    if ((res != WAIT_OBJECT_0) && (res != WAIT_TIMEOUT))
        jack_error("LockedTimedWait error err = %d", GetLastError());
    ResetEvent(fEvent);
	return (res == WAIT_OBJECT_0);
	*/
    return TimedWait(usec);
}


} // end of namespace



