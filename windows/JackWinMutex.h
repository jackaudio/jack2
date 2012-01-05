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


#ifndef __JackWinMutex__
#define __JackWinMutex__

#include "JackCompilerDeps.h"
#include "JackException.h"
#include <windows.h>

namespace Jack
{

/*!
\brief Mutex abstraction.
*/
class SERVER_EXPORT JackBaseWinMutex
{

    protected:

        HANDLE fMutex;
        DWORD fOwner;

    public:

        JackBaseWinMutex():fOwner(0)
        {
            // In recursive mode by default
            fMutex = (HANDLE)CreateMutex(0, FALSE, 0);
            ThrowIf(fMutex == 0, JackException("JackWinMutex: could not init the mutex"));
        }

        virtual ~JackBaseWinMutex()
        {
            CloseHandle(fMutex);
        }

        bool Lock();
        bool Trylock();
        bool Unlock();

};

class SERVER_EXPORT JackWinMutex
{

    protected:

        HANDLE fMutex;

    public:

        JackWinMutex()
        {
            // In recursive mode by default
            fMutex = (HANDLE)CreateMutex(0, FALSE, 0);
        }

        virtual ~JackWinMutex()
        {
            CloseHandle(fMutex);
        }

        bool Lock();
        bool Trylock();
        bool Unlock();

};


} // namespace

#endif

