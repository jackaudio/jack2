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
\brief A synchronization primitive built using a condition variable.
*/

class JackWinProcessSync : public JackWinMutex
{

    private:

        HANDLE fEvent;

    public:

        JackWinProcessSync(const char* name = NULL):JackWinMutex(name)
        {
            if (name) {
                char buffer[MAX_PATH];
                snprintf(buffer, sizeof(buffer), "%s_%s", "JackWinProcessSync", name);
                fEvent = CreateEvent(NULL, TRUE, FALSE, buffer);  // Needs ResetEvent
                //fEvent = CreateEvent(NULL, FALSE, FALSE, buffer);   // Auto-reset event
            } else {
                fEvent = CreateEvent(NULL, TRUE, FALSE, NULL);   // Needs ResetEvent
                //fEvent = CreateEvent(NULL, FALSE, FALSE, NULL);   // Auto-reset event
            }

            ThrowIf((fEvent == 0), JackException("JackWinProcessSync: could not init the event"));
        }
        virtual ~JackWinProcessSync()
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

