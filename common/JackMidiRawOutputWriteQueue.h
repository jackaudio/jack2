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

#ifndef __JackMidiRawOutputWriteQueue__
#define __JackMidiRawOutputWriteQueue__

#include "JackMidiAsyncQueue.h"
#include "JackMidiSendQueue.h"

namespace Jack {

    /**
     * This queue enqueues valid MIDI events and modifies them for raw output
     * to a write queue.  It has a couple of advantages over straight MIDI
     * event copying:
     *
     * -Running status: Status bytes can be omitted when the status byte of the
     * current MIDI message is the same as the status byte of the last sent
     * MIDI message.
     *
     * -Realtime messages: Realtime messages are given priority over
     * non-realtime messages.  Realtime bytes are interspersed with
     * non-realtime bytes so that realtime messages can be sent as close as
     * possible to the time they're scheduled for sending.
     *
     * Use this queue if the MIDI API you're interfacing with allows you to
     * send raw MIDI bytes.
     */

    class SERVER_EXPORT JackMidiRawOutputWriteQueue:
        public JackMidiWriteQueue {

    private:

        jack_midi_event_t *non_rt_event;
        jack_nframes_t non_rt_event_time;
        JackMidiAsyncQueue *non_rt_queue;
        jack_midi_event_t *rt_event;
        jack_nframes_t rt_event_time;
        JackMidiAsyncQueue *rt_queue;
        jack_midi_data_t running_status;
        JackMidiSendQueue *send_queue;

        void
        DequeueNonRealtimeEvent();

        void
        DequeueRealtimeEvent();

        bool
        SendByte(jack_nframes_t time, jack_midi_data_t byte);

        bool
        SendNonRTBytes(jack_nframes_t boundary_frame);

    protected:

        /**
         * Override this method to specify what happens when the write queue
         * says that a 1-byte event is too large for its buffer.  Basically,
         * this should never happen.
         */

        virtual void
        HandleWriteQueueBug(jack_nframes_t time, jack_midi_data_t byte);

    public:

        using JackMidiWriteQueue::EnqueueEvent;

        /**
         * Called to create a new raw write queue.  The `send_queue` argument
         * is the queue to write raw bytes to.  The optional `max_rt_messages`
         * argument specifies the number of messages that can be enqueued in
         * the internal realtime queue.  The optional `max_non_rt_messages`
         * argument specifies the number of messages that can be enqueued in
         * the internal non-realtime queue.  The optional `non_rt_size`
         * argument specifies the total number of MIDI bytes that can be put in
         * the non-realtime queue.
         */

        JackMidiRawOutputWriteQueue(JackMidiSendQueue *send_queue,
                                    size_t non_rt_size=4096,
                                    size_t max_non_rt_messages=1024,
                                    size_t max_rt_messages=128);

        ~JackMidiRawOutputWriteQueue();

        EnqueueResult
        EnqueueEvent(jack_nframes_t time, size_t size,
                     jack_midi_data_t *buffer);

        /**
         * The `Process()` method should be called each time the
         * `EnqueueEvent()` method returns 'OK'.  The `Process()` method will
         * return the next frame at which an event should be sent.  The return
         * value from `Process()` depends upon the result of writing bytes to
         * the write queue:
         *
         * -If the return value is '0', then all events that have been enqueued
         * in this queue have been sent successfully to the write queue.  Don't
         * call `Process()` again until another event has been enqueued.
         *
         * -If the return value is an earlier frame or the current frame, it
         * means that the write queue returned 'BUFFER_FULL', 'ERROR', or
         * 'EVENT_EARLY' when this queue attempted to send the next byte, and
         * that the byte should have already been sent, or is scheduled to be
         * sent *now*.  `Process()` should be called again when the write queue
         * can enqueue events again successfully.  How to determine when this
         * will happen is left up to the caller.
         *
         * -If the return value is in the future, then `Process()` should be
         * called again at that time, or after another event is enqueued.
         */

        jack_nframes_t
        Process(jack_nframes_t boundary_frame=0);

    };

}

#endif
