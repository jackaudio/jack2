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

#ifndef __JackAtomic_WIN32__
#define __JackAtomic_WIN32__

#include "JackTypes.h"

#ifndef __MINGW32__
#ifdef __SMP__
#	define LOCK lock
#else
#	define LOCK
#endif

#ifndef inline
	#define inline __inline
#endif

//----------------------------------------------------------------
// CAS functions
//----------------------------------------------------------------
inline char CAS(volatile UInt32 value, UInt32 newvalue, volatile void * addr)
{
    register char c;
    __asm {
        push	ebx
        push	esi
        mov	esi, addr
        mov	eax, value
        mov	ebx, newvalue
        LOCK cmpxchg dword ptr [esi], ebx
        sete	c
        pop	esi
        pop	ebx
    }
    return c;
}

#else

#define LOCK "lock ; "

static inline char CAS(volatile UInt32 value, UInt32 newvalue, volatile void* addr)
{
    register char ret;
    __asm__ __volatile__ (
        "# CAS \n\t"
        LOCK "cmpxchg %2, (%1) \n\t"
        "sete %0               \n\t"
    : "=a" (ret)
                : "c" (addr), "d" (newvalue), "a" (value)
            );
    return ret;
}

#endif

#endif

