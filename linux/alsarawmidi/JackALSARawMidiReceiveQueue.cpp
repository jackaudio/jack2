/*
Copyright (C) 2011 Devin Anderson

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

#include "JackALSARawMidiReceiveQueue.h"
#include "JackError.h"
#include "JackMidiUtil.h"

using Jack::JackALSARawMidiReceiveQueue;

JackALSARawMidiReceiveQueue::
JackALSARawMidiReceiveQueue(snd_rawmidi_t *rawmidi, size_t buffer_size)
{
    buffer = new jack_midi_data_t[buffer_size];
    this->buffer_size = buffer_size;
    this->rawmidi = rawmidi;
}

JackALSARawMidiReceiveQueue::~JackALSARawMidiReceiveQueue()
{
    delete[] buffer;
}

jack_midi_event_t *
JackALSARawMidiReceiveQueue::DequeueEvent()
{
    ssize_t result = snd_rawmidi_read(rawmidi, buffer, buffer_size);
    if (result > 0) {
        event.buffer = buffer;
        event.size = (size_t) result;
        event.time = GetCurrentFrame();
        return &event;
    }
    if (result && (result != -EWOULDBLOCK)) {
        jack_error("JackALSARawMidiReceiveQueue::DequeueEvent - "
                   "snd_rawmidi_read: %s", snd_strerror(result));
    }
    return 0;
}
