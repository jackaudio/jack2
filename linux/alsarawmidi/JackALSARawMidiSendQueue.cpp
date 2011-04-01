#include <cassert>

#include "JackALSARawMidiSendQueue.h"
#include "JackMidiUtil.h"

using Jack::JackALSARawMidiSendQueue;

JackALSARawMidiSendQueue::JackALSARawMidiSendQueue(snd_rawmidi_t *rawmidi)
{
    this->rawmidi = rawmidi;
    blocked = false;
}

Jack::JackMidiWriteQueue::EnqueueResult
JackALSARawMidiSendQueue::EnqueueEvent(jack_nframes_t time, size_t size,
                                       jack_midi_data_t *buffer)
{
    assert(size == 1);
    if (time > GetCurrentFrame()) {
        return EVENT_EARLY;
    }
    ssize_t result = snd_rawmidi_write(rawmidi, buffer, 1);
    switch (result) {
    case 1:
        blocked = false;
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
