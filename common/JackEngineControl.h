/*
Copyright (C) 2003 Paul Davis
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

#ifndef __JackEngineControl__
#define __JackEngineControl__

#include "JackShmMem.h"
#include "JackFrameTimer.h"
#include "JackTransportEngine.h"
#include "types.h"
#include <stdio.h>

namespace Jack
{

class JackClientInterface;
class JackGraphManager;

#define JACK_ENGINE_ROLLING_COUNT 32
#define JACK_ENGINE_ROLLING_INTERVAL 1024

/*!
\brief Engine control in shared memory.
*/

struct SERVER_EXPORT JackEngineControl : public JackShmMem
{
    // Shared state
    jack_nframes_t fBufferSize;
    jack_nframes_t fSampleRate;
    bool fSyncMode;
    bool fTemporary;
    jack_time_t fPeriodUsecs;
    jack_time_t fTimeOutUsecs;
    float fMaxDelayedUsecs;
    float fXrunDelayedUsecs;
    bool fTimeOut;
    bool fRealTime;
    int fPriority;
    char fServerName[64];
    JackTransportEngine fTransport;
    bool fVerbose;

    // CPU Load
    jack_time_t fPrevCycleTime;
    jack_time_t fCurCycleTime;
    jack_time_t fSpareUsecs;
    jack_time_t fMaxUsecs;
    jack_time_t fRollingClientUsecs[JACK_ENGINE_ROLLING_COUNT];
    int	fRollingClientUsecsCnt;
    int	fRollingClientUsecsIndex;
    int	fRollingInterval;
    float fCPULoad;

    // For OSX thread
    UInt64 fPeriod;
    UInt64 fComputation;
    UInt64 fConstraint;

    // Timer
    JackFrameTimer fFrameTimer;

    JackEngineControl(bool sync, bool temporary, long timeout, bool rt, long priority, bool verbose, const char* server_name)
    {
        fBufferSize = 512;
        fSampleRate = 48000;
        fPeriodUsecs = jack_time_t(1000000.f / fSampleRate * fBufferSize);
        fSyncMode = sync;
        fTemporary = temporary;
        fTimeOut = (timeout > 0);
        fTimeOutUsecs = timeout * 1000;
        fRealTime = rt;
        fPriority = priority;
        fVerbose = verbose;
        fPrevCycleTime = 0;
        fCurCycleTime = 0;
        fSpareUsecs = 0;
        fMaxUsecs = 0;
        ResetRollingUsecs();
        snprintf(fServerName, sizeof(fServerName), server_name);
        fPeriod = 0;
        fComputation = 0;
        fConstraint = 0;
        fMaxDelayedUsecs = 0.f;
        fXrunDelayedUsecs = 0.f;
    }
    ~JackEngineControl()
    {}

    // Cycle
    void CycleIncTime(jack_time_t callback_usecs);
    void CycleBegin(JackClientInterface** table, JackGraphManager* manager, jack_time_t cur_cycle_begin, jack_time_t prev_cycle_end);
    void CycleEnd(JackClientInterface** table);

    // Timer
    void InitFrameTime();
    void ResetFrameTime(jack_time_t callback_usecs);
    void ReadFrameTime(JackTimer* timer);
    
    // XRun
    void NotifyXRun(float delayed_usecs);
    void ResetXRun();

    // Private
    void CalcCPULoad(JackClientInterface** table, JackGraphManager* manager, jack_time_t cur_cycle_begin, jack_time_t prev_cycle_end);
    void ResetRollingUsecs();

};

} // end of namespace

#endif
