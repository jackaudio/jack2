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

#include "JackWinMutex.h"
#include "JackError.h"

namespace Jack
{

    bool JackBaseWinMutex::Lock()
    {
        if (fOwner != GetCurrentThreadId()) {
            DWORD res = WaitForSingleObject(fMutex, INFINITE);
            if (res == WAIT_OBJECT_0) {
                fOwner = GetCurrentThreadId();
                return true;
            } else {
                jack_log("JackBaseWinMutex::Lock res = %d", res);
                return false;
            }
        } else {
            jack_error("JackBaseWinMutex::Lock mutex already locked by thread = %d", GetCurrentThreadId());
            return false;
        }
    }

    bool JackBaseWinMutex::Trylock()
    {
        if (fOwner != GetCurrentThreadId()) {
            DWORD res = WaitForSingleObject(fMutex, 0);
            if (res == WAIT_OBJECT_0) {
                fOwner = GetCurrentThreadId();
                return true;
            } else {
                jack_log("JackBaseWinMutex::Trylock res = %d", res);
                return false;
            }
        } else {
            jack_error("JackBaseWinMutex::Trylock mutex already locked by thread = %d", GetCurrentThreadId());
            return false;
        }
    }

    bool JackBaseWinMutex::Unlock()
    {
        if (fOwner == GetCurrentThreadId()) {
            fOwner = 0;
            int res = ReleaseMutex(fMutex);
            if (res != 0) {
                return true;
            } else {
                jack_log("JackBaseWinMutex::Unlock res = %d", res);
                return false;
            }
        } else {
            jack_error("JackBaseWinMutex::Unlock mutex not locked by thread = %d", GetCurrentThreadId());
            return false;
        }
    }

    bool JackWinMutex::Lock()
    {
        if (WAIT_OBJECT_0 == WaitForSingleObject(fMutex, INFINITE)) {
            return true;
        } else  {
            jack_log("JackWinProcessSync::Lock WaitForSingleObject err = %d", GetLastError());
            return false;
        }
    }

    bool JackWinMutex::Trylock()
    {
        if (WAIT_OBJECT_0 == WaitForSingleObject(fMutex, 0)) {
            return true;
        } else  {
            jack_log("JackWinProcessSync::Trylock WaitForSingleObject err = %d", GetLastError());
            return false;
        }
    }

    bool JackWinMutex::Unlock()
    {
        if (!ReleaseMutex(fMutex)) {
            jack_log("JackWinProcessSync::Unlock ReleaseMutex err = %d", GetLastError());
            return false;
        } else  {
            return true;
        }
    }

    bool JackWinCriticalSection::Lock()
    {
        EnterCriticalSection(&fSection);
        return true;
    }

    bool JackWinCriticalSection::Trylock()
    {
        return (TryEnterCriticalSection(&fSection));
    }

    bool JackWinCriticalSection::Unlock()
    {
        LeaveCriticalSection(&fSection);
        return true;
    }

} // namespace


