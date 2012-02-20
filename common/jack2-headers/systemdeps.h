/*
Copyright (C) 2004-2009 Grame

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
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

#if defined(WIN32) && !defined(__CYGWIN__) && !defined(GNU_WIN32)

#include <windows.h>

#ifdef _MSC_VER     /* Microsoft compiler */
    #define __inline__ inline
    #if (!defined(int8_t) && !defined(_STDINT_H))
        #define __int8_t_defined
        typedef char int8_t;
        typedef unsigned char uint8_t;
        typedef short int16_t;
        typedef unsigned short uint16_t;
        typedef long int32_t;
        typedef unsigned long uint32_t;
        typedef LONGLONG int64_t;
        typedef ULONGLONG uint64_t;
    #endif
#elif __MINGW32__   /* MINGW */
    #include <stdint.h>
    #include <sys/types.h>
#else               /* other compilers ...*/
    #include <inttypes.h>
    #include <pthread.h>
    #include <sys/types.h>
#endif

#if !defined(_PTHREAD_H) && !defined(PTHREAD_WIN32)
    /**
     *  to make jack API independent of different thread implementations,
     *  we define jack_native_thread_t to HANDLE here.
     */
    typedef HANDLE jack_native_thread_t;
#else
    #ifdef PTHREAD_WIN32            // Added by JE - 10-10-2011
        #include <ptw32/pthread.h>  // Makes sure we #include the ptw32 version !
    #endif
    /**
     *  to make jack API independent of different thread implementations,
     *  we define jack_native_thread_t to pthread_t here.
     */
    typedef pthread_t jack_native_thread_t;
#endif

#endif // WIN32 && !__CYGWIN__ && !GNU_WIN32 */

#if defined(__APPLE__) || defined(__linux__) || defined(__sun__) || defined(sun) || defined(__unix__) || defined(__CYGWIN__) || defined(GNU_WIN32)

#if defined(__CYGWIN__) || defined(GNU_WIN32)
    #include <stdint.h>
#endif
    #include <inttypes.h>
    #include <pthread.h>
    #include <sys/types.h>

    /**
     *  to make jack API independent of different thread implementations,
     *  we define jack_native_thread_t to pthread_t here.
     */
    typedef pthread_t jack_native_thread_t;

#endif /* __APPLE__ || __linux__ || __sun__ || sun */

#endif
