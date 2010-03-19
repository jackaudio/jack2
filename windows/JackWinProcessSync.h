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


#ifndef __JackWinProcessSync__
#define __JackWinProcessSync__

#include "JackWinMutex.h"

namespace Jack
{

/*!
\brief  A synchronization primitive built using a condition variable.
*/

class JackWinProcessSync : public JackWinMutex
{

    private:

        HANDLE fEvent;

    public:

        JackWinProcessSync():JackWinMutex()
        {
            fEvent = (HANDLE)CreateEvent(NULL, FALSE, FALSE, NULL);
        }
        ~JackWinProcessSync()
        {
            CloseHandle(fEvent);
        }
        
        bool TimedWait(long usec);
        bool LockedTimedWait(long usec);
        
        void Wait();
        void LockedWait();
         
        void Signal();
        void LockedSignal();
         
        void SignalAll();
        void LockedSignalAll();
};

} // end of namespace

#endif

