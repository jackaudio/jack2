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

#include "JackALSARawMidiInputPort.h"
#include "JackMidiUtil.h"
#include "JackError.h"

using Jack::JackALSARawMidiInputPort;

JackALSARawMidiInputPort::JackALSARawMidiInputPort(snd_rawmidi_info_t *info,
                                                   size_t index,
                                                   size_t max_bytes,
                                                   size_t max_messages):
    JackALSARawMidiPort(info, index, POLLIN)
{
    alsa_event = 0;
    jack_event = 0;
    receive_queue = new JackALSARawMidiReceiveQueue(rawmidi, max_bytes);
    std::auto_ptr<JackALSARawMidiReceiveQueue> receive_ptr(receive_queue);
    thread_queue = new JackMidiAsyncQueue(max_bytes, max_messages);
    std::auto_ptr<JackMidiAsyncQueue> thread_ptr(thread_queue);
    write_queue = new JackMidiBufferWriteQueue();
    std::auto_ptr<JackMidiBufferWriteQueue> write_ptr(write_queue);
    raw_queue = new JackMidiRawInputWriteQueue(thread_queue, max_bytes,
                                               max_messages);
    write_ptr.release();
    thread_ptr.release();
    receive_ptr.release();
}

JackALSARawMidiInputPort::~JackALSARawMidiInputPort()
{
    delete raw_queue;
    delete receive_queue;
    delete thread_queue;
    delete write_queue;
}

bool
JackALSARawMidiInputPort::ProcessJack(JackMidiBuffer *port_buffer,
                                      jack_nframes_t frames)
{
    write_queue->ResetMidiBuffer(port_buffer, frames);
    bool dequeued = false;
    if (! jack_event) {
        goto dequeue_event;
    }
    for (;;) {
        switch (write_queue->EnqueueEvent(jack_event, frames)) {
        case JackMidiWriteQueue::BUFFER_TOO_SMALL:
            jack_error("JackALSARawMidiInputPort::ProcessJack - The write "
                       "queue couldn't enqueue a %d-byte event.  Dropping "
                       "event.", jack_event->size);
            // Fallthrough on purpose.
        case JackMidiWriteQueue::OK:
            break;
        default:
            goto trigger_queue_event;
        }
    dequeue_event:
        jack_event = thread_queue->DequeueEvent();
        if (! jack_event) {
            break;
        }
        dequeued = true;
    }
 trigger_queue_event:
    return dequeued ? TriggerQueueEvent() : true;
}

bool
JackALSARawMidiInputPort::ProcessPollEvents(jack_nframes_t current_frame)
{
    if (GetQueuePollEvent() == -1) {
        return false;
    }
    int io_event = GetIOPollEvent();
    switch (io_event) {
    case -1:
        return false;
    case 1:
        alsa_event = receive_queue->DequeueEvent();
    }
    if (alsa_event) {
        size_t size = alsa_event->size;
        size_t space = raw_queue->GetAvailableSpace();
        bool enough_room = space >= size;
        if (enough_room) {
            assert(raw_queue->EnqueueEvent(current_frame, size,
                                           alsa_event->buffer) ==
                   JackMidiWriteQueue::OK);
            alsa_event = 0;
        } else if (space) {
            assert(raw_queue->EnqueueEvent(current_frame, space,
                                           alsa_event->buffer) ==
                   JackMidiWriteQueue::OK);
            alsa_event->buffer += space;
            alsa_event->size -= space;
        }
        SetIOEventsEnabled(enough_room);
    }
    raw_queue->Process();
    return true;
}
