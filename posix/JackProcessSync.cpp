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

#include "JackProcessSync.h"
#include "JackError.h"

namespace Jack
{

void JackProcessSync::Signal()
{
    int res = pthread_cond_signal(&fCond);
    if (res != 0)
        jack_error("pthread_cond_signal error err = %s", strerror(res));
}

void JackProcessSync::LockedSignal()
{
    int res;
    res = pthread_mutex_lock(&fMutex);
    if (res != 0)
        jack_error("pthread_mutex_lock error err = %s", strerror(res));
    res = pthread_cond_signal(&fCond);
    if (res != 0)
        jack_error("pthread_cond_signal error err = %s", strerror(res));
    res = pthread_mutex_unlock(&fMutex);
    if (res != 0)
        jack_error("pthread_mutex_unlock error err = %s", strerror(res));
}

void JackProcessSync::SignalAll()
{
    int res = pthread_cond_broadcast(&fCond);
    if (res != 0)
        jack_error("pthread_cond_broadcast error err = %s", strerror(res));
}

void  JackProcessSync::LockedSignalAll()
{
    int res;
    res = pthread_mutex_lock(&fMutex);
    if (res != 0)
        jack_error("pthread_mutex_lock error err = %s", strerror(res));
    res = pthread_cond_broadcast(&fCond);
    if (res != 0)
        jack_error("pthread_cond_broadcast error err = %s", strerror(res));
    res = pthread_mutex_unlock(&fMutex);
    if (res != 0)
        jack_error("pthread_mutex_unlock error err = %s", strerror(res));
}

void JackProcessSync::Wait()
{
    int res;
    if ((res = pthread_cond_wait(&fCond, &fMutex)) != 0)
        jack_error("pthread_cond_wait error err = %s", strerror(errno));
 }

void JackProcessSync::LockedWait()
{
    int res;
    res = pthread_mutex_lock(&fMutex);
    if (res != 0)
        jack_error("pthread_mutex_lock error err = %s", strerror(res));
    if ((res = pthread_cond_wait(&fCond, &fMutex)) != 0)
        jack_error("pthread_cond_wait error err = %s", strerror(errno));
    res = pthread_mutex_unlock(&fMutex);
    if (res != 0)
        jack_error("pthread_mutex_unlock error err = %s", strerror(res));
}

bool JackProcessSync::TimedWait(long usec)
{
    struct timeval T0, T1;
    timespec time;
    struct timeval now;
    int res;

    jack_log("JackProcessSync::TimedWait time out = %ld", usec);
    gettimeofday(&T0, 0);

    gettimeofday(&now, 0);
    unsigned int next_date_usec = now.tv_usec + usec;
    time.tv_sec = now.tv_sec + (next_date_usec / 1000000);
    time.tv_nsec = (next_date_usec % 1000000) * 1000;
    res = pthread_cond_timedwait(&fCond, &fMutex, &time);
    if (res != 0)
        jack_error("pthread_cond_timedwait error usec = %ld err = %s", usec, strerror(res));

    gettimeofday(&T1, 0);
    jack_log("JackProcessSync::TimedWait finished delta = %5.1lf",
             (1e6 * T1.tv_sec - 1e6 * T0.tv_sec + T1.tv_usec - T0.tv_usec));
    return (res == 0);
}

bool JackProcessSync::LockedTimedWait(long usec)
{
    struct timeval T0, T1;
    timespec time;
    struct timeval now;
    int res;

    res = pthread_mutex_lock(&fMutex);
    if (res != 0)
        jack_error("pthread_mutex_lock error err = %s", usec, strerror(res));
        
    jack_log("JackProcessSync::TimedWait time out = %ld", usec);
    gettimeofday(&T0, 0);

    gettimeofday(&now, 0);
    unsigned int next_date_usec = now.tv_usec + usec;
    time.tv_sec = now.tv_sec + (next_date_usec / 1000000);
    time.tv_nsec = (next_date_usec % 1000000) * 1000;
    res = pthread_cond_timedwait(&fCond, &fMutex, &time);
    if (res != 0)
        jack_error("pthread_cond_timedwait error usec = %ld err = %s", usec, strerror(res));

    gettimeofday(&T1, 0);
    
    pthread_mutex_unlock(&fMutex);
    if (res != 0)
        jack_error("pthread_mutex_unlock error err = %s", usec, strerror(res));
        
    jack_log("JackProcessSync::TimedWait finished delta = %5.1lf",
             (1e6 * T1.tv_sec - 1e6 * T0.tv_sec + T1.tv_usec - T0.tv_usec));
    return (res == 0);
}


} // end of namespace

