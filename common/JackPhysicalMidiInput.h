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

#ifndef __JackPhysicalMidiInput__
#define __JackPhysicalMidiInput__

#include "JackMidiPort.h"
#include "ringbuffer.h"

namespace Jack {

    class JackPhysicalMidiInput {

    private:

        size_t buffered_bytes;
        size_t expected_data_bytes;
        jack_ringbuffer_t *input_ring;
        JackMidiBuffer *port_buffer;
        jack_midi_data_t status_byte;
        size_t unbuffered_bytes;

        void
        Clear();

        void
        WriteBufferedEvent(jack_nframes_t);

        void
        WriteBufferedSysexEvent(jack_nframes_t);

        void
        WriteByteEvent(jack_nframes_t, jack_midi_data_t);

    protected:

        /**
         * Override to specify how to react when 1 or more bytes of a MIDI
         * message are lost because there wasn't enough room in the input
         * buffer.  The first argument is the amount of bytes that couldn't be
         * buffered, and the second argument is the total amount of bytes in
         * the MIDI message.  The default implementation calls 'jack_error'
         * with a basic error message.
         */

        virtual void
        HandleBufferFailure(size_t, size_t);

        /**
         * Override to specify how to react when a new status byte is received
         * before all of the data bytes in a message are received.  The
         * argument is the number of bytes being discarded.  The default
         * implementation calls 'jack_error' with a basic error message.
         */

        virtual void
        HandleIncompleteMessage(size_t);

        /**
         * Override to specify how to react when an invalid status byte (0xf4,
         * 0xf5, 0xfd) is received.  The argument contains the invalid status
         * byte.  The default implementation calls 'jack_error' with a basic
         * error message.
         */

        virtual void
        HandleInvalidStatusByte(jack_midi_data_t);

        /**
         * Override to specify how to react when a sysex end byte (0xf7) is
         * received without first receiving a sysex start byte (0xf0).  The
         * argument contains the amount of bytes that will be discarded.  The
         * default implementation calls 'jack_error' with a basic error
         * message.
         */

        virtual void
        HandleUnexpectedSysexEnd(size_t);

        /**
         * Override to specify how to react when a MIDI message can not be
         * written to the port buffer.  The argument specifies the length of
         * the MIDI message.  The default implementation calls 'jack_error'
         * with a basic error message.
         */

        virtual void
        HandleWriteFailure(size_t);

        /**
         * This method *must* be overridden to handle receiving MIDI bytes.
         * The first argument is a pointer to the memory location at which the
         * MIDI byte should be stored.  The second argument is the last frame
         * at which a MIDI byte was received, except at the beginning of the
         * period when the value is 0.  The third argument is the total number
         * of frames in the period.  The return value is the frame at which the
         * MIDI byte is received at, or the value of the third argument is no
         * more MIDI bytes can be received in this period.
         */

        virtual jack_nframes_t
        Receive(jack_midi_data_t *, jack_nframes_t, jack_nframes_t) = 0;

    public:

        JackPhysicalMidiInput(size_t buffer_size=1024);
        virtual ~JackPhysicalMidiInput();

        /**
         * Called to process MIDI data during a period.
         */

        void
        Process(jack_nframes_t);

        /**
         * Set the MIDI buffer that will receive incoming messages.
         */

        inline void
        SetPortBuffer(JackMidiBuffer *port_buffer)
        {
            this->port_buffer = port_buffer;
        }

    };

}

#endif
