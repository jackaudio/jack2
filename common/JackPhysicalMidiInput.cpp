/*
Copyright (C) 2009 Devin Anderson

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
#include <cstring>
#include <new>

#include "JackError.h"
#include "JackPhysicalMidiInput.h"

namespace Jack {

JackPhysicalMidiInput::JackPhysicalMidiInput(size_t buffer_size)
{
    size_t datum_size = sizeof(jack_midi_data_t);
    assert(buffer_size > 0);
    input_ring = jack_ringbuffer_create((buffer_size + 1) * datum_size);
    if (! input_ring) {
        throw std::bad_alloc();
    }
    jack_ringbuffer_mlock(input_ring);
    Clear();
    expected_data_bytes = 0;
    status_byte = 0;
}

JackPhysicalMidiInput::~JackPhysicalMidiInput()
{
    jack_ringbuffer_free(input_ring);
}

void
JackPhysicalMidiInput::Clear()
{
    jack_ringbuffer_reset(input_ring);
    buffered_bytes = 0;
    unbuffered_bytes = 0;
}

void
JackPhysicalMidiInput::HandleBufferFailure(size_t unbuffered_bytes,
                                           size_t total_bytes)
{
    jack_error("%d MIDI byte(s) of a %d byte message could not be buffered - "
               "message dropped", unbuffered_bytes, total_bytes);
}

void
JackPhysicalMidiInput::HandleIncompleteMessage(size_t bytes)
{
    jack_error("Discarding %d MIDI byte(s) - incomplete message (cable "
               "unplugged?)", bytes);
}

void
JackPhysicalMidiInput::HandleInvalidStatusByte(jack_midi_data_t status)
{
    jack_error("Dropping invalid MIDI status byte '%x'",
               (unsigned int) status);
}

void
JackPhysicalMidiInput::HandleUnexpectedSysexEnd(size_t bytes)
{
    jack_error("Discarding %d MIDI byte(s) - received sysex end without sysex "
               "start (cable unplugged?)", bytes);
}

void
JackPhysicalMidiInput::HandleWriteFailure(size_t bytes)
{
    jack_error("Failed to write a %d byte MIDI message to the port buffer",
               bytes);
}

void
JackPhysicalMidiInput::Process(jack_nframes_t frames)
{
    assert(port_buffer);
    port_buffer->Reset(frames);
    jack_nframes_t current_frame = 0;
    size_t datum_size = sizeof(jack_midi_data_t);
    for (;;) {
        jack_midi_data_t datum;
        current_frame = Receive(&datum, current_frame, frames);
        if (current_frame >= frames) {
            break;
        }

        jack_log("JackPhysicalMidiInput::Process (%d) - Received '%x' byte",
                 current_frame, (unsigned int) datum);

        if (datum >= 0xf8) {
            // Realtime
            if (datum == 0xfd) {
                HandleInvalidStatusByte(datum);
            } else {

                jack_log("JackPhysicalMidiInput::Process - Writing realtime "
                         "event.");

                WriteByteEvent(current_frame, datum);
            }
            continue;
        }
        if (datum == 0xf7) {
            // Sysex end
            if (status_byte != 0xf0) {
                HandleUnexpectedSysexEnd(buffered_bytes + unbuffered_bytes);
                Clear();
                expected_data_bytes = 0;
                status_byte = 0;
            } else {

                jack_log("JackPhysicalMidiInput::Process - Writing sysex "
                         "event.");

                WriteBufferedSysexEvent(current_frame);
            }
            continue;
        }
        if (datum >= 0x80) {

            // We're handling a non-realtime status byte

            jack_log("JackPhysicalMidiInput::Process - Handling non-realtime "
                     "status byte.");

            if (buffered_bytes || unbuffered_bytes) {
                HandleIncompleteMessage(buffered_bytes + unbuffered_bytes + 1);
                Clear();
            }
            status_byte = datum;
            switch (datum & 0xf0) {
            case 0x80:
            case 0x90:
            case 0xa0:
            case 0xb0:
            case 0xe0:
                // Note On, Note Off, Aftertouch, Control Change, Pitch Wheel
                expected_data_bytes = 2;
                break;
            case 0xc0:
            case 0xd0:
                // Program Change, Channel Pressure
                expected_data_bytes = 1;
                break;
            case 0xf0:
                switch (datum) {
                case 0xf0:
                    // Sysex message
                    expected_data_bytes = 0;
                    break;
                case 0xf1:
                case 0xf3:
                    // MTC Quarter frame, Song Select
                    expected_data_bytes = 1;
                    break;
                case 0xf2:
                    // Song Position
                    expected_data_bytes = 2;
                    break;
                case 0xf4:
                case 0xf5:
                    // Undefined
                    HandleInvalidStatusByte(datum);
                    expected_data_bytes = 0;
                    status_byte = 0;
                    break;
                case 0xf6:
                    // Tune Request
                    WriteByteEvent(current_frame, datum);
                    expected_data_bytes = 0;
                    status_byte = 0;
                }
                break;
            }
            continue;
        }

        // We're handling a data byte

        jack_log("JackPhysicalMidiInput::Process - Buffering data byte.");

        if (jack_ringbuffer_write(input_ring, (const char *) &datum,
                                  datum_size) == datum_size) {
            buffered_bytes++;
        } else {
            unbuffered_bytes++;
        }
        unsigned long total_bytes = buffered_bytes + unbuffered_bytes;
        assert((! expected_data_bytes) ||
               (total_bytes <= expected_data_bytes));
        if (total_bytes == expected_data_bytes) {
            if (! unbuffered_bytes) {

                jack_log("JackPhysicalMidiInput::Process - Writing buffered "
                         "event.");

                WriteBufferedEvent(current_frame);
            } else {
                HandleBufferFailure(unbuffered_bytes, total_bytes);
                Clear();
            }
            if (status_byte >= 0xf0) {
                expected_data_bytes = 0;
                status_byte = 0;
            }
        }
    }
}

void
JackPhysicalMidiInput::WriteBufferedEvent(jack_nframes_t frame)
{
    assert(port_buffer && port_buffer->IsValid());
    size_t space = jack_ringbuffer_read_space(input_ring);
    jack_midi_data_t *event = port_buffer->ReserveEvent(frame, space + 1);
    if (event) {
        jack_ringbuffer_data_t vector[2];
        jack_ringbuffer_get_read_vector(input_ring, vector);
        event[0] = status_byte;
        size_t data_length_1 = vector[0].len;
        memcpy(event + 1, vector[0].buf, data_length_1);
        size_t data_length_2 = vector[1].len;
        if (data_length_2) {
            memcpy(event + data_length_1 + 1, vector[1].buf, data_length_2);
        }
    } else {
        HandleWriteFailure(space + 1);
    }
    Clear();
}

void
JackPhysicalMidiInput::WriteBufferedSysexEvent(jack_nframes_t frame)
{
    assert(port_buffer && port_buffer->IsValid());
    size_t space = jack_ringbuffer_read_space(input_ring);
    jack_midi_data_t *event = port_buffer->ReserveEvent(frame, space + 2);
    if (event) {
        jack_ringbuffer_data_t vector[2];
        jack_ringbuffer_get_read_vector(input_ring, vector);
        event[0] = status_byte;
        size_t data_length_1 = vector[0].len;
        memcpy(event + 1, vector[0].buf, data_length_1);
        size_t data_length_2 = vector[1].len;
        if (data_length_2) {
            memcpy(event + data_length_1 + 1, vector[1].buf, data_length_2);
        }
        event[data_length_1 + data_length_2 + 1] = 0xf7;
    } else {
        HandleWriteFailure(space + 2);
    }
    Clear();
}

void
JackPhysicalMidiInput::WriteByteEvent(jack_nframes_t frame,
                                      jack_midi_data_t datum)
{
    assert(port_buffer && port_buffer->IsValid());
    jack_midi_data_t *event = port_buffer->ReserveEvent(frame, 1);
    if (event) {
        event[0] = datum;
    } else {
        HandleWriteFailure(1);
    }
}

}
