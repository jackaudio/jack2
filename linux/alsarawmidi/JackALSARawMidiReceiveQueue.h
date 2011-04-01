/*
Copyright (C) 2011 Devin Anderson

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef __JackALSARawMidiReceiveQueue__
#define __JackALSARawMidiReceiveQueue__

#include <alsa/asoundlib.h>

#include "JackMidiReceiveQueue.h"

namespace Jack {

    class JackALSARawMidiReceiveQueue: public JackMidiReceiveQueue {

    private:

        jack_midi_data_t *buffer;
        size_t buffer_size;
        jack_midi_event_t event;
        snd_rawmidi_t *rawmidi;

    public:

        JackALSARawMidiReceiveQueue(snd_rawmidi_t *rawmidi,
                                    size_t buffer_size=4096);
        ~JackALSARawMidiReceiveQueue();

        jack_midi_event_t *
        DequeueEvent();

    };

}

#endif
