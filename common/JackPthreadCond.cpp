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

#include "JackPthreadCond.h"
#include "JackConstants.h"
#include "JackError.h"

namespace Jack
{

JackPthreadCondArray* JackPthreadCondServer::fTable = NULL;
long JackPthreadCondServer::fCount = 0;

JackShmReadWritePtr1<JackPthreadCondArray> JackPthreadCondClient::fTable;
long JackPthreadCondClient::fCount = 0;

JackPthreadCondArray::JackPthreadCondArray()
{
    for (int i = 0; i < MAX_ITEM; i++) {
        strcpy(fTable[i].fName, "");
    }
}

void JackPthreadCond::BuildName(const char* name, char* res)
{
    sprintf(res, "%s/jack_sem.%s", jack_client_dir, name);
}

bool JackPthreadCond::Signal()
{
    //pthread_mutex_lock(&fSynchro->fLock);
    //JackLog("JackPthreadCond::Signal...\n");
    pthread_cond_signal(&fSynchro->fCond);
    //pthread_mutex_unlock(&fSynchro->fLock);
    return true;
}

bool JackPthreadCond::SignalAll()
{
    pthread_cond_broadcast(&fSynchro->fCond);
    return true;
}

bool JackPthreadCond::Wait()
{
    pthread_mutex_lock(&fSynchro->fLock);
    //JackLog("JackPthreadCond::Wait...\n");
    pthread_cond_wait(&fSynchro->fCond, &fSynchro->fLock);
    pthread_mutex_unlock(&fSynchro->fLock);
    //JackLog("JackProcessSync::Wait finished\n");
    return true;
}

bool JackPthreadCond::TimedWait(long usec)
{
    timespec time;
    struct timeval now;
    gettimeofday(&now, 0);
    time.tv_sec = now.tv_sec + usec / 1000000;
    time.tv_nsec = (now.tv_usec + (usec % 1000000)) * 1000;
    pthread_mutex_lock(&fSynchro->fLock);
    JackLog("JackProcessSync::Wait...\n");
    pthread_cond_timedwait(&fSynchro->fCond, &fSynchro->fLock, &time);
    pthread_mutex_unlock(&fSynchro->fLock);
    JackLog("JackProcessSync::Wait finished\n");
    return true;
}

// Client side : get the published semaphore from server
bool JackPthreadCond::ConnectInput(const char* name)
{
    BuildName(name, fName);
    JackLog("JackPthreadCond::Connect %s\n", fName);

    // Temporary...
    if (fSynchro) {
        JackLog("Already connected name = %s\n", name);
        return true;
    }

    for (int i = 0; i < MAX_ITEM; i++) {
        JackPthreadCondItem* synchro = &(GetTable()->fTable[i]);
        if (strcmp(fName, synchro->fName) == 0) {
            fSynchro = synchro;
            return true;
        }
    }

    return false;
}

bool JackPthreadCond::Connect(const char* name)
{
    return ConnectInput(name);
}

bool JackPthreadCond::ConnectOutput(const char* name)
{
    return ConnectInput(name);
}

bool JackPthreadCond::Disconnect()
{
    JackLog("JackPthreadCond::Disconnect %s\n", fName);

    if (fSynchro) {
        strcpy(fSynchro->fName, "");
        fSynchro = NULL;
        return true;
    } else {
        return false;
    }
}

JackPthreadCondServer::JackPthreadCondServer(): JackPthreadCond()
{
    if (fCount++ == 0 && !fTable) {
        fTable = new JackPthreadCondArray();
    }
    if (fCount == MAX_ITEM)
        throw new std::bad_alloc;
}

JackPthreadCondServer::~JackPthreadCondServer()
{
    if (--fCount == 0 && fTable) {
        delete fTable;
        fTable = NULL;
    }
}

bool JackPthreadCondServer::Allocate(const char* name, int value)
{
    BuildName(name, fName);
    JackLog("JackPthreadCond::Allocate name = %s val = %ld\n", fName, value);

    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_NORMAL);

    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);

    for (int i = 0; i < MAX_ITEM; i++) {
        if (strcmp(fTable->fTable[i].fName, "") == 0) { // first empty place
            fSynchro = &fTable->fTable[i];
            if (pthread_mutex_init(&fSynchro->fLock, &mutex_attr) != 0) {
                jack_error("Allocate: can't check in named semaphore name = %s err = %s", fName, strerror(errno));
                return false;
            }
            if (pthread_cond_init(&fSynchro->fCond, &cond_attr) != 0) {
                jack_error("Allocate: can't check in named semaphore name = %s err = %s", fName, strerror(errno));
                return false;
            }
            strcpy(fSynchro->fName, fName);
            return true;
        }
    }

    return false;
}

void JackPthreadCondServer::Destroy()
{
    if (fSynchro != NULL) {
        pthread_mutex_destroy(&fSynchro->fLock);
        pthread_cond_destroy(&fSynchro->fCond);
        strcpy(fSynchro->fName, "");
        fSynchro = NULL;
    } else {
        jack_error("JackPthreadCond::Destroy semaphore == NULL");
    }
}

JackPthreadCondClient::JackPthreadCondClient(int shared_index): JackPthreadCond()
{
    if (fCount++ == 0 && !fTable) {
        fTable = shared_index;
    }
    if (fCount == MAX_ITEM)
        throw new std::bad_alloc;
}

JackPthreadCondClient::~JackPthreadCondClient()
{
    if (--fCount == 0 && fTable)
        delete fTable;
}

} // end of namespace

