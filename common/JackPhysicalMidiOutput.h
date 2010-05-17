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

#ifndef __JackPhysicalMidiOutput__
#define __JackPhysicalMidiOutput__

#include "JackMidiPort.h"
#include "ringbuffer.h"

namespace Jack {

    class JackPhysicalMidiOutput {

    private:

        jack_midi_data_t
        ApplyRunningStatus(jack_midi_data_t **, size_t *);

        jack_ringbuffer_t *output_ring;
        JackMidiBuffer *port_buffer;
        jack_ringbuffer_t *rt_output_ring;
        jack_midi_data_t running_status;

    protected:

        /**
         * Override to specify the next frame at which a midi byte can be sent.
         * The returned frame must be greater than or equal to the frame
         * argument.  The default returns the frame passed to it.
         */

        virtual jack_nframes_t
        Advance(jack_nframes_t);

        /**
         * Override to customize how to react when a MIDI event can't be
         * buffered and can't be sent immediately.  The default calls
         * 'jack_error' and specifies the number of bytes lost.
         */

        virtual void
        HandleEventLoss(JackMidiEvent *);

        /**
         * This method *must* be overridden to specify what happens when a MIDI
         * byte is sent at the specfied frame.  The frame argument specifies
         * the frame at which the MIDI byte should be sent, and the second
         * argument specifies the byte itself. The return value is the next
         * frame at which a MIDI byte can be sent, and must be greater than or
         * equal to the frame argument.
         */

        virtual jack_nframes_t
        Send(jack_nframes_t, jack_midi_data_t) = 0;

        /**
         * Override to optimize behavior when sending MIDI data that's in the
         * ringbuffer.  The first frame argument is the current frame, and the
         * second frame argument is the boundary frame.  The function returns
         * the next frame at which MIDI data can be sent, regardless of whether
         * or not the boundary is reached.  The default implementation calls
         * 'Send' with each byte in the ringbuffer until either the ringbuffer
         * is empty, or a frame beyond the boundary frame is returned by
         * 'Send'.
         */

        virtual jack_nframes_t
        SendBufferedData(jack_ringbuffer_t *, jack_nframes_t, jack_nframes_t);

    public:

        /**
         * The non-realtime buffer size and the realtime buffer size are both
         * optional arguments.
         */

        JackPhysicalMidiOutput(size_t non_rt_buffer_size=1024,
                               size_t rt_buffer_size=64);
        virtual ~JackPhysicalMidiOutput();

        /**
         * Called to process MIDI data during a period.
         */

        void
        Process(jack_nframes_t);

        /**
         * Set the MIDI buffer that will contain the outgoing MIDI messages.
         */

        inline void
        SetPortBuffer(JackMidiBuffer *port_buffer)
        {
            this->port_buffer = port_buffer;
        }

    };

}

#endif
