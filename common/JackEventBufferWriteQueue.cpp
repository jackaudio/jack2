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

#include "JackEventBufferWriteQueue.h"
#include "JackMidiUtil.h"
#include "JackError.h"

using Jack::JackEventBufferWriteQueue;

JackEventBufferWriteQueue::JackEventBufferWriteQueue()
{
    // Empty
}

Jack::JackEventWriteQueue::EnqueueResult
JackEventBufferWriteQueue::EnqueueEvent(jack_nframes_t time, size_t size,
                                       jack_evemt_data_t *data)
{
    if (time >= next_frame_time) {
        return EVENT_EARLY;
    }
    if (time < last_frame_time) {
        time = last_frame_time;
    }
    jack_evemt_data_t *dst = buffer->ReserveEvent(time - last_frame_time, size);
    if (! dst) {
        return size > max_bytes ? BUFFER_TOO_SMALL : BUFFER_FULL;
    }
    memcpy(dst, data, size);
    return OK;
}

void
JackEventBufferWriteQueue::ResetEventBuffer(JackEventBuffer *buffer,
                                          jack_nframes_t frames)
{
    if (! buffer) {
        jack_error("JackEventBufferWriteQueue::ResetEventBuffer - buffer reset "
                   "to NULL");
    } else if (! buffer->IsValid()) {
        jack_error("JackEventBufferWriteQueue::ResetEventBuffer - buffer reset "
                   "to invalid buffer");
    } else {
        this->buffer = buffer;
        buffer->Reset(frames);
        last_frame_time = GetLastFrame();
        max_bytes = buffer->MaxEventSize();
        next_frame_time = last_frame_time + frames;
    }
}
