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

#ifndef __JackWinMMEInputPort__
#define __JackWinMMEInputPort__

#include <mmsystem.h>

#include "JackMidiAsyncQueue.h"
#include "JackMidiBufferWriteQueue.h"

namespace Jack {

    class JackWinMMEInputPort {

    private:

        static void CALLBACK
        HandleMidiInputEvent(HMIDIIN handle, UINT message, DWORD port,
                             DWORD param1, DWORD param2);

        void
        EnqueueMessage(jack_nframes_t time, size_t length,
                       jack_midi_data_t *data);

        void
        GetErrorString(MMRESULT error, LPTSTR text);

        void
        ProcessWinMME(UINT message, DWORD param1, DWORD param2);

        void
        WriteError(const char *jack_func, const char *mm_func,
                   MMRESULT result);

        char alias[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
        char name[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];

        HMIDIIN handle;
        jack_midi_event_t *jack_event;
        jack_midi_data_t *sysex_buffer;
        MIDIHDR sysex_header;
        JackMidiAsyncQueue *thread_queue;
        JackMidiBufferWriteQueue *write_queue;

        bool started;

    public:

        JackWinMMEInputPort(const char *alias_name, const char *client_name,
                            const char *driver_name, UINT index,
                            size_t max_bytes=4096, size_t max_messages=1024);

        ~JackWinMMEInputPort();

        const char *
        GetAlias();

        const char *
        GetName();

        void
        ProcessJack(JackMidiBuffer *port_buffer, jack_nframes_t frames);

        bool
        Start();

        bool
        Stop();

    };

}

#endif
