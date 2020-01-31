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
    GetGlobal()->fServerRunning = false;
    JackClient::ShutDown(code, message);
}

JackLibClient::JackLibClient(JackGlobals* global): JackClient(global)
{
    jack_log("JackLibClient::JackLibClient table = %x", global->GetSynchroTable());
    fChannel = new JackClientChannel(global);
}

JackLibClient::~JackLibClient()
{
    jack_log("JackLibClient::~JackLibClient");
    delete fChannel;
}

int JackLibClient::Open(const char* server_name, const char* name, jack_uuid_t uuid, jack_options_t options, jack_status_t* status)
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
        JackLibGlobals *lib_globals = dynamic_cast<Jack::JackLibGlobals*>(GetGlobal());
        assert(lib_globals);
        lib_globals->fEngineControl.SetShmIndex(shared_engine, fServerName);
        lib_globals->fGraphManager.SetShmIndex(shared_graph, fServerName);
        fClientControl.SetShmIndex(shared_client, fServerName);
        JackGlobals::fVerbose = GetEngineControl()->fVerbose;
    } catch (...) {
        jack_error("Map shared memory segments exception");
        goto error;
    }

    SetupDriverSync(false);

    // Connect shared synchro : the synchro must be usable in I/O mode when several clients live in the same process
    assert(GetGlobal()->fSynchroMutex);
    GetGlobal()->fSynchroMutex->Lock();
    res = fSynchroTable[GetClientControl()->fRefNum].Connect(name_res, fServerName);
    GetGlobal()->fSynchroMutex->Unlock();
    if (!res) {
        jack_error("Cannot ConnectSemaphore %s client", name_res);
        goto error;
    }

    GetGlobal()->fClientTable[GetClientControl()->fRefNum] = this;
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
    assert(GetGlobal()->fSynchroMutex);
    GetGlobal()->fSynchroMutex->Lock();

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

    GetGlobal()->fSynchroMutex->Unlock();
    return res;
}

JackGraphManager* JackLibClient::GetGraphManager() const
{
    assert(GetGlobal()->GetGraphManager());
    return GetGlobal()->GetGraphManager();
}

JackEngineControl* JackLibClient::GetEngineControl() const
{
    assert(GetGlobal()->GetEngineControl());
    return GetGlobal()->GetEngineControl();
}

JackClientControl* JackLibClient::GetClientControl() const
{
    return fClientControl;
}

} // end of namespace



