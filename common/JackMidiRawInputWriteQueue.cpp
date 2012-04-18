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
#include <memory>
#include <new>

#include "JackMidiRawInputWriteQueue.h"
#include "JackError.h"

using Jack::JackMidiRawInputWriteQueue;

JackMidiRawInputWriteQueue::
JackMidiRawInputWriteQueue(JackMidiWriteQueue *write_queue,
                           size_t max_packet_data, size_t max_packets)
{
    packet_queue = new JackMidiAsyncQueue(max_packet_data, max_packets);
    std::auto_ptr<JackMidiAsyncQueue> packet_queue_ptr(packet_queue);
    input_buffer = new jack_midi_data_t[max_packet_data];
    Clear();
    expected_bytes = 0;
    event_pending = false;
    input_buffer_size = max_packet_data;
    packet = 0;
    status_byte = 0;
    this->write_queue = write_queue;
    packet_queue_ptr.release();
}

JackMidiRawInputWriteQueue::~JackMidiRawInputWriteQueue()
{
    delete[] input_buffer;
    delete packet_queue;
}

void
JackMidiRawInputWriteQueue::Clear()
{
    total_bytes = 0;
    unbuffered_bytes = 0;
}

Jack::JackMidiWriteQueue::EnqueueResult
JackMidiRawInputWriteQueue::EnqueueEvent(jack_nframes_t time, size_t size,
                                         jack_midi_data_t *buffer)
{
    return packet_queue->EnqueueEvent(time, size, buffer);
}

size_t
JackMidiRawInputWriteQueue::GetAvailableSpace()
{
    return packet_queue->GetAvailableSpace();
}

void
JackMidiRawInputWriteQueue::HandleBufferFailure(size_t unbuffered_bytes,
                                                size_t total_bytes)
{
    jack_error("JackMidiRawInputWriteQueue::HandleBufferFailure - %d MIDI "
               "byte(s) of a %d byte message could not be buffered.  The "
               "message has been dropped.", unbuffered_bytes, total_bytes);
}

void
JackMidiRawInputWriteQueue::HandleEventLoss(jack_midi_event_t *event)
{
    jack_error("JackMidiRawInputWriteQueue::HandleEventLoss - A %d byte MIDI "
               "event scheduled for frame '%d' could not be processed because "
               "the write queue cannot accomodate an event of that size.  The "
               "event has been discarded.", event->size, event->time);
}

void
JackMidiRawInputWriteQueue::HandleIncompleteMessage(size_t total_bytes)
{
    jack_error("JackMidiRawInputWriteQueue::HandleIncompleteMessage - "
               "Discarding %d MIDI byte(s) of an incomplete message.  The "
               "MIDI cable may have been unplugged.", total_bytes);
}

void
JackMidiRawInputWriteQueue::HandleInvalidStatusByte(jack_midi_data_t byte)
{
    jack_error("JackMidiRawInputWriteQueue::HandleInvalidStatusByte - "
               "Dropping invalid MIDI status byte '%x'.", (unsigned int) byte);
}

void
JackMidiRawInputWriteQueue::HandleUnexpectedSysexEnd(size_t total_bytes)
{
    jack_error("JackMidiRawInputWriteQueue::HandleUnexpectedSysexEnd - "
               "Received a sysex end byte without first receiving a sysex "
               "start byte.  Discarding %d MIDI byte(s).  The cable may have "
               "been unplugged.", total_bytes);
}

bool
JackMidiRawInputWriteQueue::PrepareBufferedEvent(jack_nframes_t time)
{
    bool result = ! unbuffered_bytes;
    if (! result) {
        HandleBufferFailure(unbuffered_bytes, total_bytes);
    } else {
        PrepareEvent(time, total_bytes, input_buffer);
    }
    Clear();
    if (status_byte >= 0xf0) {
        expected_bytes = 0;
        status_byte = 0;
    }
    return result;
}

bool
JackMidiRawInputWriteQueue::PrepareByteEvent(jack_nframes_t time,
                                             jack_midi_data_t byte)
{
    event_byte = byte;
    PrepareEvent(time, 1, &event_byte);
    return true;
}

