/*
Copyright (C) 2001 Paul Davis
Copyright (C) 2004-2008 Grame

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

#ifndef __JackThread__
#define __JackThread__

#ifdef WIN32
	#include <windows.h>
typedef HANDLE pthread_t;
typedef ULONGLONG UInt64;
#else
	#include <pthread.h>
typedef unsigned long long UInt64;
#endif

namespace Jack
{

/*!
\brief The base class for runnable objects, that have an <B> Init </B> and <B> Execute </B> method to be called in a thread.
*/

class JackRunnableInterface
{

    public:

        JackRunnableInterface()
        {}
        virtual ~JackRunnableInterface()
        {}

        virtual bool Init()          /*! Called once when the thread is started */
        {
            return true;
        }
        virtual bool Execute() = 0;  /*! Must be implemented by subclasses */
};

/*!
\brief The thread base class.
*/

class JackThread
{

    protected:

        JackRunnableInterface* fRunnable;
        int fPriority;
        bool fRealTime;
        volatile bool fRunning;
        int fCancellation;

    public:

        JackThread(JackRunnableInterface* runnable, int priority, bool real_time, int cancellation):
                fRunnable(runnable), fPriority(priority), fRealTime(real_time), fRunning(false), fCancellation(cancellation)
        {}
        virtual ~JackThread()
        {}

        virtual int Start() = 0;
        virtual int StartSync() = 0;
        virtual int Kill() = 0;
        virtual int Stop() = 0;
		virtual void Terminate() = 0;

        virtual int AcquireRealTime() = 0;
        virtual int AcquireRealTime(int priority) = 0;
        virtual int DropRealTime() = 0;

        virtual void SetParams(UInt64 period, UInt64 computation, UInt64 constraint) // Empty implementation, will only make sense on OSX...
        {}

        virtual pthread_t GetThreadID() = 0;
};

} // end of namespace

#endif
