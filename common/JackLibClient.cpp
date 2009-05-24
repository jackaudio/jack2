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

int JackLibClient::Open(const char* server_name, const char* name, jack_options_t options, jack_status_t* status)
{
    int shared_engine, shared_client, shared_graph, result;
    jack_log("JackLibClient::Open name = %s", name);

    strncpy(fServerName, server_name, sizeof(fServerName));

    // Open server/client channel
    char name_res[JACK_CLIENT_NAME_SIZE + 1];
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
    fChannel->ClientOpen(name_res, JackTools::GetPID(), &shared_engine, &shared_client, &shared_graph, &result);
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
 
    // Connect shared synchro : the synchro must be usable in I/O mode when several clients live in the same process
    if (!fSynchroTable[GetClientControl()->fRefNum].Connect(name_res, fServerName)) {
        jack_error("Cannot ConnectSemaphore %s client", name_res);
        goto error;
    }
  
    JackGlobals::fClientTable[GetClientControl()->fRefNum] = this;
    JackGlobals::fServerRunning = true;
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
int JackLibClient::ClientNotifyImp(int refnum, const char* name, int notify, int sync, int value1, int value2)
{
    int res = 0;

    // Done all time
    switch (notify) {

        case kAddClient:
            jack_log("JackClient::AddClient name = %s, ref = %ld ", name, refnum);
            // the synchro must be usable in I/O mode when several clients live in the same process
            res = fSynchroTable[refnum].Connect(name, fServerName) ? 0 : -1;
            break;

        case kRemoveClient:
            jack_log("JackClient::RemoveClient name = %s, ref = %ld ", name, refnum);
            if (strcmp(GetClientControl()->fName, name) != 0)
                res = fSynchroTable[refnum].Disconnect() ? 0 : -1;
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

int
JackLibClient::NoSelfConnectCheck(const char* src, const char* dst)
{
    // this check is to prevent apps to self connect to other apps
    // TODO: make this work with multiple clients per app
    {
        const char * sep_ptr;
        const char * client_name_ptr;
        int src_self;
        int dst_self;

        client_name_ptr = GetClientControl()->fName;

        //jack_info("Client '%s' (dis)connecting '%s' to '%s'", client_name_ptr, src, dst);

        sep_ptr = strchr(src, ':');
        if (sep_ptr == NULL)
        {
            jack_error("source port '%s' is invalid", src);
            return -1;
        }

        src_self = strncmp(client_name_ptr, src, sep_ptr - src) == 0 ? 0 : 1;

        sep_ptr = strchr(dst, ':');
        if (sep_ptr == NULL)
        {
            jack_error("destination port '%s' is invalid", dst);
            return -1;
        }

        dst_self = strncmp(client_name_ptr, dst, sep_ptr - dst) == 0 ? 0 : 1;

        //jack_info("src_self is %s", src_self ? "true" : "false");
        //jack_info("dst_self is %s", dst_self ? "true" : "false");

        // 0 means client is connecting other client ports (i.e. control app patchbay functionality)
        // 1 means client is connecting its own port to port of other client (i.e. self hooking into system app)
        // 2 means client is connecting its own ports (i.e. for app internal functionality)
        // TODO: Make this check an engine option and more tweakable (return error or success)
        // MAYBE: make the engine option changable on the fly and expose it through client or control API
        if (src_self + dst_self == 1)
        {
            jack_info("ignoring self hook to other client ports ('%s': '%s' -> '%s')", client_name_ptr, src, dst);
            return 0;
        }
    }

    return 1;
}

int JackLibClient::PortConnect(const char* src, const char* dst)
{
    int ret;

    //jack_info("Client connecting '%s' to '%s'");

    ret = NoSelfConnectCheck(src, dst);
    if (ret > 0)
    {
        return JackClient::PortConnect(src, dst);
    }

    return ret;
}

int JackLibClient::PortDisconnect(const char* src, const char* dst)
{
    int ret;

    //jack_info("Client disconnecting '%s' to '%s'");

    ret = NoSelfConnectCheck(src, dst);
    if (ret > 0)
    {
        return JackClient::PortDisconnect(src, dst);
    }

    return ret;
}

} // end of namespace



