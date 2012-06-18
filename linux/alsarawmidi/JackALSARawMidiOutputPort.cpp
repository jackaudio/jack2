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

#include "JackALSARawMidiOutputPort.h"
#include "JackError.h"

using Jack::JackALSARawMidiOutputPort;

JackALSARawMidiOutputPort::JackALSARawMidiOutputPort(snd_rawmidi_info_t *info,
                                                     size_t index,
                                                     size_t max_bytes_per_poll,
                                                     size_t max_bytes,
                                                     size_t max_messages):
    JackALSARawMidiPort(info, index, POLLOUT)
{
    alsa_event = 0;
    read_queue = new JackMidiBufferReadQueue();
    std::auto_ptr<JackMidiBufferReadQueue> read_ptr(read_queue);
    send_queue = new JackALSARawMidiSendQueue(rawmidi, max_bytes_per_poll);
    std::auto_ptr<JackALSARawMidiSendQueue> send_ptr(send_queue);
    thread_queue = new JackMidiAsyncQueue(max_bytes, max_messages);
    std::auto_ptr<JackMidiAsyncQueue> thread_ptr(thread_queue);
    raw_queue = new JackMidiRawOutputWriteQueue(send_queue, max_bytes,
                                                max_messages, max_messages);
    thread_ptr.release();
    send_ptr.release();
    read_ptr.release();
}

JackALSARawMidiOutputPort::~JackALSARawMidiOutputPort()
{
    delete raw_queue;
    delete read_queue;
    delete send_queue;
    delete thread_queue;
}

bool
JackALSARawMidiOutputPort::ProcessJack(JackMidiBuffer *port_buffer,
                                       jack_nframes_t frames)
{
    read_queue->ResetMidiBuffer(port_buffer);
    bool enqueued = false;
    for (jack_midi_event_t *event = read_queue->DequeueEvent(); event;
         event = read_queue->DequeueEvent()) {
        switch (thread_queue->EnqueueEvent(event, frames)) {
        case JackMidiWriteQueue::BUFFER_FULL:
            jack_error("JackALSARawMidiOutputPort::ProcessJack - The thread "
                       "queue doesn't have enough room to enqueue a %d-byte "
                       "event.  Dropping event.", event->size);
            continue;
        case JackMidiWriteQueue::BUFFER_TOO_SMALL:
            jack_error("JackALSARawMidiOutputPort::ProcessJack - The thread "
                       "queue is too small to enqueue a %d-byte event.  "
                       "Dropping event.", event->size);
            continue;
        default:
            enqueued = true;
        }
    }
    return enqueued ? TriggerQueueEvent() : true;
}

bool
JackALSARawMidiOutputPort::ProcessPollEvents(bool handle_output, bool timeout,
                                             jack_nframes_t *frame)
{
    int io_event;
    int queue_event;
    send_queue->ResetPollByteCount();
    if (! handle_output) {
        assert(timeout);
        goto process_raw_queue;
    }
    io_event = GetIOPollEvent();
    if (io_event == -1) {
        return false;
    }
    queue_event = GetQueuePollEvent();
    if (queue_event == -1) {
        return false;
    }
    if (io_event || timeout) {
    process_raw_queue:
        // We call the 'Process' event early because there are events waiting
        // to be processed that either need to be sent now, or before now.
        raw_queue->Process();
    } else if (! queue_event) {
        return true;
    }
    if (! alsa_event) {
        alsa_event = thread_queue->DequeueEvent();
    }
    for (; alsa_event; alsa_event = thread_queue->DequeueEvent()) {
        switch (raw_queue->EnqueueEvent(alsa_event)) {
        case JackMidiWriteQueue::BUFFER_TOO_SMALL:
            jack_error("JackALSARawMidiOutputPort::ProcessQueues - The raw "
                       "output queue couldn't enqueue a %d-byte event.  "
                       "Dropping event.", alsa_event->size);
            // Fallthrough on purpose.
        case JackMidiWriteQueue::OK:
            continue;
        default:
            ;
        }

        // Try to free up some space by processing events early.
        *frame = raw_queue->Process();

        switch (raw_queue->EnqueueEvent(alsa_event)) {
        case JackMidiWriteQueue::BUFFER_FULL:
            goto set_io_events;
        case JackMidiWriteQueue::BUFFER_TOO_SMALL:
            // This shouldn't happen.
            assert(false);
        default:
            ;
        }
    }
    *frame = raw_queue->Process();
 set_io_events:
    bool blocked = send_queue->IsBlocked();
    SetIOEventsEnabled(blocked);
    if (blocked) {
        *frame = 0;
    }
    return true;
}
