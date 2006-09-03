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

#include "JackInternalClient.h"
#include "JackEngine.h"
#include "JackServer.h"
#include "JackGraphManager.h"
#include "JackEngineControl.h"
#include "JackClientControl.h"
#include "JackInternalClientChannel.h"
#include <assert.h>


namespace Jack
{

JackGraphManager* JackInternalClient::fGraphManager = NULL;
JackEngineControl* JackInternalClient::fEngineControl = NULL;

// Used for external C API (JackAPI.cpp)
JackGraphManager* GetGraphManager()
{
    return JackServer::fInstance->GetGraphManager();
}

JackEngineControl* GetEngineControl()
{
    return JackServer::fInstance->GetEngineControl();
}

JackSynchro** GetSynchroTable()
{
    return JackServer::fInstance->GetSynchroTable();
}

JackInternalClient::JackInternalClient(JackServer* server, JackSynchro** table): JackClient(table)
{
    fClientControl = new JackClientControl();
    fChannel = new JackInternalClientChannel(server);
}

JackInternalClient::~JackInternalClient()
{
    delete fClientControl;
    delete fChannel;
}

int JackInternalClient::Open(const char* name)
{
    int result;
    JackLog("JackInternalClient::Open name = %s\n", name);
    strcpy(fClientControl->fName, name);

    // Require new client
    fChannel->ClientNew(name, &fClientControl->fRefNum, &fEngineControl, &fGraphManager, this, &result);
    if (result < 0) {
        jack_error("Cannot open client name = %s", name);
        goto error;
    }

    SetupDriverSync(false);
    return 0;

error:
    fChannel->Stop();
    fChannel->Close();
    return -1;
}

JackGraphManager* JackInternalClient::GetGraphManager() const
{
    assert(fGraphManager);
    return fGraphManager;
}

JackEngineControl* JackInternalClient::GetEngineControl() const
{
    assert(fEngineControl);
    return fEngineControl;
}

JackClientControl* JackInternalClient::GetClientControl() const
{
    return fClientControl;
}

} // end of namespace

