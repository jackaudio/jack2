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

#include "JackEngineTiming.h"
#include "JackGraphManager.h"
#include "JackClientControl.h"
#include "JackClientInterface.h"
#include "JackTime.h"

namespace Jack
{

void JackEngineTiming::GetTimeMeasure(JackClientInterface** table, 
                                            JackGraphManager* manager, 
                                            jack_time_t cur_cycle_begin, 
                                            jack_time_t prev_cycle_end)
{
    int pos = (++fAudioCycle) % TIME_POINTS;
    
    fPrevCycleTime = fCurCycleTime;
    fCurCycleTime = cur_cycle_begin;

    if (fPrevCycleTime > 0) {
        fMeasure[pos].fEngineTime = fPrevCycleTime;
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
    fPrevCycleTime = fCurCycleTime = 0;
}
    
} // end of namespace
