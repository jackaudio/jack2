/*
Copyright (C) 2004-2005 Grame

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

#ifndef __JackCompilerDeps_POSIX__
#define __JackCompilerDeps_POSIX__

#include "JackConstants.h"

#if __GNUC__
    #ifndef POST_PACKED_STRUCTURE
        /* POST_PACKED_STRUCTURE needs to be a macro which
        expands into a compiler directive. The directive must
        tell the compiler to arrange the preceding structure
        declaration so that it is packed on byte-boundaries rather
        than use the natural alignment of the processor and/or
        compiler.
        */
        #if (__GNUC__< 4)  /* Does not seem to work with GCC 3.XX serie */
            #define POST_PACKED_STRUCTURE
        #elif defined(JACK_32_64)
            #define POST_PACKED_STRUCTURE __attribute__((__packed__))
        #else
            #define POST_PACKED_STRUCTURE
        #endif
    #endif
    #define MEM_ALIGN(x,y) x __attribute__((aligned(y)))
    #define EXPORT __attribute__((visibility("default")))
    #ifdef SERVER_SIDE
        #if (__GNUC__< 4)
            #define SERVER_EXPORT
        #else
            #define SERVER_EXPORT __attribute__((visibility("default")))
        #endif
    #else
        #define SERVER_EXPORT __attribute__((visibility("hidden")))
    #endif
#else
    #define MEM_ALIGN(x,y) x
    #define EXPORT
    #define SERVER_EXPORT
    /* Add other things here for non-gcc platforms for POST_PACKED_STRUCTURE */
#endif

#endif

