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

#include <cassert>

#include "JackFFADOMidiSendQueue.h"
#include "JackMidiUtil.h"

using Jack::JackFFADOMidiSendQueue;

JackFFADOMidiSendQueue::JackFFADOMidiSendQueue()
{
    // Empty
}

Jack::JackMidiWriteQueue::EnqueueResult
JackFFADOMidiSendQueue::EnqueueEvent(jack_nframes_t time, size_t size,
                                     jack_midi_data_t *buffer)
{
    assert(size == 1);
    jack_nframes_t relative_time = (time < last_frame) ? 0 : time - last_frame;
    if (index < relative_time) {
        index = (relative_time % 8) ?
            (relative_time & (~ ((jack_nframes_t) 7))) + 8 : relative_time;
    }
    if (index >= length) {
        return BUFFER_FULL;
    }
    output_buffer[index] = 0x01000000 | ((uint32_t) *buffer);
    index += 8;
    return OK;
}

jack_nframes_t
JackFFADOMidiSendQueue::GetNextScheduleFrame()
{
    return last_frame + index;
}

void
JackFFADOMidiSendQueue::ResetOutputBuffer(uint32_t *output_buffer,
                                          jack_nframes_t length)
{
    index = 0;
    last_frame = GetLastFrame();
    this->length = length;
    this->output_buffer = output_buffer;
}
