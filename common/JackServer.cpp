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
#include "JackLoopbackDriver.h"
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

namespace Jack
{

JackServer::JackServer(bool sync, bool temporary, long timeout, bool rt, long priority, long loopback, bool verbose, const char* server_name)
{
    if (rt) {
        jack_info("JACK server starting in realtime mode with priority %ld", priority);
    } else {
        jack_info("JACK server starting in non-realtime mode");
    }

    fGraphManager = new JackGraphManager();
    fEngineControl = new JackEngineControl(sync, temporary, timeout, rt, priority, verbose, server_name);
    fEngine = new JackLockedEngine(fGraphManager, GetSynchroTable(), fEngineControl);
    fFreewheelDriver = new JackThreadedDriver(new JackFreewheelDriver(fEngine, GetSynchroTable()));
    fLoopbackDriver = new JackLoopbackDriver(fEngine, GetSynchroTable());
    fAudioDriver = NULL;
    fFreewheel = false;
    fLoopback = loopback;
    JackServerGlobals::fInstance = this;   // Unique instance
    JackServerGlobals::fUserCount = 1;     // One user
    jack_verbose = verbose;
}

JackServer::~JackServer()
{
    delete fGraphManager;
    delete fAudioDriver;
    delete fFreewheelDriver;
    delete fLoopbackDriver;
    delete fEngine;
    delete fEngineControl;
}

int JackServer::Open(jack_driver_desc_t* driver_desc, JSList* driver_params)
{
    // TODO: move that in reworked JackServerGlobals::Init()
    JackMessageBuffer::Create();
  
    if (fChannel.Open(fEngineControl->fServerName, this) < 0) {
        jack_error("Server channel open error");
        goto fail_close1;
    }
  
    if (fEngine->Open() != 0) {
        jack_error("Cannot open engine");
        goto fail_close2;
    }

    if ((fAudioDriver = fDriverInfo.Open(driver_desc, fEngine, GetSynchroTable(), driver_params)) == NULL) {
        jack_error("Cannot initialize driver");
        goto fail_close3;
    }
 
    if (fFreewheelDriver->Open() != 0) { // before engine open
        jack_error("Cannot open driver");
        goto fail_close4;
    }
    
    if (fLoopbackDriver->Open(fEngineControl->fBufferSize, fEngineControl->fSampleRate, 1, 1, fLoopback, fLoopback, false, "loopback", "loopback", 0, 0) != 0) {
        jack_error("Cannot open driver");
        goto fail_close5;
    }

    if (fAudioDriver->Attach() != 0) {
        jack_error("Cannot attach audio driver");
        goto fail_close6;
    }
   
    if (fLoopback > 0 && fLoopbackDriver->Attach() != 0) {
        jack_error("Cannot attach loopback driver");
        goto fail_close7;
    }
 
    fFreewheelDriver->SetMaster(false);
    fAudioDriver->SetMaster(true);
    if (fLoopback > 0)
        fAudioDriver->AddSlave(fLoopbackDriver);
    fAudioDriver->AddSlave(fFreewheelDriver); // After ???
    InitTime();
    return 0;

fail_close7:     
    fAudioDriver->Detach();
  
fail_close6:     
    fLoopbackDriver->Close();

fail_close5:
    fFreewheelDriver->Close();

fail_close4:
    fAudioDriver->Close();

fail_close3:
    fEngine->Close();
 
fail_close2:     
    fChannel.Close();

fail_close1:     
    JackMessageBuffer::Destroy();
    return -1;
}

int JackServer::Close()
{
    jack_log("JackServer::Close");
    fChannel.Close();
    fAudioDriver->Detach();
    if (fLoopback > 0)
        fLoopbackDriver->Detach();
    fAudioDriver->Close();
    fFreewheelDriver->Close();
    fLoopbackDriver->Close();
    fEngine->Close();
    // TODO: move that in reworked JackServerGlobals::Destroy()
    JackMessageBuffer::Destroy();
    return 0;
}

int JackServer::InternalClientLoad(const char* client_name, const char* so_name, const char* objet_data, int options, int* int_ref, int* status)
{
    JackLoadableInternalClient* client = new JackLoadableInternalClient1(JackServerGlobals::fInstance, GetSynchroTable(), objet_data);
    assert(client);
    return InternalClientLoadAux(client, so_name, client_name, options, int_ref, status);
 }

int JackServer::InternalClientLoad(const char* client_name, const char* so_name, const JSList * parameters, int options, int* int_ref, int* status)
{
    JackLoadableInternalClient* client = new JackLoadableInternalClient2(JackServerGlobals::fInstance, GetSynchroTable(), parameters);
    assert(client);
    return InternalClientLoadAux(client, so_name, client_name, options, int_ref, status);
}

int JackServer::InternalClientLoadAux(JackLoadableInternalClient* client, const char* so_name, const char* client_name, int options, int* int_ref, int* status)
{
    // Clear status
    *status = 0;
    if ((client->Init(so_name) < 0) || (client->Open(JACK_DEFAULT_SERVER_NAME, client_name, (jack_options_t)options, (jack_status_t*)status) < 0)) {
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

int JackServer::Start()
{
    jack_log("JackServer::Start");
    fEngineControl->InitFrameTime();
    return fAudioDriver->Start();
}

int JackServer::Stop()
{
    jack_log("JackServer::Stop");
    return fAudioDriver->Stop();
}

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
        fFreewheelDriver->SetBufferSize(buffer_size);
        fEngine->NotifyBufferSize(buffer_size);
        fEngineControl->InitFrameTime();
        return fAudioDriver->Start();
    } else { // Failure: try to restore current value
        jack_error("Cannot SetBufferSize for audio driver, restore current value %ld", current_buffer_size);
        fAudioDriver->SetBufferSize(current_buffer_size);
        fFreewheelDriver->SetBufferSize(current_buffer_size);
        fEngineControl->InitFrameTime();
        fAudioDriver->Start();
        // SetBufferSize actually failed, so return an error...
        return -1;
    }
}

/*
Freewheel mode is implemented by switching from the (audio + freewheel) driver to the freewheel driver only:

    - "global" connection state is saved
    - all audio driver ports are deconnected, thus there is no more dependancies with the audio driver
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
            fFreewheelDriver->Stop();
            fGraphManager->Restore(&fConnectionState);   // Restore previous connection state
            fEngine->NotifyFreewheel(onoff);
            fFreewheelDriver->SetMaster(false);
            fEngineControl->InitFrameTime();
            return fAudioDriver->Start();
        }
    } else {
        if (onoff) {
            fFreewheel = true;
            fAudioDriver->Stop();
            fGraphManager->Save(&fConnectionState);     // Save connection state
            fGraphManager->DisconnectAllPorts(fAudioDriver->GetClientControl()->fRefNum);
            fEngine->NotifyFreewheel(onoff);
            fFreewheelDriver->SetMaster(true);
            return fFreewheelDriver->Start();
        } else {
            return -1;
        }
    }
}

// Coming from the RT thread
void JackServer::Notify(int refnum, int notify, int value)
{
    switch (notify) {

        case kGraphOrderCallback:
            fEngine->NotifyGraphReorder();
            break;

        case kXRunCallback:
            fEngine->NotifyXRun(refnum);
            break;

    }
}

void JackServer::ClientKill(int refnum)
{
    jack_log("JackServer::ClientKill ref = %ld", refnum);
    if (fEngine->ClientDeactivate(refnum) < 0) {
        jack_error("JackServer::ClientKill ref = %ld cannot be removed from the graph !!", refnum);
    }
    if (fEngine->ClientExternalClose(refnum) < 0) {
        jack_error("JackServer::ClientKill ref = %ld cannot be closed", refnum);
    }
}

//----------------------
// Backend management
//----------------------

JackDriverInfo* JackServer::AddSlave(jack_driver_desc_t* driver_desc, JSList* driver_params)
{
    JackDriverInfo* info = new JackDriverInfo();
    JackDriverClientInterface* backend = info->Open(driver_desc, fEngine, GetSynchroTable(), driver_params);
    if (backend == NULL) {
        delete info;
        return NULL;
    } else {
        backend->Attach();
        fAudioDriver->AddSlave(backend);
        return info;
    }
}

void JackServer::RemoveSlave(JackDriverInfo* info)
{
    JackDriverClientInterface* backend = info->GetBackend();
    fAudioDriver->RemoveSlave(info->GetBackend());
    backend->Detach();
    backend->Close();
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

