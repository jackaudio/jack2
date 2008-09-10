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

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "JackSystemDeps.h"

#ifndef WIN32
#ifndef ADDON_DIR
#include "config.h"
#endif
#endif

#include "JackGraphManager.h"
#include "JackInternalClient.h"
#include "JackLockedEngine.h"
#include "JackServer.h"
#include "JackEngineControl.h"
#include "JackClientControl.h"
#include "JackInternalClientChannel.h"
#include "JackTools.h"
#include <assert.h>

namespace Jack
{

#ifdef WIN32

static void BuildClientPath(char* path_to_so, int path_len, const char* so_name)
{
    snprintf(path_to_so, path_len, ADDON_DIR "/%s.dll", so_name);
}

static void PrintLoadError(const char* so_name)
{
    // Retrieve the system error message for the last-error code
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process
    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)so_name) + 40) * sizeof(TCHAR));
    _snprintf((LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("error loading %s err = %s"), so_name, lpMsgBuf);

    jack_error((LPCTSTR)lpDisplayBuf);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}

#else

static void BuildClientPath(char* path_to_so, int path_len, const char* so_name)
{
    snprintf(path_to_so, path_len, ADDON_DIR "/%s.so", so_name);
}

#endif

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

JackSynchro* GetSynchroTable()
{
    return JackServer::fInstance->GetSynchroTable();
}

JackInternalClient::JackInternalClient(JackServer* server, JackSynchro* table): JackClient(table)
{
    fChannel = new JackInternalClientChannel(server);
}

JackInternalClient::~JackInternalClient()
{
    delete fChannel;
}

int JackInternalClient::Open(const char* server_name, const char* name, jack_options_t options, jack_status_t* status)
{
    int result;
    char name_res[JACK_CLIENT_NAME_SIZE + 1];
    jack_log("JackInternalClient::Open name = %s", name);

    snprintf(fServerName, sizeof(fServerName), server_name);

    fChannel->ClientCheck(name, name_res, JACK_PROTOCOL_VERSION, (int)options, (int*)status, &result);
    if (result < 0) {
        int status1 = *status;
        if (status1 & JackVersionError)
            jack_error("JACK protocol mismatch %d", JACK_PROTOCOL_VERSION);
        else
            jack_error("Client name = %s conflits with another running client", name);
        goto error;
    }

    strcpy(fClientControl.fName, name_res);

    // Require new client
    fChannel->ClientOpen(name_res, &fClientControl.fRefNum, &fEngineControl, &fGraphManager, this, &result);
    if (result < 0) {
        jack_error("Cannot open client name = %s", name_res);
        goto error;
    }

    SetupDriverSync(false);
    fClientTable[fClientControl.fRefNum] = this;
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
    return const_cast<JackClientControl*>(&fClientControl);
}

int JackLoadableInternalClient::Init(const char* so_name)
{
    char path_to_so[JACK_PATH_MAX + 1];
    BuildClientPath(path_to_so, sizeof(path_to_so), so_name);
 
    fHandle = LoadJackModule(path_to_so);
    jack_log("JackLoadableInternalClient::JackLoadableInternalClient path_to_so = %s", path_to_so);

    if (fHandle == 0) {
        PrintLoadError(so_name);
        return -1;
    }

    fFinish = (FinishCallback)GetJackProc(fHandle, "jack_finish");
    if (!fFinish) {
        UnloadJackModule(fHandle);
        jack_error("symbol jack_finish cannot be found in %s", so_name);
        return -1;
    }

    fDescriptor = (JackDriverDescFunction)GetJackProc(fHandle, "jack_get_descriptor");
    if (!fDescriptor) {
        jack_info("No jack_get_descriptor entry-point for %s", so_name);
    }
    return 0;
}

int JackLoadableInternalClient1::Init(const char* so_name)
{
    if (JackLoadableInternalClient::Init(so_name) < 0) {
	return -1;
    }
    
    fInitialize = (InitializeCallback)GetJackProc(fHandle, "jack_initialize");
    if (!fInitialize) {
        UnloadJackModule(fHandle);
        jack_error("symbol jack_initialize cannot be found in %s", so_name);
        return -1;
    }

    return 0;
}

int JackLoadableInternalClient2::Init(const char* so_name)
{
    if (JackLoadableInternalClient::Init(so_name) < 0) {
	return -1;
    }
    
    fInitialize = (InternalInitializeCallback)GetJackProc(fHandle, "jack_internal_initialize");
    if (!fInitialize) {
        UnloadJackModule(fHandle);
        jack_error("symbol jack_internal_initialize cannot be found in %s", so_name);
        return -1;
    }

    return 0;
}

JackLoadableInternalClient1::JackLoadableInternalClient1(JackServer* server, JackSynchro* table, const char* so_name, const char* object_data)
        : JackLoadableInternalClient(server, table)
{
    snprintf(fObjectData, JACK_LOAD_INIT_LIMIT, object_data);   
}

JackLoadableInternalClient2::JackLoadableInternalClient2(JackServer* server, JackSynchro* table, const char* so_name, const JSList*  parameters)
        : JackLoadableInternalClient(server, table)
{
    fParameters = parameters;
}

JackLoadableInternalClient::~JackLoadableInternalClient()
{
    if (fFinish)
        fFinish(fProcessArg);
    UnloadJackModule(fHandle);
}

int JackLoadableInternalClient1::Open(const char* server_name, const char* name, jack_options_t options, jack_status_t* status)
{
    int res = -1;
    
    if (JackInternalClient::Open(server_name, name, options, status) == 0) {
        if (fInitialize((jack_client_t*)this, fObjectData) == 0) {
            res = 0;
        } else {
            JackInternalClient::Close();
            fFinish = NULL;
        }
    }
    
    return res;
}

int JackLoadableInternalClient2::Open(const char* server_name, const char* name, jack_options_t options, jack_status_t* status)
{
    int res = -1;
    
    if (JackInternalClient::Open(server_name, name, options, status) == 0) {
        if (fInitialize((jack_client_t*)this, fParameters) == 0) {
            res = 0;
        } else {
            JackInternalClient::Close();
            fFinish = NULL;
        }
    }
    
    return res;
}

} // end of namespace

