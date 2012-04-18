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

#include <cassert>

#include "JackALSARawMidiSendQueue.h"
#include "JackMidiUtil.h"
#include "JackError.h"

using Jack::JackALSARawMidiSendQueue;

JackALSARawMidiSendQueue::JackALSARawMidiSendQueue(snd_rawmidi_t *rawmidi,
                                                   size_t bytes_per_poll)
{
    assert(bytes_per_poll > 0);
    this->bytes_per_poll = bytes_per_poll;
    this->rawmidi = rawmidi;
    blocked = false;
    bytes_available = bytes_per_poll;
}

Jack::JackMidiWriteQueue::EnqueueResult
JackALSARawMidiSendQueue::EnqueueEvent(jack_nframes_t time, size_t size,
                                       jack_midi_data_t *buffer)
{
    assert(size == 1);
    if (time > GetCurrentFrame()) {
        return EVENT_EARLY;
    }
    if (! bytes_available) {
        return BUFFER_FULL;
    }
    ssize_t result = snd_rawmidi_write(rawmidi, buffer, 1);
    switch (result) {
    case 1:
        blocked = false;
        bytes_available--;
        return OK;
    case -EWOULDBLOCK:
        blocked = true;
        return BUFFER_FULL;
    }
    jack_error("JackALSARawMidiSendQueue::EnqueueEvent - snd_rawmidi_write: "
               "%s", snd_strerror(result));
    return EN_ERROR;
}

bool
JackALSARawMidiSendQueue::IsBlocked()
{
    return blocked;
}

void
JackALSARawMidiSendQueue::ResetPollByteCount()
{
    bytes_available = bytes_per_poll;
}
