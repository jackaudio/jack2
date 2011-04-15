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

#ifndef __JackMidiSendQueue__
#define __JackMidiSendQueue__

#include "JackMidiWriteQueue.h"

namespace Jack {

    /**
     * Implemented by MIDI output connections.
     */

    class SERVER_EXPORT JackMidiSendQueue: public JackMidiWriteQueue {

    public:

        using JackMidiWriteQueue::EnqueueEvent;

        virtual
        ~JackMidiSendQueue();

        /**
         * Returns the next frame that a MIDI message can be sent at.  The
         * default method returns the current frame.
         */

        virtual jack_nframes_t
        GetNextScheduleFrame();

    };

}

#endif
