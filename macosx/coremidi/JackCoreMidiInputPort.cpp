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

/**
 * Takes a MIDI status byte as argument and returns the expected size of the
 * associated MIDI event. Returns -1 on invalid status bytes AND on variable
 * size events (SysEx events).
 */
inline static int _expectedEventSize(const unsigned char& byte) {
    if (byte < 0x80) return -1; // not a valid status byte
    if (byte < 0xC0) return 3; // note on/off, note pressure, control change
    if (byte < 0xE0) return 2; // program change, channel pressure
    if (byte < 0xF0) return 3; // pitch wheel
    if (byte == 0xF0) return -1; // sysex message (variable size)
    if (byte == 0xF1) return 2; // time code per quarter frame
    if (byte == 0xF2) return 3; // sys. common song position pointer
    if (byte == 0xF3) return 2; // sys. common song select
    if (byte == 0xF4) return -1; // sys. common undefined / reserved
    if (byte == 0xF5) return -1; // sys. common undefined / reserved
    return 1; // tune request, end of SysEx, system real-time events
}

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
    running_status_buf[0] = 0;
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

    // TODO: maybe parsing should be done by JackMidiRawInputWriteQueue instead

    unsigned int packet_count = packet_list->numPackets;
    assert(packet_count);
    MIDIPacket *packet = (MIDIPacket *) packet_list->packet;
    for (unsigned int i = 0; i < packet_count; i++) {
        jack_midi_data_t *data = packet->data;
        size_t size = packet->length;
        assert(size);
        jack_midi_event_t event;
        // In a MIDIPacket there can be more than one (non SysEx) MIDI event.
        // However if the packet contains a SysEx event, it is guaranteed that
        // there are no other events in the same MIDIPacket.
        int k = 0; // index of the current MIDI event within current MIDIPacket
        int eventSize = 0; // theoretical size of the current MIDI event
        int chunkSize = 0; // actual size of the current MIDI event data consumed

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
                k = size; // don't loop in a MIDIPacket if its a SysEx
                goto send_event;
            }
            goto get_next_packet;
        }

    parse_event:
        if (data[k+0] == 0xf0) {
            // Must actually never happen, since CoreMIDI guarantees a SysEx
            // message to be alone in one MIDIPaket, but safety first. The SysEx
            // buffer code is not written to handle this case, so skip packet.
            if (k != 0) {
                jack_error("JackCoreMidiInputPort::ProcessCoreMidi - Non "
                           "isolated SysEx message in one packet, discarding.");
                goto get_next_packet;
            }

            if (data[size - 1] != 0xf7) {
                goto buffer_sysex_bytes;
            }
        }

        // not a regular status byte ?
        if (!(data[k+0] & 0x80) && running_status_buf[0]) { // "running status" mode ...
            eventSize = _expectedEventSize(running_status_buf[0]);
            chunkSize = (eventSize < 0) ? size - k : eventSize - 1;
            if (chunkSize <= 0) goto get_next_packet;
            if (chunkSize + 1 <= sizeof(running_status_buf)) {
                memcpy(&running_status_buf[1], &data[k], chunkSize);
                event.buffer = running_status_buf;
                event.size = chunkSize + 1;
                k += chunkSize;
                goto send_event;
            }
        }

        // valid status byte (or invalid "running status") ...

        eventSize = _expectedEventSize(data[k+0]);
        if (eventSize < 0) eventSize = size - k;
        if (eventSize <= 0) goto get_next_packet;
        event.buffer = &data[k];
        event.size = eventSize;
        // store status byte for eventual "running status" in next event
        if (data[k+0] & 0x80) {
            if (data[k+0] < 0xf0) {
                // "running status" is only allowed for channel messages
                running_status_buf[0] = data[k+0];
            } else if (data[k+0] < 0xf8) {
                // "system common" messages (0xf0..0xf7) shall reset any running
                // status, however "realtime" messages (0xf8..0xff) shall be
                // ignored here
                running_status_buf[0] = 0;
            }
        }
        k += eventSize;

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
        if (k < size) goto parse_event;

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
    running_status_buf[0] = 0;
    return true;
}

bool
JackCoreMidiInputPort::Stop()
{
    return true;
}
