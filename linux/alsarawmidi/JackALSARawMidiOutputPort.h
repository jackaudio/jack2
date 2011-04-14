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

#ifndef __JackALSARawMidiOutputPort__
#define __JackALSARawMidiOutputPort__

#include "JackALSARawMidiPort.h"
#include "JackALSARawMidiSendQueue.h"
#include "JackMidiAsyncQueue.h"
#include "JackMidiBufferReadQueue.h"
#include "JackMidiRawOutputWriteQueue.h"

namespace Jack {

    class JackALSARawMidiOutputPort: public JackALSARawMidiPort {

    private:

        jack_midi_event_t *alsa_event;
        JackMidiRawOutputWriteQueue *raw_queue;
        JackMidiBufferReadQueue *read_queue;
        JackALSARawMidiSendQueue *send_queue;
        JackMidiAsyncQueue *thread_queue;

    public:

        JackALSARawMidiOutputPort(snd_rawmidi_info_t *info, size_t index,
                                  size_t max_bytes_per_poll=3,
                                  size_t max_bytes=4096,
                                  size_t max_messages=1024);

        ~JackALSARawMidiOutputPort();

        bool
        ProcessJack(JackMidiBuffer *port_buffer, jack_nframes_t frames);

        bool
        ProcessPollEvents(bool handle_output, bool timeout,
                          jack_nframes_t *frame);

    };

}

#endif
