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

#include "JackExports.h"

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

/*!
 \brief The thread base class.
 */

namespace detail
{

class EXPORT JackThread
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
        
        JackThread(JackRunnableInterface* runnable, int priority, bool real_time, int cancellation):
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
        
};
    
}

} // end of namespace

#if defined(WIN32)
typedef DWORD jack_tls_key;
#else
typedef pthread_key_t jack_tls_key;
#endif

/*
EXPORT bool jack_tls_allocate_key(jack_tls_key *key_ptr);
EXPORT bool jack_tls_free_key(jack_tls_key key);

EXPORT bool jack_tls_set(jack_tls_key key, void *data_ptr);
EXPORT void *jack_tls_get(jack_tls_key key);
*/

bool jack_tls_allocate_key(jack_tls_key *key_ptr);
bool jack_tls_free_key(jack_tls_key key);

bool jack_tls_set(jack_tls_key key, void *data_ptr);
void *jack_tls_get(jack_tls_key key);


#endif
