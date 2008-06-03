/*
Copyright (C) 2005 Grame

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

#ifndef __JackLibGlobals__
#define __JackLibGlobals__

#include "JackShmMem.h"
#include "JackEngineControl.h"
#ifdef __APPLE__
#include "JackMachPort.h"
#include <map>
#endif
#include "JackGlobals.h"
#include "JackPlatformSynchro.h"
#include "JackGraphManager.h"
#include "JackMessageBuffer.h"
#include "JackTime.h"
#include "JackError.h"
#include <assert.h>

namespace Jack
{

class JackClient;

/*!
\brief Global library static structure: singleton kind of pattern.
*/

struct JackLibGlobals
{
    JackShmReadWritePtr<JackGraphManager> fGraphManager;	/*! Shared memory Port manager */
    JackShmReadWritePtr<JackEngineControl> fEngineControl;	/*! Shared engine control */  // transport engine has to be writable
    JackSynchro fSynchroTable[CLIENT_NUM];                  /*! Shared synchro table */
#ifdef __APPLE__
    std::map<mach_port_t, JackClient*> fClientTable;        /*! Client table */
#endif
 
    static int fClientCount;
    static JackLibGlobals* fGlobals;

    JackLibGlobals()
    {
        jack_log("JackLibGlobals");
        JackMessageBuffer::Create();
        fGraphManager = -1;
        fEngineControl = -1;
    }

    virtual ~JackLibGlobals()
    {
        jack_log("~JackLibGlobals");
        for (int i = 0; i < CLIENT_NUM; i++) {
            fSynchroTable[i].Disconnect();
        }
        JackMessageBuffer::Destroy();
    }

    static void Init()
    {
        if (fClientCount++ == 0 && !fGlobals) {
            jack_log("JackLibGlobals Init %x", fGlobals);
            InitTime();
            fGlobals = new JackLibGlobals();
        }
    }

    static void Destroy()
    {
        if (--fClientCount == 0 && fGlobals) {
            jack_log("JackLibGlobals Destroy %x", fGlobals);
            delete fGlobals;
            fGlobals = NULL;
        }
    }

    static void CheckContext()
    {
        if (!(fClientCount > 0 && fGlobals)) {
            jack_error("Error !!! : client accessing an already desallocated library context");
        }
    }

};

} // end of namespace

#endif

