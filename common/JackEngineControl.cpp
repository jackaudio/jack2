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

#include "JackEngineControl.h"
#include "JackGraphManager.h"
#include "JackClientControl.h"
#include <algorithm>
#include <math.h>

namespace Jack
{

void JackEngineControl::CycleIncTime(jack_time_t callback_usecs)
{
    // Timer
    fFrameTimer.IncFrameTime(fBufferSize, callback_usecs, fPeriodUsecs);
}

void JackEngineControl::CycleBegin(JackClientInterface** table, JackGraphManager* manager, jack_time_t callback_usecs)
{
    // Transport
    fTransport.CycleBegin(fSampleRate, callback_usecs);

    // Timing
    GetTimeMeasure(table, manager, callback_usecs);
    CalcCPULoad(table, manager);
}

void JackEngineControl::CycleEnd(JackClientInterface** table)
{
    fTransport.CycleEnd(table, fSampleRate, fBufferSize);
}

void JackEngineControl::InitFrameTime()
{
    fFrameTimer.InitFrameTime();
}

void JackEngineControl::ResetFrameTime(jack_time_t callback_usecs)
{
    fFrameTimer.ResetFrameTime(fSampleRate, callback_usecs, fPeriodUsecs);
}

void JackEngineControl::ReadFrameTime(JackTimer* timer)
{
    fFrameTimer.ReadFrameTime(timer);
}

// Private
#ifdef WIN32
inline jack_time_t MAX(jack_time_t a, jack_time_t b)
{
    return (a < b) ? b : a;
}
#else
#define MAX(a,b) std::max((a),(b))
#endif

void JackEngineControl::CalcCPULoad(JackClientInterface** table, JackGraphManager* manager)
{
    jack_time_t lastCycleEnd = fLastProcessTime;

    for (int i = REAL_REFNUM; i < CLIENT_NUM; i++) {
        JackClientInterface* client = table[i];
        JackClientTiming* timing = manager->GetClientTiming(i);
        if (client && client->GetClientControl()->fActive && timing->fStatus == Finished) {
            lastCycleEnd = MAX(lastCycleEnd, timing->fFinishedAt);
        }
    }

    /* store the execution time for later averaging */
    fRollingClientUsecs[fRollingClientUsecsIndex++] = lastCycleEnd - fLastTime;

    if (fRollingClientUsecsIndex >= JACK_ENGINE_ROLLING_COUNT) {
        fRollingClientUsecsIndex = 0;
    }

    /* every so often, recompute the current maximum use over the
       last JACK_ENGINE_ROLLING_COUNT client iterations.
    */

    if (++fRollingClientUsecsCnt % fRollingInterval == 0) {

        jack_time_t maxUsecs = 0;
        for (int i = 0; i < JACK_ENGINE_ROLLING_COUNT; i++) {
            maxUsecs = MAX(fRollingClientUsecs[i], maxUsecs);
        }

        fMaxUsecs = MAX(fMaxUsecs, maxUsecs);
        fSpareUsecs = jack_time_t((maxUsecs < fPeriodUsecs) ? fPeriodUsecs - maxUsecs : 0);
        fCPULoad = ((1.f - (float(fSpareUsecs) / float(fPeriodUsecs))) * 50.f + (fCPULoad * 0.5f));
    }
}

void JackEngineControl::ResetRollingUsecs()
{
    memset(fRollingClientUsecs, 0, sizeof(fRollingClientUsecs));
    fRollingClientUsecsIndex = 0;
    fRollingClientUsecsCnt = 0;
    fSpareUsecs = 0;
    fRollingInterval = (int)floor((JACK_ENGINE_ROLLING_INTERVAL * 1000.f) / fPeriodUsecs);
}

void JackEngineControl::GetTimeMeasure(JackClientInterface** table, JackGraphManager* manager, jack_time_t callback_usecs)
{
    int pos = (++fAudioCycle) % TIME_POINTS;

    fLastTime = fCurTime;
    fCurTime = callback_usecs;

    fLastProcessTime = fProcessTime;
    fProcessTime = GetMicroSeconds();

    if (fLastTime > 0) {
        fMeasure[pos].fEngineTime = fLastTime;
        fMeasure[pos].fAudioCycle = fAudioCycle;

        for (int i = 0; i < CLIENT_NUM; i++) {
            JackClientInterface* client = table[i];
            JackClientTiming* timing = manager->GetClientTiming(i);
            if (client && client->GetClientControl()->fActive) {
                fMeasure[pos].fClientTable[i].fRefNum = i;
                fMeasure[pos].fClientTable[i].fSignaledAt = timing->fSignaledAt;
                fMeasure[pos].fClientTable[i].fAwakeAt = timing->fAwakeAt;
                fMeasure[pos].fClientTable[i].fFinishedAt = timing->fFinishedAt;
                fMeasure[pos].fClientTable[i].fStatus = timing->fStatus;
            }
        }
    }
}

void JackEngineControl::ClearTimeMeasures()
{
    for (int i = 0; i < TIME_POINTS; i++) {
        for (int j = 0; j < CLIENT_NUM; j++) {
            fMeasure[i].fClientTable[j].fRefNum = 0;
            fMeasure[i].fClientTable[j].fSignaledAt = 0;
            fMeasure[i].fClientTable[j].fAwakeAt = 0;
            fMeasure[i].fClientTable[j].fFinishedAt = 0;
        }
    }
    fLastTime = fCurTime = 0;
}
    
void JackEngineControl::NotifyXRun(float delayed_usecs)
{
    fXrunDelayedUsecs = delayed_usecs;
    if (delayed_usecs > fMaxDelayedUsecs)
        fMaxDelayedUsecs = delayed_usecs;
}
    
void JackEngineControl::ResetXRun()
{
    fMaxDelayedUsecs = 0.f;
}

} // end of namespace
