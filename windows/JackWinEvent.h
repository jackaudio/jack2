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

#ifndef __JackWinEvent__
#define __JackWinEvent__

#include "JackSynchro.h"
#include <windows.h>

namespace Jack
{

// http://bob.developpez.com/tutapiwin/article_56.php

/*!
\brief Inter process synchronization using system wide events.
*/

class JackWinEvent : public JackSynchro
{

    private:

        HANDLE fEvent;

    protected:

        void BuildName(const char* name, const char* server_name, char* res, int size);

    public:

        JackWinEvent(): JackSynchro(), fEvent(NULL)
        {}
        virtual ~JackWinEvent()
        {}

        bool Signal();
        bool SignalAll();
        bool Wait();
        bool TimedWait(long usec);

        bool Allocate(const char* name, const char* server_name, int value);
        bool Connect(const char* name, const char* server_name);
        bool ConnectInput(const char* name, const char* server_name);
        bool ConnectOutput(const char* name, const char* server_name);
        bool Disconnect();
        void Destroy();
};

} // end of namespace

#endif

