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

#ifndef __JackAtomic_android__
#define __JackAtomic_android__

#include "JackTypes.h"

#include <sys/atomics.h>
static inline char CAS(volatile UInt32 value, UInt32 newvalue, volatile void* addr)
{
#if 1
    return !__atomic_cmpxchg(value, newvalue, (volatile int *)addr);
#else
    //slow compare_and_swap_32
    if (*(UInt32*)addr == value) {
        *(UInt32*)addr = newvalue;
        return true;
    }
    return false;
#endif
}

#endif

