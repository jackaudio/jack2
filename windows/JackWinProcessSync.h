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

#ifndef __JackWinProcessSync__
#define __JackWinProcessSync__

#include "JackWinMutex.h"

namespace Jack
{

/*!
\brief A synchronization primitive built using a condition variable.
*/

class JackWinProcessSync : public JackWinMutex
{

    private:

        HANDLE fEvent;

    public:

        JackWinProcessSync(const char* name = NULL):JackWinMutex(name)
        {
            if (name) {
                char buffer[MAX_PATH];
                snprintf(buffer, sizeof(buffer), "%s_%s", "JackWinProcessSync", name);
                fEvent = CreateEvent(NULL, TRUE, FALSE, buffer);  // Needs ResetEvent
                //fEvent = CreateEvent(NULL, FALSE, FALSE, buffer);   // Auto-reset event
            } else {
                fEvent = CreateEvent(NULL, TRUE, FALSE, NULL);   // Needs ResetEvent
                //fEvent = CreateEvent(NULL, FALSE, FALSE, NULL);   // Auto-reset event
            }

            ThrowIf((fEvent == 0), JackException("JackWinProcessSync: could not init the event"));
        }
        virtual ~JackWinProcessSync()
        {
            CloseHandle(fEvent);
        }

        bool TimedWait(long usec);
        bool LockedTimedWait(long usec);

        void Wait();
        void LockedWait();

        void Signal();
        void LockedSignal();

        void SignalAll();
        void LockedSignalAll();
};

#ifdef __MINGW64__

class JackWinCondVar {

        CONDITION_VARIABLE fCondVar;
        CRITICAL_SECTION fMutex;

    public:
    
        JackWinCondVar(const char* name = NULL)
        {
             InitializeCriticalSection(&fMutex);
             InitializeConditionVariable(&fCondVar);
        }
        
        virtual ~JackWinCondVar()
        {
             DeleteCriticalSection(&fMutex);
        }
        
        bool TimedWait(long usec)
        {
            return SleepConditionVariableCS(&fCondVar, &fMutex, usec / 1000);
        }
        
        bool LockedTimedWait(long usec)
        {
            EnterCriticalSection(&fMutex);
            return SleepConditionVariableCS(&fCondVar, &fMutex, usec / 1000);
        }
        
        void Wait()
        {
            SleepConditionVariableCS(&fCondVar, &fMutex, INFINITE);
        }
        
        void LockedWait()
        {
            EnterCriticalSection(&fMutex);
            SleepConditionVariableCS(&fCondVar, &fMutex, INFINITE);
        }
        
        void Signal()
        {
            WakeConditionVariable(&fCondVar);
        }
        
        void LockedSignal()
        {
            EnterCriticalSection(&fMutex);
            WakeConditionVariable(&fCondVar);
            LeaveCriticalSection(&fMutex);
        }

        void SignalAll()
        {
            WakeAllConditionVariable(&fCondVar);
        }
        
        void LockedSignalAll()
        {
            EnterCriticalSection(&fMutex);
            WakeAllConditionVariable(&fCondVar);
            LeaveCriticalSection(&fMutex);
        }
               
        bool Lock()
        {
            EnterCriticalSection(&fMutex);
            return true;
        }

        bool Trylock()
        {
            return (TryEnterCriticalSection(&fMutex));
        }

        bool Unlock()
        {
            LeaveCriticalSection(&fMutex);
            return true;
        }
        
};

#endif

} // end of namespace

#endif

