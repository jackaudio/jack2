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

#ifndef __JackShmMem__
#define __JackShmMem__

#include "shm.h"
#include "JackError.h"

#include <new>  // GCC 4.0
#include <errno.h> 
#include <stdlib.h>

#ifdef WIN32
	#include <windows.h>
	#define CHECK_MLOCK(ptr, size) (VirtualLock((ptr), (size)) != 0)
	#define CHECK_MUNLOCK(ptr, size) (VirtualUnlock((ptr), (size)) != 0)
	#define CHECK_MLOCKALL()(false)
	#define CHECK_MUNLOCKALL()(false)
#else
	#include <sys/types.h>
	#include <sys/mman.h>
	#define CHECK_MLOCK(ptr, size) (mlock((ptr), (size)) == 0)
	#define CHECK_MUNLOCK(ptr, size) (munlock((ptr), (size)) == 0)
	#define CHECK_MLOCKALL() (mlockall(MCL_CURRENT | MCL_FUTURE) == 0)
	#define CHECK_MUNLOCKALL() (munlockall() == 0)
#endif
  
namespace Jack
{

void LockMemoryImp(void* ptr, size_t size);
void UnlockMemoryImp(void* ptr, size_t size);

class JackMem
{
	private:

		size_t fSize;
		static size_t gSize;
	
	public:

        void* operator new(size_t size)
		{	
			gSize = size;
			return calloc(1, size);
		}
		
        void operator delete(void* ptr, size_t size)
		{	
			free(ptr);
		}

        JackMem():fSize(gSize)
        {}

        virtual ~JackMem()
        {
			UnlockMemoryImp(this, fSize);
		}
		
		void LockMemory()
		{
			LockMemoryImp(this, fSize);
		}
		
		void UnlockMemory()
		{
			UnlockMemoryImp(this, fSize);
		}

};

/*!
\brief The base class for shared memory management.
 
A class which objects need to be allocated in shared memory derives from this class.
*/

class JackShmMem 
{

    protected:

        jack_shm_info_t fInfo;
        static unsigned long fSegmentNum;
        static unsigned long fSegmentCount;
        static jack_shm_info_t gInfo;

    public:

        void* operator new(size_t size);
        void operator delete(void* p, size_t size);

        JackShmMem()
        {
            fInfo.index = gInfo.index;
			fInfo.attached_at = gInfo.attached_at;
			fInfo.size = gInfo.size;
		}

        virtual ~JackShmMem()
        {
			UnlockMemoryImp(this, fInfo.size);
		}

        int GetShmIndex()
        {
            return fInfo.index;
        }

        char* GetShmAddress()
        {
            return (char*)fInfo.attached_at;
        }
		
		void LockMemory()
		{
			LockMemoryImp(this, fInfo.size);
		}
		
		void UnlockMemory()
		{
			UnlockMemoryImp(this, fInfo.size);
		}

};

/*!
\brief Pointer on shared memory segment in the client side.
*/

template <class T>
class JackShmReadWritePtr
{

    private:

        jack_shm_info_t fInfo;

        void Init(int index)
        {
            if (fInfo.index < 0 && index >= 0) {
                JackLog("JackShmReadWritePtr::Init %ld %ld\n", index, fInfo.index);
                if (jack_initialize_shm_client() < 0)
                    throw - 1;
                fInfo.index = index;
                if (jack_attach_shm(&fInfo)) {
                    //jack_error("cannot attach shared memory segment", strerror(errno));
                    throw - 2;
                }
            }
        }

    public:

        JackShmReadWritePtr()
        {
            fInfo.index = -1;
            fInfo.attached_at = NULL;
        }

        JackShmReadWritePtr(int index)
        {
            Init(index);
        }

        virtual ~JackShmReadWritePtr()
        {
            if (fInfo.index >= 0) {
                JackLog("JackShmReadWritePtr::~JackShmReadWritePtr %ld\n", fInfo.index);
                jack_release_shm(&fInfo);
                fInfo.index = -1;
            }
        }

