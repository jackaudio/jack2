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

#include <new>

#include "JackMidiAsyncQueue.h"

using Jack::JackMidiAsyncQueue;

JackMidiAsyncQueue::JackMidiAsyncQueue(size_t max_bytes, size_t max_messages)
{
    data_buffer = new jack_midi_data_t[max_bytes];
    byte_ring = jack_ringbuffer_create((max_bytes * sizeof(jack_midi_data_t)) +
                                       1);
    if (byte_ring) {
        info_ring = jack_ringbuffer_create((max_messages * INFO_SIZE) + 1);
        if (info_ring) {
            jack_ringbuffer_mlock(byte_ring);
            jack_ringbuffer_mlock(info_ring);
            this->max_bytes = max_bytes;
            return;
        }
        jack_ringbuffer_free(byte_ring);
    }
    delete data_buffer;
    throw std::bad_alloc();
}

JackMidiAsyncQueue::~JackMidiAsyncQueue()
{
    jack_ringbuffer_free(byte_ring);
    jack_ringbuffer_free(info_ring);
    delete[] data_buffer;
}

jack_midi_event_t *
JackMidiAsyncQueue::DequeueEvent()
{
    jack_midi_event_t *event = 0;
    if (jack_ringbuffer_read_space(info_ring) >= INFO_SIZE) {
        event = &dequeue_event;
        jack_ringbuffer_read(info_ring, (char *) &(event->time),
                             sizeof(jack_nframes_t));
        size_t size;
        jack_ringbuffer_read(info_ring, (char *) &size, sizeof(size_t));
        event->buffer = data_buffer;
        event->size = size;
        jack_ringbuffer_data_t vector[2];
        jack_ringbuffer_get_read_vector(byte_ring, vector);
        size_t size1 = vector[0].len;
        memcpy(data_buffer, vector[0].buf, size1 * sizeof(jack_midi_data_t));
        if (size1 < size) {
            memcpy(data_buffer + size1, vector[1].buf,
                   (size - size1) * sizeof(jack_midi_data_t));
        }
        jack_ringbuffer_read_advance(byte_ring,
                                     size * sizeof(jack_midi_data_t));
    }
    return event;
}

Jack::JackMidiWriteQueue::EnqueueResult
JackMidiAsyncQueue::EnqueueEvent(jack_nframes_t time, size_t size,
                                 jack_midi_data_t *buffer)
{
    if (size > max_bytes) {
        return BUFFER_TOO_SMALL;
    }
    if (! ((jack_ringbuffer_write_space(info_ring) >= INFO_SIZE) &&
           (jack_ringbuffer_write_space(byte_ring) >=
            (size * sizeof(jack_midi_data_t))))) {
        return BUFFER_FULL;
    }
    jack_ringbuffer_write(byte_ring, (const char *) buffer,
                          size * sizeof(jack_midi_data_t));
    jack_ringbuffer_write(info_ring, (const char *) (&time),
                          sizeof(jack_nframes_t));
    jack_ringbuffer_write(info_ring, (const char *) (&size), sizeof(size_t));
    return OK;
}

size_t
JackMidiAsyncQueue::GetAvailableSpace()
{
    return jack_ringbuffer_write_space(info_ring) < INFO_SIZE ? 0 :
        max_bytes - jack_ringbuffer_read_space(byte_ring);
}
