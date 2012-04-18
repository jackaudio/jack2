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

#ifndef __JackCompilerDeps_APPLE__
#define __JackCompilerDeps_APPLE__

#include "JackConstants.h"

#if __GNUC__

    #define PRE_PACKED_STRUCTURE

    #ifndef POST_PACKED_STRUCTURE
        /* POST_PACKED_STRUCTURE needs to be a macro which
        expands into a compiler directive. The directive must
        tell the compiler to arrange the preceding structure
        declaration so that it is packed on byte-boundaries rather
        than use the natural alignment of the processor and/or
        compiler.
        */
        #define POST_PACKED_STRUCTURE __attribute__((__packed__))
    #endif
    #define MEM_ALIGN(x,y) x __attribute__((aligned(y)))
    #define LIB_EXPORT __attribute__((visibility("default")))
    #ifdef SERVER_SIDE
        #define SERVER_EXPORT __attribute__((visibility("default")))
    #else
        #define SERVER_EXPORT
    #endif
#else
    #define MEM_ALIGN(x,y) x
    #define LIB_EXPORT
    #define SERVER_EXPORT
    /* Add other things here for non-gcc platforms for PRE and POST_PACKED_STRUCTURE */
#endif

#endif

