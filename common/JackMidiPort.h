/*
Copyright (C) 2007 Dmitry Baikov
Original JACK MIDI API implementation Copyright (C) 2004 Ian Esten

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

#ifndef __JackMidiPort__
#define __JackMidiPort__

#include "types.h"
#include "JackConstants.h"
#include "JackPlatformPlug.h"
#include <stddef.h>

/** Type for raw event data contained in @ref jack_midi_event_t. */
typedef unsigned char jack_midi_data_t;

/** A Jack MIDI event. */
struct jack_midi_event_t
{
    jack_nframes_t    time;   /**< Sample index at which event is valid */
    size_t            size;   /**< Number of bytes of data in \a buffer */
    jack_midi_data_t *buffer; /**< Raw MIDI data */
};

/** A Jack MIDI port type. */
#define JACK_DEFAULT_MIDI_TYPE "8 bit raw midi"

namespace Jack
{

struct SERVER_EXPORT JackMidiEvent
{
    // Most MIDI events are < 4 bytes in size, so we can save a lot, storing them inplace.
    enum { INLINE_SIZE_MAX = sizeof(jack_shmsize_t) };

    uint32_t time;
    jack_shmsize_t size;
    union {
        jack_shmsize_t   offset;
        jack_midi_data_t data[INLINE_SIZE_MAX];
    };

    jack_midi_data_t* GetData(void* buffer)
    {
        if (size <= INLINE_SIZE_MAX)
            return data;
        else
            return (jack_midi_data_t*)buffer + offset;
    }
};

/*
 * To store events with arbitrarily sized payload, but still have O(1) indexed access
 * we use a trick here:
 * Events are stored in an linear array from the beginning of the buffer,
 * but their data (if not inlined) is stored from the end of the same buffer.
 */

struct SERVER_EXPORT JackMidiBuffer
{
    enum { MAGIC = 0x900df00d };

    uint32_t magic;
    jack_shmsize_t buffer_size;
    jack_nframes_t nframes;
    jack_shmsize_t write_pos; //!< data write position from the end of the buffer.
    uint32_t event_count;
    uint32_t lost_events;
    uint32_t mix_index;

    JackMidiEvent events[1]; // Using 0 size does not compile with older GCC versions, so use 1 here.

    int IsValid() const
    {
        return magic == MAGIC;
    }
    void Reset(jack_nframes_t nframes);
    jack_shmsize_t MaxEventSize() const;

    // checks only size constraints.
    jack_midi_data_t* ReserveEvent(jack_nframes_t time, jack_shmsize_t size);
};

} // namespace Jack

#endif
