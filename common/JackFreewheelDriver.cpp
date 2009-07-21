/*
 Copyright (C) 2001 Paul Davis
 Copyright (C) 2004-2008 Grame

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

#include "JackSystemDeps.h"
#include "JackFreewheelDriver.h"
#include "JackEngineControl.h"
#include "JackLockedEngine.h"

namespace Jack
{

int JackFreewheelDriver::Process()
{
    if (fIsMaster) {
        jack_log("JackFreewheelDriver::Process master %lld", fEngineControl->fTimeOutUsecs);
        JackDriver::CycleTakeBeginTime();
        fEngine->Process(fBeginDateUst, fEndDateUst);
        fGraphManager->ResumeRefNum(&fClientControl, fSynchroTable); // Signal all clients
        if (fGraphManager->SuspendRefNum(&fClientControl, fSynchroTable, FREEWHEEL_DRIVER_TIMEOUT * 1000000) < 0) { // Wait for all clients to finish for 10 sec
            jack_error("JackFreewheelDriver::ProcessSync SuspendRefNum error");
            /* We have a client time-out error, but still continue to process, until a better recovery strategy is chosen */
            return 0;
        }
    } else {
        fGraphManager->ResumeRefNum(&fClientControl, fSynchroTable); // Signal all clients
        if (fEngineControl->fSyncMode) {
            if (fGraphManager->SuspendRefNum(&fClientControl, fSynchroTable, DRIVER_TIMEOUT_FACTOR * fEngineControl->fTimeOutUsecs) < 0) {
                jack_error("JackFreewheelDriver::ProcessSync SuspendRefNum error");
                return -1;
            }
        }
    }
    return 0;
}

} // end of namespace
