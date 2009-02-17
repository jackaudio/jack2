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

#ifndef __JackWinMutex__
#define __JackWinMutex__

#include <windows.h>

namespace Jack
{
/*!
\brief Mutex abstraction.
*/
class JackWinMutex
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

        void Lock()
        {
			WaitForSingleObject(fMutex, INFINITE);
        }

        bool Trylock()
        {
            return (WAIT_OBJECT_0 == WaitForSingleObject(fMutex, 0));
        }

        void Unlock()
        {
            ReleaseMutex(fMutex);
        }

};

} // namespace

#endif
