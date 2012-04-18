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

#include "JackWinThread.h"
#include "JackError.h"
#include "JackTime.h"
#include <assert.h>
#include <stdio.h>

namespace Jack
{

DWORD WINAPI JackWinThread::ThreadHandler(void* arg)
{
    JackWinThread* obj = (JackWinThread*)arg;
    JackRunnableInterface* runnable = obj->fRunnable;

    // Signal creation thread when started with StartSync
    jack_log("JackWinThread::ThreadHandler : start");
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

    SetEvent(obj->fEvent);
    jack_log("JackWinThread::ThreadHandler : exit");
    return 0;
}

JackWinThread::JackWinThread(JackRunnableInterface* runnable)
        : JackMMCSS(), JackThreadInterface(runnable, 0, false, 0)
{
    fEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    fThread = (HANDLE)NULL;
    assert(fEvent);
}

JackWinThread::~JackWinThread()
{
    CloseHandle(fEvent);
    CloseHandle(fThread);
}

int JackWinThread::Start()
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

int JackWinThread::StartSync()
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

int JackWinThread::StartImp(jack_native_thread_t* thread, int priority, int realtime, ThreadCallback start_routine, void* arg)
{
    DWORD id;
    *thread = CreateThread(NULL, 0, start_routine, arg, 0, &id);

    if (*thread == NULL) {
        jack_error("Cannot create thread error = %d", GetLastError());
        return -1;
    }

    if (realtime) {

        jack_log("JackWinThread::StartImp : create RT thread");
        if (!SetThreadPriority(*thread, THREAD_PRIORITY_TIME_CRITICAL)) {
            jack_error("Cannot set priority class = %d", GetLastError());
            return -1;
        }

    } else {
        jack_log("JackWinThread::StartImp : create non RT thread");
    }

    return 0;
}

// voir http://www.microsoft.com/belux/msdn/nl/community/columns/ldoc/multithread1.mspx

int JackWinThread::Kill()
{
    if (fThread != (HANDLE)NULL) { // If thread has been started
        TerminateThread(fThread, 0);
        WaitForSingleObject(fThread, INFINITE);
        CloseHandle(fThread);
        jack_log("JackWinThread::Kill");
        fThread = (HANDLE)NULL;
        fStatus = kIdle;
        return 0;
    } else {
        return -1;
    }
}

int JackWinThread::Stop()
{
    if (fThread != (HANDLE)NULL) { // If thread has been started
        jack_log("JackWinThread::Stop");
        fStatus = kIdle; // Request for the thread to stop
        WaitForSingleObject(fEvent, INFINITE);
        CloseHandle(fThread);
        fThread = (HANDLE)NULL;
        return 0;
    } else {
        return -1;
    }
}

int JackWinThread::KillImp(jack_native_thread_t thread)
{
    if (thread != (HANDLE)NULL) { // If thread has been started
        TerminateThread(thread, 0);
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
        return 0;
    } else {
        return -1;
    }
}

int JackWinThread::StopImp(jack_native_thread_t thread)
{
    if (thread) { // If thread has been started
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
        return 0;
    } else {
        return -1;
    }
}

int JackWinThread::AcquireRealTime()
{
    return (fThread != (HANDLE)NULL) ? AcquireRealTimeImp(fThread, fPriority) : -1;
}

int JackWinThread::AcquireSelfRealTime()
{
    return AcquireRealTimeImp(GetCurrentThread(), fPriority);
}

int JackWinThread::AcquireRealTime(int priority)
{
    fPriority = priority;
    return AcquireRealTime();
}

int JackWinThread::AcquireSelfRealTime(int priority)
{
    fPriority = priority;
    return AcquireSelfRealTime();
}

int JackWinThread::AcquireRealTimeImp(jack_native_thread_t thread, int priority)
{
    jack_log("JackWinThread::AcquireRealTimeImp priority = %d", priority);

    if (priority >= 90 && MMCSSAcquireRealTime(thread) == 0) {
        jack_info("MMCSS API used to acquire RT for thread");
        return 0;
    } else {
        jack_info("MMCSS API not used...");
        if (SetThreadPriority(thread, THREAD_PRIORITY_TIME_CRITICAL)) {
            return 0;
        } else {
            jack_error("Cannot set thread priority = %d", GetLastError());
            return -1;
        }
    }
}

int JackWinThread::DropRealTime()
{
    return (fThread != (HANDLE)NULL) ? DropRealTimeImp(fThread) : -1;
}

int JackWinThread::DropSelfRealTime()
{
    return DropRealTimeImp(GetCurrentThread());
}

int JackWinThread::DropRealTimeImp(jack_native_thread_t thread)
{
    if (MMCSSDropRealTime(thread) == 0 ) {
        jack_info("MMCSS API used to drop RT for thread");
        return 0;
    } else if (SetThreadPriority(thread, THREAD_PRIORITY_NORMAL)) {
        return 0;
    } else {
        jack_error("Cannot set thread priority = %d", GetLastError());
        return -1;
    }
}

jack_native_thread_t JackWinThread::GetThreadID()
{
    return fThread;
}

bool JackWinThread::IsThread()
{
    return GetCurrentThread() == fThread;
}

void JackWinThread::Terminate()
{
    jack_log("JackWinThread::Terminate");
    ExitThread(0);
}

SERVER_EXPORT void ThreadExit()
{
    jack_log("ThreadExit");
    ExitThread(0);
}

} // end of namespace

bool jack_get_thread_realtime_priority_range(int * min_ptr, int * max_ptr)
{
    return false;
}

bool jack_tls_allocate_key(jack_tls_key *key_ptr)
{
    DWORD key;

    key = TlsAlloc();
    if (key == TLS_OUT_OF_INDEXES)
    {
        jack_error("TlsAlloc() failed. Error is %d", (unsigned int)GetLastError());
        return false;
    }

    *key_ptr = key;
    return true;
}

bool jack_tls_free_key(jack_tls_key key)
{
    if (!TlsFree(key))
    {
        jack_error("TlsFree() failed. Error is %d", (unsigned int)GetLastError());
        return false;
    }

    return true;
}

bool jack_tls_set(jack_tls_key key, void *data_ptr)
{
    if (!TlsSetValue(key, data_ptr))
    {
        jack_error("TlsSetValue() failed. Error is %d", (unsigned int)GetLastError());
        return false;
    }

    return true;
}

void *jack_tls_get(jack_tls_key key)
{
    return TlsGetValue(key);
}
