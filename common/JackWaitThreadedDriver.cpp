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
#include "JackWaitThreadedDriver.h"
#include "JackGlobals.h"
#include "JackClient.h"
#include "JackEngineControl.h"
#include "JackException.h"
#include "JackError.h"
#include "JackTools.h"

namespace Jack
{

bool JackWaitThreadedDriver::Init()
{
    return (fStarter.Start() == 0);
}

bool JackWaitThreadedDriver::Execute()
{
    try {

        SetRealTime();

        // Process a null cycle until NetDriver has started
        while (!fStarter.fRunning && fThread.GetStatus() == JackThread::kRunning) {
            // Use base class method
            assert(static_cast<JackWaiterDriver*>(fDriver));
            static_cast<JackWaiterDriver*>(fDriver)->ProcessNull();
        }

        // Switch to keep running even in case of error
        while (fThread.GetStatus() == JackThread::kRunning) {
            fDriver->Process();
        }

        return false;

    } catch (JackNetException& e) {

        e.PrintMessage();
        jack_info("Driver is restarted");
        fThread.DropSelfRealTime();

        // Thread has been stopped...
        if (fThread.GetStatus() == JackThread::kIdle) {
            return false;
        }

        // Thread in kIniting status again...
        fThread.SetStatus(JackThread::kIniting);
        if (Init()) {
            // Thread in kRunning status again...
            fThread.SetStatus(JackThread::kRunning);
            return true;
        }

        return false;
    }
}

} // end of namespace
