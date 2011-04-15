/*
Copyright (C) 2010 Devin Anderson

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

#ifndef __JackMidiUtil__
#define __JackMidiUtil__

#include "JackMidiPort.h"

namespace Jack {

    /**
     * Use this function to optimize MIDI output by omitting unnecessary status
     * bytes.  This can't be used with all MIDI APIs, so before using this
     * function, make sure that your MIDI API doesn't require complete MIDI
     * messages to be sent.
     *
     * To start using this function, call this method with pointers to the
     * `size` and `buffer` arguments of the MIDI message you want to send, and
     * set the `running_status` argument to '0'.  For each subsequent MIDI
     * message, call this method with pointers to its `size` and `buffer`
     * arguments, and set the `running_status` argument to the return value of
     * the previous call to this function.
     *
     * Note: This function will alter the `size` and `buffer` of your MIDI
     * message for each message that can be optimized.
     */

    SERVER_EXPORT jack_midi_data_t
    ApplyRunningStatus(size_t *size, jack_midi_data_t **buffer,
                       jack_midi_data_t running_status=0);

    /**
     * A wrapper function for the above `ApplyRunningStatus` function.
     */

    SERVER_EXPORT jack_midi_data_t
    ApplyRunningStatus(jack_midi_event_t *event,
                       jack_midi_data_t running_status);

    /**
     * Gets the estimated current time in frames.  This function has the same
     * functionality as the JACK client API function `jack_frame_time`.
     */

    SERVER_EXPORT jack_nframes_t
    GetCurrentFrame();

    /**
     * Gets the estimated frame that will be occurring at the given time.  This
     * function has the same functionality as the JACK client API function
     * `jack_time_to_frames`.
     */

    SERVER_EXPORT jack_nframes_t
    GetFramesFromTime(jack_time_t time);

    /**
     * Gets the precise time at the start of the current process cycle.  This
     * function has the same functionality as the JACK client API function
     * `jack_last_frame_time`.
     */

    SERVER_EXPORT jack_nframes_t
    GetLastFrame();

    /**
     * Returns the expected message length for the status byte.  Returns 0 if
     * the status byte is a system exclusive status byte, or -1 if the status
     * byte is invalid.
     */

    SERVER_EXPORT int
    GetMessageLength(jack_midi_data_t status_byte);

    /**
     * Gets the estimated time at which the given frame will occur.  This
     * function has the same functionality as the JACK client API function
     * `jack_frames_to_time`.
     */

    SERVER_EXPORT jack_time_t
    GetTimeFromFrames(jack_nframes_t frames);

};

#endif
