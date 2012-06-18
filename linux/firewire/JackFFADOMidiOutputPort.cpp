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

#include "JackFFADOMidiOutputPort.h"
#include "JackMidiUtil.h"
#include "JackError.h"

using Jack::JackFFADOMidiOutputPort;

JackFFADOMidiOutputPort::JackFFADOMidiOutputPort(size_t non_rt_size,
                                                 size_t max_non_rt_messages,
                                                 size_t max_rt_messages)
{
    event = 0;
    read_queue = new JackMidiBufferReadQueue();
    std::auto_ptr<JackMidiBufferReadQueue> read_queue_ptr(read_queue);
    send_queue = new JackFFADOMidiSendQueue();
    std::auto_ptr<JackFFADOMidiSendQueue> send_queue_ptr(send_queue);
    raw_queue = new JackMidiRawOutputWriteQueue(send_queue, non_rt_size,
                                                max_non_rt_messages,
                                                max_rt_messages);
    send_queue_ptr.release();
    read_queue_ptr.release();
}

JackFFADOMidiOutputPort::~JackFFADOMidiOutputPort()
{
    delete raw_queue;
    delete read_queue;
    delete send_queue;
}

void
JackFFADOMidiOutputPort::Process(JackMidiBuffer *port_buffer,
                                 uint32_t *output_buffer,
                                 jack_nframes_t frames)
{
    read_queue->ResetMidiBuffer(port_buffer);
    send_queue->ResetOutputBuffer(output_buffer, frames);
    jack_nframes_t boundary_frame = GetLastFrame() + frames;
    if (! event) {
        event = read_queue->DequeueEvent();
    }
    for (; event; event = read_queue->DequeueEvent()) {
        switch (raw_queue->EnqueueEvent(event)) {
        case JackMidiWriteQueue::BUFFER_FULL:

            // Processing events early might free up some space in the raw
            // output queue.

            raw_queue->Process(boundary_frame);
            switch (raw_queue->EnqueueEvent(event)) {
            case JackMidiWriteQueue::BUFFER_TOO_SMALL:
                // This shouldn't really happen.  It indicates a bug if it
                // does.
                jack_error("JackFFADOMidiOutputPort::Process - **BUG** "
                           "JackMidiRawOutputWriteQueue::EnqueueEvent "
                           "returned `BUFFER_FULL`, and then returned "
                           "`BUFFER_TOO_SMALL` after a `Process()` call.");
                // Fallthrough on purpose
            case JackMidiWriteQueue::OK:
                continue;
            default:
                return;
            }
        case JackMidiWriteQueue::BUFFER_TOO_SMALL:
            jack_error("JackFFADOMidiOutputPort::Process - The write queue "
                       "couldn't enqueue a %d-byte event. Dropping event.",
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