void
JackMidiRawInputWriteQueue::PrepareEvent(jack_nframes_t time, size_t size,
                                         jack_midi_data_t *buffer)
{
    event.buffer = buffer;
    event.size = size;
    event.time = time;
    event_pending = true;
}

jack_nframes_t
JackMidiRawInputWriteQueue::Process(jack_nframes_t boundary_frame)
{
    if (event_pending) {
        if (! WriteEvent(boundary_frame)) {
            return event.time;
        }
    }
    if (! packet) {
        packet = packet_queue->DequeueEvent();
    }
    for (; packet; packet = packet_queue->DequeueEvent()) {
        for (; packet->size; (packet->buffer)++, (packet->size)--) {
            if (ProcessByte(packet->time, *(packet->buffer))) {
                if (! WriteEvent(boundary_frame)) {
                    (packet->buffer)++;
                    (packet->size)--;
                    return event.time;
                }
            }
        }
    }
    return 0;
}

bool
JackMidiRawInputWriteQueue::ProcessByte(jack_nframes_t time,
                                        jack_midi_data_t byte)
{
    if (byte >= 0xf8) {
        // Realtime
        if (byte == 0xfd) {
            HandleInvalidStatusByte(byte);
            return false;
        }
        return PrepareByteEvent(time, byte);
    }
    if (byte == 0xf7) {
        // Sysex end
        if (status_byte == 0xf0) {
            RecordByte(byte);
            return PrepareBufferedEvent(time);
        }
        HandleUnexpectedSysexEnd(total_bytes);
        Clear();
        expected_bytes = 0;
        status_byte = 0;
        return false;
    }
    if (byte >= 0x80) {
        // Non-realtime status byte
        if (total_bytes) {
            HandleIncompleteMessage(total_bytes);
            Clear();
        }
        status_byte = byte;
        switch (byte & 0xf0) {
        case 0x80:
        case 0x90:
        case 0xa0:
        case 0xb0:
        case 0xe0:
            // Note On, Note Off, Aftertouch, Control Change, Pitch Wheel
            expected_bytes = 3;
            break;
        case 0xc0:
        case 0xd0:
            // Program Change, Channel Pressure
            expected_bytes = 2;
            break;
        case 0xf0:
            switch (byte) {
            case 0xf0:
                // Sysex
                expected_bytes = 0;
                break;
            case 0xf1:
            case 0xf3:
                // MTC Quarter Frame, Song Select
                expected_bytes = 2;
                break;
            case 0xf2:
                // Song Position
                expected_bytes = 3;
                break;
            case 0xf4:
            case 0xf5:
                // Undefined
                HandleInvalidStatusByte(byte);
                expected_bytes = 0;
                status_byte = 0;
                return false;
            case 0xf6:
                // Tune Request
                bool result = PrepareByteEvent(time, byte);
                if (result) {
                    expected_bytes = 0;
                    status_byte = 0;
                }
                return result;
            }
        }
        RecordByte(byte);
        return false;
    }
    // Data byte
    if (! status_byte) {
        // Data bytes without a status will be discarded.
        total_bytes++;
        unbuffered_bytes++;
        return false;
    }
    if (! total_bytes) {
        // Apply running status.
        RecordByte(status_byte);
    }
    RecordByte(byte);
    return (total_bytes == expected_bytes) ? PrepareBufferedEvent(time) :
        false;
}

void
JackMidiRawInputWriteQueue::RecordByte(jack_midi_data_t byte)
{
    if (total_bytes < input_buffer_size) {
        input_buffer[total_bytes] = byte;
    } else {
        unbuffered_bytes++;
    }
    total_bytes++;
}

bool
JackMidiRawInputWriteQueue::WriteEvent(jack_nframes_t boundary_frame)
{
    if ((! boundary_frame) || (event.time < boundary_frame)) {
        switch (write_queue->EnqueueEvent(&event)) {
        case BUFFER_TOO_SMALL:
            HandleEventLoss(&event);
            // Fallthrough on purpose
        case OK:
            event_pending = false;
            return true;
        default:
            // This is here to stop compilers from warning us about not
            // handling enumeration values.
            ;
        }
    }
    return false;
}
