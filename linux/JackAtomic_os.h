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

#ifndef __JackAtomic_linux__
#define __JackAtomic_linux__

#include "JackTypes.h"

#ifdef __PPC__

static inline int CAS(register UInt32 value, register UInt32 newvalue, register volatile void* addr)
{
    register int result;
    register UInt32 tmp;
    asm volatile (
        "# CAS					\n"
        "	lwarx	%4, 0, %1	\n"         // creates a reservation on addr
        "	cmpw	%4, %2		\n"        //  test value at addr
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
        : "r" (addr), "r" (value), "r" (newvalue), "r" (tmp)
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



#if defined(__thumb__)
/*
 * This Compare And Swap code is based off the version found
 * in MutekH, http://www.mutekh.org/trac/mutekh
 *
 * Copyright Alexandre Becoulet <alexandre.becoulet@lip6.fr> (c) 2006
 */

static inline char CAS(volatile UInt32 value, UInt32 newvalue, volatile void* addr)
{
  UInt32 tmp, loaded;
  UInt32 thumb_tmp;

  asm volatile(
    ".align 2                                  \n\t"
    "mov  %[adr], pc                           \n\t"
    "add  %[adr], %[adr], #4                   \n\t"
    "bx   %[adr]                               \n\t"
    "nop                                       \n\t"
    ".arm                                      \n\t"
    "1:                                        \n\t"
    "ldrex   %[loaded], [%[atomic]]            \n\t"
    "cmp     %[loaded], %[value]               \n\t"
    "bne     2f                                \n\t"
    "strex   %[tmp], %[newvalue], [%[atomic]]  \n\t"
    "tst     %[tmp], #1                        \n\t"
    "bne     1b                                \n\t"
    "2:                                        \n\t"
    "add  %[adr], pc, #1                       \n\t"
    "bx   %[adr]                               \n\t"
    : [tmp] "=&r" (tmp), [loaded] "=&r" (loaded), "=m" (*(volatile UInt32*)addr)
    , [adr] "=&l" (thumb_tmp)
    : [value] "r" (value), [newvalue] "r" (newvalue), [atomic] "r" (addr)
    );

  return loaded == value;
}

#endif



#if !defined(__i386__) && !defined(__x86_64__)  && !defined(__PPC__) && !defined(__thumb__)
#warning using builtin gcc (version > 4.1) atomic

static inline char CAS(volatile UInt32 value, UInt32 newvalue, volatile void* addr)
{
    return __sync_bool_compare_and_swap (&addr, value, newvalue);
}
#endif


#endif

