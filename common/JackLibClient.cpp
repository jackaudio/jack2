/*
Copyright (C) 2004-2008 Grame

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

#include "JackLibClient.h"
#include "JackTime.h"
#include "JackLibGlobals.h"
#include "JackGlobals.h"
#include "JackPlatformPlug.h"
#include "JackTools.h"

namespace Jack
{

// Used for external C API (JackAPI.cpp)
JackGraphManager* GetGraphManager()
{
    if (JackLibGlobals::fGlobals) {
        return JackLibGlobals::fGlobals->fGraphManager;
    } else {
        return NULL;
    }
}

JackEngineControl* GetEngineControl()
{
    if (JackLibGlobals::fGlobals) {
        return JackLibGlobals::fGlobals->fEngineControl;
    } else {
        return NULL;
    }
}

JackSynchro* GetSynchroTable()
{
    return (JackLibGlobals::fGlobals ? JackLibGlobals::fGlobals->fSynchroTable : 0);
}

//-------------------
// Client management
//-------------------

/*
ShutDown is called:
- from the RT thread when Execute method fails
- possibly from a "closed" notification channel
(Not needed since the synch object used (Sema of Fifo will fails when server quits... see ShutDown))
*/

void JackLibClient::ShutDown(jack_status_t code, const char* message)
{
    jack_log("JackLibClient::ShutDown");
    JackGlobals::fServerRunning = false;
    JackClient::ShutDown(code, message);
}

JackLibClient::JackLibClient(JackSynchro* table): JackClient(table)
{
    jack_log("JackLibClient::JackLibClient table = %x", table);
    fChannel = new JackClientChannel();
}

JackLibClient::~JackLibClient()
{
    jack_log("JackLibClient::~JackLibClient");
    delete fChannel;
}

int JackLibClient::Open(const char* server_name, const char* name, int uuid, jack_options_t options, jack_status_t* status)
{
    int shared_engine, shared_client, shared_graph, result;
    bool res;
    jack_log("JackLibClient::Open name = %s", name);
    
    if (strlen(name) >= JACK_CLIENT_NAME_SIZE) {
        jack_error("\"%s\" is too long to be used as a JACK client name.\n"
                   "Please use %lu characters or less",
                   name,
                   JACK_CLIENT_NAME_SIZE - 1);
        return -1; 
    }
    
    strncpy(fServerName, server_name, sizeof(fServerName));

    // Open server/client channel
    char name_res[JACK_CLIENT_NAME_SIZE+1];
    if (fChannel->Open(server_name, name, uuid, name_res, this, options, status) < 0) {
        jack_error("Cannot connect to the server");
        goto error;
    }

    // Start receiving notifications
    if (fChannel->Start() < 0) {
        jack_error("Cannot start channel");
        goto error;
    }

    // Require new client
    fChannel->ClientOpen(name_res, JackTools::GetPID(), uuid, &shared_engine, &shared_client, &shared_graph, &result);
    if (result < 0) {
        jack_error("Cannot open %s client", name_res);
        goto error;
    }

    try {
        // Map shared memory segments
        JackLibGlobals::fGlobals->fEngineControl.SetShmIndex(shared_engine, fServerName);
        JackLibGlobals::fGlobals->fGraphManager.SetShmIndex(shared_graph, fServerName);
        fClientControl.SetShmIndex(shared_client, fServerName);
        JackGlobals::fVerbose = GetEngineControl()->fVerbose;
    } catch (...) {
        jack_error("Map shared memory segments exception");
        goto error;
    }

    SetupDriverSync(false);

    // Connect shared synchro : the synchro must be usable in I/O mode when several clients live in the same process
    assert(JackGlobals::fSynchroMutex);
    JackGlobals::fSynchroMutex->Lock();
    res = fSynchroTable[GetClientControl()->fRefNum].Connect(name_res, fServerName);
    JackGlobals::fSynchroMutex->Unlock();
    if (!res) {
        jack_error("Cannot ConnectSemaphore %s client", name_res);
        goto error;
    }

    JackGlobals::fClientTable[GetClientControl()->fRefNum] = this;
    SetClockSource(GetEngineControl()->fClockSource);
    jack_log("JackLibClient::Open name = %s refnum = %ld", name_res, GetClientControl()->fRefNum);
    return 0;

error:
    fChannel->Stop();
    fChannel->Close();
    return -1;
}

// Notifications received from the server
// TODO this should be done once for all clients in the process, when a shared notification channel
// will be shared by all clients...
int JackLibClient::ClientNotifyImp(int refnum, const char* name, int notify, int sync, const char* message, int value1, int value2)
{
    int res = 0;
    assert(JackGlobals::fSynchroMutex);
    JackGlobals::fSynchroMutex->Lock();

    // Done all time
    switch (notify) {

        case kAddClient:
            jack_log("JackClient::AddClient name = %s, ref = %ld ", name, refnum);
            // the synchro must be usable in I/O mode when several clients live in the same process
            res = fSynchroTable[refnum].Connect(name, fServerName) ? 0 : -1;
            break;

        case kRemoveClient:
            jack_log("JackClient::RemoveClient name = %s, ref = %ld ", name, refnum);
            if (GetClientControl() && strcmp(GetClientControl()->fName, name) != 0) {
                res = fSynchroTable[refnum].Disconnect() ? 0 : -1;
            }
            break;
    }

    JackGlobals::fSynchroMutex->Unlock();
    return res;
}

JackGraphManager* JackLibClient::GetGraphManager() const
{
    assert(JackLibGlobals::fGlobals->fGraphManager);
    return JackLibGlobals::fGlobals->fGraphManager;
}

JackEngineControl* JackLibClient::GetEngineControl() const
{
    assert(JackLibGlobals::fGlobals->fEngineControl);
    return JackLibGlobals::fGlobals->fEngineControl;
}

JackClientControl* JackLibClient::GetClientControl() const
{
    return fClientControl;
}

} // end of namespace



