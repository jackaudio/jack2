/*
Copyright (C) 2001 Paul Davis
Copyright (C) 2004-2008 Grame
Copyright (C) 2008 Nedko Arnaudov

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

#ifndef __JackError__
#define __JackError__

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "JackExports.h"
#include "types.h"

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

    typedef void (* jack_log_function_t)(int level, const char *message);

    void change_thread_log_function(jack_log_function_t log_function);
    void jack_log_function(int level, const char *message);
   
    EXPORT void set_threaded_log_function();

	extern int jack_verbose;

#ifdef __cplusplus
}
#endif

#endif
