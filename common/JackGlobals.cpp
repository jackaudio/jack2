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

#include "JackGlobals.h"
#include "JackError.h"

namespace Jack
{

JackFactoryImpl* JackGlobals::fInstance;

void JackGlobals::InitServer()
{
	jack_log("JackGlobals InitServer");
	if (!fInstance) {
		
#ifdef __APPLE__
		fInstance = new JackFactoryOSXServer();
#endif
		
#ifdef WIN32
		fInstance = new JackFactoryWindowsServer();
#endif
		
#ifdef __linux__
		fInstance = new JackFactoryLinuxServer();
#endif
		
	}
}

void JackGlobals::InitClient()
{
	jack_log("JackGlobals InitClient");
	if (!fInstance) {
		
#ifdef __APPLE__
		fInstance = new JackFactoryOSXClient();
#endif
		
#ifdef WIN32
		fInstance = new JackFactoryWindowsClient();
#endif
		
#ifdef __linux__
		fInstance = new JackFactoryLinuxClient();
#endif
		
	}
}

void JackGlobals::Destroy()
{
	jack_log("JackGlobals Destroy");
	if (fInstance) {
		delete fInstance;
		fInstance = NULL;
	}
}
 
} // end of namespace

static bool gKeyRealtimeInitialized = false;
static bool g_key_log_function_initialized = false;

jack_tls_key gRealTime;
jack_tls_key g_key_log_function;

void jack_init()
{
    if (!gKeyRealtimeInitialized) {
        gKeyRealtimeInitialized = jack_tls_allocate_key(&gRealTime);
    }
    
    if (!g_key_log_function_initialized)
        g_key_log_function_initialized = jack_tls_allocate_key(&g_key_log_function);
}

void jack_uninit()
{
    if (gKeyRealtimeInitialized) {
        jack_tls_free_key(gRealTime);
        gKeyRealtimeInitialized = false;
    }
    
    if (g_key_log_function_initialized) {
        jack_tls_free_key(g_key_log_function);
        g_key_log_function_initialized = false;
    }
}

// Initialisation at library load time

#ifdef WIN32

BOOL WINAPI DllEntryPoint(HINSTANCE  hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            jack_init();
            break;
        case DLL_PROCESS_DETACH:
            jack_uninit();
            break;
    }
    return TRUE;
}

#endif
