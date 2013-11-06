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

// When used in "master" mode

int JackFreewheelDriver::Process()
{
   jack_log("JackFreewheelDriver::Process master %lld", fEngineControl->fTimeOutUsecs);
   JackDriver::CycleTakeBeginTime();

   if (fEngine->Process(fBeginDateUst, fEndDateUst)) {

        // Resume connected clients in the graph
        if (ResumeRefNum() < 0) {
            jack_error("JackFreewheelDriver::Process: ResumeRefNum error");
        }

        // Special "SuspendRefNum" with longer timeout
        if (SuspendRefNum() < 0) { // Wait for all clients to finish for FREEWHEEL_DRIVER_TIMEOUT sec
            jack_error("JackFreewheelDriver::Process: SuspendRefNum error");
        }

   } else { // Graph not finished: do not activate it
        jack_error("JackFreewheelDriver::Process: Process error");
   }

   return 0;
}

// When used in "slave" mode

int JackFreewheelDriver::ProcessReadSync()
{
    // Resume connected clients in the graph
    if (ResumeRefNum() < 0) {      // Signal all clients
        jack_error("JackFreewheelDriver::ProcessReadSync: ResumeRefNum error");
        return -1;
    }
    return 0;
}

int JackFreewheelDriver::ProcessWriteSync()
{
    // Generic "SuspendRefNum" here
    if (JackDriver::SuspendRefNum() < 0) {
        jack_error("JackFreewheelDriver::ProcessSync: SuspendRefNum error");
        return -1;
    }
    return 0;
}

int JackFreewheelDriver::ProcessReadAsync()
{
    // Resume connected clients in the graph
    if (ResumeRefNum() < 0) {      // Signal all clients
        jack_error("JackFreewheelDriver::ProcessReadAsync: ResumeRefNum error");
        return -1;
    }
    return 0;
}

int JackFreewheelDriver::ProcessWriteAsync()
{
    return 0;
}

int JackFreewheelDriver::SuspendRefNum()
{
    return fGraphManager->SuspendRefNum(&fClientControl, fSynchroTable, FREEWHEEL_DRIVER_TIMEOUT * 1000000);
}

} // end of namespace
