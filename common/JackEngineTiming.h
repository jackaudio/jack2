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

#ifndef __JackEngineTiming__
#define __JackEngineTiming__

#include "types.h"
#include "JackGraphManager.h"

namespace Jack
{

#define TIME_POINTS 1000
#define JACK_ENGINE_ROLLING_COUNT 32
#define JACK_ENGINE_ROLLING_INTERVAL 1024

class JackClientInterface;
struct JackEngineControl;

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
    long fAudioCycle;
    jack_time_t fEngineTime;
    JackTimingMeasureClient fClientTable[CLIENT_NUM];
};

/*!
\brief Engine timing management.
*/

class JackEngineTiming
{
    private:

        JackClientInterface** fClientTable;
        JackGraphManager* fGraphManager;
        JackEngineControl* fEngineControl;

        JackTimingMeasure fMeasure[TIME_POINTS];
        jack_time_t fLastTime;
        jack_time_t fCurTime;
        jack_time_t fProcessTime;
        jack_time_t fLastProcessTime;
        jack_time_t fSpareUsecs;
        jack_time_t fMaxUsecs;
        uint32_t fAudioCycle;

        jack_time_t fRollingClientUsecs[JACK_ENGINE_ROLLING_COUNT];
        int	fRollingClientUsecsCnt;
        int	fRollingClientUsecsIndex;
        int	fRollingInterval;

        void CalcCPULoad();
        void GetTimeMeasure(jack_time_t callback_usecs);

    public:

        JackEngineTiming(JackClientInterface** table, JackGraphManager* manager, JackEngineControl* control);
        virtual ~JackEngineTiming()
        {}

        void UpdateTiming(jack_time_t callback_usecs);
        void ResetRollingUsecs();

        void ClearTimeMeasures();
};

} // end of namespace

#endif