        T* operator->() const
        {
            return (T*)fInfo.attached_at;
        }

        operator T*() const
        {
            return (T*)fInfo.attached_at;
        }

        JackShmReadWritePtr& operator=(int index)
        {
            Init(index);
            return *this;
        }

        int GetShmIndex()
        {
            return fInfo.index;
        }

        T* GetShmAddress()
        {
            return (T*)fInfo.attached_at;
        }
};

/*!
\brief Pointer on shared memory segment in the client side: destroy the segment (used client control)
*/

template <class T>
class JackShmReadWritePtr1
{

    private:

        jack_shm_info_t fInfo;

        void Init(int index)
        {
            if (fInfo.index < 0 && index >= 0) {
                JackLog("JackShmReadWritePtr1::Init %ld %ld\n", index, fInfo.index);
                if (jack_initialize_shm_client() < 0)
                    throw - 1;
                fInfo.index = index;
                if (jack_attach_shm(&fInfo)) {
                    //jack_error("cannot attach shared memory segment", strerror(errno));
                    throw - 2;
                }
                /*
                nobody else needs to access this shared memory any more, so
                destroy it. because we have our own attachment to it, it won't
                vanish till we exit (and release it).
                */
                jack_destroy_shm(&fInfo);
            }
        }

    public:

        JackShmReadWritePtr1()
        {
            fInfo.index = -1;
            fInfo.attached_at = NULL;
        }

        JackShmReadWritePtr1(int index)
        {
            Init(index);
        }

        virtual ~JackShmReadWritePtr1()
        {
            if (fInfo.index >= 0) {
                JackLog("JackShmReadWritePtr1::~JackShmReadWritePtr1 %ld\n", fInfo.index);
                jack_release_shm(&fInfo);
                fInfo.index = -1;
            }
        }

        T* operator->() const
        {
            return (T*)fInfo.attached_at;
        }

        operator T*() const
        {
            return (T*)fInfo.attached_at;
        }

        JackShmReadWritePtr1& operator=(int index)
        {
            Init(index);
            return *this;
        }

        int GetShmIndex()
        {
            return fInfo.index;
        }

        T* GetShmAddress()
        {
            return (T*)fInfo.attached_at;
        }
};

/*!
\brief Pointer on shared memory segment in the client side.
*/

template <class T>
class JackShmReadPtr
{

    private:

        jack_shm_info_t fInfo;

        void Init(int index)
        {
            if (fInfo.index < 0 && index >= 0) {
                JackLog("JackShmPtrRead::Init %ld %ld\n", index, fInfo.index);
                if (jack_initialize_shm_client() < 0)
                    throw - 1;
                fInfo.index = index;
                if (jack_attach_shm_read(&fInfo)) {
                    //jack_error("cannot attach shared memory segment", strerror(errno));
                    throw - 2;
                }
            }
        }

    public:

        JackShmReadPtr()
        {
            fInfo.index = -1;
            fInfo.attached_at = NULL;
        }

        JackShmReadPtr(int index)
        {
            Init(index);
        }

        virtual ~JackShmReadPtr()
        {
            if (fInfo.index >= 0) {
                JackLog("JackShmPtrRead::~JackShmPtrRead %ld\n", fInfo.index);
                jack_release_shm(&fInfo);
                fInfo.index = -1;
            }
        }

        T* operator->() const
        {
            return (T*)fInfo.attached_at;
        }

        operator T*() const
        {
            return (T*)fInfo.attached_at;
        }

        JackShmReadPtr& operator=(int index)
        {
            Init(index);
            return *this;
        }

        int GetShmIndex()
        {
            return fInfo.index;
        }

        T* GetShmAddress()
        {
            return (T*)fInfo.attached_at;
        }

};

} // end of namespace

#endif
