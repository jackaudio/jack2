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
                jack_log("JackWinMutex::Lock res = %d", res);
                return false;
            }
        } else {
            jack_error("JackWinMutex::Lock mutex already locked by thread = %d", GetCurrentThreadId());
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
                jack_log("JackWinMutex::Trylock res = %d", res);
                return false;
            }
        } else {
            jack_error("JackWinMutex::Trylock mutex already locked by thread = %d", GetCurrentThreadId());
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
                jack_log("JackWinMutex::Unlock res = %d", res);
                return false;
            }
        } else {
            jack_error("JackWinMutex::Unlock mutex not locked by thread = %d", GetCurrentThreadId());
            return false;
        }
    }

    bool JackWinMutex::Lock()
    {
        return (WAIT_OBJECT_0 == WaitForSingleObject(fMutex, INFINITE));
    }

    bool JackWinMutex::Trylock()
    {
        return (WAIT_OBJECT_0 == WaitForSingleObject(fMutex, 0));
    }

    bool JackWinMutex::Unlock()
    {
        return(ReleaseMutex(fMutex) != 0);
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


