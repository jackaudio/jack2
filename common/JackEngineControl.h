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
#include "JackConstants.h"
#include "types.h"
#include <stdio.h>

#ifdef JACK_MONITOR
#include "JackEngineProfiling.h"
#endif

namespace Jack
{

class JackClientInterface;
class JackGraphManager;

#define JACK_ENGINE_ROLLING_COUNT 32
#define JACK_ENGINE_ROLLING_INTERVAL 1024

/*!
\brief Engine control in shared memory.
*/

PRE_PACKED_STRUCTURE
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
    bool fSavedRealTime;  // RT state saved and restored during Freewheel mode
    int fServerPriority;
    int fClientPriority;
    int fMaxClientPriority;
    char fServerName[JACK_SERVER_NAME_SIZE+1];
    alignas(UInt32) JackTransportEngine fTransport;
    jack_timer_type_t fClockSource;
    int fDriverNum;
    bool fVerbose;

    // CPU Load
    jack_time_t fPrevCycleTime;
    jack_time_t fCurCycleTime;
    jack_time_t fSpareUsecs;
    jack_time_t fMaxUsecs;
    jack_time_t fRollingClientUsecs[JACK_ENGINE_ROLLING_COUNT];
    unsigned int fRollingClientUsecsCnt;
    int	fRollingClientUsecsIndex;
    int	fRollingInterval;
    float fCPULoad;

    // For OSX thread
    UInt64 fPeriod;
    UInt64 fComputation;
    UInt64 fConstraint;

    // Timer
    alignas(UInt32) JackFrameTimer fFrameTimer;

#ifdef JACK_MONITOR
    JackEngineProfiling fProfiler;
#endif

    JackEngineControl(bool sync, bool temporary, long timeout, bool rt, long priority, bool verbose, jack_timer_type_t clock, const char* server_name)
      {
        static_assert(offsetof(JackEngineControl, fTransport) % sizeof(UInt32) == 0,
                      "fTransport must be aligned within JackEngineControl");
        static_assert(offsetof(JackEngineControl, fFrameTimer) % sizeof(UInt32) == 0,
                      "fFrameTimer must be aligned within JackEngineControl");
        fBufferSize = 512;
        fSampleRate = 48000;
        fPeriodUsecs = jack_time_t(1000000.f / fSampleRate * fBufferSize);
        fSyncMode = sync;
        fTemporary = temporary;
        fTimeOut = (timeout > 0);
        fTimeOutUsecs = timeout * 1000;
        fRealTime = rt;
        fSavedRealTime = false;
        fServerPriority = priority;

    #ifdef WIN32
        fClientPriority = (rt) ? priority - 3 : 0;
    #else
        fClientPriority = (rt) ? priority - 5 : 0;
    #endif
        fMaxClientPriority = (rt) ? priority - 1 : 0;
        fVerbose = verbose;
        fPrevCycleTime = 0;
        fCurCycleTime = 0;
        fSpareUsecs = 0;
        fMaxUsecs = 0;
        ResetRollingUsecs();
        strncpy(fServerName, server_name, sizeof(fServerName));
        fServerName[sizeof(fServerName) - 1] = 0;
        fCPULoad = 0.f;
        fPeriod = 0;
        fComputation = 0;
        fConstraint = 0;
        fMaxDelayedUsecs = 0.f;
        fXrunDelayedUsecs = 0.f;
        fClockSource = clock;
        fDriverNum = 0;
    }

    ~JackEngineControl()
    {}

    void UpdateTimeOut()
    {
        fPeriodUsecs = jack_time_t(1000000.f / fSampleRate * fBufferSize); // In microsec
        if (!(fTimeOut && fTimeOutUsecs > 2 * fPeriodUsecs)) {
            fTimeOutUsecs = 2 * fPeriodUsecs;
        }
    }

    // Cycle
    void CycleIncTime(jack_time_t callback_usecs)
    {
        // Timer
        fFrameTimer.IncFrameTime(fBufferSize, callback_usecs, fPeriodUsecs);
    }

    void CycleBegin(JackClientInterface** table, JackGraphManager* manager, jack_time_t cur_cycle_begin, jack_time_t prev_cycle_end)
    {
        fTransport.CycleBegin(fSampleRate, cur_cycle_begin);
        CalcCPULoad(table, manager, cur_cycle_begin, prev_cycle_end);
#ifdef JACK_MONITOR
        fProfiler.Profile(table, manager, fPeriodUsecs, cur_cycle_begin, prev_cycle_end);
#endif
    }

    void CycleEnd(JackClientInterface** table)
    {
        fTransport.CycleEnd(table, fSampleRate, fBufferSize);
    }

    // Timer
    void InitFrameTime()
    {
        fFrameTimer.InitFrameTime();
    }

    void ResetFrameTime(jack_time_t callback_usecs)
    {
        fFrameTimer.ResetFrameTime(callback_usecs);
    }

    void ReadFrameTime(JackTimer* timer)
    {
        fFrameTimer.ReadFrameTime(timer);
    }

    // XRun
    void NotifyXRun(jack_time_t callback_usecs, float delayed_usecs);
    void ResetXRun()
    {
        fMaxDelayedUsecs = 0.f;
    }

    // Private
    void CalcCPULoad(JackClientInterface** table, JackGraphManager* manager, jack_time_t cur_cycle_begin, jack_time_t prev_cycle_end);
    void ResetRollingUsecs();

} POST_PACKED_STRUCTURE;

} // end of namespace

#endif
