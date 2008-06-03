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

#ifndef __JackInternalClient__
#define __JackInternalClient__

#include "JackClient.h"

namespace Jack
{

struct JackEngineControl;

/*!
\brief Internal clients in the server.
*/

class JackInternalClient : public JackClient
{

    private:

        JackClientControl* fClientControl;     /*! Client control */

    public:

        JackInternalClient(JackServer* server, JackSynchro* table);
        virtual ~JackInternalClient();

        int Open(const char* server_name, const char* name, jack_options_t options, jack_status_t* status);

        JackGraphManager* GetGraphManager() const;
        JackEngineControl* GetEngineControl() const;
        JackClientControl* GetClientControl() const;

        static JackGraphManager* fGraphManager;		/*! Shared memory Port manager */
        static JackEngineControl* fEngineControl;	/*! Shared engine cotrol */
};

/*!
\brief Loadable internal clients in the server.
*/

#ifdef WIN32

#include <windows.h>
#define HANDLE HINSTANCE
#define LoadJackModule(name) LoadLibrary((name));
#define UnloadJackModule(handle) FreeLibrary((handle));
#define GetJackProc(handle, name) GetProcAddress((handle), (name));

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

#include <dlfcn.h>
#define HANDLE void*
#define LoadJackModule(name) dlopen((name), RTLD_NOW | RTLD_LOCAL);
#define UnloadJackModule(handle) dlclose((handle));
#define GetJackProc(handle, name) dlsym((handle), (name));
#define PrintLoadError(so_name) jack_log("error loading %s err = %s", so_name, dlerror());

#endif

typedef int (*InitializeCallback)(jack_client_t*, const char*);
typedef void (*FinishCallback)(void *);

class JackLoadableInternalClient : public JackInternalClient
{

    private:

        HANDLE fHandle;
        InitializeCallback fInitialize;
        FinishCallback fFinish;
        char fObjectData[JACK_LOAD_INIT_LIMIT];

    public:

        JackLoadableInternalClient(JackServer* server, JackSynchro* table, const char* so_name, const char* object_data);
        virtual ~JackLoadableInternalClient();

        int Open(const char* server_name, const char* name, jack_options_t options, jack_status_t* status);

};


} // end of namespace

#endif
