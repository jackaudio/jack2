/*
Copyright (C) 2004-2005 Grame  

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

#ifndef __JackPthreadCond__
#define __JackPthreadCond__

#include "JackSynchro.h"
#include "JackShmMem.h"
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <assert.h>

#define MAX_ITEM 8

namespace Jack
{

struct JackPthreadCondItem
{
    char fName[SYNC_MAX_NAME_SIZE];
    pthread_mutex_t fLock;
    pthread_cond_t fCond;
};

struct JackPthreadCondArray : public JackShmMem
{
    JackPthreadCondItem fTable[MAX_ITEM];

    JackPthreadCondArray();
    virtual ~JackPthreadCondArray()
    {}
};

/*!
\brief Inter process synchronization using pthread condition variables.
*/

class JackPthreadCond : public JackSynchro
{

    protected:

        JackPthreadCondItem* fSynchro;
        void BuildName(const char* name, char* res);
        virtual JackPthreadCondArray* GetTable() = 0;

    public:

        JackPthreadCond(): fSynchro(NULL)
        {}
        virtual ~JackPthreadCond()
        {}

        bool Signal();
        bool SignalAll();
        bool Wait();
        bool TimedWait(long usec);

        bool Connect(const char* name);
        bool ConnectInput(const char* name);
        bool ConnectOutput(const char* name);
        bool Disconnect();
};

class JackPthreadCondServer : public JackPthreadCond
{

    private:

        static JackPthreadCondArray* fTable;
        static long fCount;

    protected:

        JackPthreadCondArray* GetTable()
        {
            return fTable;
        }

    public:

        JackPthreadCondServer();
        virtual ~JackPthreadCondServer();

        bool Allocate(const char* name, int value);
        void Destroy();
};

class JackPthreadCondClient : public JackPthreadCond
{

    private:

        static JackShmReadWritePtr1<JackPthreadCondArray> fTable;
        static long fCount;

    protected:

        JackPthreadCondArray* GetTable()
        {
            return fTable;
        }

    public:

        JackPthreadCondClient(int shared_index);
        virtual ~JackPthreadCondClient();
};

} // end of namespace

#endif

