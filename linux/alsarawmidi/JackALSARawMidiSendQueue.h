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

#ifndef __JackALSARawMidiSendQueue__
#define __JackALSARawMidiSendQueue__

#include <alsa/asoundlib.h>

#include "JackMidiSendQueue.h"

namespace Jack {

    class JackALSARawMidiSendQueue: public JackMidiSendQueue {

    private:

        bool blocked;
        size_t bytes_available;
        size_t bytes_per_poll;
        snd_rawmidi_t *rawmidi;

    public:

        JackALSARawMidiSendQueue(snd_rawmidi_t *rawmidi,
                                 size_t bytes_per_poll=0);

        JackMidiWriteQueue::EnqueueResult
        EnqueueEvent(jack_nframes_t time, size_t size,
                     jack_midi_data_t *buffer);

        bool
        IsBlocked();

        void
        ResetPollByteCount();

    };

}

#endif
