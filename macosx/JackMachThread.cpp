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


#include "JackMachThread.h"
#include "JackError.h"

#ifdef MY_TARGET_OS_IPHONE
#include "/Developer/Extras/CoreAudio/PublicUtility/CAHostTimeBase.h"
#endif

namespace Jack
{

int JackMachThread::SetThreadToPriority(jack_native_thread_t thread, UInt32 inPriority, Boolean inIsFixed, UInt64 period, UInt64 computation, UInt64 constraint)
{
    if (inPriority == 96) {
        // REAL-TIME / TIME-CONSTRAINT THREAD
        thread_time_constraint_policy_data_t	theTCPolicy;

#ifdef MY_TARGET_OS_IPHONE
        theTCPolicy.period = CAHostTimeBase::ConvertFromNanos(period);
        theTCPolicy.computation = CAHostTimeBase::ConvertFromNanos(computation);
        theTCPolicy.constraint = CAHostTimeBase::ConvertFromNanos(constraint);
#else
        theTCPolicy.period = AudioConvertNanosToHostTime(period);
        theTCPolicy.computation = AudioConvertNanosToHostTime(computation);
        theTCPolicy.constraint = AudioConvertNanosToHostTime(constraint);
#endif
        theTCPolicy.preemptible = true;
        kern_return_t res = thread_policy_set(pthread_mach_thread_np(thread), THREAD_TIME_CONSTRAINT_POLICY, (thread_policy_t) &theTCPolicy, THREAD_TIME_CONSTRAINT_POLICY_COUNT);
        jack_log("JackMachThread::thread_policy_set res = %ld", res);
        return (res == KERN_SUCCESS) ? 0 : -1;
    } else {
        // OTHER THREADS
        thread_extended_policy_data_t theFixedPolicy;
        thread_precedence_policy_data_t thePrecedencePolicy;
        SInt32 relativePriority;

        // [1] SET FIXED / NOT FIXED
        theFixedPolicy.timeshare = !inIsFixed;
        thread_policy_set(pthread_mach_thread_np(thread), THREAD_EXTENDED_POLICY, (thread_policy_t)&theFixedPolicy, THREAD_EXTENDED_POLICY_COUNT);

        // [2] SET PRECEDENCE
        // N.B.: We expect that if thread A created thread B, and the program wishes to change
        // the priority of thread B, then the call to change the priority of thread B must be
        // made by thread A.
        // This assumption allows us to use pthread_self() to correctly calculate the priority
        // of the feeder thread (since precedency policy's importance is relative to the
        // spawning thread's priority.)
        relativePriority = inPriority - GetThreadSetPriority(pthread_self());

        thePrecedencePolicy.importance = relativePriority;
        kern_return_t res = thread_policy_set(pthread_mach_thread_np(thread), THREAD_PRECEDENCE_POLICY, (thread_policy_t) &thePrecedencePolicy, THREAD_PRECEDENCE_POLICY_COUNT);
        jack_log("JackMachThread::thread_policy_set res = %ld", res);
        return (res == KERN_SUCCESS) ? 0 : -1;
    }
}

// returns the thread's priority as it was last set by the API
UInt32 JackMachThread::GetThreadSetPriority(jack_native_thread_t thread)
{
    return GetThreadPriority(thread, THREAD_SET_PRIORITY);
}

// returns the thread's priority as it was last scheduled by the Kernel
UInt32 JackMachThread::GetThreadScheduledPriority(jack_native_thread_t thread)
{
    return GetThreadPriority(thread, THREAD_SCHEDULED_PRIORITY);
}

UInt32 JackMachThread::GetThreadPriority(jack_native_thread_t thread, int inWhichPriority)
{
    thread_basic_info_data_t threadInfo;
    policy_info_data_t thePolicyInfo;
    unsigned int count;

    // get basic info
    count = THREAD_BASIC_INFO_COUNT;
    thread_info(pthread_mach_thread_np(thread), THREAD_BASIC_INFO, (thread_info_t)&threadInfo, &count);

    switch (threadInfo.policy) {
        case POLICY_TIMESHARE:
            count = POLICY_TIMESHARE_INFO_COUNT;
            thread_info(pthread_mach_thread_np(thread), THREAD_SCHED_TIMESHARE_INFO, (thread_info_t)&(thePolicyInfo.ts), &count);
            if (inWhichPriority == THREAD_SCHEDULED_PRIORITY) {
                return thePolicyInfo.ts.cur_priority;
            } else {
                return thePolicyInfo.ts.base_priority;
            }
            break;

        case POLICY_FIFO:
            count = POLICY_FIFO_INFO_COUNT;
            thread_info(pthread_mach_thread_np(thread), THREAD_SCHED_FIFO_INFO, (thread_info_t)&(thePolicyInfo.fifo), &count);
            if ( (thePolicyInfo.fifo.depressed) && (inWhichPriority == THREAD_SCHEDULED_PRIORITY) ) {
                return thePolicyInfo.fifo.depress_priority;
            }
            return thePolicyInfo.fifo.base_priority;
            break;

        case POLICY_RR:
            count = POLICY_RR_INFO_COUNT;
            thread_info(pthread_mach_thread_np(thread), THREAD_SCHED_RR_INFO, (thread_info_t)&(thePolicyInfo.rr), &count);
            if ( (thePolicyInfo.rr.depressed) && (inWhichPriority == THREAD_SCHEDULED_PRIORITY) ) {
                return thePolicyInfo.rr.depress_priority;
            }
            return thePolicyInfo.rr.base_priority;
            break;
    }

    return 0;
}

int JackMachThread::GetParams(jack_native_thread_t thread, UInt64* period, UInt64* computation, UInt64* constraint)
{
    thread_time_constraint_policy_data_t theTCPolicy;
    mach_msg_type_number_t count = THREAD_TIME_CONSTRAINT_POLICY_COUNT;
    boolean_t get_default = false;

    kern_return_t res = thread_policy_get(pthread_mach_thread_np(thread),
                                          THREAD_TIME_CONSTRAINT_POLICY,
                                          (thread_policy_t) & theTCPolicy,
                                          &count,
                                          &get_default);
    if (res == KERN_SUCCESS) {
    #ifdef MY_TARGET_OS_IPHONE
        *period = CAHostTimeBase::ConvertToNanos(theTCPolicy.period);
        *computation = CAHostTimeBase::ConvertToNanos(theTCPolicy.computation);
        *constraint = CAHostTimeBase::ConvertToNanos(theTCPolicy.constraint);
    #else
        *period = AudioConvertHostTimeToNanos(theTCPolicy.period);
        *computation = AudioConvertHostTimeToNanos(theTCPolicy.computation);
        *constraint = AudioConvertHostTimeToNanos(theTCPolicy.constraint);
    #endif
      
        jack_log("JackMachThread::GetParams period = %ld computation = %ld constraint = %ld", long(*period / 1000.0f), long(*computation / 1000.0f), long(*constraint / 1000.0f));
        return 0;
    } else {
        return -1;
    }
}

int JackMachThread::Kill()
{
    // pthread_cancel still not yet implemented in Darwin (TO CHECK ON TIGER)
    jack_log("JackMachThread::Kill");

    if (fThread != (jack_native_thread_t)NULL)  { // If thread has been started
        mach_port_t machThread = pthread_mach_thread_np(fThread);
        int res = (thread_terminate(machThread) == KERN_SUCCESS) ? 0 : -1;
        fThread = (jack_native_thread_t)NULL;
        return res;
    } else {
        return -1;
    }
}

int JackMachThread::AcquireRealTime()
{
    jack_log("JackMachThread::AcquireRealTime fPeriod = %ld fComputation = %ld fConstraint = %ld",
             long(fPeriod / 1000), long(fComputation / 1000), long(fConstraint / 1000));
    return (fThread != (jack_native_thread_t)NULL) ? AcquireRealTimeImp(fThread, fPeriod, fComputation, fConstraint) : -1;
}

int JackMachThread::AcquireSelfRealTime()
{
    jack_log("JackMachThread::AcquireSelfRealTime fPeriod = %ld fComputation = %ld fConstraint = %ld",
             long(fPeriod / 1000), long(fComputation / 1000), long(fConstraint / 1000));
    return AcquireRealTimeImp(pthread_self(), fPeriod, fComputation, fConstraint);
}

int JackMachThread::AcquireRealTime(int priority)
{
    fPriority = priority;
    return AcquireRealTime();
}

int JackMachThread::AcquireSelfRealTime(int priority)
{
    fPriority = priority;
    return AcquireSelfRealTime();
}

int JackMachThread::AcquireRealTimeImp(jack_native_thread_t thread, UInt64 period, UInt64 computation, UInt64 constraint)
{
    SetThreadToPriority(thread, 96, true, period, computation, constraint);
    return 0;
}

int JackMachThread::DropRealTime()
{
    return (fThread != (jack_native_thread_t)NULL) ? DropRealTimeImp(fThread) : -1;
}

int JackMachThread::DropSelfRealTime()
{
    return DropRealTimeImp(pthread_self());
}

int JackMachThread::DropRealTimeImp(jack_native_thread_t thread)
{
    SetThreadToPriority(thread, 63, false, 0, 0, 0);
    return 0;
}

void JackMachThread::SetParams(UInt64 period, UInt64 computation, UInt64 constraint)
{
    fPeriod = period;
    fComputation = computation;
    fConstraint = constraint;
}

} // end of namespace

