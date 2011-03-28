/*
Copyright (C) 2010 Devin Anderson

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

#include <new>

#include "JackMidiAsyncWaitQueue.h"
#include "JackMidiUtil.h"
#include "JackTime.h"

using Jack::JackMidiAsyncWaitQueue;

JackMidiAsyncWaitQueue::JackMidiAsyncWaitQueue(size_t max_bytes,
                                               size_t max_messages):
    JackMidiAsyncQueue(max_bytes, max_messages)
{
    if (semaphore.Allocate("JackMidiAsyncWaitQueue", "midi-thread", 0)) {
        throw std::bad_alloc();
    }
}

JackMidiAsyncWaitQueue::~JackMidiAsyncWaitQueue()
{
    semaphore.Destroy();
}

jack_midi_event_t *
JackMidiAsyncWaitQueue::DequeueEvent()
{
    return DequeueEvent((long) 0);
}

jack_midi_event_t *
JackMidiAsyncWaitQueue::DequeueEvent(jack_nframes_t frame)
{

    // XXX: I worry about timer resolution on Solaris and Windows.  When the
    // resolution for the `JackSynchro` object is milliseconds, the worst-case
    // scenario for processor objects is that the wait time becomes less than a
    // millisecond, and the processor object continually calls this method,
    // expecting to wait a certain amount of microseconds, and ends up not
    // waiting at all each time, essentially busy-waiting until the current
    // frame is reached.  Perhaps there should be a #define that indicates the
    // wait time resolution for `JackSynchro` objects so that we can wait a
    // little longer if necessary.

    jack_time_t frame_time = GetTimeFromFrames(frame);
    jack_time_t current_time = GetMicroSeconds();
    return DequeueEvent((frame_time < current_time) ? 0 :
                        (long) (frame_time - current_time));
}

jack_midi_event_t *
JackMidiAsyncWaitQueue::DequeueEvent(long usec)
{
    return ((usec < 0) ? semaphore.Wait() : semaphore.TimedWait(usec)) ?
        JackMidiAsyncQueue::DequeueEvent() : 0;
}

Jack::JackMidiWriteQueue::EnqueueResult
JackMidiAsyncWaitQueue::EnqueueEvent(jack_nframes_t time, size_t size,
                                     jack_midi_data_t *buffer)
{
    EnqueueResult result = JackMidiAsyncQueue::EnqueueEvent(time, size,
                                                            buffer);
    if (result == OK) {
        semaphore.Signal();
    }
    return result;
}
