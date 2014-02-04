/*
 Copyright (C) 2004-2008 Grame
 Copyright (C) 2013 Samsung Electronics

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

#ifndef __JackCompilerDeps_android__
#define __JackCompilerDeps_android__

#include "JackConstants.h"

#if __GNUC__

    #define MEM_ALIGN(x,y) x __attribute__((aligned(y)))
    #define LIB_EXPORT __attribute__((visibility("default")))
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
    #define LIB_EXPORT
    #define SERVER_EXPORT

    /* Add other things here for non-gcc platforms */
#endif

#endif /* __JackCompilerDeps_android__ */
