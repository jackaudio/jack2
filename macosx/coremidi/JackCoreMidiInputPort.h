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

#ifndef __JackCoreMidiInputPort__
#define __JackCoreMidiInputPort__

#include "JackCoreMidiPort.h"
#include "JackMidiAsyncQueue.h"
#include "JackMidiBufferWriteQueue.h"

namespace Jack {

    class JackCoreMidiInputPort: public JackCoreMidiPort {

    private:

        jack_nframes_t
        GetFramesFromTimeStamp(MIDITimeStamp timestamp);

        jack_midi_event_t *jack_event;
        jack_midi_data_t *sysex_buffer;
        size_t sysex_bytes_sent;
        JackMidiAsyncQueue *thread_queue;
        JackMidiBufferWriteQueue *write_queue;

    protected:

        void
        Initialize(const char *alias_name, const char *client_name,
                   const char *driver_name, int index,
                   MIDIEndpointRef endpoint);

    public:

        JackCoreMidiInputPort(double time_ratio, size_t max_bytes=4096,
                              size_t max_messages=1024);

        virtual
        ~JackCoreMidiInputPort();

        void
        ProcessCoreMidi(const MIDIPacketList *packet_list);

        void
        ProcessJack(JackMidiBuffer *port_buffer, jack_nframes_t frames);

        bool
        Start();

        bool
        Stop();

    };

}

#endif
