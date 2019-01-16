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

#include "JackPosixProcessSync.h"
#include "JackError.h"
#include "JackPosixCommon.h"

namespace Jack
{

void JackPosixProcessSync::Signal()
{
    int res = pthread_cond_signal(&fCond);
    if (res != 0) {
        jack_error("JackPosixProcessSync::Signal error err = %s", strerror(res));
    }
}

// TO DO : check thread consistency?
void JackPosixProcessSync::LockedSignal()
{
    int res = pthread_mutex_lock(&fMutex);
    if (res != 0) {
        jack_error("JackPosixProcessSync::LockedSignal error err = %s", strerror(res));
    }
    res = pthread_cond_signal(&fCond);
    if (res != 0) {
        jack_error("JackPosixProcessSync::LockedSignal error err = %s", strerror(res));
    }
    res = pthread_mutex_unlock(&fMutex);
    if (res != 0) {
        jack_error("JackPosixProcessSync::LockedSignal error err = %s", strerror(res));
    }
}

void JackPosixProcessSync::SignalAll()
{
    int res = pthread_cond_broadcast(&fCond);
    if (res != 0) {
        jack_error("JackPosixProcessSync::SignalAll error err = %s", strerror(res));
    }
}

// TO DO : check thread consistency?
void JackPosixProcessSync::LockedSignalAll()
{
    int res = pthread_mutex_lock(&fMutex);
    if (res != 0) {
        jack_error("JackPosixProcessSync::LockedSignalAll error err = %s", strerror(res));
    }
    res = pthread_cond_broadcast(&fCond);
    if (res != 0) {
        jack_error("JackPosixProcessSync::LockedSignalAll error err = %s", strerror(res));
    }
    res = pthread_mutex_unlock(&fMutex);
    if (res != 0) {
        jack_error("JackPosixProcessSync::LockedSignalAll error err = %s", strerror(res));
    }
}

void JackPosixProcessSync::Wait()
{
    ThrowIf(!pthread_equal(pthread_self(), fOwner), JackException("JackPosixProcessSync::Wait: a thread has to have locked a mutex before it can wait"));
    fOwner = 0;

    int res = pthread_cond_wait(&fCond, &fMutex);
    if (res != 0) {
        jack_error("JackPosixProcessSync::Wait error err = %s", strerror(res));
    } else {
        fOwner = pthread_self();
    }
}

// TO DO : check thread consistency?
void JackPosixProcessSync::LockedWait()
{
    int res;
    res = pthread_mutex_lock(&fMutex);
    if (res != 0) {
        jack_error("JackPosixProcessSync::LockedWait error err = %s", strerror(res));
    }
    if ((res = pthread_cond_wait(&fCond, &fMutex)) != 0) {
        jack_error("JackPosixProcessSync::LockedWait error err = %s", strerror(res));
    }
    res = pthread_mutex_unlock(&fMutex);
    if (res != 0) {
        jack_error("JackPosixProcessSync::LockedWait error err = %s", strerror(res));
    }
}

// TO DO : check thread consistency?
bool JackPosixProcessSync::LockedTimedWait(long usec)
{
    int res1, res2, res3;
    struct timespec rel_timeout, now_mono, end_mono, now_real, end_real;
    struct timespec diff_mono, final_time;
    int old_errno;
    bool mono_available = true;
    double delta;

    res1 = pthread_mutex_lock(&fMutex);
    if (res1 != 0) {
        jack_error("JackPosixProcessSync::LockedTimedWait error err = %s",
            usec, strerror(res1));
    }

    jack_log("JackPosixProcessSync::LockedTimedWait time out = %ld", usec);
    /* Convert usec argument to timespec */
    rel_timeout.tv_sec = usec / 1000000;
    rel_timeout.tv_nsec = (usec % 1000000) * 1000;

    /* Calculate absolute monotonic timeout */
    res3 = clock_gettime(CLOCK_MONOTONIC, &now_mono);
    if (res3 != 0) {
        mono_available = false;
    }
    JackPosixTools::TimespecAdd(&now_mono, &rel_timeout, &end_mono);

    /* pthread_cond_timedwait() is affected by abrupt time jumps, i.e. when the
     * system time is changed. To protect against this, measure the time
     * difference between and after the sem_timedwait() call and if it suggests
     * that there has been a time jump, restart the call. */
    if (mono_available) {
        for (;;) {
            /* Calculate absolute realtime timeout, assuming no steps */
            res3 = clock_gettime(CLOCK_REALTIME, &now_real);
            assert(res3 == 0);
            JackPosixTools::TimespecSub(&end_mono, &now_mono, &diff_mono);
            JackPosixTools::TimespecAdd(&now_real, &diff_mono, &end_real);

            res2 = pthread_cond_timedwait(&fCond, &fMutex, &end_real);
            if (res2 != ETIMEDOUT) {
                break;
            }

            /* Compare with monotonic timeout, in case a step happened */
            old_errno = errno;
            res3 = clock_gettime(CLOCK_MONOTONIC, &now_mono);
            assert(res3 == 0);
            errno = old_errno;
            if (JackPosixTools::TimespecCmp(&now_mono, &end_mono) >= 0) {
                break;
            }
        }
    } else {
        /* CLOCK_MONOTONIC is not supported, do not check for time skips. */
        res3 = clock_gettime(CLOCK_REALTIME, &now_real);
        assert(res3 == 0);
        JackPosixTools::TimespecAdd(&now_real, &rel_timeout, &end_real);
        res2 = pthread_cond_timedwait(&fCond, &fMutex, &end_real);
    }

    res1 = pthread_mutex_unlock(&fMutex);
    if (res1 != 0) {
        jack_error("JackPosixProcessSync::LockedTimedWait error err = %s",
            strerror(res1));
    }

    if (res2 != 0) {
        jack_error("JackPosixProcessSync::LockedTimedWait error usec = %ld err = %s",
            usec, strerror(res2));
    }

    old_errno = errno;
    if (mono_available) {
        res3 = clock_gettime(CLOCK_MONOTONIC, &final_time);
        assert(res3 == 0);
        delta = 1e6 * final_time.tv_sec - 1e6 * now_mono.tv_sec +
            (final_time.tv_nsec - now_mono.tv_nsec) / 1000;
    } else {
        res3 = clock_gettime(CLOCK_REALTIME, &final_time);
        assert(res3 == 0);
        delta = 1e6 * final_time.tv_sec - 1e6 * now_real.tv_sec +
            (final_time.tv_nsec - now_real.tv_nsec) / 1000;
    }
    errno = old_errno;
    jack_log("JackPosixProcessSync::LockedTimedWait finished delta = %5.1lf", delta);
    return res2 == 0;
}


} // end of namespace

