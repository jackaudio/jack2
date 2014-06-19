/*
Copyright (C) 2001 Paul Davis
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

#include "JackAndroidThread.h"
#include "JackError.h"
#include "JackTime.h"
#include "JackGlobals.h"
#include <string.h> // for memset
#include <unistd.h> // for _POSIX_PRIORITY_SCHEDULING check
#include <signal.h>

#ifdef JACK_ANDROID_REALTIME_SCHED
#include "SchedulingPolicyService.h"
#endif

//#define JACK_SCHED_POLICY SCHED_RR
#define JACK_SCHED_POLICY SCHED_FIFO

namespace Jack
{

void JackAndroidThread::thread_exit_handler(int sig)
{
    printf("this signal is %d \n", sig);
    pthread_exit(0);
}

void* JackAndroidThread::ThreadHandler(void* arg)
{
    JackAndroidThread* obj = (JackAndroidThread*)arg;
    JackRunnableInterface* runnable = obj->fRunnable;
    int err;

    // Signal creation thread when started with StartSync
    jack_log("JackAndroidThread::ThreadHandler : start");
    obj->fStatus = kIniting;

    // Call Init method
    if (!runnable->Init()) {
        jack_error("Thread init fails: thread quits");
        return 0;
    }

    obj->fStatus = kRunning;

    // If Init succeed, start the thread loop
    bool res = true;
    while (obj->fStatus == kRunning && res) {
        res = runnable->Execute();
    }

    jack_log("JackAndroidThread::ThreadHandler : exit");
    pthread_exit(0);
    return 0; // never reached
}

int JackAndroidThread::Start()
{
    fStatus = kStarting;

    // Check if the thread was correctly started
    if (StartImp(&fThread, fPriority, fRealTime, ThreadHandler, this) < 0) {
        fStatus = kIdle;
        return -1;
    } else {
        return 0;
    }
}

int JackAndroidThread::StartSync()
{
    fStatus = kStarting;

    if (StartImp(&fThread, fPriority, fRealTime, ThreadHandler, this) < 0) {
        fStatus = kIdle;
        return -1;
    } else {
        int count = 0;
        while (fStatus == kStarting && ++count < 1000) {
            JackSleep(1000);
        }
        return (count == 1000) ? -1 : 0;
    }
}

int JackAndroidThread::StartImp(jack_native_thread_t* thread, int priority, int realtime, void*(*start_routine)(void*), void* arg)
{
    pthread_attr_t attributes;
    struct sched_param rt_param;
    pthread_attr_init(&attributes);
    int res;

    struct sigaction actions;
    memset(&actions, 0, sizeof(actions));
    sigemptyset(&actions.sa_mask);
    actions.sa_flags = 0;
    actions.sa_handler = thread_exit_handler;
    sigaction(SIGUSR1,&actions,NULL);

    if ((res = pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_JOINABLE))) {
        jack_error("Cannot request joinable thread creation for thread res = %d", res);
        return -1;
    }

    if ((res = pthread_attr_setscope(&attributes, PTHREAD_SCOPE_SYSTEM))) {
        jack_error("Cannot set scheduling scope for thread res = %d", res);
        return -1;
    }

    if (realtime) {

        jack_log("JackAndroidThread::StartImp : create RT thread");

        if ((res = pthread_attr_setschedpolicy(&attributes, JACK_SCHED_POLICY))) {
            jack_error("Cannot set RR scheduling class for RT thread res = %d", res);
            return -1;
        }

        memset(&rt_param, 0, sizeof(rt_param));
        rt_param.sched_priority = priority;

        if ((res = pthread_attr_setschedparam(&attributes, &rt_param))) {
            jack_error("Cannot set scheduling priority for RT thread res = %d", res);
            return -1;
        }

    } else {
        jack_log("JackAndroidThread::StartImp : create non RT thread");
    }

    if ((res = pthread_attr_setstacksize(&attributes, THREAD_STACK))) {
        jack_error("Cannot set thread stack size res = %d", res);
        return -1;
    }

    if ((res = JackGlobals::fJackThreadCreator(thread, &attributes, start_routine, arg))) {
        jack_error("Cannot create thread res = %d", res);
        return -1;
    }

    pthread_attr_destroy(&attributes);
    return 0;
}

int JackAndroidThread::Kill()
{
    if (fThread != (jack_native_thread_t)NULL) { // If thread has been started
        jack_log("JackAndroidThread::Kill");
        void* status;

        pthread_kill(fThread, SIGUSR1);
        pthread_join(fThread, &status);
        fStatus = kIdle;
        fThread = (jack_native_thread_t)NULL;
        return 0;
    } else {
        return -1;
    }
}

int JackAndroidThread::Stop()
{
    if (fThread != (jack_native_thread_t)NULL) { // If thread has been started
        jack_log("JackAndroidThread::Stop");
        void* status;
        fStatus = kIdle; // Request for the thread to stop
        pthread_join(fThread, &status);
        fThread = (jack_native_thread_t)NULL;
        return 0;
    } else {
        return -1;
    }
}

int JackAndroidThread::KillImp(jack_native_thread_t thread)
{
    if (thread != (jack_native_thread_t)NULL) { // If thread has been started
        jack_log("JackAndroidThread::Kill");
        void* status;
        pthread_kill(thread, SIGUSR1);
        pthread_join(thread, &status);
        return 0;
    } else {
        return -1;
    }
}

int JackAndroidThread::StopImp(jack_native_thread_t thread)
{
    if (thread != (jack_native_thread_t)NULL) { // If thread has been started
        jack_log("JackAndroidThread::Stop");
        void* status;
        pthread_join(thread, &status);
        return 0;
    } else {
        return -1;
    }
}

int JackAndroidThread::AcquireRealTime()
{
    return (fThread != (jack_native_thread_t)NULL) ? AcquireRealTimeImp(fThread, fPriority) : -1;
}

int JackAndroidThread::AcquireSelfRealTime()
{
    return AcquireRealTimeImp(pthread_self(), fPriority);
}

int JackAndroidThread::AcquireRealTime(int priority)
{
    fPriority = priority;
    return AcquireRealTime();
}

int JackAndroidThread::AcquireSelfRealTime(int priority)
{
    fPriority = priority;
    return AcquireSelfRealTime();
}
int JackAndroidThread::AcquireRealTimeImp(jack_native_thread_t thread, int priority)
{
    struct sched_param rtparam;
    int res;
    memset(&rtparam, 0, sizeof(rtparam));
    rtparam.sched_priority = priority;

    jack_log("JackAndroidThread::AcquireRealTimeImp priority = %d", priority);

#ifndef JACK_ANDROID_REALTIME_SCHED
    if ((res = pthread_setschedparam(thread, JACK_SCHED_POLICY, &rtparam)) != 0) {
        jack_error("Cannot use real-time scheduling (RR/%d)"
                   "(%d: %s)", rtparam.sched_priority, res,
                   strerror(res));
        return -1;
    }
#else
    if ((res = android::requestPriority(getpid(), gettid(), priority)) != 0) {
        jack_log("Failed to get SCHED_FIFO priority pid %d tid %d; error %d",
		    getpid(), gettid(), res);
        return -1;
    }
#endif
    return 0;
}

int JackAndroidThread::DropRealTime()
{
    return (fThread != (jack_native_thread_t)NULL) ? DropRealTimeImp(fThread) : -1;
}

int JackAndroidThread::DropSelfRealTime()
{
    return DropRealTimeImp(pthread_self());
}

int JackAndroidThread::DropRealTimeImp(jack_native_thread_t thread)
{
    struct sched_param rtparam;
    int res;
    memset(&rtparam, 0, sizeof(rtparam));
    rtparam.sched_priority = 0;

    if ((res = pthread_setschedparam(thread, SCHED_OTHER, &rtparam)) != 0) {
        jack_error("Cannot switch to normal scheduling priority(%s)", strerror(errno));
        return -1;
    }
    return 0;
}

jack_native_thread_t JackAndroidThread::GetThreadID()
{
    return fThread;
}

bool JackAndroidThread::IsThread()
{
    return pthread_self() == fThread;
}

void JackAndroidThread::Terminate()
{
    jack_log("JackAndroidThread::Terminate");
    pthread_exit(0);
}

SERVER_EXPORT void ThreadExit()
{
    jack_log("ThreadExit");
    pthread_exit(0);
}

} // end of namespace

bool jack_get_thread_realtime_priority_range(int * min_ptr, int * max_ptr)
{
#if defined(_POSIX_PRIORITY_SCHEDULING) && !defined(__APPLE__)
    int min, max;

    min = sched_get_priority_min(JACK_SCHED_POLICY);
    if (min == -1)
    {
        jack_error("sched_get_priority_min() failed.");
        return false;
    }

    max = sched_get_priority_max(JACK_SCHED_POLICY);
    if (max == -1)
    {
        jack_error("sched_get_priority_max() failed.");
        return false;
    }

    *min_ptr = min;
    *max_ptr = max;

    return true;
#else
    return false;
#endif
}

bool jack_tls_allocate_key(jack_tls_key *key_ptr)
{
    int ret;

    ret = pthread_key_create(key_ptr, NULL);
    if (ret != 0)
    {
        jack_error("pthread_key_create() failed with error %d", ret);
        return false;
    }

    return true;
}

bool jack_tls_free_key(jack_tls_key key)
{
    int ret;

    ret = pthread_key_delete(key);
    if (ret != 0)
    {
        jack_error("pthread_key_delete() failed with error %d", ret);
        return false;
    }

    return true;
}

bool jack_tls_set(jack_tls_key key, void *data_ptr)
{
    int ret;

    ret = pthread_setspecific(key, (const void *)data_ptr);
    if (ret != 0)
    {
        jack_error("pthread_setspecific() failed with error %d", ret);
        return false;
    }

    return true;
}

void *jack_tls_get(jack_tls_key key)
{
    return pthread_getspecific(key);
}
