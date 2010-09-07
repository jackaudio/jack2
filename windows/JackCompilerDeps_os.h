/*
 Copyright (C) 2004-2008 Grame
 
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


#ifndef __JackCompilerDeps_WIN32__
#define __JackCompilerDeps_WIN32__

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
	#define	EXPORT __declspec(dllexport)
    #ifdef SERVER_SIDE
        #define	SERVER_EXPORT __declspec(dllexport)
    #else
        #define	SERVER_EXPORT
    #endif
#else
	#define MEM_ALIGN(x,y) x
	#define	EXPORT __declspec(dllexport)
    #ifdef SERVER_SIDE
        #define	SERVER_EXPORT __declspec(dllexport)
    #else
        #define	SERVER_EXPORT
    #endif
#endif

#endif
