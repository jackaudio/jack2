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

#ifndef __JackMidiWriteQueue__
#define __JackMidiWriteQueue__

#include "JackMidiPort.h"

namespace Jack {

    /**
     * Interface for classes that act as write queues for MIDI messages.  Write
     * queues are used by processors to transfer data to the next processor.
     */

    class SERVER_EXPORT JackMidiWriteQueue {

    public:

        enum EnqueueResult {
            BUFFER_FULL,
            BUFFER_TOO_SMALL,
            EVENT_EARLY,
            EN_ERROR,
            OK
        };

        virtual ~JackMidiWriteQueue();

        /**
         * Enqueues a data packet in the write queue of `size` bytes contained
         * in `buffer` that will be sent the absolute time specified by `time`.
         * This method should not block unless 1.) this write queue represents
         * the actual outbound MIDI connection, 2.) the MIDI event is being
         * sent *now*, meaning that `time` is less than or equal to *now*, and
         * 3.) the method is *not* being called in the process thread.  The
         * method should return `OK` if the event was enqueued, `BUFFER_FULL`
         * if the write queue isn't able to accept the event right now,
         * `BUFFER_TOO_SMALL` if this write queue will never be able to accept
         * the event because the event is too large, `EVENT_EARLY` if this
         * queue cannot schedule events ahead of time, and `EN_ERROR` if an error
         * occurs that cannot be specified by another return code.
         */

        virtual EnqueueResult
        EnqueueEvent(jack_nframes_t time, size_t size,
                     jack_midi_data_t *buffer) = 0;

        /**
         * A wrapper method for the `EnqueueEvent` method above.  The optional
         * 'frame_offset' argument is an amount of frames to add to the event's
         * time.
         */

        inline EnqueueResult
        EnqueueEvent(jack_midi_event_t *event, jack_nframes_t frame_offset=0)
        {
            return EnqueueEvent(event->time + frame_offset, event->size,
                                event->buffer);
        }

    };

}

#endif
