/*
Copyright (C) 2001 Paul Davis 
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

#ifdef WIN32 
#pragma warning (disable : 4786)
#endif

#include "JackServer.h"
#include "JackTime.h"
#include "JackFreewheelDriver.h"
#include "JackLoopbackDriver.h"
#include "JackThreadedDriver.h"
#include "JackGlobals.h"
#include "JackEngine.h"
#include "JackAudioDriver.h"
#include "JackChannel.h"
#include "JackClientControl.h"
#include "JackEngineControl.h"
#include "JackSyncInterface.h"
#include "JackGraphManager.h"

#ifdef __APPLE_
#include <CoreFoundation/CFNotificationCenter.h>
#endif

namespace Jack
{

JackServer* JackServer::fInstance = NULL;

JackServer::JackServer(bool sync,  bool temporary, long timeout, bool rt, long priority, long loopback, bool verbose)
{
    JackGlobals::InitServer();
    for (int i = 0; i < CLIENT_NUM; i++)
        fSynchroTable[i] = JackGlobals::MakeSynchro();
    fGraphManager = new JackGraphManager();
    fEngineControl = new JackEngineControl(sync, temporary, timeout, rt, priority, verbose);
    fEngine = new JackEngine(fGraphManager, fSynchroTable, fEngineControl);
    fFreewheelDriver = new JackThreadedDriver(new JackFreewheelDriver("freewheel", fEngine, fSynchroTable));
    fLoopbackDriver = new JackLoopbackDriver("loopback", fEngine, fSynchroTable);
    fChannel = JackGlobals::MakeServerChannel();
    fState = new JackConnectionManager();
	fFreewheel = false;
    fLoopback = loopback;
    fDriverInfo = NULL;
    fAudioDriver = NULL;
    fInstance = this; // Unique instance
	jack_verbose = verbose;
}

JackServer::~JackServer()
{
    for (int i = 0; i < CLIENT_NUM; i++)
        delete fSynchroTable[i];
    delete fGraphManager;
    delete fAudioDriver;
    delete fFreewheelDriver;
    delete fLoopbackDriver;
    delete fEngine;
    delete fChannel;
    delete fEngineControl;
    delete fState;
    if (fDriverInfo) {
        UnloadDriverModule(fDriverInfo->handle);
        free(fDriverInfo);
    }
    JackGlobals::Destroy();
}

// TODO : better handling of intermediate failing cases...

int JackServer::Open(jack_driver_desc_t* driver_desc, JSList* driver_params)
{
    if (fChannel->Open(this) < 0) {
        jack_error("Server channel open error");
        return -1;
    }

    if (fEngine->Open() != 0) {
        jack_error("Cannot open engine");
        return -1;
    }

    if ((fDriverInfo = jack_load_driver(driver_desc)) == NULL) {
        return -1;
    }

    if ((fAudioDriver = fDriverInfo->initialize(fEngine, fSynchroTable, driver_params)) == NULL) {
        jack_error("Cannot initialize driver");
        return -1;
    }

    if (fFreewheelDriver->Open() != 0) { // before engine open
        jack_error("Cannot open driver");
        return -1;
    }

    // Before engine open
    if (fLoopbackDriver->Open(fEngineControl->fBufferSize, fEngineControl->fSampleRate, 1, 1, fLoopback, fLoopback, false, "loopback", "loopback", 0, 0) != 0) {
        jack_error("Cannot open driver");
        return -1;
    }

    if (fAudioDriver->Attach() != 0) {
        jack_error("Cannot attach audio driver");
        return -1;
    }
	
    if (fLoopback > 0 && fLoopbackDriver->Attach() != 0) {
        jack_error("Cannot attach loopback driver");
        return -1;
    }

    fFreewheelDriver->SetMaster(false);
    fAudioDriver->SetMaster(true);
    if (fLoopback > 0)
        fAudioDriver->AddSlave(fLoopbackDriver);
    fAudioDriver->AddSlave(fFreewheelDriver); // After ???
    InitTime();

#ifdef __APPLE__
    // Send notification to be used in the Jack Router
    CFNotificationCenterPostNotification(CFNotificationCenterGetDistributedCenter(),
                                         CFSTR("com.grame.jackserver.start"),
                                         CFSTR("com.grame.jackserver"),
                                         NULL,
                                         true);
#endif

    return 0;
}

int JackServer::Close()
{
    JackLog("JackServer::Close\n");
    fChannel->Close();
    fAudioDriver->Detach();
    if (fLoopback > 0)
        fLoopbackDriver->Detach();
    fAudioDriver->Close();
    fFreewheelDriver->Close();
    fLoopbackDriver->Close();
    fEngine->Close();

#ifdef __APPLE__
    // Send notification to be used in the Jack Router
    CFNotificationCenterPostNotification(CFNotificationCenterGetDistributedCenter(),
                                         CFSTR("com.grame.jackserver.stop"),
                                         CFSTR("com.grame.jackserver"),
                                         NULL,
                                         true);
#endif

    return 0;
}

int JackServer::Start()
{
    JackLog("JackServer::Start\n");
    fEngineControl->InitFrameTime();
    return fAudioDriver->Start();
}

int JackServer::Stop()
{
    JackLog("JackServer::Stop\n");
    return fAudioDriver->Stop();
}

int JackServer::SetBufferSize(jack_nframes_t buffer_size)
{
    JackLog("JackServer::SetBufferSize nframes = %ld\n", buffer_size);
	jack_nframes_t current_buffer_size = fEngineControl->fBufferSize;

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
		fFreewheelDriver->SetBufferSize(current_buffer_size);
		fEngineControl->InitFrameTime();
		return fAudioDriver->Start();
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
    JackLog("JackServer::SetFreewheel state = %ld\n", onoff);

    if (fFreewheel) {
        if (onoff) {
            return -1;
        } else {
            fFreewheel = false;
            fFreewheelDriver->Stop();
            fGraphManager->Restore(fState);   // Restore previous connection state
            fEngine->NotifyFreewheel(onoff);
            fFreewheelDriver->SetMaster(false);
            fEngineControl->InitFrameTime();
            return fAudioDriver->Start();
        }
    } else {
        if (onoff) {
            fFreewheel = true;
            fAudioDriver->Stop();
            fGraphManager->Save(fState);     // Save connection state
            fGraphManager->DisconnectAllPorts(fAudioDriver->GetClientControl()->fRefNum);
            fEngine->NotifyFreewheel(onoff);
            fFreewheelDriver->SetMaster(true);
            return fFreewheelDriver->Start();
        } else {
            return -1;
        }
    }
}

// Coming from the RT thread or server channel
void JackServer::Notify(int refnum, int notify, int value)
{
    switch (notify) {

        case kGraphOrderCallback:
            fEngine->NotifyGraphReorder();
            break;

        case kXRunCallback:
            fEngine->NotifyXRun(refnum);
            break;

        case kZombifyClient:
            fEngine->ZombifyClient(refnum);
            break;

        case kDeadClient:
            JackLog("JackServer: kDeadClient ref = %ld\n", refnum);
			if (fEngine->ClientDeactivate(refnum) < 0)
				jack_error("JackServer: DeadClient ref = %ld cannot be removed from the graph !!", refnum);
			fEngine->ClientExternalClose(refnum);
			break;
    }
}

JackEngine* JackServer::GetEngine()
{
    return fEngine;
}

JackSynchro** JackServer::GetSynchroTable()
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

void JackServer::PrintState()
{
    fAudioDriver->PrintState();
    fEngine->PrintState();
}

} // end of namespace

