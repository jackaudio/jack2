/*
Copyright (C) 2003 Paul Davis
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

#ifndef __JackClientControl__
#define __JackClientControl__

#include "JackShmMem.h"
#include "JackPort.h"
#include "JackSynchro.h"
#include "JackNotification.h"
#include "transport_types.h"

namespace Jack
{

/*!
\brief Client control in shared memory. 
*/

struct JackClientControl : public JackShmMem
{
    char fName[JACK_CLIENT_NAME_SIZE + 1];
	bool fCallback[kMaxNotification];
  	volatile jack_transport_state_t fTransportState;
	int fRefNum;
	bool fActive;

    JackClientControl(const char* name, int refnum)
    {
        Init(name, refnum);
    }

    JackClientControl(const char* name)
    {
        Init(name, -1);
    }

    JackClientControl()
    {
        Init("", -1);
    }

    void Init(const char* name, int refnum)
    {
        strcpy(fName, name);
		for (int i = 0; i < kMaxNotification; i++) 
			fCallback[i] = false;
		// Always activated
		fCallback[kAddClient] = true;
		fCallback[kRemoveClient] = true;
		fCallback[kActivateClient] = true;
		// So that driver synchro are corectly setup in "flush" or "normal" mode 
		fCallback[kStartFreewheelCallback] = true;
		fCallback[kStopFreewheelCallback] = true;
		fRefNum = refnum;
        fTransportState = JackTransportStopped;
        fActive = false;
    }

};

} // end of namespace


#endif
