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

#ifndef __JackEventAsyncWaitQueue__
#define __JackEventAsyncWaitQueue__

#include "JackEventAsyncQueue.h"

namespace Jack {

    /**
     * This is an asynchronous wait queue that allows a thread to wait for a
     * message, either indefinitely or for a specified time.  This is one
     * example of a way that the `JackEventAsyncQueue` class can be extended so
     * that process threads can interact with non-process threads to send
     * events.
     *
     * XXX: As of right now, this code hasn't been tested.  Also, note the
     * warning in the JackEventAsyncWaitQueue.cpp about semaphore wait
     * resolution.
     */

    class SERVER_EXPORT JackEventAsyncWaitQueue: public JackEventAsyncQueue {

    private:

        JackSynchro semaphore;

    public:

        using JackEventAsyncQueue::EnqueueEvent;

        /**
         * Creates a new asynchronous wait message queue.  The queue can
         * store up to `max_messages` messages and up to `max_bytes` of
         * messagea data before it starts rejecting messages.
         */

        JackEventAsyncWaitQueue(size_t max_bytes=4096,
                               size_t max_messages=1024);

        ~JackEventAsyncWaitQueue();

        /**
         * Dequeues and returns a MIDI event.  Returns '0' if there are no MIDI
         * events available right now.
         */

        jack_event_t *
        DequeueEvent();

        /**
         * Waits a specified time for an event to be available, or
         * indefinitely if the time is negative.  Returns the event, or
         * '0' if time runs out and no event is available.
         */

        jack_event_t *
        DequeueEvent(long usecs);

        /**
         * Waits until the specified frame for an event to be available.
         * Returns the event, or '0' if time runs out and no event is
         * available.
         */

        jack_event_t *
        DequeueEvent(jack_nframes_t frame);

        /**
         * Enqueues the event specified by the arguments.  The return
         * value indicates whether or not the event was successfully enqueued.
         */

        EnqueueResult
        EnqueueEvent(jack_nframes_t time, size_t size,
                     jack_event_data_t *buffer);

    };

}

#endif
