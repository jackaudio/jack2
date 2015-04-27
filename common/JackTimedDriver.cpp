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

#include "JackTimedDriver.h"
#include "JackEngineControl.h"
#include "JackTime.h"
#include "JackCompilerDeps.h"
#include <iostream>
#include <unistd.h>
#include <math.h>

namespace Jack
{

int JackTimedDriver::FirstCycle(jack_time_t cur_time_usec)
{
    fAnchorTimeUsec = cur_time_usec;
    return int((double(fEngineControl->fBufferSize) * 1000000) / double(fEngineControl->fSampleRate));
}

int JackTimedDriver::CurrentCycle(jack_time_t cur_time_usec)
{
    return int(((double(fCycleCount) * double(fEngineControl->fBufferSize) * 1000000.) / double(fEngineControl->fSampleRate)) - (cur_time_usec - fAnchorTimeUsec));
}

int JackTimedDriver::Start()
{
    fCycleCount = 0;
    return JackAudioDriver::Start();
}

void JackTimedDriver::ProcessWait()
{
    jack_time_t cur_time_usec = GetMicroSeconds();
    int wait_time_usec;

    if (fCycleCount++ == 0) {
        wait_time_usec = FirstCycle(cur_time_usec);
    } else {
        wait_time_usec = CurrentCycle(cur_time_usec);
    }

    if (wait_time_usec < 0) {
        NotifyXRun(cur_time_usec, float(cur_time_usec - fBeginDateUst));
        fCycleCount = 0;
        wait_time_usec = 0;
        jack_error("JackTimedDriver::Process XRun = %ld usec", (cur_time_usec - fBeginDateUst));
    }

    //jack_log("JackTimedDriver::Process wait_time = %d", wait_time_usec);
    JackSleep(wait_time_usec);
}

int JackWaiterDriver::ProcessNull()
{
    JackDriver::CycleTakeBeginTime();

    // Graph processing without Read/Write
    if (fEngineControl->fSyncMode) {
        ProcessGraphSync();
    } else {
        ProcessGraphAsync();
    }

    // Keep end cycle time
    JackDriver::CycleTakeEndTime();

    ProcessWait();
    return 0;
}

void JackRestarterDriver::SetRestartDriver(JackDriver* driver)
{
    fRestartDriver = driver;
}

int JackRestarterDriver::RestartWait()
{
    if (!fRestartDriver) {
        jack_error("JackRestartedDriver::RestartWait driver not set");
        return -1;
    }
    return fRestartDriver->Start();
}

} // end of namespace
