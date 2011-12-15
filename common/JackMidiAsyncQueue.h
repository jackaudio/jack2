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

#ifndef __JackMidiAsyncQueue__
#define __JackMidiAsyncQueue__

#include "JackMidiPort.h"
#include "JackMidiReadQueue.h"
#include "JackMidiWriteQueue.h"
#include "ringbuffer.h"

namespace Jack {

    /**
     * This is a MIDI message queue designed to allow one thread to pass MIDI
     * messages to another thread (though it can also be used to buffer events
     * internally).  This is especially useful if the MIDI API you're
     * attempting to interface with doesn't provide the ability to schedule
     * MIDI events ahead of time and/or has blocking send/receive calls, as it
     * allows a separate thread to handle input/output while the JACK process
     * thread copies events from a MIDI buffer to this queue, or vice versa.
     */

    class SERVER_EXPORT JackMidiAsyncQueue:
        public JackMidiReadQueue, public JackMidiWriteQueue {

    private:

        static const size_t INFO_SIZE =
            sizeof(jack_nframes_t) + sizeof(size_t);

        jack_ringbuffer_t *byte_ring;
        jack_midi_data_t *data_buffer;
        jack_midi_event_t dequeue_event;
        jack_ringbuffer_t *info_ring;
        size_t max_bytes;

    public:

        using JackMidiWriteQueue::EnqueueEvent;

        /**
         * Creates a new asynchronous MIDI message queue.  The queue can store
         * up to `max_messages` MIDI messages and up to `max_bytes` of MIDI
         * data before it starts rejecting messages.
         */

        JackMidiAsyncQueue(size_t max_bytes=4096, size_t max_messages=1024);

        virtual
        ~JackMidiAsyncQueue();

        /**
         * Dequeues and returns a MIDI event.  Returns '0' if there are no MIDI
         * events available.  This method may be overridden.
         */

        virtual jack_midi_event_t *
        DequeueEvent();

        /**
         * Enqueues the MIDI event specified by the arguments.  The return
         * value indiciates whether or not the event was successfully enqueued.
         * This method may be overridden.
         */

        virtual EnqueueResult
        EnqueueEvent(jack_nframes_t time, size_t size,
                     jack_midi_data_t *buffer);

        /**
         * Returns the maximum size event that can be enqueued right *now*.
         */

        size_t
        GetAvailableSpace();

    };

}

#endif
