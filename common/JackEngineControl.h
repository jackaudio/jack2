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

/*!
\brief Engine control in shared memory.
*/

struct JackEngineControl : public JackShmMem
{
    jack_nframes_t fBufferSize;
    jack_nframes_t fSampleRate;
    float fCPULoad;
	bool fSyncMode;
	bool fTemporary;
    jack_time_t fPeriodUsecs;
    jack_time_t fTimeOutUsecs;
	bool fRealTime;
	int32_t fPriority;
    UInt64 fPeriod;
    UInt64 fComputation;
    UInt64 fConstraint;
	char fServerName[64];
    JackFrameTimer fFrameTimer;
    JackTransportEngine fTransport;
    bool fVerbose;
	
	JackEngineControl(bool sync, bool temporary, long timeout, bool rt, long priority, bool verbose, const char* server_name)
					:fSyncMode(sync), 
					fTemporary(temporary),
					fTimeOutUsecs(timeout * 1000), 
					fRealTime(rt), 
					fPriority(priority), 
					fVerbose(verbose)
					
	{
		snprintf(fServerName, sizeof(fServerName), server_name);
	}
	
	void InitFrameTime()
	{
		fFrameTimer.InitFrameTime();
	}
	
	void IncFrameTime(jack_time_t callback_usecs)
	{
		fFrameTimer.IncFrameTime(fBufferSize, callback_usecs, fPeriodUsecs);
	}
	
	void ResetFrameTime(jack_time_t callback_usecs)
	{
		fFrameTimer.ResetFrameTime(fSampleRate, callback_usecs, fPeriodUsecs);
	}
	
	void ReadFrameTime(JackTimer* timer)
	{
		fFrameTimer.ReadFrameTime(timer);
	}
	
	void CycleBegin(jack_time_t callback_usecs)
	{
		fTransport.CycleBegin(fSampleRate, callback_usecs);
	}

	void CycleEnd(JackClientInterface** table)
	{
		fTransport.CycleEnd(table, fSampleRate, fBufferSize);
	}

};


} // end of namespace


#endif
