/*
Copyright (C) 2001-2005 Paul Davis
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

#ifndef __JackDriverLoader__
#define __JackDriverLoader__

#include "jslist.h"
#include "driver_interface.h"
#include "JackDriver.h"


#ifdef WIN32

#include <windows.h>
#define DRIVER_HANDLE HINSTANCE
#define LoadDriverModule(name) LoadLibrary((name))
#define UnloadDriverModule(handle) (FreeLibrary(((HMODULE)handle)))
#define GetProc(handle, name) GetProcAddress(((HMODULE)handle),(name))

#else

#include <dlfcn.h>
#define DRIVER_HANDLE void*
#define LoadDriverModule(name) dlopen((name), RTLD_NOW | RTLD_GLOBAL)
#define UnloadDriverModule(handle) dlclose((handle))
#define GetProc(handle, name) dlsym((handle), (name))

#endif

typedef jack_driver_desc_t * (*JackDriverDescFunction) ();
typedef Jack::JackDriverClientInterface* (*initialize) (Jack::JackEngine*, Jack::JackSynchro**, const JSList *);

typedef struct _jack_driver_info
{
    Jack::JackDriverClientInterface* (*initialize)(Jack::JackEngine*, Jack::JackSynchro**, const JSList *);
    DRIVER_HANDLE handle;
}
jack_driver_info_t;

EXPORT jack_driver_desc_t * jack_find_driver_descriptor (JSList * drivers, const char * name);

jack_driver_desc_t * jack_drivers_get_descriptor (JSList * drivers, const char * sofile);

EXPORT JSList * jack_drivers_load (JSList * drivers);

jack_driver_info_t * jack_load_driver (jack_driver_desc_t * driver_desc);

#endif

