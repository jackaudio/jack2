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

#include <memory>

#include "JackALSARawMidiOutputPort.h"

using Jack::JackALSARawMidiOutputPort;

JackALSARawMidiOutputPort::JackALSARawMidiOutputPort(snd_rawmidi_info_t *info,
                                                     size_t index,
                                                     size_t max_bytes,
                                                     size_t max_messages):
    JackALSARawMidiPort(info, index)
{
    alsa_event = 0;
    blocked = false;
    read_queue = new JackMidiBufferReadQueue();
    std::auto_ptr<JackMidiBufferReadQueue> read_ptr(read_queue);
    send_queue = new JackALSARawMidiSendQueue(rawmidi);
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

jack_midi_event_t *
JackALSARawMidiOutputPort::DequeueALSAEvent(int read_fd)
{
    jack_midi_event_t *event = thread_queue->DequeueEvent();
    if (event) {
        char c;
        ssize_t result = read(read_fd, &c, 1);
        if (! result) {
            jack_error("JackALSARawMidiOutputPort::DequeueALSAEvent - **BUG** "
                       "An event was dequeued from the thread queue, but no "
                       "byte was available for reading from the pipe file "
                       "descriptor.");
        } else if (result < 0) {
            jack_error("JackALSARawMidiOutputPort::DequeueALSAEvent - error "
                       "reading a byte from the pipe file descriptor: %s",
                       strerror(errno));
        }
    }
    return event;
}

bool
JackALSARawMidiOutputPort::ProcessALSA(int read_fd, jack_nframes_t *frame)
{
    unsigned short revents;
    if (! ProcessPollEvents(&revents)) {
        return false;
    }
    if (blocked) {
        if (! (revents & POLLOUT)) {
            *frame = 0;
            return true;
        }
        blocked = false;
    }
    if (! alsa_event) {
        alsa_event = DequeueALSAEvent(read_fd);
    }
    for (; alsa_event; alsa_event = DequeueALSAEvent(read_fd)) {
        switch (raw_queue->EnqueueEvent(alsa_event)) {
        case JackMidiWriteQueue::BUFFER_FULL:
            // Try to free up some space by processing events early.
            raw_queue->Process();
            switch (raw_queue->EnqueueEvent(alsa_event)) {
            case JackMidiWriteQueue::BUFFER_TOO_SMALL:
                jack_error("JackALSARawMidiOutputPort::ProcessALSA - **BUG** "
                           "JackMidiRawOutputWriteQueue::EnqueueEvent "
                           "returned `BUFFER_FULL`, and then returned "
                           "`BUFFER_TOO_SMALL` after a Process() call.");
                // Fallthrough on purpose
            case JackMidiWriteQueue::OK:
                continue;
            default:
                ;
            }
            goto process_events;
        case JackMidiWriteQueue::BUFFER_TOO_SMALL:
            jack_error("JackALSARawMidiOutputPort::ProcessALSA - The raw "
                       "output queue couldn't enqueue a %d-byte event.  "
                       "Dropping event.", alsa_event->size);
            // Fallthrough on purpose
        case JackMidiWriteQueue::OK:
            continue;
        default:
            ;
        }
        break;
    }
 process_events:
    *frame = raw_queue->Process();
    blocked = send_queue->IsBlocked();
    if (blocked) {

        jack_info("JackALSARawMidiOutputPort::ProcessALSA - MIDI port is "
                  "blocked");

        SetPollEventMask(POLLERR | POLLNVAL | POLLOUT);
        *frame = 0;
    } else {
        SetPollEventMask(POLLERR | POLLNVAL);
    }
    return true;
}

bool
JackALSARawMidiOutputPort::ProcessJack(JackMidiBuffer *port_buffer,
                                       jack_nframes_t frames, int write_fd)
{
    read_queue->ResetMidiBuffer(port_buffer);
    for (jack_midi_event_t *event = read_queue->DequeueEvent(); event;
         event = read_queue->DequeueEvent()) {
        if (event->size > thread_queue->GetAvailableSpace()) {
            jack_error("JackALSARawMidiOutputPort::ProcessJack - The thread "
                       "queue doesn't have enough room to enqueue a %d-byte "
                       "event.  Dropping event.", event->size);
            continue;
        }
        char c = 1;
        ssize_t result = write(write_fd, &c, 1);
        assert(result <= 1);
        if (result < 0) {
            jack_error("JackALSARawMidiOutputPort::ProcessJack - error "
                       "writing a byte to the pipe file descriptor: %s",
                       strerror(errno));
            return false;
        }
        if (! result) {
            // Recoverable.
            jack_error("JackALSARawMidiOutputPort::ProcessJack - Couldn't "
                       "write a byte to the pipe file descriptor.  Dropping "
                       "event.");
        } else {
            assert(thread_queue->EnqueueEvent(event, frames) ==
                   JackMidiWriteQueue::OK);
        }
    }
    return true;
}
