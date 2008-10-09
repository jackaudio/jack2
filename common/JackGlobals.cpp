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

#include "JackGlobals.h"

jack_tls_key gRealTime;
static bool gKeyRealtimeInitialized = jack_tls_allocate_key(&gRealTime);

jack_tls_key g_key_log_function;
static bool g_key_log_function_initialized = jack_tls_allocate_key(&g_key_log_function);

JackMutex* gOpenMutex = new JackMutex();
