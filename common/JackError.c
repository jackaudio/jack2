/*
    Copyright (C) 2001 Paul Davis 
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

#include <stdarg.h>
#include <stdio.h>
#include "JackError.h"

int jack_verbose = 0;

EXPORT void jack_error (const char *fmt, ...)
{
	va_list ap;
	char buffer[300];
	va_start(ap, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, ap);
	//jack_error_callback(buffer);
	fprintf(stderr, "%s\n", buffer);
	va_end(ap);
}

EXPORT void JackLog(char *fmt,...)
{
#ifdef PRINTDEBUG
	if (jack_verbose) {
		va_list ap;
		va_start(ap, fmt);
		fprintf(stderr,"Jack: ");
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
#endif
}

