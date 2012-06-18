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

#include "JackMidiBufferReadQueue.h"
#include "JackMidiUtil.h"
#include "JackError.h"

using Jack::JackMidiBufferReadQueue;

JackMidiBufferReadQueue::JackMidiBufferReadQueue()
{
    event_count = 0;
    index = 0;
}

jack_midi_event_t *
JackMidiBufferReadQueue::DequeueEvent()
{
    jack_midi_event_t *e = 0;
    if (index < event_count) {
        JackMidiEvent *event = &(buffer->events[index]);
        midi_event.buffer = event->GetData(buffer);
        midi_event.size = event->size;
        midi_event.time = last_frame_time + event->time;
        e = &midi_event;
        index++;
    }
    return e;
}

void
JackMidiBufferReadQueue::ResetMidiBuffer(JackMidiBuffer *buffer)
{
    event_count = 0;
    index = 0;
    if (! buffer) {
        jack_error("JackMidiBufferReadQueue::ResetMidiBuffer - buffer reset "
                   "to NULL");
    } else if (! buffer->IsValid()) {
        jack_error("JackMidiBufferReadQueue::ResetMidiBuffer - buffer reset "
                   "to invalid buffer");
    } else {
        uint32_t lost_events = buffer->lost_events;
        if (lost_events) {
            jack_error("JackMidiBufferReadQueue::ResetMidiBuffer - %d events "
                       "lost during mixdown", lost_events);
        }
        this->buffer = buffer;
        event_count = buffer->event_count;
        last_frame_time = GetLastFrame();
    }
}
