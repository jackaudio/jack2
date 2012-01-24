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

#include <memory>

#include "JackFFADOMidiInputPort.h"
#include "JackMidiUtil.h"
#include "JackError.h"

using Jack::JackFFADOMidiInputPort;

JackFFADOMidiInputPort::JackFFADOMidiInputPort(size_t max_bytes)
{
    event = 0;
    receive_queue = new JackFFADOMidiReceiveQueue();
    std::auto_ptr<JackFFADOMidiReceiveQueue> receive_queue_ptr(receive_queue);
    write_queue = new JackMidiBufferWriteQueue();
    std::auto_ptr<JackMidiBufferWriteQueue> write_queue_ptr(write_queue);
    raw_queue = new JackMidiRawInputWriteQueue(write_queue, max_bytes,
                                               max_bytes);
    write_queue_ptr.release();
    receive_queue_ptr.release();
}

JackFFADOMidiInputPort::~JackFFADOMidiInputPort()
{
    delete raw_queue;
    delete receive_queue;
    delete write_queue;
}

void
JackFFADOMidiInputPort::Process(JackMidiBuffer *port_buffer,
                                uint32_t *input_buffer, jack_nframes_t frames)
{
    receive_queue->ResetInputBuffer(input_buffer, frames);
    write_queue->ResetMidiBuffer(port_buffer, frames);
    jack_nframes_t boundary_frame = GetLastFrame() + frames;
    if (! event) {
        event = receive_queue->DequeueEvent();
    }
    for (; event; event = receive_queue->DequeueEvent()) {
        switch (raw_queue->EnqueueEvent(event)) {
        case JackMidiWriteQueue::BUFFER_FULL:

            // Processing events early might free up some space in the raw
            // input queue.

            raw_queue->Process(boundary_frame);
            switch (raw_queue->EnqueueEvent(event)) {
            case JackMidiWriteQueue::BUFFER_TOO_SMALL:
                // This shouldn't really happen.  It indicates a bug if it
                // does.
                jack_error("JackFFADOMidiInputPort::Process - **BUG** "
                           "JackMidiRawInputWriteQueue::EnqueueEvent returned "
                           "`BUFFER_FULL`, and then returned "
                           "`BUFFER_TOO_SMALL` after a `Process()` call.");
                // Fallthrough on purpose
            case JackMidiWriteQueue::OK:
                continue;
            default:
                return;
            }
        case JackMidiWriteQueue::BUFFER_TOO_SMALL:
            jack_error("JackFFADOMidiInputPort::Process - The write queue "
                       "couldn't enqueue a %d-byte event.  Dropping event.",
                       event->size);
            // Fallthrough on purpose
        case JackMidiWriteQueue::OK:
            continue;
        default:
            // This is here to stop compliers from warning us about not
            // handling enumeration values.
            ;
        }
        break;
    }
    raw_queue->Process(boundary_frame);
}
