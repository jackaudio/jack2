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

int JackTimedDriver::FirstCycle(jack_time_t cur_time)
{
    fAnchorTime = cur_time;
    return int((double(fEngineControl->fBufferSize) * 1000000) / double(fEngineControl->fSampleRate));
}
 
int JackTimedDriver::CurrentCycle(jack_time_t cur_time)
{
    return int((double(fCycleCount) * double(fEngineControl->fBufferSize) * 1000000.) / double(fEngineControl->fSampleRate)) - (cur_time - fAnchorTime);
}

int JackTimedDriver::ProcessAux()
{
    jack_time_t cur_time = GetMicroSeconds();
    int wait_time;
    
    if (fCycleCount++ == 0) {
        wait_time = FirstCycle(cur_time);
    } else {
        wait_time = CurrentCycle(cur_time);
    }
    
    if (wait_time < 0) {
        NotifyXRun(cur_time, float(cur_time -fBeginDateUst));
        fCycleCount = 0;
        wait_time = 0;
    }
    
    //jack_log("JackTimedDriver::Process wait_time = %d", wait_time);
    JackSleep(wait_time);
    return 0;
}

int JackTimedDriver::Process()
{
    JackDriver::CycleTakeBeginTime();
    JackAudioDriver::Process();
     
    return ProcessAux();
}

int JackTimedDriver::ProcessNull()
{
    JackDriver::CycleTakeBeginTime();
    
    if (fEngineControl->fSyncMode) {
        ProcessGraphSyncMaster();
    } else {
        ProcessGraphAsyncMaster();
    }
     
    return ProcessAux();
}

} // end of namespace
