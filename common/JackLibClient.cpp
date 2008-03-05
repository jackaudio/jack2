/*
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

#include "JackLibClient.h"
#include "JackTime.h"
#include "JackLibGlobals.h"
#include "JackGlobals.h"
#include "JackChannel.h"

namespace Jack
{

// Used for external C API (JackAPI.cpp)
JackGraphManager* GetGraphManager()
{
    if (JackLibGlobals::fGlobals)
        return JackLibGlobals::fGlobals->fGraphManager;
    else
        return NULL;
}

JackEngineControl* GetEngineControl()
{
    if (JackLibGlobals::fGlobals)
        return JackLibGlobals::fGlobals->fEngineControl;
    else
        return NULL;
}

JackSynchro** GetSynchroTable()
{
    return (JackLibGlobals::fGlobals ? JackLibGlobals::fGlobals->fSynchroTable : 0);
}

//-------------------
// Client management
//-------------------

JackLibClient::JackLibClient(JackSynchro** table): JackClient(table)
{
    JackLog("JackLibClient::JackLibClient table = %x\n", table);
    fChannel = JackGlobals::MakeClientChannel();
}

JackLibClient::~JackLibClient()
{
    JackLog("JackLibClient::~JackLibClient\n");
    delete fChannel;
}

int JackLibClient::Open(const char* server_name, const char* name, jack_options_t options, jack_status_t* status)
{
    int shared_engine, shared_client, shared_graph, result;
    JackLog("JackLibClient::Open %s\n", name);

    snprintf(fServerName, sizeof(fServerName), server_name);

    // Open server/client channel
    char name_res[JACK_CLIENT_NAME_SIZE];
    if (fChannel->Open(server_name, name, name_res, this, options, status) < 0) {
        jack_error("Cannot connect to the server");
        goto error;
    }

    // Start receiving notifications
    if (fChannel->Start() < 0) {
        jack_error("Cannot start channel");
        goto error;
    }

    // Require new client
    fChannel->ClientOpen(name_res, &shared_engine, &shared_client, &shared_graph, &result);
    if (result < 0) {
        jack_error("Cannot open %s client", name_res);
        goto error;
    }

    try {
        // Map shared memory segments
        JackLibGlobals::fGlobals->fEngineControl.SetShmIndex(shared_engine, fServerName);
        JackLibGlobals::fGlobals->fGraphManager.SetShmIndex(shared_graph, fServerName);
        fClientControl.SetShmIndex(shared_client, fServerName);
        jack_verbose = GetEngineControl()->fVerbose;
    } catch (int n) {
        jack_error("Map shared memory segments exception %d", n);
        goto error;
    }

    SetupDriverSync(false);

    /* TODO : solve WIN32 thread Kill issue
    #ifndef WIN32
        // Connect shared synchro : the synchro must be usable in I/O mode when several clients live in the same process
    	if (!fSynchroTable[fClientControl->fRefNum]->Connect(name)) {
            jack_error("Cannot ConnectSemaphore %s client", name);
            goto error;
        }
    #endif
    */
    // Connect shared synchro : the synchro must be usable in I/O mode when several clients live in the same process
    if (!fSynchroTable[fClientControl->fRefNum]->Connect(name_res, fServerName)) {
        jack_error("Cannot ConnectSemaphore %s client", name_res);
        goto error;
    }

    JackLog("JackLibClient::Open name = %s refnum = %ld\n", name_res, fClientControl->fRefNum);
    return 0;

error:
    fChannel->Stop();
    fChannel->Close();
    return -1;
}

// Notifications received from the server
// TODO this should be done once for all clients in the process, when a shared notification channel
// will be shared by all clients...
int JackLibClient::ClientNotifyImp(int refnum, const char* name, int notify, int sync, int value1, int value2)
{
    int res = 0;

    // Done all time
    switch (notify) {

        case kAddClient:
            JackLog("JackClient::AddClient name = %s, ref = %ld \n", name, refnum);
            // the synchro must be usable in I/O mode when several clients live in the same process
            res = fSynchroTable[refnum]->Connect(name, fServerName) ? 0 : -1;
            break;

        case kRemoveClient:
            JackLog("JackClient::RemoveClient name = %s, ref = %ld \n", name, refnum);
            if (strcmp(GetClientControl()->fName, name) != 0)
                res = fSynchroTable[refnum]->Disconnect() ? 0 : -1;
            break;
    }

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



