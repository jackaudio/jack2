/*
Copyright (C) 2005 Grame  

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

#ifndef __JackLibGlobals__
#define __JackLibGlobals__

#include "JackShmMem.h"
#include "JackEngineControl.h"
#ifdef __APPLE__
#include "JackMachPort.h"
#include <map>
#endif
#include "JackGlobals.h"
#include "JackTime.h"
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
    JackSynchro* fSynchroTable[CLIENT_NUM];					/*! Shared synchro table */
#ifdef __APPLE__
    std::map<mach_port_t, JackClient*> fClientTable;        /*! Client table */
#endif

    static long fClientCount;
    static JackLibGlobals* fGlobals;

    JackLibGlobals()
    {
        JackLog("JackLibGlobals\n");
        for (int i = 0; i < CLIENT_NUM; i++)
            fSynchroTable[i] = JackGlobals::MakeSynchro();
        fGraphManager = -1;
        fEngineControl = -1;
    }

    virtual ~JackLibGlobals()
    {
        JackLog("~JackLibGlobals\n");
        for (int i = 0; i < CLIENT_NUM; i++) {
            fSynchroTable[i]->Disconnect();
            delete fSynchroTable[i];
        }
    }

    static void Init()
    {
        if (fClientCount++ == 0 && !fGlobals) {
            JackLog("JackLibGlobals Init %x\n", fGlobals);
            JackGlobals::InitClient();
            InitTime();
            fGlobals = new JackLibGlobals();
	   }
    }

    static void Destroy()
    {
        if (--fClientCount == 0 && fGlobals) {
            JackLog("JackLibGlobals Destroy %x\n", fGlobals);
            delete fGlobals;
            fGlobals = NULL;
            JackGlobals::Destroy();
        }
    }

};

} // end of namespace

#endif

