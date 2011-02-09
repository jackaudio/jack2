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

#include <pthread.h>
#include <stdio.h>
#include <assert.h>
#include "JackError.h"

namespace Jack
{
/*!
\brief Mutex abstraction.
*/


class JackBasePosixMutex
{

    protected:

        pthread_mutex_t fMutex;

    public:

        JackBasePosixMutex()
        {
            pthread_mutex_init(&fMutex, NULL);
        }

        virtual ~JackBasePosixMutex()
        {
            pthread_mutex_destroy(&fMutex);
        }

        void Lock()
        {
            int res = pthread_mutex_lock(&fMutex);
            if (res != 0)
                jack_log("JackBasePosixMutex::Lock res = %d", res);
        }

        bool Trylock()
        {
            return (pthread_mutex_trylock(&fMutex) == 0);
        }

        void Unlock()
        {
            int res = pthread_mutex_unlock(&fMutex);
            if (res != 0)
                jack_log("JackBasePosixMutex::Unlock res = %d", res);
        }

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
            assert(res == 0);
            res = pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
            assert(res == 0);
            res = pthread_mutex_init(&fMutex, &mutex_attr);
            assert(res == 0);
            res = pthread_mutexattr_destroy(&mutex_attr);
            assert(res == 0);
        }

        virtual ~JackPosixMutex()
        {
            pthread_mutex_destroy(&fMutex);
        }

        bool Lock()
        {
            int res = pthread_mutex_lock(&fMutex);
            if (res != 0)
                jack_log("JackPosixMutex::Lock res = %d", res);
            return (res == 0);
        }

        bool Trylock()
        {
            return (pthread_mutex_trylock(&fMutex) == 0);
        }

        bool Unlock()
        {
            int res = pthread_mutex_unlock(&fMutex);
            if (res != 0)
                jack_log("JackPosixMutex::Unlock res = %d", res);
            return (res == 0);
        }

};


} // namespace

#endif
