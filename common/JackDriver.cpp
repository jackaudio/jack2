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

#include "JackSystemDeps.h"
#include "JackDriver.h"
#include "JackTime.h"
#include "JackError.h"
#include "JackPort.h"
#include "JackGraphManager.h"
#include "JackGlobals.h"
#include "JackEngineControl.h"
#include "JackClientControl.h"
#include "JackLockedEngine.h"
#include <math.h>
#include <assert.h>

using namespace std;

namespace Jack
{

JackDriver::JackDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table)
    :fClientControl(name)
{
    assert(strlen(name) < JACK_CLIENT_NAME_SIZE);
    fSynchroTable = table;
    strcpy(fAliasName, alias);
    fEngine = engine;
    fGraphManager = NULL;
    fBeginDateUst = 0;
    fDelayedUsecs = 0.f;
    fIsMaster = true;
    fIsRunning = false;
 }

JackDriver::JackDriver()
{
    fSynchroTable = NULL;
    fEngine = NULL;
    fGraphManager = NULL;
    fBeginDateUst = 0;
    fIsMaster = true;
    fIsRunning = false;
}

JackDriver::~JackDriver()
{
    jack_log("~JackDriver");
}

int JackDriver::Open()
{
    int refnum = -1;

    if (fEngine->ClientInternalOpen(fClientControl.fName, &refnum, &fEngineControl, &fGraphManager, this, false) != 0) {
        jack_error("Cannot allocate internal client for driver");
        return -1;
    }

    fClientControl.fRefNum = refnum;
    fClientControl.fActive = true;
    fEngineControl->fDriverNum++;
    fGraphManager->DirectConnect(fClientControl.fRefNum, fClientControl.fRefNum); // Connect driver to itself for "sync" mode
    SetupDriverSync(fClientControl.fRefNum, false);
    return 0;
}

int JackDriver::Open(bool capturing,
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
    char name_res[JACK_CLIENT_NAME_SIZE + 1];
    int status;

    // Check name and possibly rename
    if (fEngine->ClientCheck(fClientControl.fName, -1, name_res, JACK_PROTOCOL_VERSION, (int)JackNullOption, (int*)&status) < 0) {
        jack_error("Client name = %s conflits with another running client", fClientControl.fName);
        return -1;
    }
    strcpy(fClientControl.fName, name_res);

    if (fEngine->ClientInternalOpen(fClientControl.fName, &refnum, &fEngineControl, &fGraphManager, this, false) != 0) {
        jack_error("Cannot allocate internal client for driver");
        return -1;
    }

    fClientControl.fRefNum = refnum;
    fClientControl.fActive = true;
    fEngineControl->fDriverNum++;
    fCaptureLatency = capture_latency;
    fPlaybackLatency = playback_latency;

    assert(strlen(capture_driver_name) < JACK_CLIENT_NAME_SIZE);
    assert(strlen(playback_driver_name) < JACK_CLIENT_NAME_SIZE);

    strcpy(fCaptureDriverName, capture_driver_name);
    strcpy(fPlaybackDriverName, playback_driver_name);

    fEngineControl->fPeriodUsecs = jack_time_t(1000000.f / fEngineControl->fSampleRate * fEngineControl->fBufferSize); // in microsec
    if (!fEngineControl->fTimeOut)
        fEngineControl->fTimeOutUsecs = jack_time_t(2.f * fEngineControl->fPeriodUsecs);

    fGraphManager->DirectConnect(fClientControl.fRefNum, fClientControl.fRefNum); // Connect driver to itself for "sync" mode
    SetupDriverSync(fClientControl.fRefNum, false);
    return 0;
}

int JackDriver::Open(jack_nframes_t buffer_size,
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
    char name_res[JACK_CLIENT_NAME_SIZE + 1];
    int status;

    // Check name and possibly rename
    if (fEngine->ClientCheck(fClientControl.fName, -1, name_res, JACK_PROTOCOL_VERSION, (int)JackNullOption, (int*)&status) < 0) {
        jack_error("Client name = %s conflits with another running client", fClientControl.fName);
        return -1;
    }
    strcpy(fClientControl.fName, name_res);

    if (fEngine->ClientInternalOpen(fClientControl.fName, &refnum, &fEngineControl, &fGraphManager, this, false) != 0) {
        jack_error("Cannot allocate internal client for driver");
        return -1;
    }

    fClientControl.fRefNum = refnum;
    fClientControl.fActive = true;
    fEngineControl->fDriverNum++;
    fEngineControl->fBufferSize = buffer_size;
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

    fGraphManager->SetBufferSize(buffer_size);
    fGraphManager->DirectConnect(fClientControl.fRefNum, fClientControl.fRefNum); // Connect driver to itself for "sync" mode
    SetupDriverSync(fClientControl.fRefNum, false);
    return 0;
}

