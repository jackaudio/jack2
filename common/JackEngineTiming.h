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

#ifndef __JackEngineTiming__
#define __JackEngineTiming__

#include "types.h"
#include "JackTypes.h"
#include "JackConstants.h"

namespace Jack
{

#define TIME_POINTS 1000

/*!
\brief Timing stucture for a client.
*/

struct JackTimingMeasureClient
{
    int fRefNum;
    jack_time_t	fSignaledAt;
    jack_time_t	fAwakeAt;
    jack_time_t	fFinishedAt;
    jack_client_state_t fStatus;
};

/*!
\brief Timing stucture for a table of clients.
*/

struct JackTimingMeasure
{
    unsigned int fAudioCycle;
    jack_time_t fEngineTime;
    JackTimingMeasureClient fClientTable[CLIENT_NUM];
};

/*!
\brief Client timing.
*/

class JackClientInterface;
class JackGraphManager;

class  JackEngineTiming
{

    private:
    
        JackTimingMeasure fMeasure[TIME_POINTS];
        unsigned int fAudioCycle;
        jack_time_t fPrevCycleTime;
        jack_time_t fCurCycleTime;
   
    public:
    
        JackEngineTiming():fAudioCycle(0),fPrevCycleTime(0),fCurCycleTime(0)
        {}
        ~JackEngineTiming()
        {}

        void GetTimeMeasure(JackClientInterface** table, JackGraphManager* manager, jack_time_t cur_cycle_begin, jack_time_t prev_cycle_end);
        void ClearTimeMeasures();
};

} // end of namespace

#endif
