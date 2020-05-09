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

#ifndef __JackEventAsyncQueue__
#define __JackEventAsyncQueue__

#include "JackEventPort.h"
#include "JackEventReadQueue.h"
#include "JackEventWriteQueue.h"
#include "ringbuffer.h"

namespace Jack {

    /**
     * This is a event message queue designed to allow one thread to pass event
     * messages to another thread (though it can also be used to buffer events
     * internally).  This is especially useful if the event API you're
     * attempting to interface with doesn't provide the ability to schedule
     * events ahead of time and/or has blocking send/receive calls, as it
     * allows a separate thread to handle input/output while the JACK process
     * thread copies events from a buffer to this queue, or vice versa.
     */

    class SERVER_EXPORT JackEventAsyncQueue:
        public JackEventReadQueue, public JackEventWriteQueue {

    private:

        static const size_t INFO_SIZE =
            sizeof(jack_nframes_t) + sizeof(size_t);

        jack_ringbuffer_t *byte_ring;
        jack_event_data_t *data_buffer;
        jack_event_t dequeue_event;
        jack_ringbuffer_t *info_ring;
        size_t max_bytes;

    public:

        using JackEventWriteQueue::EnqueueEvent;

        /**
         * Creates a new asynchronous event message queue.  The queue can store
         * up to `max_messages` messages and up to `max_bytes` of
         * data before it starts rejecting messages.
         */

        JackEventAsyncQueue(size_t max_bytes=4096, size_t max_messages=1024);

        virtual
        ~JackEventAsyncQueue();

        /**
         * Dequeues and returns an event.  Returns '0' if there are none
         * available.  This method may be overridden.
         */

        virtual jack_event_t *
        DequeueEvent();

        /**
         * Enqueues the event specified by the arguments.  The return
         * value indicates whether or not the event was successfully enqueued.
         * This method may be overridden.
         */

        virtual EnqueueResult
        EnqueueEvent(jack_nframes_t time, size_t size,
                     jack_event_data_t *buffer);

        /**
         * Returns the maximum size event that can be enqueued right *now*.
         */

        size_t
        GetAvailableSpace();

    };

}

#endif
