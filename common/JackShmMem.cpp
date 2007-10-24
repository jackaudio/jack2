/*
Copyright (C) 2001 Paul Davis 
Copyright (C) 2004-2006 Grame

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

#include "JackShmMem.h"
#include "JackError.h"
#include <stdio.h>

namespace Jack
{

unsigned long JackShmMem::fSegmentNum = 0;
jack_shm_info_t JackShmMem::gInfo;
size_t JackMem::gSize = 0;

void* JackShmMem::operator new(size_t size)
{
    jack_shm_info_t info;
    JackShmMem* obj;
    char name[64];

    snprintf(name, sizeof(name), "/jack_shared%ld", JackShmMem::fSegmentNum++);

    if (jack_shmalloc(name, size, &info)) {
        jack_error("cannot create shared memory segment of size = %d", size, strerror(errno));
        goto error;
    }

    if (jack_attach_shm(&info)) {
        jack_error("cannot attach shared memory segment name = %s err = %s", name, strerror(errno));
        jack_destroy_shm(&info);
        goto error;
    }

    obj = (JackShmMem*)jack_shm_addr(&info);
    // It is unsafe to set object fields directly (may be overwritten during object initialization),
    // so use an intermediate global data
    gInfo.index = info.index;
	gInfo.size = size;
    gInfo.attached_at = info.attached_at;
	
    JackLog("JackShmMem::new index = %ld attached = %x size = %ld \n", info.index, info.attached_at, size);
    return obj;

error:
    jack_error("JackShmMem::new bad alloc", size);
    throw new std::bad_alloc;
}

void JackShmMem::operator delete(void* p, size_t size)
{
    jack_shm_info_t info;
    JackShmMem* obj = (JackShmMem*)p;
    info.index = obj->fInfo.index;
    info.attached_at = obj->fInfo.attached_at;

    JackLog("JackShmMem::delete size = %ld index = %ld\n", size, info.index);

    jack_release_shm(&info);
    jack_destroy_shm(&info);
}

void LockMemoryImp(void* ptr, size_t size) 
{
	if (CHECK_MLOCK(ptr, size)) {
		JackLog("Succeeded in locking %u byte memory area\n", size);		
	} else {
		jack_error("Cannot lock down memory area (%s)", strerror(errno));
	}
}

void UnlockMemoryImp(void* ptr, size_t size) 
{
	if (CHECK_MUNLOCK(ptr, size)) {
		JackLog("Succeeded in unlocking %u byte memory area\n", size);		
	} else {
		jack_error("Cannot unlock down memory area (%s)", strerror(errno));
	}
}

void LockAllMemory() 
{
	if (CHECK_MLOCKALL()) {
		JackLog("Succeeded in locking all memory\n");		
	} else {
		jack_error("Cannot lock down memory area (%s)", strerror(errno));
	}
}

void UnlockAllMemory() 
{
	if (CHECK_MUNLOCKALL()) {
		JackLog("Succeeded in unlocking all memory\n");		
	} else {
		jack_error("Cannot unlock down memory area (%s)", strerror(errno));
	}
}


} // end of namespace

