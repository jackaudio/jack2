/*
 Copyright (C) 2001 Paul Davis
 Copyright (C) 2004-2008 Grame
 Copyright (C) 2008 Nedko Arnaudov

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

#ifndef __JackError__
#define __JackError__

#include <string.h>
#include <errno.h>
#include "JackCompilerDeps.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define LOG_LEVEL_INFO   1
#define LOG_LEVEL_ERROR  2

    EXPORT void jack_error(const char *fmt, ...);

    EXPORT void jack_info(const char *fmt, ...);

    // like jack_info() but only if verbose mode is enabled
    EXPORT void jack_log(const char *fmt, ...);

    EXPORT extern void (*jack_error_callback)(const char *desc);
    EXPORT extern void (*jack_info_callback)(const char *desc);

    EXPORT extern void default_jack_error_callback(const char *desc);
    EXPORT extern void default_jack_info_callback(const char *desc);

    EXPORT extern void silent_jack_error_callback(const char *desc);
    EXPORT extern void silent_jack_info_callback(const char *desc);

    typedef void (* jack_log_function_t)(int level, const char *message);

    void jack_log_function(int level, const char *message);

    EXPORT int set_threaded_log_function();

#ifdef __cplusplus
}
#endif

#endif
