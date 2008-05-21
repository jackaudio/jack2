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

namespace Jack
{

    JackFactoryImpl* JackGlobals::fInstance;
 
} // end of namespace

static bool gKeyRealtimeInitialized = false;
jack_tls_key gRealTime;

void jack_init()
{
    if (!gKeyRealtimeInitialized) {
        gKeyRealtimeInitialized = jack_tls_allocate_key(&gRealTime);
    }
}

void jack_uninit()
{
    if (gKeyRealtimeInitialized) {
        jack_tls_free_key(gRealTime);
        gKeyRealtimeInitialized = false;
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
