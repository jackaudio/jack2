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
#include <cerrno>
#include <cstring>
#include <new>
#include <stdexcept>

#include "JackCoreMidiOutputPort.h"
#include "JackMidiUtil.h"
#include "JackTime.h"
#include "JackError.h"

using Jack::JackCoreMidiOutputPort;

JackCoreMidiOutputPort::JackCoreMidiOutputPort(double time_ratio,
                                               size_t max_bytes,
                                               size_t max_messages):
    JackCoreMidiPort(time_ratio)
{
    read_queue = new JackMidiBufferReadQueue();
    std::auto_ptr<JackMidiBufferReadQueue> read_queue_ptr(read_queue);
    thread_queue = new JackMidiAsyncQueue(max_bytes, max_messages);
    std::auto_ptr<JackMidiAsyncQueue> thread_queue_ptr(thread_queue);
    thread = new JackThread(this);
    std::auto_ptr<JackThread> thread_ptr(thread);
    snprintf(semaphore_name, sizeof(semaphore_name), "coremidi_%p", this);
    thread_queue_semaphore = sem_open(semaphore_name, O_CREAT, 0777, 0);
    if (thread_queue_semaphore == (sem_t *) SEM_FAILED) {
        throw std::runtime_error(strerror(errno));
    }
    advance_schedule_time = 0;
    thread_ptr.release();
    thread_queue_ptr.release();
    read_queue_ptr.release();
}

JackCoreMidiOutputPort::~JackCoreMidiOutputPort()
{
    delete thread;
    sem_close(thread_queue_semaphore);
    sem_unlink(semaphore_name);
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
            event = GetCoreMidiEvent(true);
        }
        jack_midi_data_t *data = event->buffer;
        jack_nframes_t send_frame = event->time;
        jack_time_t send_time =
            GetTimeFromFrames(send_frame) - advance_schedule_time;
        size_t size = event->size;
        MIDITimeStamp timestamp = GetTimeStampFromFrames(send_frame);
        packet = MIDIPacketListAdd(packet_list, PACKET_BUFFER_SIZE, packet,
                                   timestamp, size, data);
        if (packet) {
            do {
                if (GetMicroSeconds() >= send_time) {
                    event = 0;
                    break;
                }
                event = GetCoreMidiEvent(false);
                if (! event) {
                    break;
                }
                packet = MIDIPacketListAdd(packet_list, sizeof(packet_buffer),
                                           packet,
                                           GetTimeStampFromFrames(event->time),
                                           event->size, event->buffer);
            } while (packet);
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

jack_midi_event_t *
JackCoreMidiOutputPort::GetCoreMidiEvent(bool block)
{
    if (! block) {
        if (sem_trywait(thread_queue_semaphore)) {
            if (errno != EAGAIN) {
                jack_error("JackCoreMidiOutputPort::Execute - sem_trywait: %s",
                           strerror(errno));
            }
            return 0;
        }
    } else {
        while (sem_wait(thread_queue_semaphore)) {
            if (errno != EINTR) {
                jack_error("JackCoreMidiOutputPort::Execute - sem_wait: %s",
                           strerror(errno));
                return 0;
            }
        }
    }
    return thread_queue->DequeueEvent();
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

    // OSX only, values read in RT CoreMIDI thread
    UInt64 period = 0;
    UInt64 computation = 250 * 1000;
    UInt64 constraint = 500 * 1000;
    thread->SetParams(period, computation, constraint);

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
                                   MIDIEndpointRef endpoint,
                                   SInt32 advance_schedule_time)
{
    JackCoreMidiPort::Initialize(alias_name, client_name, driver_name, index,
                                 endpoint, true);
    assert(advance_schedule_time >= 0);
    this->advance_schedule_time = advance_schedule_time;
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
            break;
        case JackMidiWriteQueue::BUFFER_TOO_SMALL:
            jack_error("JackCoreMidiOutputPort::ProcessJack - The thread "
                       "queue couldn't enqueue a %d-byte event.  Dropping "
                       "event.", event->size);
            break;
        default:
            if (sem_post(thread_queue_semaphore)) {
                jack_error("JackCoreMidiOutputPort::ProcessJack - unexpected "
                           "error while posting to thread queue semaphore: %s",
                           strerror(errno));
            }
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
