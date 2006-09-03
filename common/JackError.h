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

#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "JackConstants.h"
#include "JackExports.h"


#ifdef __cplusplus
extern "C"
{
#endif

#ifdef WIN32
	#define vsnprintf _vsnprintf
	#define snprintf _snprintf
#endif

    EXPORT void jack_error(const char *fmt, ...);

    EXPORT void JackLog(char *fmt, ...);

    extern int verbose;

#ifdef __cplusplus
}
#endif
