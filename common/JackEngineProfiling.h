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

#define TIME_POINTS 100000
#define FAILURE_TIME_POINTS 10000
#define FAILURE_WINDOW 10
#define MEASURED_CLIENTS 32

/*!
\brief Timing stucture for a client.
*/

PRE_PACKED_STRUCTURE
struct JackTimingMeasureClient
{
    int fRefNum;
    jack_time_t	fSignaledAt;
    jack_time_t	fAwakeAt;
    jack_time_t	fFinishedAt;
    jack_client_state_t fStatus;
    
    JackTimingMeasureClient() 
        :fRefNum(-1),
        fSignaledAt(0),
        fAwakeAt(0),
        fFinishedAt(0),
        fStatus((jack_client_state_t)0)
    {}
    
} POST_PACKED_STRUCTURE;

/*!
\brief Timing interval in the global table for a given client
*/

PRE_PACKED_STRUCTURE
struct JackTimingClientInterval
{
    int fRefNum;
    char fName[JACK_CLIENT_NAME_SIZE + 1];
    int fBeginInterval;
    int fEndInterval;
    
    JackTimingClientInterval()
         :fRefNum(-1),
         fBeginInterval(-1),
         fEndInterval(-1)
    {}
    
} POST_PACKED_STRUCTURE;

/*!
\brief Timing stucture for a table of clients.
*/

PRE_PACKED_STRUCTURE
struct JackTimingMeasure
{
    unsigned int fAudioCycle;
    jack_time_t fPeriodUsecs;
    jack_time_t fCurCycleBegin;
    jack_time_t fPrevCycleEnd;
    JackTimingMeasureClient fClientTable[CLIENT_NUM];
    
    JackTimingMeasure()
        :fAudioCycle(0), 
        fPeriodUsecs(0),
        fCurCycleBegin(0),
        fPrevCycleEnd(0)
    {}
    
} POST_PACKED_STRUCTURE;

/*!
\brief Client timing monitoring.
*/

class JackClientInterface;
class JackGraphManager;

PRE_PACKED_STRUCTURE
class SERVER_EXPORT JackEngineProfiling
{

    private:
    
        JackTimingMeasure fProfileTable[TIME_POINTS];
        JackTimingClientInterval fIntervalTable[MEASURED_CLIENTS];
         
        unsigned int fAudioCycle;
        unsigned int fMeasuredClient;
        
        bool CheckClient(const char* name, int cur_point);
        
    public:
    
        JackEngineProfiling();
        ~JackEngineProfiling();
   
        void Profile(JackClientInterface** table, 
                    JackGraphManager* manager, 
                    jack_time_t period_usecs,
                    jack_time_t cur_cycle_begin, 
                    jack_time_t prev_cycle_end);
                    
        JackTimingMeasure* GetCurMeasure();

} POST_PACKED_STRUCTURE;

} // end of namespace

#endif
