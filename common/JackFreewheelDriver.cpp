/*
 Copyright (C) 2001 Paul Davis 
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

#include "JackFreewheelDriver.h"
#include "JackEngineControl.h"
#include "JackEngine.h"

namespace Jack
{

int JackFreewheelDriver::Process()
{
    if (fIsMaster) {
        JackLog("JackFreewheelDriver::Process master %lld\n", fEngineControl->fTimeOutUsecs);
        fLastWaitUst = GetMicroSeconds();
        fEngine->Process(fLastWaitUst);
        fGraphManager->ResumeRefNum(fClientControl, fSynchroTable); // Signal all clients
        if (fGraphManager->SuspendRefNum(fClientControl, fSynchroTable, fEngineControl->fTimeOutUsecs * 20) < 0) // Wait for all clients to finish
            jack_error("JackFreewheelDriver::ProcessSync SuspendRefNum error");
    } else {
        fGraphManager->ResumeRefNum(fClientControl, fSynchroTable); // Signal all clients
        if (fEngineControl->fSyncMode) {
            if (fGraphManager->SuspendRefNum(fClientControl, fSynchroTable, fEngineControl->fTimeOutUsecs) < 0)
                jack_error("JackFreewheelDriver::ProcessSync SuspendRefNum error");
        }
    }
    return 0;
}

} // end of namespace
