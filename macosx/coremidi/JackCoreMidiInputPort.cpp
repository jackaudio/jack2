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
#include <memory>

#include "JackCoreMidiInputPort.h"
#include "JackMidiUtil.h"
#include "JackError.h"

using Jack::JackCoreMidiInputPort;

JackCoreMidiInputPort::JackCoreMidiInputPort(double time_ratio,
                                             size_t max_bytes,
                                             size_t max_messages):
    JackCoreMidiPort(time_ratio)
{
    thread_queue = new JackMidiAsyncQueue(max_bytes, max_messages);
    std::auto_ptr<JackMidiAsyncQueue> thread_queue_ptr(thread_queue);
    write_queue = new JackMidiBufferWriteQueue();
    std::auto_ptr<JackMidiBufferWriteQueue> write_queue_ptr(write_queue);
    sysex_buffer = new jack_midi_data_t[max_bytes];
    write_queue_ptr.release();
    thread_queue_ptr.release();
    jack_event = 0;
}

JackCoreMidiInputPort::~JackCoreMidiInputPort()
{
    delete thread_queue;
    delete write_queue;
    delete[] sysex_buffer;
}

jack_nframes_t
JackCoreMidiInputPort::GetFramesFromTimeStamp(MIDITimeStamp timestamp)
{
    return GetFramesFromTime((jack_time_t) (timestamp * time_ratio));
}

void
JackCoreMidiInputPort::Initialize(const char *alias_name,
                                  const char *client_name,
                                  const char *driver_name, int index,
                                  MIDIEndpointRef endpoint)
{
    JackCoreMidiPort::Initialize(alias_name, client_name, driver_name, index, endpoint, false);
}

void
JackCoreMidiInputPort::ProcessCoreMidi(const MIDIPacketList *packet_list)
{
    set_threaded_log_function();

    unsigned int packet_count = packet_list->numPackets;
    assert(packet_count);
    MIDIPacket *packet = (MIDIPacket *) packet_list->packet;
    for (unsigned int i = 0; i < packet_count; i++) {
        jack_midi_data_t *data = packet->data;
        size_t size = packet->length;
        assert(size);
        jack_midi_event_t event;

        // XX: There might be dragons in my spaghetti.  This code is begging
        // for a rewrite.

        if (sysex_bytes_sent) {
            if (data[0] & 0x80) {
                jack_error("JackCoreMidiInputPort::ProcessCoreMidi - System "
                           "exclusive message aborted.");
                sysex_bytes_sent = 0;
                goto parse_event;
            }
        buffer_sysex_bytes:
            if ((sysex_bytes_sent + size) <= sizeof(sysex_buffer)) {
                memcpy(sysex_buffer + sysex_bytes_sent, packet,
                       size * sizeof(jack_midi_data_t));
            }
            sysex_bytes_sent += size;
            if (data[size - 1] == 0xf7) {
                if (sysex_bytes_sent > sizeof(sysex_buffer)) {
                    jack_error("JackCoreMidiInputPort::ProcessCoreMidi - "
                               "Could not buffer a %d-byte system exclusive "
                               "message.  Discarding message.",
                               sysex_bytes_sent);
                    sysex_bytes_sent = 0;
                    goto get_next_packet;
                }
                event.buffer = sysex_buffer;
                event.size = sysex_bytes_sent;
                sysex_bytes_sent = 0;
                goto send_event;
            }
            goto get_next_packet;
        }

    parse_event:
        if (data[0] == 0xf0) {
            if (data[size - 1] != 0xf7) {
                goto buffer_sysex_bytes;
            }
        }
        event.buffer = data;
        event.size = size;

    send_event:
        event.time = GetFramesFromTimeStamp(packet->timeStamp);
        switch (thread_queue->EnqueueEvent(&event)) {
        case JackMidiWriteQueue::BUFFER_FULL:
            jack_error("JackCoreMidiInputPort::ProcessCoreMidi - The thread "
                       "queue buffer is full.  Dropping event.");
            break;
        case JackMidiWriteQueue::BUFFER_TOO_SMALL:
            jack_error("JackCoreMidiInputPort::ProcessCoreMidi - The thread "
                       "queue couldn't enqueue a %d-byte packet.  Dropping "
                       "event.", event.size);
            break;
        default:
            ;
        }

    get_next_packet:
        packet = MIDIPacketNext(packet);
        assert(packet);
    }
}

void
JackCoreMidiInputPort::ProcessJack(JackMidiBuffer *port_buffer,
                                   jack_nframes_t frames)
{
    write_queue->ResetMidiBuffer(port_buffer, frames);
    if (! jack_event) {
        jack_event = thread_queue->DequeueEvent();
    }

    for (; jack_event; jack_event = thread_queue->DequeueEvent()) {
        // Add 'frames' to MIDI events to align with audio.
        switch (write_queue->EnqueueEvent(jack_event, frames)) {
        case JackMidiWriteQueue::BUFFER_TOO_SMALL:
            jack_error("JackCoreMidiInputPort::ProcessJack - The write queue "
                       "couldn't enqueue a %d-byte event. Dropping event.",
                       jack_event->size);
            // Fallthrough on purpose
        case JackMidiWriteQueue::OK:
            continue;
        default:
            ;
        }
        break;
    }
}

bool
JackCoreMidiInputPort::Start()
{
    // Hack: Get rid of any messages that might have come in before starting
    // the engine.
    while (thread_queue->DequeueEvent());
    sysex_bytes_sent = 0;
    return true;
}

bool
JackCoreMidiInputPort::Stop()
{
    return true;
}
