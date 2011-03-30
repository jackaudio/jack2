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

#ifndef __JackCoreMidiOutputPort__
#define __JackCoreMidiOutputPort__

#include <semaphore.h>

#include "JackCoreMidiPort.h"
#include "JackMidiAsyncQueue.h"
#include "JackMidiBufferReadQueue.h"
#include "JackThread.h"

namespace Jack {

    class JackCoreMidiOutputPort:
        public JackCoreMidiPort, public JackRunnableInterface {

    private:

        jack_midi_event_t *
        GetCoreMidiEvent(bool block);

        MIDITimeStamp
        GetTimeStampFromFrames(jack_nframes_t frames);

        static const size_t PACKET_BUFFER_SIZE = 65536;

        SInt32 advance_schedule_time;
        char packet_buffer[PACKET_BUFFER_SIZE];
        JackMidiBufferReadQueue *read_queue;
        char semaphore_name[128];
        JackThread *thread;
        JackMidiAsyncQueue *thread_queue;
        sem_t *thread_queue_semaphore;

    protected:

        virtual bool
        SendPacketList(MIDIPacketList *packet_list) = 0;

        void
        Initialize(const char *alias_name, const char *client_name,
                   const char *driver_name, int index,
                   MIDIEndpointRef endpoint, SInt32 advance_schedule_time);

    public:

        JackCoreMidiOutputPort(double time_ratio, size_t max_bytes=4096,
                               size_t max_messages=1024);

        virtual
        ~JackCoreMidiOutputPort();

        bool
        Execute();

        bool
        Init();

        void
        ProcessJack(JackMidiBuffer *port_buffer, jack_nframes_t frames);

        bool
        Start();

        bool
        Stop();

    };

}

#endif
