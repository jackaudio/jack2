/*
    Copyright (C) 2001 Paul Davis 
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

#include <stdarg.h>
#include <stdio.h>
#include "JackError.h"

int jack_verbose = 0;

static
void 
jack_format_and_log(const char *prefix, const char *fmt, va_list ap, void (* log_callback)(const char *))
{
    char buffer[300];
    size_t len;

    if (prefix != NULL) {
        len = strlen(prefix);
        memcpy(buffer, prefix, len);
    } else {
        len = 0;
    }

    vsnprintf(buffer + len, sizeof(buffer) - len, fmt, ap);
    log_callback(buffer);
}

EXPORT void jack_error(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	jack_format_and_log(NULL, fmt, ap, jack_error_callback);
	va_end(ap);
}

EXPORT void jack_info(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	jack_format_and_log(NULL, fmt, ap, jack_info_callback);
	va_end(ap);
}

EXPORT void jack_info_multiline(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	jack_format_and_log(NULL, fmt, ap, jack_info_callback);
	va_end(ap);
}

EXPORT void jack_log(const char *fmt,...)
{
	if (jack_verbose) {
		va_list ap;
		va_start(ap, fmt);
        jack_format_and_log("Jack: ", fmt, ap, jack_info_callback);
		va_end(ap);
	}
}

static void default_jack_error_callback(const char *desc)
{
    fprintf(stderr, "%s\n", desc);
    fflush(stdout);
}

static void default_jack_info_callback (const char *desc)
{
    fprintf(stdout, "%s\n", desc);
    fflush(stdout);
}

void (*jack_error_callback)(const char *desc) = &default_jack_error_callback;
void (*jack_info_callback)(const char *desc) = &default_jack_info_callback;
