/*
Copyright (C) 2001 Paul Davis
Copyright (C) 2004-2008 Grame

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

#ifdef WIN32
#pragma warning (disable : 4786)
#endif

#include "JackDriver.h"
#include "JackTime.h"
#include "JackError.h"
#include "JackPort.h"
#include "JackGraphManager.h"
#include "JackGlobals.h"
#include "JackEngineControl.h"
#include "JackClientControl.h"
#include "JackEngine.h"
#include <math.h>
#include <assert.h>

using namespace std;

namespace Jack
{

JackDriver::JackDriver(const char* name, const char* alias, JackEngineInterface* engine, JackSynchro** table)
{
    assert(strlen(name) < JACK_CLIENT_NAME_SIZE);
    fSynchroTable = table;
    fClientControl = new JackClientControl(name);
    strcpy(fAliasName, alias);
    fEngine = engine;
    fGraphManager = NULL;
    fLastWaitUst = 0;
    fDelayedUsecs = 0.f;
    fIsMaster = true;
}

JackDriver::JackDriver()
{
    fSynchroTable = NULL;
    fClientControl = NULL;
    fEngine = NULL;
    fGraphManager = NULL;
    fLastWaitUst = 0;
    fIsMaster = true;
}

JackDriver::~JackDriver()
{
    jack_log("~JackDriver");
    delete fClientControl;
}

int JackDriver::Open()
{
    int refnum = -1;

    if (fEngine->ClientInternalOpen(fClientControl->fName, &refnum, &fEngineControl, &fGraphManager, this, false) != 0) {
        jack_error("Cannot allocate internal client for audio driver");
        return -1;
    }

    fClientControl->fRefNum = refnum;
    fClientControl->fActive = true;
    fGraphManager->DirectConnect(fClientControl->fRefNum, fClientControl->fRefNum); // Connect driver to itself for "sync" mode
    SetupDriverSync(fClientControl->fRefNum, false);
    return 0;
}

int JackDriver::Open(jack_nframes_t nframes,
                     jack_nframes_t samplerate,
                     bool capturing,
                     bool playing,
                     int inchannels,
                     int outchannels,
                     bool monitor,
                     const char* capture_driver_name,
                     const char* playback_driver_name,
                     jack_nframes_t capture_latency,
                     jack_nframes_t playback_latency)
{
    jack_log("JackDriver::Open capture_driver_name = %s", capture_driver_name);
    jack_log("JackDriver::Open playback_driver_name = %s", playback_driver_name);
    int refnum = -1;

    if (fEngine->ClientInternalOpen(fClientControl->fName, &refnum, &fEngineControl, &fGraphManager, this, false) != 0) {
        jack_error("Cannot allocate internal client for audio driver");
        return -1;
    }

    fClientControl->fRefNum = refnum;
    fClientControl->fActive = true;
    fEngineControl->fBufferSize = nframes;
    fEngineControl->fSampleRate = samplerate;
    fCaptureLatency = capture_latency;
    fPlaybackLatency = playback_latency;

    assert(strlen(capture_driver_name) < JACK_CLIENT_NAME_SIZE);
    assert(strlen(playback_driver_name) < JACK_CLIENT_NAME_SIZE);

    strcpy(fCaptureDriverName, capture_driver_name);
    strcpy(fPlaybackDriverName, playback_driver_name);

    fEngineControl->fPeriodUsecs = jack_time_t(1000000.f / fEngineControl->fSampleRate * fEngineControl->fBufferSize); // in microsec
    if (!fEngineControl->fTimeOut)
        fEngineControl->fTimeOutUsecs = jack_time_t(2.f * fEngineControl->fPeriodUsecs);

    fGraphManager->SetBufferSize(nframes);
    fGraphManager->DirectConnect(fClientControl->fRefNum, fClientControl->fRefNum); // Connect driver to itself for "sync" mode
    SetupDriverSync(fClientControl->fRefNum, false);
    return 0;
}

int JackDriver::Close()
{
    jack_log("JackDriver::Close");
    fGraphManager->DirectDisconnect(fClientControl->fRefNum, fClientControl->fRefNum); // Disconnect driver from itself for sync
    fClientControl->fActive = false;
    return fEngine->ClientInternalClose(fClientControl->fRefNum, false);
}

/*!
	In "async" mode, the server does not synchronize itself on the output drivers, thus it would never "consume" the activations.
	The synchronization primitives for drivers are setup in "flush" mode that to not keep unneeded activations.
	Drivers synchro are setup in "flush" mode if server is "async" and NOT freewheel.
*/
void JackDriver::SetupDriverSync(int ref, bool freewheel)
{
    if (!freewheel && !fEngineControl->fSyncMode) {
        jack_log("JackDriver::SetupDriverSync driver sem in flush mode");
        fSynchroTable[ref]->SetFlush(true);
    } else {
        jack_log("JackDriver::SetupDriverSync driver sem in normal mode");
        fSynchroTable[ref]->SetFlush(false);
    }
}

int JackDriver::ClientNotify(int refnum, const char* name, int notify, int sync, int value1, int value2)
{
    switch (notify) {

        case kStartFreewheelCallback:
            jack_log("JackDriver::kStartFreewheel");
            SetupDriverSync(fClientControl->fRefNum, true);
            break;

        case kStopFreewheelCallback:
            jack_log("JackDriver::kStopFreewheel");
            SetupDriverSync(fClientControl->fRefNum, false);
            break;
    }

    return 0;
}

bool JackDriver::IsRealTime()
{
    return fEngineControl->fRealTime;
}

JackClientControl* JackDriver::GetClientControl() const
{
    return fClientControl;
}

void JackDriver::NotifyXRun(jack_time_t callback_usecs, float delayed_usecs)
{
    fEngine->NotifyXRun(callback_usecs, delayed_usecs);
}

void JackDriverClient::SetMaster(bool onoff)
{
    fIsMaster = onoff;
}

bool JackDriverClient::GetMaster()
{
    return fIsMaster;
}

void JackDriverClient::AddSlave(JackDriverInterface* slave)
{
    fSlaveList.push_back(slave);
}

void JackDriverClient::RemoveSlave(JackDriverInterface* slave)
{
    fSlaveList.remove(slave);
}

int JackDriverClient::ProcessSlaves()
{
    int res = 0;
    list<JackDriverInterface*>::const_iterator it;
    for (it = fSlaveList.begin(); it != fSlaveList.end(); it++) {
        JackDriverInterface* slave = *it;
        if (slave->Process() < 0)
            res = -1;
    }
    return res;
}

} // end of namespace
