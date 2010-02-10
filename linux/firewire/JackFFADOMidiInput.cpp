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

#include <cassert>

#include "JackFFADOMidiInput.h"

namespace Jack {

JackFFADOMidiInput::JackFFADOMidiInput(size_t buffer_size):
    JackPhysicalMidiInput(buffer_size)
{
    new_period = true;
}

JackFFADOMidiInput::~JackFFADOMidiInput()
{
    // Empty
}

jack_nframes_t
JackFFADOMidiInput::Receive(jack_midi_data_t *datum,
                            jack_nframes_t current_frame,
                            jack_nframes_t total_frames)
{
    assert(input_buffer);
    if (! new_period) {
        current_frame += 8;
    } else {
        new_period = false;
    }
    for (; current_frame < total_frames; current_frame += 8) {
        uint32_t data = input_buffer[current_frame];
        if (data & 0xff000000) {
            *datum = (jack_midi_data_t) (data & 0xff);
            return current_frame;
        }
    }
    new_period = true;
    return total_frames;
}

}
