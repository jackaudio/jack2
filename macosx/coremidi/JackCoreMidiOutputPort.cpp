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
#include <new>

#include "JackCoreMidiOutputPort.h"
#include "JackMidiUtil.h"

using Jack::JackCoreMidiOutputPort;

JackCoreMidiOutputPort::JackCoreMidiOutputPort(double time_ratio,
                                               size_t max_bytes,
                                               size_t max_messages):
    JackCoreMidiPort(time_ratio)
{
    read_queue = new JackMidiBufferReadQueue();
    std::auto_ptr<JackMidiBufferReadQueue> read_ptr(read_queue);
    thread_queue = new JackMidiAsyncWaitQueue(max_bytes, max_messages);
    std::auto_ptr<JackMidiAsyncWaitQueue> thread_ptr(thread_queue);
    thread = new JackThread(this);
    thread_ptr.release();
    read_ptr.release();
}

JackCoreMidiOutputPort::~JackCoreMidiOutputPort()
{
    Stop();
    delete thread;
    delete read_queue;
    delete thread_queue;
}

bool
JackCoreMidiOutputPort::Execute()
{
    jack_midi_event_t *event = 0;
    MIDIPacketList *packet_list = (MIDIPacketList *) packet_buffer;
    for (;;) {
        MIDIPacket *packet = MIDIPacketListInit(packet_list);
        assert(packet);
        if (! event) {
            event = thread_queue->DequeueEvent((long) 0);
        }
        jack_midi_data_t *data = event->buffer;

        // This is the latest time that the packet list can be sent out.  We
        // may want to consider subtracting some frames to leave room for the
        // CoreMIDI driver/client to handle all of the events.  There's a
        // property called 'kMIDIPropertyAdvanceScheduleTimeMuSec' that might
        // be useful in this case.
        jack_nframes_t send_time = event->time;

        size_t size = event->size;
        MIDITimeStamp timestamp = GetTimeStampFromFrames(send_time);
        packet = MIDIPacketListAdd(packet_list, PACKET_BUFFER_SIZE, packet,
                                   timestamp, size, data);
        if (packet) {
            while (GetCurrentFrame() < send_time) {
                event = thread_queue->DequeueEvent();
                if (! event) {
                    break;
                }
                packet = MIDIPacketListAdd(packet_list, sizeof(packet_buffer),
                                           packet,
                                           GetTimeStampFromFrames(event->time),
                                           event->size, event->buffer);
                if (! packet) {
                    break;
                }
            }
            SendPacketList(packet_list);
        } else {

            // We have a large system exclusive event.  We'll have to send it
            // out in multiple packets.
            size_t bytes_sent = 0;
            do {
                packet = MIDIPacketListInit(packet_list);
                assert(packet);
                size_t num_bytes = 0;
                for (; bytes_sent < size; bytes_sent += num_bytes) {
                    size_t num_bytes = size - bytes_sent;

                    // We use 256 because the MIDIPacket struct defines the
                    // size of the 'data' member to be 256 bytes.  I believe
                    // this prevents packets from being dynamically allocated
                    // by 'MIDIPacketListAdd', but I might be wrong.
                    if (num_bytes > 256) {
                        num_bytes = 256;
                    }
                    packet = MIDIPacketListAdd(packet_list,
                                               sizeof(packet_buffer), packet,
                                               timestamp, num_bytes,
                                               data + bytes_sent);
                    if (! packet) {
                        break;
                    }
                }
                if (! SendPacketList(packet_list)) {
                    // An error occurred.  The error message has already been
                    // output.  We lick our wounds and move along.
                    break;
                }
            } while (bytes_sent < size);
            event = 0;
        }
    }
    return false;
}

MIDITimeStamp
JackCoreMidiOutputPort::GetTimeStampFromFrames(jack_nframes_t frames)
{
    return GetTimeFromFrames(frames) / time_ratio;
}

bool
JackCoreMidiOutputPort::Init()
{
    set_threaded_log_function();

    // OSX only...
    UInt64 period = 0;
    UInt64 computation = 500 * 1000;
    UInt64 constraint = 500 * 1000;
    thread->SetParams(period, computation, constraint);

    // Use the server priority : y
    if (thread->AcquireSelfRealTime()) {
        jack_error("JackCoreMidiOutputPort::Init - could not acquire realtime "
                   "scheduling.  Continuing anyway.");
    }
    return true;
}

void
JackCoreMidiOutputPort::Initialize(const char *alias_name,
                                  const char *client_name,
                                  const char *driver_name, int index,
                                  MIDIEndpointRef endpoint)
{
    JackCoreMidiPort::Initialize(alias_name, client_name, driver_name, index, endpoint, true);
}

void
JackCoreMidiOutputPort::ProcessJack(JackMidiBuffer *port_buffer,
                                   jack_nframes_t frames)
{
    read_queue->ResetMidiBuffer(port_buffer);
    for (jack_midi_event_t *event = read_queue->DequeueEvent(); event;
         event = read_queue->DequeueEvent()) {
        switch (thread_queue->EnqueueEvent(event, frames)) {
        case JackMidiWriteQueue::BUFFER_FULL:
            jack_error("JackCoreMidiOutputPort::ProcessJack - The thread "
                       "queue buffer is full.  Dropping event.");
            continue;
        case JackMidiWriteQueue::BUFFER_TOO_SMALL:
            jack_error("JackCoreMidiOutputPort::ProcessJack - The thread "
                       "queue couldn't enqueue a %d-byte event.  Dropping "
                       "event.", event->size);
            // Fallthrough on purpose
        default:
            ;
        }
    }
}

bool
JackCoreMidiOutputPort::Start()
{
    bool result = thread->GetStatus() != JackThread::kIdle;
    if (! result) {
        result = ! thread->StartSync();
        if (! result) {
            jack_error("JackCoreMidiOutputPort::Start - failed to start MIDI "
                       "processing thread.");
        }
    }
    return result;
}

bool
JackCoreMidiOutputPort::Stop()
{
    bool result = thread->GetStatus() == JackThread::kIdle;
    if (! result) {
        result = ! thread->Kill();
        if (! result) {
            jack_error("JackCoreMidiOutputPort::Stop - failed to stop MIDI "
                       "processing thread.");
        }
    }
    return result;
}
