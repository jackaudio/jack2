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

#include "JackEventBufferReadQueue.h"
#include "JackMidiUtil.h"
#include "JackError.h"

using Jack::JackEventBufferReadQueue;

JackEventBufferReadQueue::JackEventBufferReadQueue()
{
    event_count = 0;
    index = 0;
}

jack_event_t *
JackEventBufferReadQueue::DequeueEvent()
{
    jack_event_t *e = 0;
    if (index < event_count) {
        JackEvent *jack_event = &(buffer->events[index]);
        event.buffer = jack_event->GetData(buffer);
        event.size = jack_event->size;
        event.time = last_frame_time + jack_event->time;
        e = &event;
        index++;
    }
    return e;
}

void
JackEventBufferReadQueue::ResetEventBuffer(JackEventBuffer *buffer)
{
    event_count = 0;
    index = 0;
    if (! buffer) {
        jack_error("JackEventBufferReadQueue::ResetEventBuffer - buffer reset "
                   "to NULL");
    } else if (! buffer->IsValid()) {
        jack_error("JackEventBufferReadQueue::ResetEventBuffer - buffer reset "
                   "to invalid buffer");
    } else {
        uint32_t lost_events = buffer->lost_events;
        if (lost_events) {
            jack_error("JackEventBufferReadQueue::ResetEventBuffer - %d events "
                       "lost during mixdown", lost_events);
        }
        this->buffer = buffer;
        event_count = buffer->event_count;
        last_frame_time = GetLastFrame();
    }
}
