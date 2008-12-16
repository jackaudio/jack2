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

#include <stdarg.h>
#include <stdio.h>
#include "JackError.h"
#include "JackGlobals.h"
#include "JackMessageBuffer.h"

int jack_verbose = 0;

using namespace Jack;

void change_thread_log_function(jack_log_function_t log_function)
{
    if (!jack_tls_set(JackGlobals::fKeyLogFunction, (void*)log_function))
    {
        jack_error("failed to set thread log function");
    }
}

SERVER_EXPORT void set_threaded_log_function()
{
    change_thread_log_function(JackMessageBufferAdd);
}

void jack_log_function(int level, const char *message)
{
    void (* log_callback)(const char *);

    switch (level)
    {
    case LOG_LEVEL_INFO:
        log_callback = jack_info_callback;
        break;
    case LOG_LEVEL_ERROR:
        log_callback = jack_error_callback;
        break;
    default:
        return;
    }

    log_callback(message);
}

static void jack_format_and_log(int level, const char *prefix, const char *fmt, va_list ap)
{
    char buffer[300];
    size_t len;
    jack_log_function_t log_function;

    if (prefix != NULL) {
        len = strlen(prefix);
        memcpy(buffer, prefix, len);
    } else {
        len = 0;
    }

    vsnprintf(buffer + len, sizeof(buffer) - len, fmt, ap);

    log_function = (jack_log_function_t)jack_tls_get(JackGlobals::fKeyLogFunction);

    /* if log function is not overriden for thread, use default one */
    if (log_function == NULL)
    {
        log_function = jack_log_function;
        //log_function(LOG_LEVEL_INFO, "------ Using default log function");
    }
    else
    {
        //log_function(LOG_LEVEL_INFO, "++++++ Using thread-specific log function");
    }

    log_function(level, buffer);
}

SERVER_EXPORT void jack_error(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	jack_format_and_log(LOG_LEVEL_ERROR, NULL, fmt, ap);
	va_end(ap);
}

SERVER_EXPORT void jack_info(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	jack_format_and_log(LOG_LEVEL_INFO, NULL, fmt, ap);
	va_end(ap);
}

SERVER_EXPORT void jack_log(const char *fmt,...)
{
	if (jack_verbose) {
		va_list ap;
		va_start(ap, fmt);
        jack_format_and_log(LOG_LEVEL_INFO, "Jack: ", fmt, ap);
		va_end(ap);
	}
}

static void default_jack_error_callback(const char *desc)
{
    fprintf(stderr, "%s\n", desc);
    fflush(stderr);
}

static void default_jack_info_callback (const char *desc)
{
    fprintf(stdout, "%s\n", desc);
    fflush(stdout);
}

SERVER_EXPORT void (*jack_error_callback)(const char *desc) = &default_jack_error_callback;
SERVER_EXPORT void (*jack_info_callback)(const char *desc) = &default_jack_info_callback;