int JackDriver::Close()
{
    if (fClientControl.fRefNum >= 0) {
        jack_log("JackDriver::Close");
        fGraphManager->DirectDisconnect(fClientControl.fRefNum, fClientControl.fRefNum); // Disconnect driver from itself for sync
        fClientControl.fActive = false;
        fEngineControl->fDriverNum--;
        return fEngine->ClientInternalClose(fClientControl.fRefNum, false);
    } else {
        return -1;
    }
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
        fSynchroTable[ref].SetFlush(true);
    } else {
        jack_log("JackDriver::SetupDriverSync driver sem in normal mode");
        fSynchroTable[ref].SetFlush(false);
    }
}

int JackDriver::ClientNotify(int refnum, const char* name, int notify, int sync, const char* message, int value1, int value2)
{
    switch (notify) {

        case kStartFreewheelCallback:
            jack_log("JackDriver::kStartFreewheel");
            SetupDriverSync(fClientControl.fRefNum, true);
            break;

        case kStopFreewheelCallback:
            jack_log("JackDriver::kStopFreewheel");
            SetupDriverSync(fClientControl.fRefNum, false);
            break;
   }

    return 0;
}

bool JackDriver::IsRealTime() const
{
    return fEngineControl->fRealTime;
}

void JackDriver::CycleIncTime()
{
    fEngineControl->CycleIncTime(fBeginDateUst);
}

void JackDriver::CycleTakeBeginTime()
{
    fBeginDateUst = GetMicroSeconds();  // Take callback date here
    fEngineControl->CycleIncTime(fBeginDateUst);
}

void JackDriver::CycleTakeEndTime()
{
    fEndDateUst = GetMicroSeconds();    // Take end date here
}

JackClientControl* JackDriver::GetClientControl() const
{
    return (JackClientControl*)&fClientControl;
}

void JackDriver::NotifyXRun(jack_time_t cur_cycle_begin, float delayed_usecs)
{
    fEngine->NotifyXRun(cur_cycle_begin, delayed_usecs);
}

void JackDriver::NotifyBufferSize(jack_nframes_t buffer_size)
{
    fEngine->NotifyBufferSize(buffer_size);
    fEngineControl->InitFrameTime();
}

void JackDriver::NotifySampleRate(jack_nframes_t sample_rate)
{
    fEngine->NotifySampleRate(sample_rate);
    fEngineControl->InitFrameTime();
}

void JackDriver::NotifyFailure(int code, const char* reason)
{
    fEngine->NotifyFailure(code, reason);
}

void JackDriver::SetMaster(bool onoff)
{
    fIsMaster = onoff;
}

bool JackDriver::GetMaster()
{
    return fIsMaster;
}

void JackDriver::AddSlave(JackDriverInterface* slave)
{
    fSlaveList.push_back(slave);
}

void JackDriver::RemoveSlave(JackDriverInterface* slave)
{
    fSlaveList.remove(slave);
}

int JackDriver::ProcessSlaves()
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

int JackDriver::Process()
{
    return 0;
}

int JackDriver::ProcessNull()
{
    return 0;
}

int JackDriver::Attach()
{
    return 0;
}

int JackDriver::Detach()
{
    return 0;
}

int JackDriver::Read()
{
    return 0;
}

int JackDriver::Write()
{
    return 0;
}

int JackDriver::Start()
{
    if (fIsMaster) {
        fEngineControl->InitFrameTime();
    }
    fIsRunning = true;
    return 0;
}

int JackDriver::StartSlaves()
{
    int res = 0;
    list<JackDriverInterface*>::const_iterator it;
    for (it = fSlaveList.begin(); it != fSlaveList.end(); it++) {
        JackDriverInterface* slave = *it;
        if (slave->Start() < 0) {
            res = -1;

            // XXX: We should attempt to stop all of the slaves that we've
            // started here.

            break;
        }
    }
    return res;
}

int JackDriver::Stop()
{
    fIsRunning = false;
    return 0;
}

int JackDriver::StopSlaves()
{
    int res = 0;
    list<JackDriverInterface*>::const_iterator it;
    for (it = fSlaveList.begin(); it != fSlaveList.end(); it++) {
        JackDriverInterface* slave = *it;
        if (slave->Stop() < 0)
            res = -1;
    }
    return res;
}

bool JackDriver::IsFixedBufferSize()
{
    return true;
}

int JackDriver::SetBufferSize(jack_nframes_t buffer_size)
{
    return 0;
}

int JackDriver::SetSampleRate(jack_nframes_t sample_rate)
{
    return 0;
}

bool JackDriver::Initialize()
{
    return true;
}


} // end of namespace
