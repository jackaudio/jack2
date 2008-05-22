/*
Copyright (C) 2004-2006 Grame

This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef __JackWinProcessSync__
#define __JackWinProcessSync__

#include "JackSyncInterface.h"
#include <windows.h>
#include <new>

namespace Jack
{

/*!
\brief  A synchronization primitive built using a condition variable.
*/

class JackWinProcessSync : public JackSyncInterface
{

    private:

        HANDLE fEvent;

    public:

        JackWinProcessSync()
        {
            fEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (fEvent == NULL)
                throw new std::bad_alloc;
        }
        virtual ~JackWinProcessSync()
        {
            CloseHandle(fEvent);
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

        bool TimedWait(long usec)
        {
            DWORD res = WaitForSingleObject(fEvent, usec / 1000);
            return (res == WAIT_OBJECT_0);
        }

        void Wait()
        {
            WaitForSingleObject(fEvent, INFINITE);
        }

        void SignalAll()
        {
            SetEvent(fEvent);
        }

};

} // end of namespace

#endif

