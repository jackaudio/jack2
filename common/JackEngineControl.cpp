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

#include "JackClientInterface.h"
#include "JackEngineControl.h"
#include "JackGraphManager.h"
#include "JackClientControl.h"
#include <algorithm>
#include <math.h>

namespace Jack
{

static inline jack_time_t JACK_MAX(jack_time_t a, jack_time_t b)
{
    return (a < b) ? b : a;
}

void JackEngineControl::CalcCPULoad(JackClientInterface** table,
                                    JackGraphManager* manager,
                                    jack_time_t cur_cycle_begin,
                                    jack_time_t prev_cycle_end)
{
    fPrevCycleTime = fCurCycleTime;
    fCurCycleTime = cur_cycle_begin;
    jack_time_t last_cycle_end = prev_cycle_end;

    // In Asynchronous mode, last cycle end is the max of client end dates
    if (!fSyncMode) {
        for (int i = fDriverNum; i < CLIENT_NUM; i++) {
            JackClientInterface* client = table[i];
            JackClientTiming* timing = manager->GetClientTiming(i);
            if (client && client->GetClientControl()->fActive && timing->fStatus == Finished)
                last_cycle_end = JACK_MAX(last_cycle_end, timing->fFinishedAt);
        }
    }

    // Store the execution time for later averaging
    fRollingClientUsecs[fRollingClientUsecsIndex++] = last_cycle_end - fPrevCycleTime;
    if (fRollingClientUsecsIndex >= JACK_ENGINE_ROLLING_COUNT)
        fRollingClientUsecsIndex = 0;

    // Every so often, recompute the current maximum use over the
    // last JACK_ENGINE_ROLLING_COUNT client iterations.

    if (++fRollingClientUsecsCnt % fRollingInterval == 0) {

        jack_time_t max_usecs = 0;
        for (int i = 0; i < JACK_ENGINE_ROLLING_COUNT; i++)
            max_usecs = JACK_MAX(fRollingClientUsecs[i], max_usecs);

        fMaxUsecs = JACK_MAX(fMaxUsecs, max_usecs);
        fSpareUsecs = jack_time_t((max_usecs < fPeriodUsecs) ? fPeriodUsecs - max_usecs : 0);
        fCPULoad = ((1.f - (float(fSpareUsecs) / float(fPeriodUsecs))) * 50.f + (fCPULoad * 0.5f));
    }
}

void JackEngineControl::ResetRollingUsecs()
{
    memset(fRollingClientUsecs, 0, sizeof(fRollingClientUsecs));
    fRollingClientUsecsIndex = 0;
    fRollingClientUsecsCnt = 0;
    fSpareUsecs = 0;
    fRollingInterval = int(floor((JACK_ENGINE_ROLLING_INTERVAL * 1000.f) / fPeriodUsecs));
}

void JackEngineControl::NotifyXRun(jack_time_t callback_usecs, float delayed_usecs)
{
    ResetFrameTime(callback_usecs);
    fXrunDelayedUsecs = delayed_usecs;
    if (delayed_usecs > fMaxDelayedUsecs)
        fMaxDelayedUsecs = delayed_usecs;
}

} // end of namespace
