/*
Copyright (C) 2001 Paul Davis
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

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "JackPosixThread.h"
#include "JackError.h"
#include "JackTime.h"
#include <string.h> // for memset

namespace Jack
{

void* JackPosixThread::ThreadHandler(void* arg)
{
    JackPosixThread* obj = (JackPosixThread*)arg;
    JackRunnableInterface* runnable = obj->fRunnable;
    int err;

    if ((err = pthread_setcanceltype(obj->fCancellation, NULL)) != 0) {
        jack_error("pthread_setcanceltype err = %s", strerror(err));
    }
    
    // Signal creation thread when started with StartSync
    jack_log("ThreadHandler: start");
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

    jack_log("ThreadHandler: exit");
    pthread_exit(0);
    return 0; // never reached
}

int JackPosixThread::Start()
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
    
int JackPosixThread::StartSync()
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
    
int JackPosixThread::StartImp(pthread_t* thread, int priority, int realtime, void*(*start_routine)(void*), void* arg)
{
    pthread_attr_t attributes;
    struct sched_param rt_param;
    pthread_attr_init(&attributes);
    int res;

    if ((res = pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_JOINABLE))) {
        jack_error("Cannot request joinable thread creation for RT thread res = %d err = %s", res, strerror(errno));
        return -1;
    }

    if ((res = pthread_attr_setscope(&attributes, PTHREAD_SCOPE_SYSTEM))) {
        jack_error("Cannot set scheduling scope for RT thread res = %d err = %s", res, strerror(errno));
        return -1;
    }

    if (realtime) {
    
        jack_log("Create RT thread");

        if ((res = pthread_attr_setinheritsched(&attributes, PTHREAD_EXPLICIT_SCHED))) {
            jack_error("Cannot request explicit scheduling for RT thread res = %d err = %s", res, strerror(errno));
            return -1;
    	}
    
        //if ((res = pthread_attr_setschedpolicy(&attributes, SCHED_FIFO))) {
        if ((res = pthread_attr_setschedpolicy(&attributes, SCHED_RR))) {
            jack_error("Cannot set RR scheduling class for RT thread res = %d err = %s", res, strerror(errno));
            return -1;
        }
    } else {
        jack_log("Create non RT thread");
    }

    memset(&rt_param, 0, sizeof(rt_param));
    rt_param.sched_priority = priority;

    if ((res = pthread_attr_setschedparam(&attributes, &rt_param))) {
        jack_error("Cannot set scheduling priority for RT thread res = %d err = %s", res, strerror(errno));
        return -1;
    }

    if ((res = pthread_attr_setstacksize(&attributes, THREAD_STACK))) {
        jack_error("Cannot set thread stack size res = %d err = %s", res, strerror(errno));
        return -1;
    }

    if ((res = pthread_create(thread, &attributes, start_routine, arg))) {
        jack_error("Cannot create thread res = %d err = %s", res, strerror(errno));
        return -1;
    }

    return 0;
}

int JackPosixThread::Kill()
{
    if (fThread != (pthread_t)NULL) { // If thread has been started
        jack_log("JackPosixThread::Kill");
        void* status;
        pthread_cancel(fThread);
        pthread_join(fThread, &status);
        fStatus = kIdle;
        fThread = (pthread_t)NULL;
        return 0;
    } else {
        return -1;
    }
}

int JackPosixThread::Stop()
{
    if (fThread != (pthread_t)NULL) { // If thread has been started
        jack_log("JackPosixThread::Stop");
        void* status;
        fStatus = kIdle; // Request for the thread to stop
        pthread_join(fThread, &status);
        fThread = (pthread_t)NULL;
        return 0;
    } else {
        return -1;
    }
}

int JackPosixThread::KillImp(pthread_t thread)
{
    if (thread != (pthread_t)NULL) { // If thread has been started
        jack_log("JackPosixThread::Kill");
        void* status;
        pthread_cancel(thread);
        pthread_join(thread, &status);
        return 0;
    } else {
        return -1;
    }
}

int JackPosixThread::StopImp(pthread_t thread)
{
    if (thread != (pthread_t)NULL) { // If thread has been started
        jack_log("JackPosixThread::Stop");
        void* status;
        pthread_join(thread, &status);
        return 0;
    } else {
        return -1;
    }
}

int JackPosixThread::AcquireRealTime()
{
    return (fThread != (pthread_t)NULL) ? AcquireRealTimeImp(fThread, fPriority) : -1;
}

int JackPosixThread::AcquireRealTime(int priority)
{
    fPriority = priority;
    return AcquireRealTime();
}

int JackPosixThread::AcquireRealTimeImp(pthread_t thread, int priority)
{
    struct sched_param rtparam;
    int res;
    memset(&rtparam, 0, sizeof(rtparam));
    rtparam.sched_priority = priority;

    //if ((res = pthread_setschedparam(fThread, SCHED_FIFO, &rtparam)) != 0) {

    if ((res = pthread_setschedparam(thread, SCHED_RR, &rtparam)) != 0) {
        jack_error("Cannot use real-time scheduling (RR/%d) "
                   "(%d: %s)", rtparam.sched_priority, res,
                   strerror(res));
        return -1;
    }
    return 0;
}

int JackPosixThread::DropRealTime()
{
    return (fThread != (pthread_t)NULL) ? DropRealTimeImp(fThread) : -1;
}

int JackPosixThread::DropRealTimeImp(pthread_t thread)
{
    struct sched_param rtparam;
    int res;
    memset(&rtparam, 0, sizeof(rtparam));
    rtparam.sched_priority = 0;

    if ((res = pthread_setschedparam(thread, SCHED_OTHER, &rtparam)) != 0) {
        jack_error("Cannot switch to normal scheduling priority(%s)\n", strerror(errno));
        return -1;
    }
    return 0;
}

pthread_t JackPosixThread::GetThreadID()
{
    return fThread;
}

void JackPosixThread::Terminate()
{
    jack_log("JackPosixThread::Terminate");
    pthread_exit(0);
}

SERVER_EXPORT void ThreadExit()
{
    jack_log("ThreadExit");
    pthread_exit(0);
}

} // end of namespace

bool jack_tls_allocate_key(jack_tls_key *key_ptr)
{
    int ret;
    
    ret = pthread_key_create(key_ptr, NULL);
    if (ret != 0)
    {
        jack_error("pthread_key_create() failed with error %d errno %s", ret, strerror(errno));
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
        jack_error("pthread_key_delete() failed with error %d errno %s", ret, strerror(errno));
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
        jack_error("pthread_setspecific() failed with error %d errno %s", ret, strerror(errno));
        return false;
    }
    
    return true;
}

void *jack_tls_get(jack_tls_key key)
{
    return pthread_getspecific(key);
}
