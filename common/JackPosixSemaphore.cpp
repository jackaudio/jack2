/*
Copyright (C) 2004-2008 Grame

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

#include "JackPosixSemaphore.h"
#include "JackConstants.h"
#include "JackError.h"
#include <fcntl.h>
#include <sys/time.h>

namespace Jack
{

void JackPosixSemaphore::BuildName(const char* name, const char* server_name, char* res)
{
    sprintf(res, "%s/jack_sem.%s_%s", jack_client_dir, server_name, name);
}

bool JackPosixSemaphore::Signal()
{
    int res;

    if (!fSemaphore) {
        jack_error("JackPosixSemaphore::Signal name = %s already desallocated!!", fName);
        return false;
    }

    if (fFlush)
        return true;

    if ((res = sem_post(fSemaphore)) != 0) {
        jack_error("JackPosixSemaphore::Signal name = %s err = %s", fName, strerror(errno));
    }
    return (res == 0);
}

bool JackPosixSemaphore::SignalAll()
{
    int res;

    if (!fSemaphore) {
        jack_error("JackPosixSemaphore::SignalAll name = %s already desallocated!!", fName);
        return false;
    }

    if (fFlush)
        return true;

    if ((res = sem_post(fSemaphore)) != 0) {
        jack_error("JackPosixSemaphore::SignalAll name = %s err = %s", fName, strerror(errno));
    }
    return (res == 0);
}

/*
bool JackPosixSemaphore::Wait()
{
    int res;

	if (!fSemaphore) {
		jack_error("JackPosixSemaphore::Wait name = %s already desallocated!!", fName);
		return false;
	}

    if ((res = sem_wait(fSemaphore)) != 0) {
        jack_error("JackPosixSemaphore::Wait name = %s err = %s", fName, strerror(errno));
    }
    return (res == 0);
}
*/

bool JackPosixSemaphore::Wait()
{
    int res;

    while ((res = sem_wait(fSemaphore) < 0)) {
        jack_error("JackPosixSemaphore::Wait name = %s err = %s", fName, strerror(errno));
        if (errno != EINTR)
            break;
    }
    return (res == 0);
}


/*
#ifdef __linux__

bool JackPosixSemaphore::TimedWait(long usec) // unusable semantic !!
{
	int res;
	struct timeval now;
	timespec time;

	if (!fSemaphore) {
		jack_error("JackPosixSemaphore::TimedWait name = %s already desallocated!!", fName);
		return false;
	}
	gettimeofday(&now, 0);
	time.tv_sec = now.tv_sec + usec / 1000000;
	time.tv_nsec = (now.tv_usec + (usec % 1000000)) * 1000;

    if ((res = sem_timedwait(fSemaphore, &time)) != 0) {
        jack_error("JackPosixSemaphore::TimedWait err = %s", strerror(errno));
		jack_log("now %ld %ld ", now.tv_sec, now.tv_usec);
		jack_log("next %ld %ld ", time.tv_sec, time.tv_nsec/1000);
	}
    return (res == 0);
}

#else
#warning "JackPosixSemaphore::TimedWait is not supported: Jack in SYNC mode with JackPosixSemaphore will not run properly !!"

bool JackPosixSemaphore::TimedWait(long usec)
{
	return Wait();
}
#endif
*/

#warning JackPosixSemaphore::TimedWait not available : synchronous mode may not work correctly if POSIX semaphore are used

bool JackPosixSemaphore::TimedWait(long usec)
{
    return Wait();
}

// Server side : publish the semaphore in the global namespace
bool JackPosixSemaphore::Allocate(const char* name, const char* server_name, int value)
{
    BuildName(name, server_name, fName);
    jack_log("JackPosixSemaphore::Allocate name = %s val = %ld", fName, value);

    if ((fSemaphore = sem_open(fName, O_CREAT, 0777, value)) == (sem_t*)SEM_FAILED) {
        jack_error("Allocate: can't check in named semaphore name = %s err = %s", fName, strerror(errno));
        return false;
    } else {
        return true;
    }
}

// Client side : get the published semaphore from server
bool JackPosixSemaphore::ConnectInput(const char* name, const char* server_name)
{
    BuildName(name, server_name, fName);
    jack_log("JackPosixSemaphore::Connect %s", fName);

    // Temporary...
    if (fSemaphore) {
        jack_log("Already connected name = %s", name);
        return true;
    }

    if ((fSemaphore = sem_open(fName, O_CREAT)) == (sem_t*)SEM_FAILED) {
        jack_error("Connect: can't connect named semaphore name = %s err = %s", fName, strerror(errno));
        return false;
    } else {
        int val = 0;
        sem_getvalue(fSemaphore, &val);
        jack_log("JackPosixSemaphore::Connect sem_getvalue %ld", val);
        return true;
    }
}

bool JackPosixSemaphore::Connect(const char* name, const char* server_name)
{
    return ConnectInput(name, server_name);
}

bool JackPosixSemaphore::ConnectOutput(const char* name, const char* server_name)
{
    return ConnectInput(name, server_name);
}

bool JackPosixSemaphore::Disconnect()
{
    jack_log("JackPosixSemaphore::Disconnect %s", fName);

    if (fSemaphore) {
        if (sem_close(fSemaphore) != 0) {
            jack_error("Disconnect: can't disconnect named semaphore name = %s err = %s", fName, strerror(errno));
            return false;
        } else {
            fSemaphore = NULL;
            return true;
        }
    } else {
        return true;
    }
}

// Server side : destroy the semaphore
void JackPosixSemaphore::Destroy()
{
    if (fSemaphore != NULL) {
        jack_log("JackPosixSemaphore::Destroy");
        sem_unlink(fName);
        if (sem_close(fSemaphore) != 0) {
            jack_error("Destroy: can't destroy semaphore name = %s err = %s", fName, strerror(errno));
        }
        fSemaphore = NULL;
    } else {
        jack_error("JackPosixSemaphore::Destroy semaphore == NULL");
    }
}

} // end of namespace

