/*
Copyright (C) 2004-2006 Grame  

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

#include "JackEngineTiming.h"
#include "JackClientInterface.h"
#include "JackEngineControl.h"
#include "JackClientControl.h"
#include <math.h>
#include <algorithm>
#include <iostream> 
//#include <fstream>
#include <assert.h>

namespace Jack
{

inline jack_time_t MAX(jack_time_t a, jack_time_t b)
{
    return (a < b) ? b : a;
}

JackEngineTiming::JackEngineTiming(JackClientInterface** table, JackGraphManager* manager, JackEngineControl* control)
{
    fClientTable = table;
    fGraphManager = manager;
    fEngineControl = control;
    fLastTime = 0;
    fCurTime = 0;
    fProcessTime = 0;
    fLastProcessTime = 0;
    fSpareUsecs = 0;
    fMaxUsecs = 0;
    fAudioCycle = 0;
}

void JackEngineTiming::UpdateTiming(jack_time_t callback_usecs)
{
    GetTimeMeasure(callback_usecs);
    CalcCPULoad();
}

void JackEngineTiming::CalcCPULoad()
{
    jack_time_t lastCycleEnd = fLastProcessTime;

    for (int i = REAL_REFNUM; i < CLIENT_NUM; i++) {
        JackClientInterface* client = fClientTable[i];
		JackClientTiming* timing = fGraphManager->GetClientTiming(i);
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
        fSpareUsecs = jack_time_t((maxUsecs < fEngineControl->fPeriodUsecs) ? fEngineControl->fPeriodUsecs - maxUsecs : 0);
        fEngineControl->fCPULoad
        = ((1.0f - (float(fSpareUsecs) / float(fEngineControl->fPeriodUsecs))) * 50.0f + (fEngineControl->fCPULoad * 0.5f));
    }
}

void JackEngineTiming::ResetRollingUsecs()
{
    memset(fRollingClientUsecs, 0, sizeof(fRollingClientUsecs));
    fRollingClientUsecsIndex = 0;
    fRollingClientUsecsCnt = 0;
    fSpareUsecs = 0;
    fRollingInterval = (int)floor((JACK_ENGINE_ROLLING_INTERVAL * 1000.0f) / fEngineControl->fPeriodUsecs);
}

void JackEngineTiming::GetTimeMeasure(jack_time_t callbackUsecs)
{
    int pos = (++fAudioCycle) % TIME_POINTS;

    fLastTime = fCurTime;
    fCurTime = callbackUsecs;

    fLastProcessTime = fProcessTime;
    fProcessTime = GetMicroSeconds();

    if (fLastTime > 0) {
        fMeasure[pos].fEngineTime = fLastTime;
        fMeasure[pos].fAudioCycle = fAudioCycle;

        for (int i = 0; i < CLIENT_NUM; i++) {
            JackClientInterface* client = fClientTable[i];
			JackClientTiming* timing = fGraphManager->GetClientTiming(i);
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

void JackEngineTiming::PrintState()
{
    jack_time_t prevtime, time;
    prevtime = time = fMeasure[0].fEngineTime;
    int cycle, prevcycle = fMeasure[0].fAudioCycle;
    /*

       std::ofstream f("measure.txt");

       if (f.is_open()) {

           //std::cout << "---------------------------------------------" << std::endl;

           for (int i = 0; i < CLIENT_NUM; i++) { // client
               JackClientInterface* client = fClientTable[i];
    		if (client && client->GetClientControl()->fActive) {
                   // f << "Client : " << i << std::endl;
                   long maxsignalledat = 0;
                   long maxawakedat = 0;
                   long maxfinisheddat = 0;
                   bool max = false;
                   prevtime = fMeasure[0].fEngineTime;
                   prevcycle = fMeasure[0].fAudioCycle;

    			// TODO
    			
                   for (int j = 0; j < TIME_POINTS; j++) { // measure
                       time = fMeasure[j].fEngineTime;
                       cycle = fMeasure[j].fAudioCycle;

                       if (fMeasure[j].fClientTable[i].fRefNum > 0) {

                           if (fMeasure[j].fClientTable[i].fStatus != Finished) {
                               f << "error status " << '\t'
                               << prevtime << '\t'
                               << time << '\t'
                               << fMeasure[j + 1].fEngineTime << '\t'
                               << prevcycle << '\t'
                               << cycle << '\t'
                               << fMeasure[j].fClientTable[i].fSignaledAt << '\t'
                               << fMeasure[j].fClientTable[i].fAwakeAt << '\t'
                               << fMeasure[j].fClientTable[i].fFinishedAt
                               << std::endl;
                           }

                           if (long(time - prevtime) > 0) {

                               f << long(time - prevtime) << '\t'
                               << fMeasure[j].fClientTable[i].fSignaledAt - time << '\t'
                               << fMeasure[j].fClientTable[i].fAwakeAt - time << '\t'
                               << fMeasure[j].fClientTable[i].fFinishedAt - time << '\t'
                               << fMeasure[j].fClientTable[i].fStatus
                               << std::endl;
                           } else {
                               f << "error time : " << j << " " << long(time - prevtime) << std::endl;
                           }

                           maxsignalledat = MAX(maxsignalledat, long(fMeasure[j].fClientTable[i].fSignaledAt - time));
                           maxawakedat = MAX(maxawakedat, long(fMeasure[j].fClientTable[i].fAwakeAt - time));
                           maxfinisheddat = MAX(maxfinisheddat, long(fMeasure[j].fClientTable[i].fFinishedAt - time));
                           max = true;
                       }
                       prevtime = time;
                       prevcycle = cycle;
                   }
    			

                   f << std::endl;
                   if (max) {
                       f << "maxsignalledat: " << maxsignalledat
                       << '\t' << "maxawakedat: " << maxawakedat
                       << '\t' << "maxfinisheddat: " << maxfinisheddat
                       << '\t' << std::endl;
                   }
               }
           }

           f.close();
       }
    */
}

void JackEngineTiming::ClearTimeMeasures()
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


} // end of namespace

