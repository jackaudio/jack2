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
#include "JackCompilerDeps.h"

#include <pthread.h>
#include <stdio.h>
#include <assert.h>

namespace Jack
{
/*!
\brief Mutex abstraction.
*/

class SERVER_EXPORT JackBasePosixMutex
{

    protected:

        pthread_mutex_t fMutex;
        pthread_t fOwner;

    public:

        JackBasePosixMutex(const char* name = NULL);
        virtual ~JackBasePosixMutex();

        bool Lock();
        bool Trylock();
        bool Unlock();

};

class SERVER_EXPORT JackPosixMutex
{
    protected:

        pthread_mutex_t fMutex;

    public:

        JackPosixMutex(const char* name = NULL);
        virtual ~JackPosixMutex();

        bool Lock();
        bool Trylock();
        bool Unlock();
};

} // namespace

#endif
