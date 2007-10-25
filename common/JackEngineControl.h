/*
Copyright (C) 2003 Paul Davis
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

#ifndef __JackEngineControl__
#define __JackEngineControl__

#include "JackShmMem.h"
#include "JackFrameTimer.h"
#include "JackTransportEngine.h"
#include "types.h"

namespace Jack
{

class JackClientInterface;
class JackGraphManager;

#define TIME_POINTS 1000
#define JACK_ENGINE_ROLLING_COUNT 32
#define JACK_ENGINE_ROLLING_INTERVAL 1024

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
\brief Engine control in shared memory.
*/

struct JackEngineControl : public JackShmMem
{
	// Shared state
    jack_nframes_t fBufferSize;
    jack_nframes_t fSampleRate;
	bool fSyncMode;
	bool fTemporary;
    jack_time_t fPeriodUsecs;
    jack_time_t fTimeOutUsecs;
	bool fRealTime;
	int32_t fPriority;
	char fServerName[64];
    JackTransportEngine fTransport;
    bool fVerbose;
	
	// Timing
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
	float fCPULoad;
	
	// Fos OSX thread
    UInt64 fPeriod;
    UInt64 fComputation;
    UInt64 fConstraint;
	
	// Timer
	JackFrameTimer fFrameTimer;

	JackEngineControl(bool sync, bool temporary, long timeout, bool rt, long priority, bool verbose, const char* server_name)
	{
		fSyncMode = sync;
		fTemporary = temporary;
		fTimeOutUsecs = timeout * 1000;
		fRealTime = rt;
		fPriority = priority;
		fVerbose = verbose;
		fLastTime = 0;
		fCurTime = 0;
		fProcessTime = 0;
		fLastProcessTime = 0;
		fSpareUsecs = 0;
		fMaxUsecs = 0;
		fAudioCycle = 0;
		ClearTimeMeasures();
		ResetRollingUsecs();
		snprintf(fServerName, sizeof(fServerName), server_name);
	}
	~JackEngineControl()
	{}
	
	// Cycle
	void CycleBegin(JackClientInterface** table, JackGraphManager* manager, jack_time_t callback_usecs);
	void CycleEnd(JackClientInterface** table);
	
	// Timer
	void InitFrameTime();
	void ResetFrameTime(jack_time_t callback_usecs);
	void ReadFrameTime(JackTimer* timer);

	// Private
	void CalcCPULoad(JackClientInterface** table, JackGraphManager* manager);
	void GetTimeMeasure(JackClientInterface** table, JackGraphManager* manager, jack_time_t callback_usecs);
	void ClearTimeMeasures();
	void ResetRollingUsecs();
	
};

} // end of namespace

#endif
