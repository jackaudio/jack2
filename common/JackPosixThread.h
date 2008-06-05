/*
Copyright (C) 2001 Paul Davis
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

#ifndef __JackPosixThread__
#define __JackPosixThread__

#include "JackThread.h"
#include <pthread.h>

namespace Jack
{

/* use 512KB stack per thread - the default is way too high to be feasible
 * with mlockall() on many systems */
#define THREAD_STACK 524288

/*!
\brief The POSIX thread base class.
*/

class EXPORT JackPosixThread : public detail::JackThread
{

    protected:

        pthread_t fThread;
        static void* ThreadHandler(void* arg);

    public:

        JackPosixThread(JackRunnableInterface* runnable, bool real_time, int priority, int cancellation)
                : JackThread(runnable, priority, real_time, cancellation), fThread((pthread_t)NULL)
        {}
        JackPosixThread(JackRunnableInterface* runnable, int cancellation = PTHREAD_CANCEL_ASYNCHRONOUS)
                : JackThread(runnable, 0, false, cancellation), fThread((pthread_t)NULL)
        {}

        int Start();
        int StartSync();
        int Kill();
        int Stop();
        void Terminate();

        int AcquireRealTime();
        int AcquireRealTime(int priority);
        int DropRealTime();

        pthread_t GetThreadID();

        static int AcquireRealTimeImp(pthread_t thread, int priority);
        static int DropRealTimeImp(pthread_t thread);
        static int StartImp(pthread_t* thread, int priority, int realtime, void*(*start_routine)(void*), void* arg);

};

} // end of namespace


#endif
