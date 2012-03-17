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
    if (!SetEvent(fEvent)) {
        jack_error("JackWinProcessSync::Signal SetEvent err = %d", GetLastError());
    }
}

void JackWinProcessSync::LockedSignal()
{
    DWORD res = WaitForSingleObject(fMutex, INFINITE);
    if (res != WAIT_OBJECT_0) {
        jack_error("JackWinProcessSync::LockedSignal WaitForSingleObject err = %d", GetLastError());
    }
    if (!SetEvent(fEvent)) {
        jack_error("JackWinProcessSync::LockedSignal SetEvent err = %d", GetLastError());
    }
    if (!ReleaseMutex(fMutex)) {
        jack_error("JackWinProcessSync::LockedSignal ReleaseMutex err = %d", GetLastError());
    }
}

void JackWinProcessSync::SignalAll()
{
    Signal();
}

void JackWinProcessSync::LockedSignalAll()
{
    LockedSignal();
}

/*
void JackWinProcessSync::Wait()
{
    if (!ReleaseMutex(fMutex)) {
        jack_error("JackWinProcessSync::Wait ReleaseMutex err = %d", GetLastError());
    }
	DWORD res = WaitForSingleObject(fEvent, INFINITE);
    if (res != WAIT_OBJECT_0) {
        jack_error("JackWinProcessSync::Wait WaitForSingleObject err = %d", GetLastError());
    }
}
*/

void JackWinProcessSync::LockedWait()
{
    // Does it make sense on Windows, use non-locked version for now...
	Wait();
}

/*
bool JackWinProcessSync::TimedWait(long usec)
{
    if (!ReleaseMutex(fMutex)) {
        jack_error("JackWinProcessSync::TimedWait ReleaseMutex err = %d", GetLastError());
    }

	DWORD res = WaitForSingleObject(fEvent, usec / 1000);
    if (res != WAIT_OBJECT_0) {
        jack_error("JackWinProcessSync::TimedWait WaitForSingleObject err = %d", GetLastError());
    }

	return (res == WAIT_OBJECT_0);
}
*/
bool JackWinProcessSync::LockedTimedWait(long usec)
{
    // Does it make sense on Windows, use non-locked version for now...
    return TimedWait(usec);
}

void JackWinProcessSync::Wait()
{
    // In case Wait is called in a "locked" context
    if (ReleaseMutex(fMutex)) {
        HANDLE handles[] = { fMutex, fEvent };
        DWORD res = WaitForMultipleObjects(2, handles, true, INFINITE);
        if (res != WAIT_OBJECT_0) {
            jack_error("JackWinProcessSync::Wait WaitForMultipleObjects err = %d", GetLastError());
        }
    // In case Wait is called in a "non-locked" context
    } else {
        jack_error("JackWinProcessSync::Wait ReleaseMutex err = %d", GetLastError());
        DWORD res = WaitForSingleObject(fEvent, INFINITE);
        if (res != WAIT_OBJECT_0) {
            jack_error("JackWinProcessSync::Wait WaitForSingleObject err = %d", GetLastError());
        }
    }

    if (!ResetEvent(fEvent)) {
        jack_error("JackWinProcessSync::Wait ResetEvent err = %d", GetLastError());
    }
}

bool JackWinProcessSync::TimedWait(long usec)
{
    DWORD res = 0;

    // In case TimedWait is called in a "locked" context
  	if (ReleaseMutex(fMutex)) {
        HANDLE handles[] = { fMutex, fEvent };
        res = WaitForMultipleObjects(2, handles, true, usec / 1000);
        if ((res != WAIT_OBJECT_0) && (res != WAIT_TIMEOUT)) {
            jack_error("JackWinProcessSync::TimedWait WaitForMultipleObjects err = %d", GetLastError());
        }
    // In case TimedWait is called in a "non-locked" context
    } else {
        jack_error("JackWinProcessSync::TimedWait ReleaseMutex err = %d", GetLastError());
        res = WaitForSingleObject(fEvent, usec / 1000);
        if (res != WAIT_OBJECT_0) {
            jack_error("JackWinProcessSync::TimedWait WaitForSingleObject err = %d", GetLastError());
        }
    }

	if (!ResetEvent(fEvent)) {
        jack_error("JackWinProcessSync::TimedWait ResetEvent err = %d", GetLastError());
    }

	return (res == WAIT_OBJECT_0);
}

