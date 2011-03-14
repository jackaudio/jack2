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

#ifndef __JackThread__
#define __JackThread__

#include "JackCompilerDeps.h"
#include "JackTypes.h"

namespace Jack
{

/*!
 \brief The base class for runnable objects, that have an <B> Init </B> and <B> Execute </B> method to be called in a thread.
 */

class JackRunnableInterface
{

    protected:

        JackRunnableInterface()
        {}
        virtual ~JackRunnableInterface()
        {}

    public:

        virtual bool Init()          /*! Called once when the thread is started */
        {
            return true;
        }
        virtual bool Execute() = 0;  /*! Must be implemented by subclasses */
};

namespace detail
{

/*!
 \brief The thread base class.
 */

class SERVER_EXPORT JackThreadInterface
{

    public:

        enum kThreadState {kIdle, kStarting, kIniting, kRunning};

    protected:

        JackRunnableInterface* fRunnable;
        int fPriority;
        bool fRealTime;
        volatile kThreadState fStatus;
        int fCancellation;

    public:

        JackThreadInterface(JackRunnableInterface* runnable, int priority, bool real_time, int cancellation):
        fRunnable(runnable), fPriority(priority), fRealTime(real_time), fStatus(kIdle), fCancellation(cancellation)
        {}

        kThreadState GetStatus()
        {
            return fStatus;
        }
        void SetStatus(kThreadState status)
        {
            fStatus = status;
        }

        void SetParams(UInt64 period, UInt64 computation, UInt64 constraint) // Empty implementation, will only make sense on OSX...
        {}

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
        static int AcquireRealTimeImp(jack_native_thread_t thread, int priority, UInt64 period, UInt64 computation, UInt64 constraint);
        static int DropRealTimeImp(jack_native_thread_t thread);
        static int StartImp(jack_native_thread_t* thread, int priority, int realtime, void*(*start_routine)(void*), void* arg);
        static int StopImp(jack_native_thread_t thread);
        static int KillImp(jack_native_thread_t thread);
};

}

} // end of namespace

bool jack_get_thread_realtime_priority_range(int * min_ptr, int * max_ptr);

bool jack_tls_allocate_key(jack_tls_key *key_ptr);
bool jack_tls_free_key(jack_tls_key key);

bool jack_tls_set(jack_tls_key key, void *data_ptr);
void *jack_tls_get(jack_tls_key key);

#endif
