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

#ifdef WIN32
#pragma warning (disable : 4786)
#endif

#include "JackThreadedDriver.h"
#include "JackError.h"
#include "JackGlobals.h"
#include "JackClient.h"
#include "JackEngineControl.h"

namespace Jack
{

JackThreadedDriver::JackThreadedDriver(JackDriverClient* driver)
{
    fThread = JackGlobals::MakeThread(this);
    fDriver = driver;
}

JackThreadedDriver::~JackThreadedDriver()
{
    delete fThread;
    delete fDriver;
}

int JackThreadedDriver::Start()
{
    JackLog("JackThreadedDriver::Start\n");
    int res;

    if ((res = fDriver->Start()) < 0) {
        jack_error("Cannot start driver");
        return res;
    }
    if ((res = fThread->Start()) < 0) {
        jack_error("Cannot start thread");
        return res;
    }

    if (fDriver->IsRealTime()) {
        JackLog("JackThreadedDriver::Start IsRealTime\n");
        if (fThread->AcquireRealTime(GetEngineControl()->fPriority) < 0)
            jack_error("AcquireRealTime error");
    }

    return 0;
}

int JackThreadedDriver::Stop()
{
    JackLog("JackThreadedDriver::Stop\n");
    int res;

    if ((res = fThread->Stop()) < 0) {  // Stop when the thread cycle is finished
        jack_error("Cannot stop thread");
        return res;
    }
    if ((res = fDriver->Stop()) < 0) {
        jack_error("Cannot stop driver");
        return res;
    }
    return 0;
}

bool JackThreadedDriver::Execute()
{
    return (Process() == 0);
}

} // end of namespace
