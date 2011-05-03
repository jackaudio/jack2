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

#ifndef __JackWinThread__
#define __JackWinThread__

#include "JackThread.h"
#include "JackMMCSS.h"
#include "JackCompilerDeps.h"
#include "JackSystemDeps.h"
#include <windows.h>

namespace Jack
{

typedef DWORD (WINAPI *ThreadCallback)(void *arg);

/*!
\brief Windows threads.
*/

class SERVER_EXPORT JackWinThread : public JackMMCSS, public detail::JackThreadInterface
{

    private:

        HANDLE fThread;
        HANDLE fEvent;

        static DWORD WINAPI ThreadHandler(void* arg);

    public:

        JackWinThread(JackRunnableInterface* runnable);
        ~JackWinThread();

        int Start();
        int StartSync();
        int Kill();
        int Stop();
        void Terminate();

        int AcquireRealTime();                  // Used when called from another thread
        int AcquireSelfRealTime();              // Used when called from thread itself

        int AcquireRealTime(int priority);      // Used when called from another thread
        int AcquireSelfRealTime(int priority);  // Used when called from thread itself

        int DropRealTime();                     // Used when called from another thread
        int DropSelfRealTime();                 // Used when called from thread itself

        jack_native_thread_t GetThreadID();
        bool IsThread();

        static int AcquireRealTimeImp(jack_native_thread_t thread, int priority);
        static int AcquireRealTimeImp(jack_native_thread_t thread, int priority, UInt64 period, UInt64 computation, UInt64 constraint)
        {
            return JackWinThread::AcquireRealTimeImp(thread, priority);
        }
        static int DropRealTimeImp(jack_native_thread_t thread);
        static int StartImp(jack_native_thread_t* thread, int priority, int realtime, void*(*start_routine)(void*), void* arg)
        {
            return JackWinThread::StartImp(thread, priority, realtime, (ThreadCallback) start_routine, arg);
        }
        static int StartImp(jack_native_thread_t* thread, int priority, int realtime, ThreadCallback start_routine, void* arg);
        static int StopImp(jack_native_thread_t thread);
        static int KillImp(jack_native_thread_t thread);

};

SERVER_EXPORT void ThreadExit();

} // end of namespace

#endif

