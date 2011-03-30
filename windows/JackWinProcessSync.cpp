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
	WaitForSingleObject(fEvent, INFINITE);
}

void JackWinProcessSync::LockedWait()
{
    /* Does it make sense on Windows, use non-locked version for now... */
	Wait();
}

bool JackWinProcessSync::TimedWait(long usec)
{
    ReleaseMutex(fMutex);
	DWORD res = WaitForSingleObject(fEvent, usec / 1000);
	return (res == WAIT_OBJECT_0);
}

bool JackWinProcessSync::LockedTimedWait(long usec)
{
    /* Does it make sense on Windows, use non-locked version for now...*/
    return TimedWait(usec);
}

/*
Code from APPLE CAGuard.cpp : does not seem to work as expected...

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
    WaitForSingleObject(fMutex, INFINITE);
    ReleaseMutex(fMutex);
	HANDLE handles[] = { fMutex, fEvent };
	DWORD res = WaitForMultipleObjects(2, handles, true, INFINITE);
	if ((res != WAIT_OBJECT_0) && (res != WAIT_TIMEOUT))
        jack_error("LockedWait error err = %d", GetLastError());
    ResetEvent(fEvent);
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
    WaitForSingleObject(fMutex, INFINITE);
    ReleaseMutex(fMutex);
    HANDLE handles[] = { fMutex, fEvent };
	DWORD res = WaitForMultipleObjects(2, handles, true, usec / 1000);
    if ((res != WAIT_OBJECT_0) && (res != WAIT_TIMEOUT))
        jack_error("LockedTimedWait error err = %d", GetLastError());
    ResetEvent(fEvent);
	return (res == WAIT_OBJECT_0);
}
*/

} // end of namespace



