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

#ifndef __JackMidiRawInputWriteQueue__
#define __JackMidiRawInputWriteQueue__

#include "JackMidiAsyncQueue.h"
#include "JackMidiWriteQueue.h"

namespace Jack {

    /**
     * This queue enqueues raw, unparsed MIDI packets, and outputs complete
     * MIDI messages to a write queue.
     *
     * Use this queue if the MIDI API you're interfacing with gives you raw
     * MIDI bytes that must be parsed.
     */

    class SERVER_EXPORT JackMidiRawInputWriteQueue: public JackMidiWriteQueue {

    private:

        jack_midi_event_t event;
        jack_midi_data_t event_byte;
        bool event_pending;
        size_t expected_bytes;
        jack_midi_data_t *input_buffer;
        size_t input_buffer_size;
        jack_midi_event_t *packet;
        JackMidiAsyncQueue *packet_queue;
        jack_midi_data_t status_byte;
        size_t total_bytes;
        size_t unbuffered_bytes;
        JackMidiWriteQueue *write_queue;

        void
        Clear();

        bool
        PrepareBufferedEvent(jack_nframes_t time);

        bool
        PrepareByteEvent(jack_nframes_t time, jack_midi_data_t byte);

        void
        PrepareEvent(jack_nframes_t time, size_t size,
                     jack_midi_data_t *buffer);

        bool
        ProcessByte(jack_nframes_t time, jack_midi_data_t byte);

        void
        RecordByte(jack_midi_data_t byte);

        bool
        WriteEvent(jack_nframes_t boundary_frame);

    protected:

        /**
         * Override this method to specify what happens when there isn't enough
         * room in the ringbuffer to contain a parsed event.  The default
         * method outputs an error message.
         */

        virtual void
        HandleBufferFailure(size_t unbuffered_bytes, size_t total_bytes);

        /**
         * Override this method to specify what happens when a parsed event
         * can't be written to the write queue because the event's size exceeds
         * the total possible space in the write queue.  The default method
         * outputs an error message.
         */

        virtual void
        HandleEventLoss(jack_midi_event_t *event);

        /**
         * Override this method to specify what happens when an incomplete MIDI
         * message is parsed.  The default method outputs an error message.
         */

        virtual void
        HandleIncompleteMessage(size_t total_bytes);

        /**
         * Override this method to specify what happens when an invalid MIDI
         * status byte is parsed.  The default method outputs an error message.
         */

        virtual void
        HandleInvalidStatusByte(jack_midi_data_t byte);

        /**
         * Override this method to specify what happens when a sysex end byte
         * is parsed without first parsing a sysex begin byte.  The default
         * method outputs an error message.
         */

        virtual void
        HandleUnexpectedSysexEnd(size_t total_bytes);

    public:

        using JackMidiWriteQueue::EnqueueEvent;

        /**
         * Called to create a new raw input write queue.  The `write_queue`
         * argument is the queue to write parsed messages to.  The optional
         * `max_packets` argument specifies the number of packets that can be
         * enqueued in the internal queue.  The optional `max_packet_data`
         * argument specifies the total number of MIDI bytes that can be put in
         * the internal queue, AND the maximum size for an event that can be
         * written to the write queue.
         */

        JackMidiRawInputWriteQueue(JackMidiWriteQueue *write_queue,
                                   size_t max_packet_data=4096,
                                   size_t max_packets=1024);

        ~JackMidiRawInputWriteQueue();

        EnqueueResult
        EnqueueEvent(jack_nframes_t time, size_t size,
                     jack_midi_data_t *buffer);

        /**
         * Returns the maximum size event that can be enqueued right *now*.
         */

        size_t
        GetAvailableSpace();

        /**
         * The `Process()` method should be called each time the
         * `EnqueueEvent()` method returns `OK`.  The `Process()` method will
         * return the next frame at which an event should be sent.  The return
         * value from `Process()` depends upon the result of writing bytes to
         * the write queue:
         *
         * -If the return value is '0', then all *complete* events have been
         * sent successfully to the write queue.  Don't call `Process()` again
         * until another event has been enqueued.
         *
         * -If the return value is a non-zero value, then it specifies the
         * frame that a pending event is scheduled to sent at.  If the frame is
         * in the future, then `Process()` should be called again at that time;
         * otherwise, `Process()` should be called as soon as the write queue
         * will accept events again.
         */

        jack_nframes_t
        Process(jack_nframes_t boundary_frame=0);

    };

}

#endif
