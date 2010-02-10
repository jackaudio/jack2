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

#include "JackError.h"
#include "JackFFADOMidiOutput.h"

namespace Jack {

JackFFADOMidiOutput::JackFFADOMidiOutput(size_t non_rt_buffer_size,
                                         size_t rt_buffer_size):
    JackPhysicalMidiOutput(non_rt_buffer_size, rt_buffer_size)
{
    // Empty
}

JackFFADOMidiOutput::~JackFFADOMidiOutput()
{
    // Empty
}

jack_nframes_t
JackFFADOMidiOutput::Advance(jack_nframes_t current_frame)
{
    if (current_frame % 8) {
        current_frame = (current_frame & (~ ((jack_nframes_t) 7))) + 8;
    }
    return current_frame;
}

jack_nframes_t
JackFFADOMidiOutput::Send(jack_nframes_t current_frame, jack_midi_data_t datum)
{
    assert(output_buffer);

    jack_log("JackFFADOMidiOutput::Send (%d) - Sending '%x' byte.",
             current_frame, (unsigned int) datum);

    output_buffer[current_frame] = 0x01000000 | ((uint32_t) datum);
    return current_frame + 8;
}

}
