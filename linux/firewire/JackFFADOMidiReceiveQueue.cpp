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

#include "JackFFADOMidiReceiveQueue.h"
#include "JackMidiUtil.h"

using Jack::JackFFADOMidiReceiveQueue;

JackFFADOMidiReceiveQueue::JackFFADOMidiReceiveQueue()
{
    // Empty
}

jack_midi_event_t *
JackFFADOMidiReceiveQueue::DequeueEvent()
{
    for (; index < length; index += 8) {
        uint32_t data = input_buffer[index];
        if (data & 0xff000000) {
            byte = (jack_midi_data_t) (data & 0xff);
            event.buffer = &byte;
            event.size = 1;
            event.time = last_frame + index;
            index += 8;
            return &event;
        }
    }
    return 0;
}

void
JackFFADOMidiReceiveQueue::ResetInputBuffer(uint32_t *input_buffer,
                                            jack_nframes_t length)
{
    this->input_buffer = input_buffer;
    index = 0;
    last_frame = GetLastFrame();
    this->length = length;
}
