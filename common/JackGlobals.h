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

#ifndef __JackGlobals__
#define __JackGlobals__

#include "JackPlatformPlug.h"
#include "JackSystemDeps.h"
#include "JackConstants.h"

#ifdef __CLIENTDEBUG__
#include <iostream>
#include <fstream>
#include <string>
#include <time.h>
#endif

namespace Jack
{

// Globals used for client management on server or library side.
struct JackGlobals {

    static jack_tls_key fRealTimeThread;
    static jack_tls_key fNotificationThread;
    static jack_tls_key fKeyLogFunction;
    static JackMutex* fOpenMutex;
    static volatile bool fServerRunning;
    static JackClient* fClientTable[];
    static bool fVerbose;
#ifndef WIN32
    static jack_thread_creator_t fJackThreadCreator;
#endif

#ifdef __CLIENTDEBUG__
    static std::ofstream* fStream;
#endif
     static void CheckContext(const char* name);
};

// Each "side" server and client will implement this to get the shared graph manager, engine control and inter-process synchro table.
extern SERVER_EXPORT JackGraphManager* GetGraphManager();
extern SERVER_EXPORT JackEngineControl* GetEngineControl();
extern SERVER_EXPORT JackSynchro* GetSynchroTable();

} // end of namespace

#endif
