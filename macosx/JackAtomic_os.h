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

#ifndef __JackAtomic_APPLE__
#define __JackAtomic_APPLE__

#include "JackTypes.h"

#if defined(__ppc__) || defined(__ppc64__)

static inline int CAS(register UInt32 value, register UInt32 newvalue, register volatile void* addr)
{
    register int result;
    asm volatile (
        "# CAS					\n"
        "	lwarx	r0, 0, %1	\n"         // creates a reservation on addr
        "	cmpw	r0, %2		\n"			//  test value at addr
        "	bne-	1f          \n"
        "	sync            	\n"         //  synchronize instructions
        "	stwcx.	%3, 0, %1	\n"         //  if the reservation is not altered
        //  stores the new value at addr
        "	bne-	1f          \n"
        "   li      %0, 1       \n"
        "	b		2f          \n"
        "1:                     \n"
        "   li      %0, 0       \n"
        "2:                     \n"
        : "=r" (result)
        : "r" (addr), "r" (value), "r" (newvalue)
        : "r0"
        );
    return result;
}

#endif

#if defined(__i386__) || defined(__x86_64__)

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

