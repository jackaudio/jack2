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

#include "JackPosixMutex.h"
#include "JackError.h"

namespace Jack
{

    JackBasePosixMutex::JackBasePosixMutex(const char* name)
        :fOwner(0)
    {
        int res = pthread_mutex_init(&fMutex, NULL);
        ThrowIf(res != 0, JackException("JackBasePosixMutex: could not init the mutex"));
    }

    JackBasePosixMutex::~JackBasePosixMutex()
    {
        pthread_mutex_destroy(&fMutex);
    }

    bool JackBasePosixMutex::Lock()
    {
        pthread_t current_thread = pthread_self();

        if (!pthread_equal(current_thread, fOwner)) {
            int res = pthread_mutex_lock(&fMutex);
            if (res == 0) {
                fOwner = current_thread;
                return true;
            } else {
                jack_error("JackBasePosixMutex::Lock res = %d", res);
                return false;
            }
        } else {
            jack_error("JackBasePosixMutex::Lock mutex already locked by thread = %d", current_thread);
            return false;
        }
    }

    bool JackBasePosixMutex::Trylock()
    {
        pthread_t current_thread = pthread_self();

        if (!pthread_equal(current_thread, fOwner)) {
            int res = pthread_mutex_trylock(&fMutex);
            if (res == 0) {
                fOwner = current_thread;
                return true;
            } else {
                return false;
            }
        } else {
            jack_error("JackBasePosixMutex::Trylock mutex already locked by thread = %d", current_thread);
            return false;
        }
    }

    bool JackBasePosixMutex::Unlock()
    {
        if (pthread_equal(pthread_self(), fOwner)) {
            fOwner = 0;
            int res = pthread_mutex_unlock(&fMutex);
            if (res == 0) {
                return true;
            } else {
                jack_error("JackBasePosixMutex::Unlock res = %d", res);
                return false;
            }
        } else {
            jack_error("JackBasePosixMutex::Unlock mutex not locked by thread = %d owner %d", pthread_self(), fOwner);
            return false;
        }
    }

    JackPosixMutex::JackPosixMutex(const char* name)
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

    JackPosixMutex::~JackPosixMutex()
    {
        pthread_mutex_destroy(&fMutex);
    }

    bool JackPosixMutex::Lock()
    {
        int res = pthread_mutex_lock(&fMutex);
        if (res != 0) {
            jack_log("JackPosixMutex::Lock res = %d", res);
        }
        return (res == 0);
    }

    bool JackPosixMutex::Trylock()
    {
        return (pthread_mutex_trylock(&fMutex) == 0);
    }

    bool JackPosixMutex::Unlock()
    {
        int res = pthread_mutex_unlock(&fMutex);
        if (res != 0) {
            jack_log("JackPosixMutex::Unlock res = %d", res);
        }
        return (res == 0);
    }


} // namespace
