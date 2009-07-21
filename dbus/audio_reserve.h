/*
    Copyright (C) 2009 Grame
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef __audio_reserve__
#define __audio_reserve__

#include "JackCompilerDeps.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

SERVER_EXPORT int audio_reservation_init();
SERVER_EXPORT int audio_reservation_finish();

SERVER_EXPORT bool audio_acquire(const char * device_name);
SERVER_EXPORT void audio_release(const char * device_name);
SERVER_EXPORT void audio_reserve_loop();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
