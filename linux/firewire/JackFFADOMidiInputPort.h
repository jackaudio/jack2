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

#ifndef __JackFFADOMidiInputPort__
#define __JackFFADOMidiInputPort__

#include "JackFFADOMidiReceiveQueue.h"
#include "JackMidiBufferWriteQueue.h"
#include "JackMidiRawInputWriteQueue.h"

namespace Jack {

    class JackFFADOMidiInputPort {

    private:

        jack_midi_event_t *event;
        JackMidiRawInputWriteQueue *raw_queue;
        JackFFADOMidiReceiveQueue *receive_queue;
        JackMidiBufferWriteQueue *write_queue;

    public:

        JackFFADOMidiInputPort(size_t max_bytes=4096);
        ~JackFFADOMidiInputPort();

        void
        Process(JackMidiBuffer *port_buffer, uint32_t *input_buffer,
                jack_nframes_t frames);

    };

}

#endif
