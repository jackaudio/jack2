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

#ifndef __JackALSARawMidiInputPort__
#define __JackALSARawMidiInputPort__

#include "JackALSARawMidiPort.h"
#include "JackALSARawMidiReceiveQueue.h"
#include "JackMidiAsyncQueue.h"
#include "JackMidiBufferWriteQueue.h"
#include "JackMidiRawInputWriteQueue.h"

namespace Jack {

    class JackALSARawMidiInputPort: public JackALSARawMidiPort {

    private:

        jack_midi_event_t *alsa_event;
        jack_midi_event_t *jack_event;
        JackMidiRawInputWriteQueue *raw_queue;
        JackALSARawMidiReceiveQueue *receive_queue;
        JackMidiAsyncQueue *thread_queue;
        JackMidiBufferWriteQueue *write_queue;

    public:

        JackALSARawMidiInputPort(snd_rawmidi_info_t *info, size_t index,
                                 size_t max_bytes=4096,
                                 size_t max_messages=1024);

        ~JackALSARawMidiInputPort();

        bool
        ProcessJack(JackMidiBuffer *port_buffer, jack_nframes_t frames);

        bool
        ProcessPollEvents(jack_nframes_t current_frame);

    };

}

#endif
