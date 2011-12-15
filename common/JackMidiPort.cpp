/*
Copyright (C) 2007 Dmitry Baikov
Original JACK MIDI implementation Copyright (C) 2004 Ian Esten

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

#include "JackError.h"
#include "JackPortType.h"
#include "JackMidiPort.h"
#include <assert.h>
#include <string.h>

namespace Jack
{

SERVER_EXPORT void JackMidiBuffer::Reset(jack_nframes_t nframes)
{
    /* This line ate 1 hour of my life... dsbaikov */
    this->nframes = nframes;
    write_pos = 0;
    event_count = 0;
    lost_events = 0;
    mix_index = 0;
}

SERVER_EXPORT jack_shmsize_t JackMidiBuffer::MaxEventSize() const
{
    assert (((jack_shmsize_t) - 1) < 0); // jack_shmsize_t should be signed
    jack_shmsize_t left = buffer_size - (sizeof(JackMidiBuffer) + sizeof(JackMidiEvent) * (event_count + 1) + write_pos);
    if (left < 0)
        return 0;
    if (left <= JackMidiEvent::INLINE_SIZE_MAX)
        return JackMidiEvent::INLINE_SIZE_MAX;
    return left;
}

SERVER_EXPORT jack_midi_data_t* JackMidiBuffer::ReserveEvent(jack_nframes_t time, jack_shmsize_t size)
{
    jack_shmsize_t space = MaxEventSize();
    if (space == 0 || size > space) {
        jack_error("JackMidiBuffer::ReserveEvent - the buffer does not have "
                   "enough room to enqueue a %lu byte event", size);
        lost_events++;
        return 0;
    }
    JackMidiEvent* event = &events[event_count++];
    event->time = time;
    event->size = size;
    if (size <= JackMidiEvent::INLINE_SIZE_MAX)
        return event->data;

    write_pos += size;
    event->offset = buffer_size - write_pos;
    return (jack_midi_data_t*)this + event->offset;
}

static void MidiBufferInit(void* buffer, size_t buffer_size, jack_nframes_t nframes)
{
    JackMidiBuffer* midi = (JackMidiBuffer*)buffer;
    midi->magic = JackMidiBuffer::MAGIC;
    /* Since port buffer has actually always BUFFER_SIZE_MAX frames, we can safely use all the size */
    midi->buffer_size = BUFFER_SIZE_MAX * sizeof(jack_default_audio_sample_t);
    midi->Reset(nframes);
}

/*
 * The mixdown function below, is a simplest (read slowest) implementation possible.
 * But, since it is unlikely that it will mix many buffers with many events,
 * it should perform quite good.
 * More efficient (and possibly, fastest possible) implementation (it exists),
 * using calendar queue algorithm is about 3 times bigger, and uses alloca().
 * So, let's listen to D.Knuth about premature optimisation, a leave the current
 * implementation as is, until it is proved to be a bottleneck.
 * Dmitry Baikov.
 */
static void MidiBufferMixdown(void* mixbuffer, void** src_buffers, int src_count, jack_nframes_t nframes)
{
    JackMidiBuffer* mix = static_cast<JackMidiBuffer*>(mixbuffer);
    if (!mix->IsValid()) {
        jack_error("Jack::MidiBufferMixdown - invalid mix buffer");
        return;
    }
    mix->Reset(nframes);

    int event_count = 0;
    for (int i = 0; i < src_count; ++i) {
        JackMidiBuffer* buf = static_cast<JackMidiBuffer*>(src_buffers[i]);
        if (!buf->IsValid()) {
            jack_error("Jack::MidiBufferMixdown - invalid source buffer");
            return;
        }
        buf->mix_index = 0;
        event_count += buf->event_count;
        mix->lost_events += buf->lost_events;
    }

    int events_done;
    for (events_done = 0; events_done < event_count; ++events_done) {
        JackMidiBuffer* next_buf = 0;
        JackMidiEvent* next_event = 0;

        // find the earliest event
        for (int i = 0; i < src_count; ++i) {
            JackMidiBuffer* buf = static_cast<JackMidiBuffer*>(src_buffers[i]);
            if (buf->mix_index >= buf->event_count)
                continue;
            JackMidiEvent* e = &buf->events[buf->mix_index];
            if (!next_event || e->time < next_event->time) {
                next_event = e;
                next_buf = buf;
            }
        }
        assert(next_event != 0);

        // write the event
        jack_midi_data_t* dest = mix->ReserveEvent(next_event->time, next_event->size);
        if (!dest)
            break;
        memcpy(dest, next_event->GetData(next_buf), next_event->size);
        next_buf->mix_index++;
    }
    mix->lost_events += event_count - events_done;
}

static size_t MidiBufferSize()
{
    return BUFFER_SIZE_MAX * sizeof(jack_default_audio_sample_t);
}

const JackPortType gMidiPortType =
{
    JACK_DEFAULT_MIDI_TYPE,
    MidiBufferSize,
    MidiBufferInit,
    MidiBufferMixdown
};

} // namespace Jack
