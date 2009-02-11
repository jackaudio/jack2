/*
Copyright (C) 2008 Grame & RTL

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

#ifndef __JackEngineProfiling__
#define __JackEngineProfiling__

#include "types.h"
#include "JackTypes.h"
#include "JackConstants.h"
#include "JackShmMem.h"

namespace Jack
{

#define TIME_POINTS 250000
#define FAILURE_TIME_POINTS 10000
#define FAILURE_WINDOW 10

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
    jack_time_t fPeriodUsecs;
    jack_time_t fCurCycleBegin;
    jack_time_t fPrevCycleEnd;
    JackTimingMeasureClient fClientTable[CLIENT_NUM];
};

/*!
\brief Client timing monitoring.
*/

class JackClientInterface;
class JackGraphManager;

class SERVER_EXPORT JackEngineProfiling
{

    private:
    
        JackTimingMeasure fProfileTable[TIME_POINTS];
        char fNameTable[CLIENT_NUM][JACK_CLIENT_NAME_SIZE + 1];
        unsigned int fAudioCycle;
  
    public:
    
        JackEngineProfiling();
        ~JackEngineProfiling();
   
        void Profile(JackClientInterface** table, 
                    JackGraphManager* manager, 
                    jack_time_t period_usecs,
                    jack_time_t cur_cycle_begin, 
                    jack_time_t prev_cycle_end);
                    
        JackTimingMeasure* GetCurMeasure();

};

} // end of namespace

#endif