/*
// Code from APPLE CAGuard.cpp : does not seem to work as expected...

void JackWinProcessSync::Wait()
{
    if (!ReleaseMutex(fMutex)) {
        jack_error("JackWinProcessSync::Wait ReleaseMutex err = %d", GetLastError());
    }
	DWORD res = WaitForSingleObject(fEvent, INFINITE);
    if (res != WAIT_OBJECT_0) {
        jack_error("JackWinProcessSync::Wait WaitForSingleObject err = %d", GetLastError());
    }
}

// Variant that behaves differently depending of the mutex state
void JackWinProcessSync::Wait()
{
    if (ReleaseMutex(fMutex)) {
        HANDLE handles[] = { fMutex, fEvent };
        DWORD res = WaitForMultipleObjects(2, handles, true, INFINITE);
        if (res != WAIT_OBJECT_0) {
            jack_error("JackWinProcessSync::LockedWait WaitForMultipleObjects err = %d", GetLastError());
        }
    } else {
        jack_error("JackWinProcessSync::Wait ReleaseMutex err = %d", GetLastError());
        DWORD res = WaitForSingleObject(fEvent, INFINITE);
        if (res != WAIT_OBJECT_0) {
            jack_error("JackWinProcessSync::Wait WaitForSingleObject err = %d", GetLastError());
        }
    }

    if (!ResetEvent(fEvent)) {
        jack_error("JackWinProcessSync::LockedWait ResetEvent err = %d", GetLastError());
    }
}

void JackWinProcessSync::LockedWait()
{
    if (!ReleaseMutex(fMutex)) {
        jack_error("JackWinProcessSync::LockedWait ReleaseMutex err = %d", GetLastError());
    }

	HANDLE handles[] = { fMutex, fEvent };
	DWORD res = WaitForMultipleObjects(2, handles, true, INFINITE);
	if (res != WAIT_OBJECT_0) {
        jack_error("JackWinProcessSync::LockedWait WaitForMultipleObjects err = %d", GetLastError());
    }

    if (!ResetEvent(fEvent)) {
        jack_error("JackWinProcessSync::LockedWait ResetEvent err = %d", GetLastError());
    }
}

bool JackWinProcessSync::TimedWait(long usec)
{
  	if (!ReleaseMutex(fMutex)) {
        jack_error("JackWinProcessSync::TimedWait ReleaseMutex err = %d", GetLastError());
    }

	DWORD res = WaitForSingleObject(fEvent, usec / 1000);
    if (res != WAIT_OBJECT_0) {
        jack_error("JackWinProcessSync::TimedWait WaitForSingleObject err = %d", GetLastError());
    }

	return (res == WAIT_OBJECT_0);
}

// Variant that behaves differently depending of the mutex state
bool JackWinProcessSync::TimedWait(long usec)
{
  	if (ReleaseMutex(fMutex)) {
        HANDLE handles[] = { fMutex, fEvent };
        DWORD res = WaitForMultipleObjects(2, handles, true, usec / 1000);
        if ((res != WAIT_OBJECT_0) && (res != WAIT_TIMEOUT)) {
            jack_error("JackWinProcessSync::LockedTimedWait WaitForMultipleObjects err = %d", GetLastError());
        }
    } else {
        jack_error("JackWinProcessSync::TimedWait ReleaseMutex err = %d", GetLastError());
        DWORD res = WaitForSingleObject(fEvent, usec / 1000);
        if (res != WAIT_OBJECT_0) {
            jack_error("JackWinProcessSync::TimedWait WaitForSingleObject err = %d", GetLastError());
        }
    }

	if (!ResetEvent(fEvent)) {
        jack_error("JackWinProcessSync::LockedTimedWait ResetEvent err = %d", GetLastError());
    }

	return (res == WAIT_OBJECT_0);
}

bool JackWinProcessSync::LockedTimedWait(long usec)
{
   if (!ReleaseMutex(fMutex)) {
        jack_error("JackWinProcessSync::LockedTimedWait ReleaseMutex err = %d", GetLastError());
    }

	HANDLE handles[] = { fMutex, fEvent };
	DWORD res = WaitForMultipleObjects(2, handles, true, usec / 1000);
	if ((res != WAIT_OBJECT_0) && (res != WAIT_TIMEOUT)) {
        jack_error("JackWinProcessSync::LockedTimedWait WaitForMultipleObjects err = %d", GetLastError());
    }

    if (!ResetEvent(fEvent)) {
        jack_error("JackWinProcessSync::LockedTimedWait ResetEvent err = %d", GetLastError());
    }

    return (res == WAIT_OBJECT_0);
}
*/

} // end of namespace



