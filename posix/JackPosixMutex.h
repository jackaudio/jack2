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

#ifndef __JackPosixMutex__
#define __JackPosixMutex__

#include "JackException.h"
#include <pthread.h>
#include <stdio.h>
#include <assert.h>

namespace Jack
{
/*!
\brief Mutex abstraction.
*/

class JackBasePosixMutex
{

    protected:

        pthread_mutex_t fMutex;
        pthread_t fOwner;

    public:

        JackBasePosixMutex():fOwner(0)
        {
            int res = pthread_mutex_init(&fMutex, NULL);
            ThrowIf(res != 0, JackException("JackBasePosixMutex: could not init the mutex"));
        }

        virtual ~JackBasePosixMutex()
        {
            pthread_mutex_destroy(&fMutex);
        }

        bool Lock();
        bool Trylock();
        bool Unlock();

};

class JackPosixMutex
{
    protected:

        pthread_mutex_t fMutex;

    public:

        JackPosixMutex()
        {
            // Use recursive mutex
            pthread_mutexattr_t mutex_attr;
            int res;
            res = pthread_mutexattr_init(&mutex_attr);
            ThrowIf(res != 0, JackException("JackBasePosixMutex: could not init the mutex attribute"));
            res = pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
            ThrowIf(res != 0, JackException("JackBasePosixMutex: could not settype the mutex"));
            res = pthread_mutex_init(&fMutex, &mutex_attr);
            ThrowIf(res != 0, JackException("JackBasePosixMutex: could not init the mutex"));
            pthread_mutexattr_destroy(&mutex_attr);
        }

        virtual ~JackPosixMutex()
        {
            pthread_mutex_destroy(&fMutex);
        }

        bool Lock();
        bool Trylock();
        bool Unlock();
};


} // namespace

#endif
