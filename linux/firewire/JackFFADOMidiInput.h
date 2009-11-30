/*
Copyright (C) 2009 Devin Anderson

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

#ifndef __JackFFADOMidiInput__
#define __JackFFADOMidiInput__

#include "JackPhysicalMidiInput.h"

namespace Jack {

    class JackFFADOMidiInput: public JackPhysicalMidiInput {

    private:

        uint32_t *input_buffer;
        bool new_period;

    protected:

        jack_nframes_t
        Receive(jack_midi_data_t *, jack_nframes_t, jack_nframes_t);

    public:

        JackFFADOMidiInput(size_t buffer_size=1024);
        ~JackFFADOMidiInput();

        inline void
        SetInputBuffer(uint32_t *input_buffer)
        {
            this->input_buffer = input_buffer;
        }

    };

}

#endif
