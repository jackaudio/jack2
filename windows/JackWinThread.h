/*
Copyright (C) 2001 Paul Davis
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


#ifndef __JackWinThread__
#define __JackWinThread__

#include "JackThread.h"
#include <windows.h>

namespace Jack
{

typedef DWORD  (WINAPI *ThreadCallback)(void *arg);

/*!
\brief Windows threads.
*/

class JackWinThread : public JackThread
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

        int AcquireRealTime();
        int AcquireRealTime(int priority) ;
        int DropRealTime();

        pthread_t GetThreadID();

        static int AcquireRealTimeImp(pthread_t thread, int priority);
        static int DropRealTimeImp(pthread_t thread);
        static int StartImp(pthread_t* thread, int priority, int realtime, ThreadCallback start_routine, void* arg);

};

} // end of namespace

#endif
