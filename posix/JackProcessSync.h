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

#ifndef __JackProcessSync__
#define __JackProcessSync__

#include "JackPlatformPlug.h"
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

namespace Jack
{

/*!
\brief  A synchronization primitive built using a condition variable.
*/

class JackProcessSync
{

    private:

        pthread_mutex_t fLock;  // Mutex
        pthread_cond_t fCond;   // Condition variable

    public:

        JackProcessSync()
        {
            pthread_mutex_init(&fLock, NULL);
            pthread_cond_init(&fCond, NULL);
        }

        ~JackProcessSync()
        {
            pthread_mutex_destroy(&fLock);
            pthread_cond_destroy(&fCond);
        }

        bool Allocate(const char* name)
        {
            return true;
        }

        bool Connect(const char* name)
        {
            return true;
        }

        void Destroy()
        {}

        bool TimedWait(long usec);
  
        void Wait();
        
        void Signal()
        {
            pthread_mutex_lock(&fLock);
            pthread_cond_signal(&fCond);
            pthread_mutex_unlock(&fLock);
        }
     
        void SignalAll()
        {
            //pthread_mutex_lock(&fLock);
            pthread_cond_broadcast(&fCond);
            //pthread_mutex_unlock(&fLock);
        }

};

/*!
\brief  A synchronization primitive built using an inter-process synchronization object.
*/

class JackInterProcessSync
{

    private:

        JackSynchro* fSynchro;

    public:

        JackInterProcessSync(JackSynchro* synchro): fSynchro(synchro)
        {}
        ~JackInterProcessSync()
        {
            delete fSynchro;
        }

        bool Allocate(const char* name)
        {
            return fSynchro->Allocate(name, "", 0);
        }

        void Destroy()
        {
            fSynchro->Destroy();
        }

        bool Connect(const char* name)
        {
            return fSynchro->Connect(name, "");
        }

        bool TimedWait(long usec);

        void Wait()
        {
            fSynchro->Wait();
        }

        void SignalAll()
        {
            fSynchro->SignalAll();
        }
};


} // end of namespace

#endif

