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

#ifndef __JackMidiReadQueue__
#define __JackMidiReadQueue__

#include "JackMidiPort.h"

namespace Jack {

    /**
     * Interface for objects that MIDI events can be read from.
     */

    class SERVER_EXPORT JackMidiReadQueue {

    public:

        virtual
        ~JackMidiReadQueue();

        /**
         * Dequeues an event from the queue.  Returns the event, or 0 if no
         * events are available for reading.
         *
         * An event dequeued from the read queue is guaranteed to be valid up
         * until another event is dequeued, at which all bets are off.  Make
         * sure that you handle each event you dequeue before dequeueing the
         * next event.
         */

        virtual jack_midi_event_t *
        DequeueEvent() = 0;

    };

}

#endif
