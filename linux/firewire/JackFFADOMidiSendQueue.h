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

#ifndef __JackFFADOMidiSendQueue__
#define __JackFFADOMidiSendQueue__

#include "JackMidiSendQueue.h"

namespace Jack {

    class JackFFADOMidiSendQueue: public JackMidiSendQueue {

    private:

        jack_nframes_t index;
        jack_nframes_t last_frame;
        jack_nframes_t length;
        uint32_t *output_buffer;

    public:

        JackFFADOMidiSendQueue();

        EnqueueResult
        EnqueueEvent(jack_nframes_t time, size_t size,
                     jack_midi_data_t *buffer);

        jack_nframes_t
        GetNextScheduleFrame();

        void
        ResetOutputBuffer(uint32_t *output_buffer, jack_nframes_t length);

    };

}

#endif
