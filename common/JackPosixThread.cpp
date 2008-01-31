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

#include "JackPosixThread.h"
#include "JackError.h"
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

    // Call Init method
    if (!runnable->Init()) {
        jack_error("Thread init fails: thread quits");
        return 0;
    }

    JackLog("ThreadHandler: start\n");

 	// If Init succeed, start the thread loop
	bool res = true;
    while (obj->fRunning && res) {
        res = runnable->Execute();
    }

    JackLog("ThreadHandler: exit\n");
    return 0;
}

int JackPosixThread::Start()
{
	fRunning = true;
	
	// Check if the thread was correctly started
	if (StartImp(&fThread, fPriority, fRealTime, ThreadHandler, this) < 0) { 
		fRunning = false;
		return -1;
	} else {
		return 0;
	}
}

int JackPosixThread::StartImp(pthread_t* thread, int priority, int realtime, void*(*start_routine)(void*), void* arg)
{
    int res;
 
    if (realtime) {

        JackLog("Create RT thread\n");

        /* Get the client thread to run as an RT-FIFO
           scheduled thread of appropriate priority.
        */
        pthread_attr_t attributes;
        struct sched_param rt_param;
        pthread_attr_init(&attributes);
		
		if ((res = pthread_attr_setinheritsched(&attributes, PTHREAD_EXPLICIT_SCHED))) {
            jack_error("Cannot request explicit scheduling for RT thread  %d %s", res, strerror(errno));
            return -1;
        }
		
        if ((res = pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_JOINABLE))) {
            jack_error("Cannot request joinable thread creation for RT thread  %d %s", res, strerror(errno));
            return -1;
        }
		
        if ((res = pthread_attr_setscope(&attributes, PTHREAD_SCOPE_SYSTEM))) {
            jack_error("Cannot set scheduling scope for RT thread %d %s", res, strerror(errno));
            return -1;
        }

        //if ((res = pthread_attr_setschedpolicy(&attributes, SCHED_FIFO))) {

        if ((res = pthread_attr_setschedpolicy(&attributes, SCHED_RR))) {
            jack_error("Cannot set FIFO scheduling class for RT thread  %d %s", res, strerror(errno));
            return -1;
        }

        if ((res = pthread_attr_setscope(&attributes, PTHREAD_SCOPE_SYSTEM))) {
            jack_error("Cannot set scheduling scope for RT thread %d %s", res, strerror(errno));
            return -1;
        }

        memset(&rt_param, 0, sizeof(rt_param));
        rt_param.sched_priority = priority;

        if ((res = pthread_attr_setschedparam(&attributes, &rt_param))) {
            jack_error("Cannot set scheduling priority for RT thread %d %s", res, strerror(errno));
            return -1;
        }
		
	   if ((res = pthread_attr_setstacksize(&attributes, THREAD_STACK))) {
			jack_error("setting thread stack size%d %s", res, strerror(errno));
			return -1;
        }

        if ((res = pthread_create(thread, &attributes, start_routine, arg))) {
            jack_error("Cannot set create thread %d %s", res, strerror(errno));
            return -1;
        }

        return 0;
    } else {
        JackLog("Create non RT thread\n");

        if ((res = pthread_create(thread, 0, start_routine, arg))) {
            jack_error("Cannot set create thread %d %s", res, strerror(errno));
            return -1;
        }
        return 0;
    }
}

int JackPosixThread::StartSync()
{
    jack_error("Not implemented yet");
    return -1;
}

int JackPosixThread::Kill()
{
    if (fThread) { // If thread has been started
        JackLog("JackPosixThread::Kill\n");
        void* status;
        pthread_cancel(fThread);
        pthread_join(fThread, &status);
		fRunning = false; 
		fThread = (pthread_t)NULL;
        return 0;
    } else {
        return -1;
    }
}

int JackPosixThread::Stop()
{
    if (fThread) { // If thread has been started
        JackLog("JackPosixThread::Stop\n");
        void* status;
        fRunning = false; // Request for the thread to stop
        pthread_join(fThread, &status);
		fThread = (pthread_t)NULL;
        return 0;
    } else {
        return -1;
    }
}

int JackPosixThread::AcquireRealTime()
{
 	return (fThread) ? AcquireRealTimeImp(fThread, fPriority) : -1;
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
        jack_error("Cannot use real-time scheduling (FIFO/%d) "
                   "(%d: %s)", rtparam.sched_priority, res,
                   strerror(res));
        return -1;
    }
    return 0;
}

int JackPosixThread::DropRealTime()
{
    return (fThread) ? DropRealTimeImp(fThread) : -1;
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
	JackLog("JackPosixThread::Terminate\n");
	pthread_exit(0);
}

} // end of namespace

