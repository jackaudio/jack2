/*
Copyright (C) 2007 Dmitry Baikov
Copyright (C) 2018 Filipe Coelho
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
#include "JackMessagePort.h"
#include <assert.h>
#include <string.h>

namespace Jack
{

SERVER_EXPORT void JackMessageBuffer::Reset(jack_nframes_t nframes)
{
    /* This line ate 1 hour of my life... dsbaikov */
    this->nframes = nframes;
    write_pos = 0;
    event_count = 0;
    lost_events = 0;
}

SERVER_EXPORT jack_shmsize_t JackMessageBuffer::MaxEventSize() const
{
    assert (((jack_shmsize_t) - 1) < 0); // jack_shmsize_t should be signed
    jack_shmsize_t left = buffer_size - (sizeof(JackMessageBuffer) + sizeof(JackMessageEvent) * (event_count + 1) + write_pos);
    if (left < 0) {
        return 0;
    }
    if (left <= JackMessageEvent::INLINE_SIZE_MAX) {
        return JackMessageEvent::INLINE_SIZE_MAX;
    }
    return left;
}

SERVER_EXPORT jack_message_data_t* JackMessageBuffer::ReserveEvent(jack_nframes_t time, jack_shmsize_t size)
{
    jack_shmsize_t space = MaxEventSize();
    if (space == 0 || size > space) {
        jack_error("JackMessageBuffer::ReserveEvent - the buffer does not have "
                   "enough room to enqueue a %lu byte event", size);
        lost_events++;
        return 0;
    }
    JackMessageEvent* event = &events[event_count++];
    event->time = time;
    event->size = size;
    
    if (size <= JackMessageEvent::INLINE_SIZE_MAX) {
        return event->data;
    }
   
    write_pos += size;
    event->offset = buffer_size - write_pos;
    return (jack_message_data_t*)this + event->offset;
}

void MessageBufferInit(void* buffer, size_t buffer_size, jack_nframes_t nframes)
{
    JackMessageBuffer* message = (JackMessageBuffer*)buffer;
    message->magic = JackMessageBuffer::MAGIC;
    /* Since port buffer has actually always BUFFER_SIZE_MAX frames, we can safely use all the size */
    message->buffer_size = BUFFER_SIZE_MAX * sizeof(jack_default_audio_sample_t);
    message->Reset(nframes);
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
static void MessageBufferMixdown(void* mixbuffer, void** src_buffers, int src_count, jack_nframes_t nframes)
{
    JackMessageBuffer* mix = static_cast<JackMessageBuffer*>(mixbuffer);
    if (!mix->IsValid()) {
        jack_error("Jack::MessageBufferMixdown - invalid mix buffer");
        return;
    }
    mix->Reset(nframes);

    uint32_t mix_index[src_count];
    int event_count = 0;
    for (int i = 0; i < src_count; ++i) {
        JackMessageBuffer* buf = static_cast<JackMessageBuffer*>(src_buffers[i]);
        if (!buf->IsValid()) {
            jack_error("Jack::MessageBufferMixdown - invalid source buffer");
            return;
        }
        mix_index[i] = 0;
        event_count += buf->event_count;
        mix->lost_events += buf->lost_events;
    }

    int events_done;
    for (events_done = 0; events_done < event_count; ++events_done) {
        JackMessageBuffer* next_buf = 0;
        JackMessageEvent* next_event = 0;
        uint32_t next_buf_index = 0;

        // find the earliest event
        for (int i = 0; i < src_count; ++i) {
            JackMessageBuffer* buf = static_cast<JackMessageBuffer*>(src_buffers[i]);
            if (mix_index[i] >= buf->event_count)
                continue;
            JackMessageEvent* e = &buf->events[mix_index[i]];
            if (!next_event || e->time < next_event->time) {
                next_event = e;
                next_buf = buf;
                next_buf_index = i;
            }
        }
        if (next_event == 0) {
            jack_error("Jack::MessageBufferMixdown - got invalid next event");
            break;
        }

        // write the event
        jack_message_data_t* dest = mix->ReserveEvent(next_event->time, next_event->size);
        if (!dest) break;

        memcpy(dest, next_event->GetData(next_buf), next_event->size);
        mix_index[next_buf_index]++;
    }
    mix->lost_events += event_count - events_done;
}

static size_t MessageBufferSize()
{
    return BUFFER_SIZE_MAX * sizeof(jack_default_audio_sample_t);
}

const JackPortType gMessagePortType =
{
    JACK_DEFAULT_MESSAGE_TYPE,
    MessageBufferSize,
    MessageBufferInit,
    MessageBufferMixdown
};

const JackPortType gMidiPortType = gMessagePortType;

} // namespace Jack
