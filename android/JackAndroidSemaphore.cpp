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

#include "JackAndroidSemaphore.h"
#include "JackTools.h"
#include "JackConstants.h"
#include "JackError.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/time.h>
#include "IAndroidShm.h"
#include "shm.h"
#include "android/Shm.h"  //android extension of shm.h

namespace Jack
{

pthread_mutex_t JackAndroidSemaphore::mutex = PTHREAD_MUTEX_INITIALIZER;

void JackAndroidSemaphore::BuildName(const char* client_name, const char* server_name, char* res, int size)
{
    char ext_client_name[SYNC_MAX_NAME_SIZE + 1];
    JackTools::RewriteName(client_name, ext_client_name);
    if (getenv("JACK_PROMISCUOUS_SERVER")) {
        snprintf(res, size, "jack_sem.%s_%s", server_name, ext_client_name);
    } else {
        snprintf(res, size, "jack_sem.%d_%s_%s", JackTools::GetUID(), server_name, ext_client_name);
    }
}

bool JackAndroidSemaphore::Signal()
{
    int res;

    if (!fSemaphore) {
        jack_error("JackAndroidSemaphore::Signal name = %s already deallocated!!", fName);
        return false;
    }

    if (fFlush)
        return true;

    if ((res = sem_post(fSemaphore)) != 0) {
        jack_error("JackAndroidSemaphore::Signal name = %s err = %s", fName, strerror(errno));
    }
    return (res == 0);
}

bool JackAndroidSemaphore::SignalAll()
{
    int res;

    if (!fSemaphore) {
        jack_error("JackAndroidSemaphore::SignalAll name = %s already deallocated!!", fName);
        return false;
    }

    if (fFlush)
        return true;

    if ((res = sem_post(fSemaphore)) != 0) {
        jack_error("JackAndroidSemaphore::SignalAll name = %s err = %s", fName, strerror(errno));
    }
    return (res == 0);
}

/*
bool JackAndroidSemaphore::Wait()
{
    int res;

    if (!fSemaphore) {
        jack_error("JackAndroidSemaphore::Wait name = %s already deallocated!!", fName);
        return false;
    }

    if ((res = sem_wait(fSemaphore)) != 0) {
        jack_error("JackAndroidSemaphore::Wait name = %s err = %s", fName, strerror(errno));
    }
    return (res == 0);
}
*/

bool JackAndroidSemaphore::Wait()
{
    int res;

    while ((res = sem_wait(fSemaphore) < 0)) {
        jack_error("JackAndroidSemaphore::Wait name = %s err = %s", fName, strerror(errno));
        if (errno != EINTR) {
            break;
        }
    }
    return (res == 0);
}

bool JackAndroidSemaphore::TimedWait(long usec)
{
    int res;
    struct timeval now;
    timespec time;

    if (!fSemaphore) {
        jack_error("JackAndroidSemaphore::TimedWait name = %s already deallocated!!", fName);
        return false;
    }
    gettimeofday(&now, 0);
    time.tv_sec = now.tv_sec + usec / 1000000;
    long tv_usec = (now.tv_usec + (usec % 1000000));
    time.tv_sec += tv_usec / 1000000;
    time.tv_nsec = (tv_usec % 1000000) * 1000;

    while ((res = sem_timedwait(fSemaphore, &time)) < 0) {
        jack_error("JackAndroidSemaphore::TimedWait err = %s", strerror(errno));
        jack_log("JackAndroidSemaphore::TimedWait now : %ld %ld ", now.tv_sec, now.tv_usec);
        jack_log("JackAndroidSemaphore::TimedWait next : %ld %ld ", time.tv_sec, time.tv_nsec/1000);
        if (errno == ETIMEDOUT) {
            timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec = time.tv_sec - ts.tv_sec;
            ts.tv_nsec = time.tv_nsec - ts.tv_nsec;
            if (ts.tv_nsec < 0) {
                ts.tv_nsec += 1000000000;
                ts.tv_sec  -= 1;
            }
            if (ts.tv_sec < 0 || ts.tv_nsec < 0) {
                jack_error("JackAndroidSemaphore::TimedWait time may changed or client killed already! wait again!");
                gettimeofday(&now, 0);
                time.tv_sec = now.tv_sec + usec / 1000000;
                long tv_usec = (now.tv_usec + (usec % 1000000));
                time.tv_sec += tv_usec / 1000000;
                time.tv_nsec = (tv_usec % 1000000) * 1000;
                if ((res = sem_timedwait(fSemaphore, &time)) < 0) {
                    jack_error("JackAndroidSemaphore::TimedWait err = %s", strerror(errno));
                    jack_log("JackAndroidSemaphore::TimedWait now : %ld %ld ", now.tv_sec, now.tv_usec);
                    jack_log("JackAndroidSemaphore::TimedWait next : %ld %ld ", time.tv_sec, time.tv_nsec/1000);
                }
            }
        }
        if (errno != EINTR) {
            break;
        }
    }
    return (res == 0);
}

// Server side : publish the semaphore in the global namespace
bool JackAndroidSemaphore::Allocate(const char* name, const char* server_name, int value)
{
    pthread_mutex_lock (&mutex);
    BuildName(name, server_name, fName, sizeof(fName));
    jack_log("JackAndroidSemaphore::Allocate name = %s val = %ld", fName, value);

	android::sp<android::IAndroidShm> service = android::Shm::getShmService();
	if(service == NULL){
		jack_error("shm service is null");
		return false;
	}
    fSemaphoreMemory = service->InitSemaphore(fName);
    if(fSemaphoreMemory != NULL){
        fSemaphore = (sem_t*)(fSemaphoreMemory->getBase());
    }

    if(fSemaphore == NULL) {
        jack_error("Allocate: can't check in named semaphore name = %s err = %s", fName, strerror(errno));
        pthread_mutex_unlock (&mutex);
        return false;
    } else {
        sem_init(fSemaphore, 1, 0);
        jack_log("sem_init()");
        pthread_mutex_unlock (&mutex);
        return true;
    }
}

// Client side : get the published semaphore from server
bool JackAndroidSemaphore::ConnectInput(const char* name, const char* server_name)
{
    pthread_mutex_lock (&mutex);
    BuildName(name, server_name, fName, sizeof(fName));
    jack_log("JackAndroidSemaphore::Connect name = %s", fName);

    // Temporary...
    if (fSemaphore) {
        jack_log("Already connected name = %s", name);
        pthread_mutex_unlock (&mutex);
        return true;
    }

	android::sp<android::IAndroidShm> service = android::Shm::getShmService();
	if(service == NULL){
		jack_error("shm service is null");
		return false;
	}

    fSemaphoreMemory = service->InitSemaphore(fName);
    if(fSemaphoreMemory != NULL){
        fSemaphore = (sem_t*)(fSemaphoreMemory->getBase());
    }

    if(fSemaphore == NULL) {
        jack_error("Connect: can't connect named semaphore name = %s err = %s", fName, strerror(errno));
        pthread_mutex_unlock (&mutex);
        return false;
    } else {
        int val = 0;
        sem_getvalue(fSemaphore, &val);
        jack_log("JackAndroidSemaphore::Connect sem_getvalue %ld", val);
        pthread_mutex_unlock (&mutex);
        return true;
    }
}

bool JackAndroidSemaphore::Connect(const char* name, const char* server_name)
{
    return ConnectInput(name, server_name);
}

bool JackAndroidSemaphore::ConnectOutput(const char* name, const char* server_name)
{
    return ConnectInput(name, server_name);
}

bool JackAndroidSemaphore::Disconnect()
{
    if (fSemaphore) {
        jack_log("JackAndroidSemaphore::Disconnect name = %s", fName);
        fSemaphore = NULL;
    }
    return true;
}

// Server side : destroy the semaphore
void JackAndroidSemaphore::Destroy()
{
    if (fSemaphore != NULL) {
        jack_log("JackAndroidSemaphore::Disconnect name = %s", fName);
        fSemaphore = NULL;
    } else {
        jack_error("JackAndroidSemaphore::Destroy semaphore == NULL");
    }
}

} // end of namespace

