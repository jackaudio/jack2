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
#include "JackTime.h"
#include <math.h>
#include <assert.h>

using namespace std;

namespace Jack
{

JackDriver::JackDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table)
    :fCaptureChannels(0),
    fPlaybackChannels(0),
    fClientControl(name),
    fWithMonitorPorts(false){
    assert(strlen(name) < JACK_CLIENT_NAME_SIZE);
    fSynchroTable = table;
    strcpy(fAliasName, alias);
    fEngine = engine;
    fGraphManager = NULL;
    fBeginDateUst = 0;
    fEndDateUst = 0;
    fDelayedUsecs = 0.f;
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

int JackDriver::Open(jack_nframes_t buffer_size,
                     jack_nframes_t sample_rate,
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
    if (buffer_size > 0) {
        fEngineControl->fBufferSize = buffer_size;
    }
    if (sample_rate > 0) {
        fEngineControl->fSampleRate = sample_rate;
    }
    fCaptureLatency = capture_latency;
    fPlaybackLatency = playback_latency;

    assert(strlen(capture_driver_name) < JACK_CLIENT_NAME_SIZE);
    assert(strlen(playback_driver_name) < JACK_CLIENT_NAME_SIZE);

    strcpy(fCaptureDriverName, capture_driver_name);
    strcpy(fPlaybackDriverName, playback_driver_name);

    fEngineControl->UpdateTimeOut();

    fGraphManager->SetBufferSize(fEngineControl->fBufferSize);
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
    jack_log("JackDriver::ClientNotify ref = %ld driver = %s name = %s notify = %ld", refnum, fClientControl.fName, name, notify);

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
    fEngineControl->NotifyXRun(cur_cycle_begin, delayed_usecs);
    fEngine->NotifyDriverXRun();
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

int JackDriver::ProcessReadSlaves()
{
    int res = 0;
    list<JackDriverInterface*>::const_iterator it;
    for (it = fSlaveList.begin(); it != fSlaveList.end(); it++) {
        JackDriverInterface* slave = *it;
        if (slave->IsRunning()) {
            if (slave->ProcessRead() < 0) {
                res = -1;
            }
        }
    }
    return res;
}

int JackDriver::ProcessWriteSlaves()
{
    int res = 0;
    list<JackDriverInterface*>::const_iterator it;
    for (it = fSlaveList.begin(); it != fSlaveList.end(); it++) {
        JackDriverInterface* slave = *it;
        if (slave->IsRunning()) {
            if (slave->ProcessWrite() < 0) {
                res = -1;
            }
        }
    }
    return res;
}

int JackDriver::ProcessRead()
{
    return (fEngineControl->fSyncMode) ? ProcessReadSync() : ProcessReadAsync();
}

int JackDriver::ProcessWrite()
{
    return (fEngineControl->fSyncMode) ? ProcessWriteSync() : ProcessWriteAsync();
}

int JackDriver::ProcessReadSync()
{
    return 0;
}

int JackDriver::ProcessWriteSync()
{
    return 0;
}

int JackDriver::ProcessReadAsync()
{
    return 0;
}

int JackDriver::ProcessWriteAsync()
{
    return 0;
}

int JackDriver::Process()
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
    return StartSlaves();
}

int JackDriver::Stop()
{
    fIsRunning = false;
    return StopSlaves();
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

int JackDriver::StopSlaves()
{
    int res = 0;
    list<JackDriverInterface*>::const_iterator it;
    for (it = fSlaveList.begin(); it != fSlaveList.end(); it++) {
        JackDriverInterface* slave = *it;
        if (slave->Stop() < 0) {
            res = -1;
        }
    }
    return res;
}

bool JackDriver::IsFixedBufferSize()
{
    return true;
}

int JackDriver::SetBufferSize(jack_nframes_t buffer_size)
{
    int res = 0;
    list<JackDriverInterface*>::const_iterator it;
    for (it = fSlaveList.begin(); it != fSlaveList.end(); it++) {
        JackDriverInterface* slave = *it;
        if (slave->SetBufferSize(buffer_size) < 0) {
            res = -1;
        }
    }
    return res;
}

int JackDriver::SetSampleRate(jack_nframes_t sample_rate)
{
    int res = 0;
    list<JackDriverInterface*>::const_iterator it;
    for (it = fSlaveList.begin(); it != fSlaveList.end(); it++) {
        JackDriverInterface* slave = *it;
        if (slave->SetSampleRate(sample_rate) < 0) {
            res = -1;
        }
    }
    return res;
}

bool JackDriver::Initialize()
{
    return true;
}

static string RemoveLast(const string& name)
{
    return name.substr(0, name.find_last_of(':')); // Remove end of name after last ":"
}

void JackDriver::SaveConnections(int alias)
{
    const char** connections;
    char alias1[REAL_JACK_PORT_NAME_SIZE+1];
    char alias2[REAL_JACK_PORT_NAME_SIZE+1];
    char system_alias1[REAL_JACK_PORT_NAME_SIZE+1];
    char system_alias2[REAL_JACK_PORT_NAME_SIZE+1];
    char* aliases[2];
    char* system_aliases[2];

    aliases[0] = alias1;
    aliases[1] = alias2;
    
    system_aliases[0] = system_alias1;
    system_aliases[1] = system_alias2;
    
    fConnections.clear();
   
    for (int i = 0; i < fCaptureChannels; ++i) {
        if (fCapturePortList[i] && (connections = fGraphManager->GetConnections(fCapturePortList[i])) != 0) {
            if (alias == 0) {
                for (int j = 0; connections[j]; j++) {
                    JackPort* port_id = fGraphManager->GetPort(fCapturePortList[i]);
                    fConnections.push_back(make_pair(port_id->GetType(), make_pair(port_id->GetName(), connections[j])));
                    jack_info("Save connection: %s %s", fGraphManager->GetPort(fCapturePortList[i])->GetName(), connections[j]);
                }
            } else {
                int res1 = fGraphManager->GetPort(fCapturePortList[i])->GetAliases(aliases);
                string sub_system_name;
                if (res1 >= alias) {
                    sub_system_name = aliases[alias-1];
                } else {
                    sub_system_name = fGraphManager->GetPort(fCapturePortList[i])->GetName();
                }
                for (int j = 0; connections[j]; j++) {
                    JackPort* port_id = fGraphManager->GetPort(fGraphManager->GetPort(connections[j]));
                    int res2 = port_id->GetAliases(system_aliases);
                    string sub_system;
                    if (res2 >= alias) {
                        sub_system = system_aliases[alias-1];
                    } else {
                        sub_system = connections[j];
                    }
                    fConnections.push_back(make_pair(port_id->GetType(), make_pair(sub_system_name, sub_system)));
                    jack_info("Save connection: %s %s", sub_system_name.c_str(), sub_system.c_str());
               }        
            }
            free(connections);
        }
    }

    for (int i = 0; i < fPlaybackChannels; ++i) {
        if (fPlaybackPortList[i] && (connections = fGraphManager->GetConnections(fPlaybackPortList[i])) != 0) {
            if (alias == 0) {
                for (int j = 0; connections[j]; j++) {
                    JackPort* port_id = fGraphManager->GetPort(fPlaybackPortList[i]);
                    fConnections.push_back(make_pair(port_id->GetType(), make_pair(connections[j], port_id->GetName())));
                    jack_info("Save connection: %s %s", connections[j], fGraphManager->GetPort(fPlaybackPortList[i])->GetName());
                }
            } else {
                int res1 = fGraphManager->GetPort(fPlaybackPortList[i])->GetAliases(aliases);
                string sub_system_name;
                if (res1 >= alias) {
                    sub_system_name = aliases[alias-1];
                } else {
                    sub_system_name = fGraphManager->GetPort(fPlaybackPortList[i])->GetName();
                }
                for (int j = 0; connections[j]; j++) {
                    JackPort* port_id = fGraphManager->GetPort(fGraphManager->GetPort(connections[j]));
                    int res2 = port_id->GetAliases(system_aliases);
                    string sub_name;
                    if (res2 >= alias) {
                        sub_name = system_aliases[alias-1];
                    } else {
                        sub_name = connections[j];
                    }
                    fConnections.push_back(make_pair(port_id->GetType(), make_pair(sub_name, sub_system_name)));
                    jack_info("Save connection: %s %s", sub_name.c_str(), sub_system_name.c_str());
               }        
            }
            free(connections);
        }
    }
}

string JackDriver::MatchPortName(const char* name, const char** ports, int alias, const std::string& type)
{
    char alias1[REAL_JACK_PORT_NAME_SIZE+1];
    char alias2[REAL_JACK_PORT_NAME_SIZE+1];
    char* aliases[2];
  
    aliases[0] = alias1;
    aliases[1] = alias2;
  
    for (int i = 0; ports && ports[i]; ++i) {
        
        jack_port_id_t port_id2 = fGraphManager->GetPort(ports[i]);
        JackPort* port2 = (port_id2 != NO_PORT) ? fGraphManager->GetPort(port_id2) : NULL;
        
        if (port2) {
            int res = port2->GetAliases(aliases);
            string name_str;
            if (res >= alias) {
                name_str = string(aliases[alias-1]);
            } else {
                name_str = string(ports[i]);
            }
            string sub_name = RemoveLast(name);
            if ((name_str.find(sub_name) != string::npos) && (type == string(port2->GetType()))) {
                return name_str;
            }
        }
    }
    
    return "";
}

void JackDriver::LoadConnections(int alias, bool full_name)
{
    list<pair<string, pair<string, string> > >::const_iterator it;
    
    if (full_name) {
        for (it = fConnections.begin(); it != fConnections.end(); it++) {
            pair<string, string> connection = (*it).second;
            jack_info("Load connection: %s %s", connection.first.c_str(), connection.second.c_str());
            fEngine->PortConnect(fClientControl.fRefNum, connection.first.c_str(), connection.second.c_str());
        }
    } else {
        const char** inputs = fGraphManager->GetPorts(NULL, NULL, JackPortIsInput);
        const char** outputs = fGraphManager->GetPorts(NULL, NULL, JackPortIsOutput);
        
        for (it = fConnections.begin(); it != fConnections.end(); it++) {
            pair<string, string> connection = (*it).second;
            string real_input = MatchPortName(connection.first.c_str(), outputs, alias, (*it).first);
            string real_output = MatchPortName(connection.second.c_str(), inputs, alias, (*it).first);
            if ((real_input != "") && (real_output != "")) {
                jack_info("Load connection: %s %s", real_input.c_str(), real_output.c_str());
                fEngine->PortConnect(fClientControl.fRefNum, real_input.c_str(), real_output.c_str());
            }
        }
        
        // Wait for connection change
        if (fGraphManager->IsPendingChange()) {
            JackSleep(int(fEngineControl->fPeriodUsecs * 1.1f));
        }
        
        if (inputs) {
            free(inputs);
        }
        if (outputs) {
            free(outputs);
        }
    }
}

int JackDriver::ResumeRefNum()
{
    return fGraphManager->ResumeRefNum(&fClientControl, fSynchroTable);
}

int JackDriver::SuspendRefNum()
{
    return fGraphManager->SuspendRefNum(&fClientControl, fSynchroTable, DRIVER_TIMEOUT_FACTOR * fEngineControl->fTimeOutUsecs);
}

} // end of namespace
