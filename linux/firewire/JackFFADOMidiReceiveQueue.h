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

#ifndef __JackFFADOMidiReceiveQueue__
#define __JackFFADOMidiReceiveQueue__

#include "JackMidiReceiveQueue.h"

namespace Jack {

    class JackFFADOMidiReceiveQueue: public JackMidiReceiveQueue {

    private:

        jack_midi_data_t byte;
        jack_midi_event_t event;
        jack_nframes_t index;
        uint32_t *input_buffer;
        jack_nframes_t last_frame;
        jack_nframes_t length;

    public:

        JackFFADOMidiReceiveQueue();

        jack_midi_event_t *
        DequeueEvent();

        void
        ResetInputBuffer(uint32_t *input_buffer, jack_nframes_t length);

    };

}

#endif
