/*
Copyright (C) 2004-2008 Grame
Copyright (C) 2013 Samsung Electronics

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

#ifndef __JackAndroidSemaphore__
#define __JackAndroidSemaphore__

#include "JackSynchro.h"
#include "JackCompilerDeps.h"
#include <semaphore.h>
#include <time.h>
#include <assert.h>

#include <binder/IMemory.h>
#include <binder/MemoryHeapBase.h>
#include <utils/RefBase.h>

namespace Jack
{

/*!
\brief Inter process synchronization using POSIX semaphore.
*/

class SERVER_EXPORT JackAndroidSemaphore : public detail::JackSynchro
{

    private:

        sem_t* fSemaphore;
        android::sp<android::IMemoryHeap> fSemaphoreMemory;
        static pthread_mutex_t mutex;

    protected:

        void BuildName(const char* name, const char* server_name, char* res, int size);

    public:

        JackAndroidSemaphore():JackSynchro(), fSemaphore(NULL)
        {}

        bool Signal();
        bool SignalAll();
        bool Wait();
        bool TimedWait(long usec);

        bool Allocate(const char* name, const char* server_name, int value);
        bool Connect(const char* name, const char* server_name);
        bool ConnectInput(const char* name, const char* server_name);
        bool ConnectOutput(const char* name, const char* server_name);
        bool Disconnect();
        void Destroy();
};

} // end of namespace


#endif

