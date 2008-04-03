/*

 Copyright (C) 2006 Grame

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 Grame Research Laboratory, 9 rue du Garet, 69001 Lyon - France
 grame@grame.fr

*/

#ifndef __JackMutex__
#define __JackMutex__

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include<assert.h>

namespace Jack
{

class JackMutex
{

    private:

#ifdef WIN32
        HANDLE fMutex;
#else
        pthread_mutex_t fMutex;
#endif

    public:

#ifdef WIN32

        JackMutex()
        {
            fMutex = CreateMutex(0, FALSE, 0);
        }
        virtual ~JackMutex()
        {
            CloseHandle(fMutex);
        }

        void Lock()
        {
            DWORD dwWaitResult = WaitForSingleObject(fMutex, INFINITE);
        }

        bool Trylock()
        {
           return (WAIT_OBJECT_0 == WaitForSingleObject(fMutex, 0));
        }

        void Unlock()
        {
            ReleaseMutex(fMutex);
        }

#else

        JackMutex()
        {
            // Use recursive mutex
            pthread_mutexattr_t mutex_attr;
            assert(pthread_mutexattr_init(&mutex_attr) == 0);
            assert(pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE) == 0);
            assert(pthread_mutex_init(&fMutex, &mutex_attr) == 0);
        }
        virtual ~JackMutex()
        {
            pthread_mutex_destroy(&fMutex);
        }

        void Lock()
        {
            pthread_mutex_lock(&fMutex);
        }

	bool Trylock()
        {
            return (pthread_mutex_trylock(&fMutex) == 0);
        }

        void Unlock()
        {
            pthread_mutex_unlock(&fMutex);
        }

#endif
};

class JackLockAble
{

    private:

        JackMutex fMutex;

    public:

        JackLockAble()
        {}
        virtual ~JackLockAble()
        {}

        void Lock()
        {
            fMutex.Lock();
        }

        bool Trylock()
        {
            return fMutex.Trylock();
        }

	void Unlock()
        {
            fMutex.Unlock();
        }

};

class JackLock
{
    private:

        JackLockAble* fObj;

    public:

        JackLock(JackLockAble* obj): fObj(obj)
        {
            fObj->Lock();
        }

        JackLock(const JackLockAble* obj): fObj((JackLockAble*)obj)
        {
            fObj->Lock();
        }

        virtual ~JackLock()
        {
            fObj->Unlock();
        }
};


} // namespace

#endif
