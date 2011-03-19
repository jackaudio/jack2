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

#ifndef __JackFFADOMidiOutputPort__
#define __JackFFADOMidiOutputPort__

#include "JackFFADOMidiSendQueue.h"
#include "JackMidiBufferReadQueue.h"
#include "JackMidiRawOutputWriteQueue.h"

namespace Jack {

    class JackFFADOMidiOutputPort {

    private:

        jack_midi_event_t *event;
        JackMidiRawOutputWriteQueue *raw_queue;
        JackMidiBufferReadQueue *read_queue;
        JackFFADOMidiSendQueue *send_queue;

    public:

        JackFFADOMidiOutputPort(size_t non_rt_size=4096,
                                size_t max_non_rt_messages=1024,
                                size_t max_rt_messages=128);
        ~JackFFADOMidiOutputPort();

        void
        Process(JackMidiBuffer *port_buffer, uint32_t *output_buffer,
                jack_nframes_t frames);

    };

}

#endif
