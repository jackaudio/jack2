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

#ifndef __JackShmMem__android__
#define __JackShmMem__android__

#include <sys/types.h>
#include <sys/mman.h>

#define CHECK_MLOCK(ptr, size) (mlock((ptr), (size)) == 0)
#define CHECK_MUNLOCK(ptr, size) (munlock((ptr), (size)) == 0)
#define CHECK_MLOCKALL() (false)
#define CHECK_MUNLOCKALL() (false)

/* fix for crash jack server issue:
 * case 1) jack_destroy_shm() in JackShmReadWritePtr1::Init() causes crash
 * because server lost shared memory by destroying client side ahead.
 */
#ifndef SERVER_SIDE
#define jack_destroy_shm(x)  (0)
#endif

#endif /* __JackShmMem__android__ */
