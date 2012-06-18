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

#include <assert.h>
#include "JackError.h"
#include "JackPlatformPlug.h"


namespace Jack
{
/*!
\brief Base class for "lockable" objects.
*/

class JackLockAble
{

    protected:

        JackMutex fMutex;

        JackLockAble(const char* name = NULL)
            :fMutex(name)
        {}
        ~JackLockAble()
        {}

    public:

        bool Lock()
        {
            return fMutex.Lock();
        }

        bool Trylock()
        {
            return fMutex.Trylock();
        }

        bool Unlock()
        {
            return fMutex.Unlock();
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

        ~JackLock()
        {
            fObj->Unlock();
        }
};


} // namespace

#endif
