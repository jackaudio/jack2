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

#ifndef __JackPosixThread__
#define __JackPosixThread__

#include "JackThread.h"
#include <pthread.h>

namespace Jack
{

/*!
\brief The POSIX thread base class.
*/

class JackPosixThread : public JackThread
{

    protected:

        pthread_t fThread;
        static void* ThreadHandler(void* arg);

    public:

        JackPosixThread(JackRunnableInterface* runnable, bool real_time, int priority, int cancellation)
                : JackThread(runnable, priority, real_time, cancellation), fThread((pthread_t)NULL)
        {}
        JackPosixThread(JackRunnableInterface* runnable)
                : JackThread(runnable, 0, false, PTHREAD_CANCEL_DEFERRED), fThread((pthread_t)NULL)
        {}
        JackPosixThread(JackRunnableInterface* runnable, int cancellation)
                : JackThread(runnable, 0, false, cancellation), fThread((pthread_t)NULL)
        {}

        virtual ~JackPosixThread()
        {}

        virtual int Start();
        virtual int StartSync();
        virtual int Kill();
        virtual int Stop();

        virtual int AcquireRealTime();
        virtual int AcquireRealTime(int priority);
        virtual int DropRealTime();

        pthread_t GetThreadID();
		
		static int AcquireRealTimeImp(pthread_t thread, int priority);
		static int DropRealTimeImp(pthread_t thread);
		static int StartImp(pthread_t* thread, int priority, int realtime, void*(*start_routine)(void*), void* arg);

};

} // end of namespace


#endif
