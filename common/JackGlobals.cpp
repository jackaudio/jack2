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

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "JackGlobals.h"

static bool gKeyRealtimeInitialized = false;
static bool g_key_log_function_initialized = false;

jack_tls_key gRealTime;
jack_tls_key g_key_log_function;

// Initialisation at library load time
#ifdef WIN32

static void jack_init()
{
    if (!gKeyRealtimeInitialized) {
        gKeyRealtimeInitialized = jack_tls_allocate_key(&gRealTime);
    }
    
    if (!g_key_log_function_initialized)
        g_key_log_function_initialized = jack_tls_allocate_key(&g_key_log_function);
}

static void jack_uninit()
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

#ifdef __cplusplus
extern "C"
{
#endif

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

#ifdef __cplusplus
}
#endif

#else

__attribute__ ((constructor))
static void jack_init()
{
    if (!gKeyRealtimeInitialized) {
        gKeyRealtimeInitialized = jack_tls_allocate_key(&gRealTime);
    }
    
    if (!g_key_log_function_initialized)
        g_key_log_function_initialized = jack_tls_allocate_key(&g_key_log_function);
}

__attribute__ ((destructor))
static void jack_uninit()
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

#endif
