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
#include "JackServerGlobals.h"
#include "JackTime.h"
#include "JackFreewheelDriver.h"
#include "JackThreadedDriver.h"
#include "JackGlobals.h"
#include "JackLockedEngine.h"
#include "JackAudioDriver.h"
#include "JackChannel.h"
#include "JackClientControl.h"
#include "JackEngineControl.h"
#include "JackGraphManager.h"
#include "JackInternalClient.h"
#include "JackError.h"
#include "JackMessageBuffer.h"
#include "JackInternalSessionLoader.h"

const char * jack_get_self_connect_mode_description(char mode);

namespace Jack
{

//----------------
// Server control 
//----------------
JackServer::JackServer(bool sync, bool temporary, int timeout, bool rt, int priority, int port_max, bool verbose, jack_timer_type_t clock, char self_connect_mode, const char* server_name)
{
    if (rt) {
        jack_info("JACK server starting in realtime mode with priority %ld", priority);
    } else {
        jack_info("JACK server starting in non-realtime mode");
    }

    jack_info("self-connect-mode is \"%s\"", jack_get_self_connect_mode_description(self_connect_mode));

    fGraphManager = JackGraphManager::Allocate(port_max);
    fEngineControl = new JackEngineControl(sync, temporary, timeout, rt, priority, verbose, clock, server_name);
    fEngine = new JackLockedEngine(fGraphManager, GetSynchroTable(), fEngineControl, self_connect_mode);

    // A distinction is made between the threaded freewheel driver and the
    // regular freewheel driver because the freewheel driver needs to run in
    // threaded mode when freewheel mode is active and needs to run as a slave
    // when freewheel mode isn't active.
    JackFreewheelDriver* freewheelDriver = new JackFreewheelDriver(fEngine, GetSynchroTable());
    fThreadedFreewheelDriver = new JackThreadedDriver(freewheelDriver);

    fFreewheelDriver = freewheelDriver;
    fDriverInfo = new JackDriverInfo();
    fAudioDriver = NULL;
    fFreewheel = false;
    JackServerGlobals::fInstance = this;   // Unique instance
    JackServerGlobals::fUserCount = 1;     // One user
    JackGlobals::fVerbose = verbose;
}

JackServer::~JackServer()
{
    JackGraphManager::Destroy(fGraphManager);
    delete fDriverInfo;
    delete fThreadedFreewheelDriver;
    delete fEngine;
    delete fEngineControl;
}

int JackServer::Open(jack_driver_desc_t* driver_desc, JSList* driver_params)
{
    // TODO: move that in reworked JackServerGlobals::Init()
    if (!JackMessageBuffer::Create()) {
        jack_error("Cannot create message buffer");
    }

     if ((fAudioDriver = fDriverInfo->Open(driver_desc, fEngine, GetSynchroTable(), driver_params)) == NULL) {
        jack_error("Cannot initialize driver");
        goto fail_close1;
    }

    if (fRequestChannel.Open(fEngineControl->fServerName, this) < 0) {
        jack_error("Server channel open error");
        goto fail_close2;
    }

    if (fEngine->Open() < 0) {
        jack_error("Cannot open engine");
        goto fail_close3;
    }

    if (fFreewheelDriver->Open() < 0) {
        jack_error("Cannot open freewheel driver");
        goto fail_close4;
    }

    if (fAudioDriver->Attach() < 0) {
        jack_error("Cannot attach audio driver");
        goto fail_close5;
    }

    fFreewheelDriver->SetMaster(false);
    fAudioDriver->SetMaster(true);
    fAudioDriver->AddSlave(fFreewheelDriver);
    InitTime();
    SetClockSource(fEngineControl->fClockSource);
    return 0;

fail_close5:
    fFreewheelDriver->Close();

fail_close4:
    fEngine->Close();

fail_close3:
    fRequestChannel.Close();

fail_close2:
    fAudioDriver->Close();

fail_close1:
    JackMessageBuffer::Destroy();
    return -1;
}

int JackServer::Close()
{
    jack_log("JackServer::Close");
    fRequestChannel.Close();
    fAudioDriver->Detach();
    fAudioDriver->Close();
    fFreewheelDriver->Close();
    fEngine->Close();
    // TODO: move that in reworked JackServerGlobals::Destroy()
    JackMessageBuffer::Destroy();
    EndTime();
    return 0;
}

int JackServer::Start()
{
    jack_log("JackServer::Start");
    if (fAudioDriver->Start() < 0) {
        return -1;
    }
    return fRequestChannel.Start();
}

int JackServer::Stop()
{
    jack_log("JackServer::Stop");
    int res = -1;
    
    if (fFreewheel) {
        if (fThreadedFreewheelDriver) {
            res = fThreadedFreewheelDriver->Stop();
        }
    } else {
        if (fAudioDriver) {
            res = fAudioDriver->Stop();
        }
    }
    
    fEngine->NotifyQuit();
    fRequestChannel.Stop();
    fEngine->NotifyFailure(JackFailure | JackServerError, JACK_SERVER_FAILURE);
    
    return res;
}

bool JackServer::IsRunning()
{
    jack_log("JackServer::IsRunning");
    assert(fAudioDriver);
    return fAudioDriver->IsRunning();
}

//------------------
// Internal clients 
//------------------

int JackServer::InternalClientLoad1(const char* client_name, const char* so_name, const char* objet_data, int options, int* int_ref, int uuid, int* status)
{
    JackLoadableInternalClient* client = new JackLoadableInternalClient1(JackServerGlobals::fInstance, GetSynchroTable(), objet_data);
    assert(client);
    return InternalClientLoadAux(client, so_name, client_name, options, int_ref, uuid, status);
 }

int JackServer::InternalClientLoad2(const char* client_name, const char* so_name, const JSList * parameters, int options, int* int_ref, int uuid, int* status)
{
    JackLoadableInternalClient* client = new JackLoadableInternalClient2(JackServerGlobals::fInstance, GetSynchroTable(), parameters);
    assert(client);
    return InternalClientLoadAux(client, so_name, client_name, options, int_ref, uuid, status);
}

int JackServer::InternalClientLoadAux(JackLoadableInternalClient* client, const char* so_name, const char* client_name, int options, int* int_ref, int uuid, int* status)
{
    // Clear status
    *status = 0;

    // Client object is internally kept in JackEngine
    if ((client->Init(so_name) < 0) || (client->Open(JackTools::DefaultServerName(), client_name,  uuid, (jack_options_t)options, (jack_status_t*)status) < 0)) {
        delete client;
        int my_status1 = *status | JackFailure;
        *status = (jack_status_t)my_status1;
        *int_ref = 0;
        return -1;
    } else {
        *int_ref = client->GetClientControl()->fRefNum;
        return 0;
    }
 }

//-----------------------
// Internal session file 
//-----------------------

int JackServer::LoadInternalSessionFile(const char* file)
{
    JackInternalSessionLoader loader(this);
    return loader.Load(file);
}

//---------------------------
// From request thread : API 
//---------------------------

int JackServer::SetBufferSize(jack_nframes_t buffer_size)
{
    jack_log("JackServer::SetBufferSize nframes = %ld", buffer_size);
    jack_nframes_t current_buffer_size = fEngineControl->fBufferSize;

    if (current_buffer_size == buffer_size) {
        jack_log("SetBufferSize: requirement for new buffer size equals current value");
        return 0;
    }

    if (fAudioDriver->IsFixedBufferSize()) {
        jack_log("SetBufferSize: driver only supports a fixed buffer size");
        return -1;
    }

    if (fAudioDriver->Stop() != 0) {
        jack_error("Cannot stop audio driver");
        return -1;
    }

    if (fAudioDriver->SetBufferSize(buffer_size) == 0) {
        fEngine->NotifyBufferSize(buffer_size);
        return fAudioDriver->Start();
    } else { // Failure: try to restore current value
        jack_error("Cannot SetBufferSize for audio driver, restore current value %ld", current_buffer_size);
        fAudioDriver->SetBufferSize(current_buffer_size);
        fAudioDriver->Start();
        // SetBufferSize actually failed, so return an error...
        return -1;
    }
}

/*
Freewheel mode is implemented by switching from the (audio [slaves] + freewheel) driver to the freewheel driver only:

    - "global" connection state is saved
    - all audio driver and slaves ports are deconnected, thus there is no more dependancies with the audio driver and slaves
    - the freewheel driver will be synchronized with the end of graph execution : all clients are connected to the freewheel driver
    - the freewheel driver becomes the "master"

Normal mode is restored with the connections state valid before freewheel mode was done. Thus one consider that
no graph state change can be done during freewheel mode.
*/

int JackServer::SetFreewheel(bool onoff)
{
    jack_log("JackServer::SetFreewheel is = %ld want = %ld", fFreewheel, onoff);

    if (fFreewheel) {
        if (onoff) {
            return -1;
        } else {
            fFreewheel = false;
            fThreadedFreewheelDriver->Stop();
            fGraphManager->Restore(&fConnectionState);   // Restore connection state
            fEngine->NotifyFreewheel(onoff);
            fFreewheelDriver->SetMaster(false);
            fAudioDriver->SetMaster(true);
            return fAudioDriver->Start();
        }
    } else {
        if (onoff) {
            fFreewheel = true;
            fAudioDriver->Stop();
            fGraphManager->Save(&fConnectionState);     // Save connection state
            // Disconnect all slaves
            std::list<JackDriverInterface*> slave_list = fAudioDriver->GetSlaves();
            std::list<JackDriverInterface*>::const_iterator it;
            for (it = slave_list.begin(); it != slave_list.end(); it++) {
                JackDriver* slave = dynamic_cast<JackDriver*>(*it);
                assert(slave);
                fGraphManager->DisconnectAllPorts(slave->GetClientControl()->fRefNum);
            }
            // Disconnect master
            fGraphManager->DisconnectAllPorts(fAudioDriver->GetClientControl()->fRefNum);
            fEngine->NotifyFreewheel(onoff);
            fAudioDriver->SetMaster(false);
            fFreewheelDriver->SetMaster(true);
            return fThreadedFreewheelDriver->Start();
        } else {
            return -1;
        }
    }
}

//---------------------------
// Coming from the RT thread
//---------------------------

void JackServer::Notify(int refnum, int notify, int value)
{
    switch (notify) {

        case kGraphOrderCallback:
            fEngine->NotifyGraphReorder();
            break;

        case kXRunCallback:
            fEngine->NotifyClientXRun(refnum);
            break;
    }
}

//--------------------
// Backend management
//--------------------

JackDriverInfo* JackServer::AddSlave(jack_driver_desc_t* driver_desc, JSList* driver_params)
{
    JackDriverInfo* info = new JackDriverInfo();
    JackDriverClientInterface* slave = info->Open(driver_desc, fEngine, GetSynchroTable(), driver_params);
    
    if (!slave) {
        goto error1;
    }
    if (slave->Attach() < 0) {
        goto error2;
    }
    
    slave->SetMaster(false);
    fAudioDriver->AddSlave(slave);
    return info;

error2:
    slave->Close();
    
error1:
    delete info;
    return NULL;
}

void JackServer::RemoveSlave(JackDriverInfo* info)
{
    JackDriverClientInterface* slave = info->GetBackend();
    fAudioDriver->RemoveSlave(slave);
    slave->Detach();
    slave->Close();
}

int JackServer::SwitchMaster(jack_driver_desc_t* driver_desc, JSList* driver_params)
{
    std::list<JackDriverInterface*> slave_list;
    std::list<JackDriverInterface*>::const_iterator it;
    
    // Remove current master
    fAudioDriver->Stop();
    fAudioDriver->Detach();
    fAudioDriver->Close();

    // Open new master
    JackDriverInfo* info = new JackDriverInfo();
    JackDriverClientInterface* master = info->Open(driver_desc, fEngine, GetSynchroTable(), driver_params);

    if (!master) {
       goto error;
    }

    // Get slaves list
    slave_list = fAudioDriver->GetSlaves();
 
    // Move slaves in new master
    for (it = slave_list.begin(); it != slave_list.end(); it++) {
        JackDriverInterface* slave = *it;
        master->AddSlave(slave);
    }

    // Delete old master
    delete fDriverInfo;

    // Activate master
    fAudioDriver = master;
    fDriverInfo = info;
    
    if (fAudioDriver->Attach() < 0) {
        goto error;
    }
    
    // Notify clients of new values
    fEngine->NotifyBufferSize(fEngineControl->fBufferSize);
    fEngine->NotifySampleRate(fEngineControl->fSampleRate);
    
    // And finally start
    fAudioDriver->SetMaster(true);
    return fAudioDriver->Start();

error:
    delete info;
    return -1;
}

//----------------------
// Transport management
//----------------------

int JackServer::ReleaseTimebase(int refnum)
{
    return fEngineControl->fTransport.ResetTimebase(refnum);
}

int JackServer::SetTimebaseCallback(int refnum, int conditional)
{
    return fEngineControl->fTransport.SetTimebaseMaster(refnum, conditional);
}

JackLockedEngine* JackServer::GetEngine()
{
    return fEngine;
}

JackSynchro* JackServer::GetSynchroTable()
{
    return fSynchroTable;
}

JackEngineControl* JackServer::GetEngineControl()
{
    return fEngineControl;
}

JackGraphManager* JackServer::GetGraphManager()
{
    return fGraphManager;
}

} // end of namespace

