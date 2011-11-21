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


#ifndef __JackSystemDeps_WIN32__
#define __JackSystemDeps_WIN32__

#include <windows.h>

#ifndef PATH_MAX
#define PATH_MAX   512
#endif

#define UINT32_MAX 4294967295U

#define DRIVER_HANDLE HINSTANCE
#define LoadDriverModule(name) LoadLibrary((name))
#define UnloadDriverModule(handle) (FreeLibrary(((HMODULE)handle)))
#define GetDriverProc(handle, name) GetProcAddress(((HMODULE)handle), (name))

#define JACK_HANDLE HINSTANCE
#define LoadJackModule(name) LoadLibrary((name));
#define UnloadJackModule(handle) FreeLibrary((handle));
#define GetJackProc(handle, name) GetProcAddress((handle), (name));

#ifndef ENOBUFS
#define ENOBUFS 55
#endif

#ifdef _DEBUG
#define JACK_DEBUG true
#else
#define JACK_DEBUG false
#endif

#if defined(_MSC_VER)
#define snprintf _snprintf
#endif

#endif

