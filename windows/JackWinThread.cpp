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

#include "JackWinThread.h"
#include "JackError.h"
#include <assert.h>

namespace Jack
{

DWORD WINAPI JackWinThread::ThreadHandler(void* arg)
{
    JackWinThread* obj = (JackWinThread*)arg;
    JackRunnableInterface* runnable = obj->fRunnable;

    // Call Init method
    if (!runnable->Init()) {
        jack_error("Thread init fails: thread quits");
        return 0;
    }

    // Signal creation thread when started with StartSync
    if (!obj->fRunning) {
        obj->fRunning = true;
        SetEvent(obj->fEvent);
    }

    JackLog("ThreadHandler: start\n");

    // If Init succeed, start the thread loop
    bool res = true;
    while (obj->fRunning && res) {
        res = runnable->Execute();
    }
  
    SetEvent(obj->fEvent);
    JackLog("ThreadHandler: exit\n");
    return 0;
}

JackWinThread::JackWinThread(JackRunnableInterface* runnable) 
	: JackThread(runnable, 0, false, 0)
{
	 fEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	 fThread = NULL;
	 assert(fEvent);
}

JackWinThread::~JackWinThread()
{
	CloseHandle(fEvent);
}

int JackWinThread::Start()
{
	fEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (fEvent == NULL) {
        jack_error("Cannot create event error = %d", GetLastError());
        return -1;
    }

	fRunning = true;
	
	// Check if the thread was correctly started
	if (StartImp(&fThread, fPriority, fRealTime, ThreadHandler, this) < 0) { 
		fRunning = false;
		return -1;
	} else {
		return 0;
	}
}

int JackWinThread::StartImp(pthread_t* thread, int priority, int realtime, ThreadCallback start_routine, void* arg)
{
    DWORD id;

    if (realtime) {

        JackLog("Create RT thread\n");
        *thread = CreateThread(NULL, 0, start_routine, arg, 0, &id);

        if (*thread == NULL) {
            jack_error("Cannot create thread error = %d", GetLastError());
            return -1;
        }

        if (!SetThreadPriority(*thread, THREAD_PRIORITY_TIME_CRITICAL)) {
            jack_error("Cannot set priority class = %d", GetLastError());
            return -1;
        }

        return 0;

    } else {

        JackLog("Create non RT thread\n");
        *thread = CreateThread(NULL, 0, start_routine, arg, 0, &id);

        if (thread == NULL) {
            jack_error("Cannot create thread error = %d", GetLastError());
            return -1;
        }

        return 0;
    }
}

int JackWinThread::StartSync()
{
    DWORD id;

    fEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (fEvent == NULL) {
        jack_error("Cannot create event error = %d", GetLastError());
        return -1;
    }

    if (fRealTime) {

        JackLog("Create RT thread\n");
        fThread = CreateThread(NULL, 0, ThreadHandler, (void*)this, 0, &id);

        if (fThread == NULL) {
            jack_error("Cannot create thread error = %d", GetLastError());
            return -1;
        }

        if (WaitForSingleObject(fEvent, 3000) != WAIT_OBJECT_0) { // wait 3 sec
            jack_error("Thread has not started");
            return -1;
        }

        if (!SetThreadPriority(fThread, THREAD_PRIORITY_TIME_CRITICAL)) {
            jack_error("Cannot set priority class = %d", GetLastError());
            return -1;
        }

        return 0;

    } else {

        JackLog("Create non RT thread\n");
        fThread = CreateThread(NULL, 0, ThreadHandler, (void*)this, 0, &id);

        if (fThread == NULL) {
            jack_error("Cannot create thread error = %d", GetLastError());
            return -1;
        }

        if (WaitForSingleObject(fEvent, 3000) != WAIT_OBJECT_0) { // wait 3 sec
            jack_error("Thread has not started");
            return -1;
        }

        return 0;
    }
}

// voir http://www.microsoft.com/belux/msdn/nl/community/columns/ldoc/multithread1.mspx

int JackWinThread::Kill()
{
    if (fThread) { // If thread has been started
        TerminateThread(fThread, 0);
		WaitForSingleObject(fThread, INFINITE);
        CloseHandle(fThread);
		JackLog("JackWinThread::Kill 2\n");
		fThread = NULL;
		fRunning = false; 
        return 0;
    } else {
        return -1;
    }
}

int JackWinThread::Stop()
{
    if (fThread) { // If thread has been started
        JackLog("JackWinThread::Stop\n");
        fRunning = false; // Request for the thread to stop
        WaitForSingleObject(fEvent, INFINITE);
        CloseHandle(fThread);
		fThread = NULL;
        return 0;
    } else {
        return -1;
    }
}

int JackWinThread::AcquireRealTime()
{
	return (fThread) ? AcquireRealTimeImp(fThread, fPriority) : -1;
}

int JackWinThread::AcquireRealTime(int priority)
{
 	fPriority = priority;
	return AcquireRealTime();
}

int JackWinThread::AcquireRealTimeImp(pthread_t thread, int priority)
{
    JackLog("JackWinThread::AcquireRealTime\n");

	if (SetThreadPriority(thread, THREAD_PRIORITY_TIME_CRITICAL)) {
		JackLog("JackWinThread::AcquireRealTime OK\n");
		return 0;
	} else {
		jack_error("Cannot set thread priority = %d", GetLastError());
		return -1;
	}
}
int JackWinThread::DropRealTime()
{
	return DropRealTimeImp(fThread);
}

int JackWinThread::DropRealTimeImp(pthread_t thread)
{
	if (SetThreadPriority(thread, THREAD_PRIORITY_NORMAL)) {
		return 0;
	} else {
		jack_error("Cannot set thread priority = %d", GetLastError());
		return -1;
	}
}

pthread_t JackWinThread::GetThreadID()
{
    return fThread;
}

} // end of namespace

