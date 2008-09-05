/*
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

#ifndef __jack_systemdeps_h__
#define __jack_systemdeps_h__

#ifdef WIN32

#include <windows.h>

#ifdef __MINGW32__
	#include <stdint.h>
    #include <sys/types.h>
#else
	#define __inline__ inline
	#define vsnprintf _vsnprintf
    #define snprintf _snprintf

   	typedef char int8_t;
	typedef unsigned char uint8_t;
	typedef short int16_t;
	typedef unsigned short uint16_t;
	typedef long int32_t;
	typedef unsigned long uint32_t;
	typedef LONGLONG int64_t;
	typedef ULONGLONG uint64_t;
#endif

	typedef HANDLE pthread_t;
    typedef int64_t _jack_time_t;

#endif // WIN32 */

#if defined(__APPLE__) || defined(__linux__)

#include <inttypes.h>
#include <pthread.h>
#include <sys/types.h>

typedef uint64_t _jack_time_t;

#endif // __APPLE__ || __linux__ */

#endif
