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

#ifndef __JackGlobals__
#define __JackGlobals__

#include "JackError.h"
#include "JackExports.h"

#include "JackPlatformSynchro.h"
#include "JackPlatformProcessSync.h"
#include "JackPlatformThread.h"


#ifdef __cplusplus
extern "C"
{
#endif

extern jack_tls_key gRealTime;
extern jack_tls_key g_key_log_function;

#ifdef WIN32

EXPORT void jack_init();
EXPORT void jack_uninit();

#else

void __attribute__ ((constructor)) jack_init();
void __attribute__ ((destructor)) jack_uninit();

#endif

#ifdef __cplusplus
}
#endif

#endif
