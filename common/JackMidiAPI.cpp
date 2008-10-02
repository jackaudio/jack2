/*
Copyright (C) 2007 Dmitry Baikov
Original JACK MIDI implementation Copyright (C) 2004 Ian Esten

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

#include "JackError.h"
#include "JackMidiPort.h"
#include "JackCompilerDeps.h"
#include <errno.h>
#include <string.h>
#include "JackSystemDeps.h"

#ifdef __cplusplus
extern "C"
{
#endif

    EXPORT jack_nframes_t jack_midi_get_event_count(void* port_buffer);

    EXPORT int jack_midi_event_get(jack_midi_event_t* event,
                                   void* port_buffer, jack_nframes_t event_index);

    EXPORT void jack_midi_clear_buffer(void* port_buffer);

    EXPORT size_t jack_midi_max_event_size(void* port_buffer);

    EXPORT jack_midi_data_t* jack_midi_event_reserve(void* port_buffer,
            jack_nframes_t time, size_t data_size);

    EXPORT int jack_midi_event_write(void* port_buffer,
                                     jack_nframes_t time, const jack_midi_data_t* data, size_t data_size);

    EXPORT jack_nframes_t jack_midi_get_lost_event_count(void* port_buffer);

#ifdef __cplusplus
}
#endif

using namespace Jack;

EXPORT
jack_nframes_t jack_midi_get_event_count(void* port_buffer)
{
    JackMidiBuffer *buf = (JackMidiBuffer*)port_buffer;
    if (!buf || !buf->IsValid())
        return 0;
    return buf->event_count;
}

EXPORT
int jack_midi_event_get(jack_midi_event_t *event, void* port_buffer, jack_nframes_t event_index)
{
    JackMidiBuffer *buf = (JackMidiBuffer*)port_buffer;
    if (!buf || !buf->IsValid())
        return -EINVAL;
    if (event_index >= buf->event_count)
        return -ENOBUFS;
    JackMidiEvent* ev = &buf->events[event_index];
    event->time = ev->time;
    event->size = ev->size;
    event->buffer = ev->GetData(buf);
    return 0;
}

EXPORT
void jack_midi_clear_buffer(void* port_buffer)
{
    JackMidiBuffer *buf = (JackMidiBuffer*)port_buffer;
    if (buf && buf->IsValid())
        buf->Reset(buf->nframes);
}

EXPORT
size_t jack_midi_max_event_size(void* port_buffer)
{
    JackMidiBuffer *buf = (JackMidiBuffer*)port_buffer;
    if (buf && buf->IsValid())
        return buf->MaxEventSize();
    return 0;
}

EXPORT
jack_midi_data_t* jack_midi_event_reserve(void* port_buffer, jack_nframes_t time, size_t data_size)
{
    JackMidiBuffer *buf = (JackMidiBuffer*)port_buffer;
    if (!buf && !buf->IsValid())
        return 0;
    if (time >= buf->nframes || (buf->event_count && buf->events[buf->event_count - 1].time > time))
        return 0;
    return buf->ReserveEvent(time, data_size);
}

EXPORT
int jack_midi_event_write(void* port_buffer,
                          jack_nframes_t time, const jack_midi_data_t* data, size_t data_size)
{
    JackMidiBuffer *buf = (JackMidiBuffer*)port_buffer;
    if (!buf && !buf->IsValid())
        return -EINVAL;
    if (time >= buf->nframes || (buf->event_count && buf->events[buf->event_count - 1].time > time))
        return -EINVAL;
    jack_midi_data_t* dest = buf->ReserveEvent(time, data_size);
    if (!dest)
        return -ENOBUFS;
    memcpy(dest, data, data_size);
    return 0;
}

EXPORT
jack_nframes_t jack_midi_get_lost_event_count(void* port_buffer)
{
    JackMidiBuffer *buf = (JackMidiBuffer*)port_buffer;
    if (buf && buf->IsValid())
        return buf->lost_events;
    return 0;
}
