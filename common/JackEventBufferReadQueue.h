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

#ifndef __JackEventBufferReadQueue__
#define __JackEventBufferReadQueue__

#include "JackEventReadQueue.h"

namespace Jack {

    /**
     * Wrapper class to present a JackEventBuffer in a read queue interface.
     */

    class SERVER_EXPORT JackEventBufferReadQueue: public JackEventReadQueue {

    private:

        JackEventBuffer *buffer;
        jack_nframes_t event_count;
        jack_nframes_t index;
        jack_nframes_t last_frame_time;
        jack_event_t event;

    public:

        JackEventBufferReadQueue();

        jack_event_t *
        DequeueEvent();

        /**
         * This method must be called each period to reset the buffer for
         * processing.
         */

        void
        ResetEventBuffer(JackEventBuffer *buffer);

        void
        ResetMidiBuffer(JackEventBuffer *buffer) {
            ResetEventBuffer(buffer);
        }

    };

}

#endif
