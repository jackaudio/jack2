/*
Copyright (C) 2003 Paul Davis
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

#ifndef __JackClientControl__
#define __JackClientControl__

#include "JackShmMem.h"
#include "JackPort.h"
#include "JackSynchro.h"
#include "JackNotification.h"
#include "JackSession.h"

namespace Jack
{

/*!
\brief Client control possibly in shared memory.
*/

PRE_PACKED_STRUCTURE
struct JackClientControl : public JackShmMemAble
{
    char fName[JACK_CLIENT_NAME_SIZE + 1];
    bool fCallback[kMaxNotification];
    volatile jack_transport_state_t fTransportState;
    volatile bool fTransportSync;      /* Will be true when slow-sync cb has to be called */
    volatile bool fTransportTimebase;  /* Will be true when timebase cb is called with new_pos on */
    int fRefNum;
    int fPID;
    bool fActive;

    int fSessionID;
    char fSessionCommand[JACK_SESSION_COMMAND_SIZE];
    jack_session_flags_t fSessionFlags;

    JackClientControl(const char* name, int pid, int refnum, int uuid)
    {
        Init(name, pid, refnum, uuid);
    }

    JackClientControl(const char* name)
    {
        Init(name, 0, -1, -1);
    }

    JackClientControl()
    {
        Init("", 0, -1, -1);
    }

    void Init(const char* name, int pid, int refnum, int uuid)
    {
        strcpy(fName, name);
        for (int i = 0; i < kMaxNotification; i++) {
            fCallback[i] = false;
        }
        // Always activated
        fCallback[kAddClient] = true;
        fCallback[kRemoveClient] = true;
        fCallback[kActivateClient] = true;
        fCallback[kLatencyCallback] = true;
        // So that driver synchro are correctly setup in "flush" or "normal" mode
        fCallback[kStartFreewheelCallback] = true;
        fCallback[kStopFreewheelCallback] = true;
        fRefNum = refnum;
        fPID = pid;
        fTransportState = JackTransportStopped;
        fTransportSync = false;
        fTransportTimebase = false;
        fActive = false;

        fSessionID = uuid;
    }

} POST_PACKED_STRUCTURE;

} // end of namespace


#endif
