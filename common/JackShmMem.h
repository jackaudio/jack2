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

#ifndef __JackShmMem__
#define __JackShmMem__

#include "shm.h"
#include "JackError.h"
#include "JackCompilerDeps.h"

#include <new>  // GCC 4.0
#include <errno.h>
#include <stdlib.h>

#include "JackShmMem_os.h"

namespace Jack
{

void LockMemoryImp(void* ptr, size_t size);
void InitLockMemoryImp(void* ptr, size_t size);
void UnlockMemoryImp(void* ptr, size_t size);
void LockAllMemory();
void UnlockAllMemory();

class JackMem
{
    private:

        size_t fSize;
        static size_t gSize;

    protected:

        JackMem(): fSize(gSize)
        {}
        ~JackMem()
        {}

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
\brief

A class which objects possibly want to be allocated in shared memory derives from this class.
*/

class JackShmMemAble
{
    protected:

        jack_shm_info_t fInfo;

    public:

        void Init();

        int GetShmIndex()
        {
            return fInfo.index;
        }

        char* GetShmAddress()
        {
            return (char*)fInfo.ptr.attached_at;
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
\brief The base class for shared memory management.

A class which objects need to be allocated in shared memory derives from this class.
*/

class SERVER_EXPORT JackShmMem : public JackShmMemAble
{

     protected:

        JackShmMem();
        ~JackShmMem();

    public:

        void* operator new(size_t size);
        void* operator new(size_t size, void* memory);

        void operator delete(void* p, size_t size);
		void operator delete(void* p);

};

/*!
\brief Pointer on shared memory segment in the client side.
*/

template <class T>
class JackShmReadWritePtr
{

    private:

        jack_shm_info_t fInfo;

        void Init(int index, const char* server_name = "default")
        {
            if (fInfo.index < 0 && index >= 0) {
                jack_log("JackShmReadWritePtr::Init %ld %ld", index, fInfo.index);
                if (jack_initialize_shm(server_name) < 0) {
                    throw std::bad_alloc();
                }
                fInfo.index = index;
                if (jack_attach_lib_shm(&fInfo)) {
                    throw std::bad_alloc();
                }
                GetShmAddress()->LockMemory();
            }
        }

    public:

        JackShmReadWritePtr()
        {
            fInfo.index = -1;
            fInfo.ptr.attached_at = (char*)NULL;
        }

        JackShmReadWritePtr(int index, const char* server_name)
        {
            Init(index, server_name);
        }

        ~JackShmReadWritePtr()
        {
            if (fInfo.index >= 0) {
                jack_log("JackShmReadWritePtr::~JackShmReadWritePtr %ld", fInfo.index);
                GetShmAddress()->UnlockMemory();
                jack_release_lib_shm(&fInfo);
                fInfo.index = -1;
             }
        }

        T* operator->() const
        {
            return (T*)fInfo.ptr.attached_at;
        }

        operator T*() const
        {
            return (T*)fInfo.ptr.attached_at;
        }

        JackShmReadWritePtr& operator=(int index)
        {
            Init(index);
            return *this;
        }

        void SetShmIndex(int index, const char* server_name)
        {
            Init(index, server_name);
        }

        int GetShmIndex()
        {
            return fInfo.index;
        }

        T* GetShmAddress()
        {
            return (T*)fInfo.ptr.attached_at;
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

        void Init(int index, const char* server_name = "default")
        {
            if (fInfo.index < 0 && index >= 0) {
                jack_log("JackShmReadWritePtr1::Init %ld %ld", index, fInfo.index);
                if (jack_initialize_shm(server_name) < 0) {
                    throw std::bad_alloc();
                }
                fInfo.index = index;
                if (jack_attach_lib_shm(&fInfo)) {
                    throw std::bad_alloc();
                }
                GetShmAddress()->LockMemory();
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
            fInfo.ptr.attached_at = NULL;
        }

        JackShmReadWritePtr1(int index, const char* server_name)
        {
            Init(index, server_name);
        }

        ~JackShmReadWritePtr1()
        {
            if (fInfo.index >= 0) {
                jack_log("JackShmReadWritePtr1::~JackShmReadWritePtr1 %ld", fInfo.index);
                GetShmAddress()->UnlockMemory();
                jack_release_lib_shm(&fInfo);
                fInfo.index = -1;
            }
        }

        T* operator->() const
        {
            return (T*)fInfo.ptr.attached_at;
        }

        operator T*() const
        {
            return (T*)fInfo.ptr.attached_at;
        }

        JackShmReadWritePtr1& operator=(int index)
        {
            Init(index);
            return *this;
        }

        void SetShmIndex(int index, const char* server_name)
        {
            Init(index, server_name);
        }

        int GetShmIndex()
        {
            return fInfo.index;
        }

        T* GetShmAddress()
        {
            return (T*)fInfo.ptr.attached_at;
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

        void Init(int index, const char* server_name = "default")
        {
            if (fInfo.index < 0 && index >= 0) {
                jack_log("JackShmPtrRead::Init %ld %ld", index, fInfo.index);
                if (jack_initialize_shm(server_name) < 0) {
                    throw std::bad_alloc();
                }
                fInfo.index = index;
                if (jack_attach_lib_shm_read(&fInfo)) {
                    throw std::bad_alloc();
                }
                GetShmAddress()->LockMemory();
            }
        }

    public:

        JackShmReadPtr()
        {
            fInfo.index = -1;
            fInfo.ptr.attached_at = NULL;
        }

        JackShmReadPtr(int index, const char* server_name)
        {
            Init(index, server_name);
        }

        ~JackShmReadPtr()
        {
            if (fInfo.index >= 0) {
                jack_log("JackShmPtrRead::~JackShmPtrRead %ld", fInfo.index);
                GetShmAddress()->UnlockMemory();
                jack_release_lib_shm(&fInfo);
                fInfo.index = -1;
            }
        }

        T* operator->() const
        {
            return (T*)fInfo.ptr.attached_at;
        }

        operator T*() const
        {
            return (T*)fInfo.ptr.attached_at;
        }

        JackShmReadPtr& operator=(int index)
        {
            Init(index);
            return *this;
        }

        void SetShmIndex(int index, const char* server_name)
        {
            Init(index, server_name);
        }

        int GetShmIndex()
        {
            return fInfo.index;
        }

        T* GetShmAddress()
        {
            return (T*)fInfo.ptr.attached_at;
        }

};

} // end of namespace

#endif
