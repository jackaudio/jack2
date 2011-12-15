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

#include <memory>
#include <new>

#include "JackError.h"
#include "JackMidiRawOutputWriteQueue.h"
#include "JackMidiUtil.h"

using Jack::JackMidiRawOutputWriteQueue;

#define STILL_TIME(c, b) ((! (b)) || ((c) < (b)))

JackMidiRawOutputWriteQueue::
JackMidiRawOutputWriteQueue(JackMidiSendQueue *send_queue, size_t non_rt_size,
                            size_t max_non_rt_messages, size_t max_rt_messages)
{
    non_rt_queue = new JackMidiAsyncQueue(non_rt_size, max_non_rt_messages);
    std::auto_ptr<JackMidiAsyncQueue> non_rt_ptr(non_rt_queue);
    rt_queue = new JackMidiAsyncQueue(max_rt_messages, max_rt_messages);
    std::auto_ptr<JackMidiAsyncQueue> rt_ptr(rt_queue);
    non_rt_event = 0;
    rt_event = 0;
    running_status = 0;
    this->send_queue = send_queue;
    rt_ptr.release();
    non_rt_ptr.release();
}

JackMidiRawOutputWriteQueue::~JackMidiRawOutputWriteQueue()
{
    delete non_rt_queue;
    delete rt_queue;
}

void
JackMidiRawOutputWriteQueue::DequeueNonRealtimeEvent()
{
    non_rt_event = non_rt_queue->DequeueEvent();
    if (non_rt_event) {
        non_rt_event_time = non_rt_event->time;
        running_status = ApplyRunningStatus(non_rt_event, running_status);
    }
}

void
JackMidiRawOutputWriteQueue::DequeueRealtimeEvent()
{
    rt_event = rt_queue->DequeueEvent();
    if (rt_event) {
        rt_event_time = rt_event->time;
    }
}

Jack::JackMidiWriteQueue::EnqueueResult
JackMidiRawOutputWriteQueue::EnqueueEvent(jack_nframes_t time, size_t size,
                                          jack_midi_data_t *buffer)
{
    JackMidiAsyncQueue *queue = (size == 1) && (*buffer >= 0xf8) ? rt_queue :
        non_rt_queue;
    return queue->EnqueueEvent(time, size, buffer);
}

void
JackMidiRawOutputWriteQueue::HandleWriteQueueBug(jack_nframes_t time,
                                                 jack_midi_data_t byte)
{
    jack_error("JackMidiRawOutputWriteQueue::HandleWriteQueueBug - **BUG** "
               "The write queue told us that it couldn't enqueue a 1-byte "
               "MIDI event scheduled for frame '%d'.  This is probably a bug "
               "in the write queue implementation.", time);
}

jack_nframes_t
JackMidiRawOutputWriteQueue::Process(jack_nframes_t boundary_frame)
{
    if (! non_rt_event) {
        DequeueNonRealtimeEvent();
    }
    if (! rt_event) {
        DequeueRealtimeEvent();
    }
    while (rt_event) {
        jack_nframes_t current_frame = send_queue->GetNextScheduleFrame();
        if ((rt_event_time > current_frame) && non_rt_event &&
            (non_rt_event_time < rt_event_time)) {
            if (! SendNonRTBytes(rt_event_time < boundary_frame ?
                                 rt_event_time : boundary_frame)) {
                return non_rt_event_time;
            }
            current_frame = send_queue->GetNextScheduleFrame();
        }
        if (! STILL_TIME(current_frame, boundary_frame)) {
            return (! non_rt_event) ? rt_event_time :
                non_rt_event_time < rt_event_time ? non_rt_event_time :
                rt_event_time;
        }
        if (! SendByte(rt_event_time, *(rt_event->buffer))) {
            return rt_event_time;
        }
        DequeueRealtimeEvent();
    }
    SendNonRTBytes(boundary_frame);
    return non_rt_event ? non_rt_event_time : 0;
}

bool
JackMidiRawOutputWriteQueue::SendByte(jack_nframes_t time,
                                      jack_midi_data_t byte)
{
    switch (send_queue->EnqueueEvent(time, 1, &byte)) {
    case BUFFER_TOO_SMALL:
        HandleWriteQueueBug(time, byte);
    case OK:
        return true;
    default:
        // This is here to stop compilers from warning us about not handling
        // enumeration values.
        ;
    }
    return false;
}

bool
JackMidiRawOutputWriteQueue::SendNonRTBytes(jack_nframes_t boundary_frame)
{
    while (non_rt_event) {
        for (; non_rt_event->size;
             (non_rt_event->size)--, (non_rt_event->buffer)++) {
            jack_nframes_t current_frame = send_queue->GetNextScheduleFrame();
            if (! STILL_TIME(current_frame, boundary_frame)) {
                return true;
            }
            if (! SendByte(non_rt_event_time, *(non_rt_event->buffer))) {
                return false;
            }
        }
        DequeueNonRealtimeEvent();
    }
    return true;
}
