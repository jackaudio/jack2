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

#define	LIB_EXPORT __declspec(dllexport)

#ifdef SERVER_SIDE
    #define	SERVER_EXPORT __declspec(dllexport)
#else
    #define	SERVER_EXPORT
#endif

#if __GNUC__

    #define MEM_ALIGN(x,y) x __attribute__((aligned(y)))
    
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
	
#else

    #define MEM_ALIGN(x,y) x
 
    #ifdef _MSC_VER
        #define PRE_PACKED_STRUCTURE1 __pragma(pack(push,1))
        #define PRE_PACKED_STRUCTURE    PRE_PACKED_STRUCTURE1
        /* PRE_PACKED_STRUCTURE needs to be a macro which
        expands into a compiler directive. The directive must
        tell the compiler to arrange the following structure
        declaration so that it is packed on byte-boundaries rather
        than use the natural alignment of the processor and/or
        compiler.
        */
        #define POST_PACKED_STRUCTURE ;__pragma(pack(pop))
        /* and POST_PACKED_STRUCTURE needs to be a macro which
        restores the packing to its previous setting */
    #else
        /* Other Windows compilers to go here */
        #define PRE_PACKED_STRUCTURE
        #define POST_PACKED_STRUCTURE
    #endif
    
#endif

#if defined(_MSC_VER) /* Added by JE - 31-01-2012 */
#define snprintf _snprintf
#endif

#endif
