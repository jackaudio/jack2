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

#include <cassert>

#include "JackError.h"
#include "JackPhysicalMidiOutput.h"

namespace Jack {

JackPhysicalMidiOutput::JackPhysicalMidiOutput(size_t non_rt_buffer_size,
                                               size_t rt_buffer_size)
{
    size_t datum_size = sizeof(jack_midi_data_t);
    assert(non_rt_buffer_size > 0);
    assert(rt_buffer_size > 0);
    output_ring = jack_ringbuffer_create((non_rt_buffer_size + 1) *
                                         datum_size);
    if (! output_ring) {
        throw std::bad_alloc();
    }
    rt_output_ring = jack_ringbuffer_create((rt_buffer_size + 1) *
                                            datum_size);
    if (! rt_output_ring) {
        jack_ringbuffer_free(output_ring);
        throw std::bad_alloc();
    }
    jack_ringbuffer_mlock(output_ring);
    jack_ringbuffer_mlock(rt_output_ring);
    running_status = 0;
}

JackPhysicalMidiOutput::~JackPhysicalMidiOutput()
{
    jack_ringbuffer_free(output_ring);
    jack_ringbuffer_free(rt_output_ring);
}

jack_nframes_t
JackPhysicalMidiOutput::Advance(jack_nframes_t frame)
{
    return frame;
}

inline jack_midi_data_t
JackPhysicalMidiOutput::ApplyRunningStatus(jack_midi_data_t **buffer,
                                           size_t *size)
{

    // Stolen and modified from alsa/midi_pack.h

    jack_midi_data_t status = (*buffer)[0];
    if ((status >= 0x80) && (status < 0xf0)) {
        if (status == running_status) {
            (*buffer)++;
            (*size)--;
        } else {
            running_status = status;
        }
    } else if (status < 0xf8) {
        running_status = 0;
    }
    return status;
}

void
JackPhysicalMidiOutput::HandleEventLoss(JackMidiEvent *event)
{
    jack_error("%d byte MIDI event lost", event->size);
}

void
JackPhysicalMidiOutput::Process(jack_nframes_t frames)
{
    assert(port_buffer);
    jack_nframes_t current_frame = Advance(0);
    jack_nframes_t current_midi_event = 0;
    jack_midi_data_t datum;
    size_t datum_size = sizeof(jack_midi_data_t);
    JackMidiEvent *midi_event;
    jack_midi_data_t *midi_event_buffer;
    size_t midi_event_size;
    jack_nframes_t midi_events = port_buffer->event_count;

    // First, send any realtime MIDI data that's left from last cycle.

    if ((current_frame < frames) &&
        jack_ringbuffer_read_space(rt_output_ring)) {

        jack_log("JackPhysicalMidiOutput::Process (%d) - Sending buffered "
                 "realtime data from last period.", current_frame);

        current_frame = SendBufferedData(rt_output_ring, current_frame,
                                         frames);

        jack_log("JackPhysicalMidiOutput::Process (%d) - Sent", current_frame);

    }

    // Iterate through the events in this cycle.

    for (; (current_midi_event < midi_events) && (current_frame < frames);
         current_midi_event++) {

        // Once we're inside this loop, we know that the realtime buffer
        // is empty.  As long as we don't find a realtime message, we can
        // concentrate on sending non-realtime data.

        midi_event = &(port_buffer->events[current_midi_event]);
        jack_nframes_t midi_event_time = midi_event->time;
        midi_event_buffer = midi_event->GetData(port_buffer);
        midi_event_size = midi_event->size;
        datum = ApplyRunningStatus(&midi_event_buffer, &midi_event_size);
        if (current_frame < midi_event_time) {

            // We have time before this event is scheduled to be sent.
            // Send data in the non-realtime buffer.

            if (jack_ringbuffer_read_space(output_ring)) {

                jack_log("JackPhysicalMidiOutput::Process (%d) - Sending "
                         "buffered non-realtime data from last period.",
                         current_frame);

                current_frame = SendBufferedData(output_ring, current_frame,
                                                 midi_event_time);

                jack_log("JackPhysicalMidiOutput::Process (%d) - Sent",
                         current_frame);

            }
            if (current_frame < midi_event_time) {

                // We _still_ have time before this event is scheduled to
                // be sent.  Let's send as much of this event as we can
                // (save for one byte, which will need to be sent at or
                // after its scheduled time).  First though, we need to
                // make sure that we can buffer this data if we need to.
                // Otherwise, we might start sending a message that we
                // can't finish.

                if (midi_event_size > 1) {
                    if (jack_ringbuffer_write_space(output_ring) <
                        ((midi_event_size - 1) * datum_size)) {
                        HandleEventLoss(midi_event);
                        continue;
                    }

                    // Send as much of the event as possible (save for one
                    // byte).

                    do {

                        jack_log("JackPhysicalMidiOutput::Process (%d) - "
                                 "Sending unbuffered event byte early.",
                                 current_frame);

                        current_frame = Send(current_frame,
                                             *midi_event_buffer);

                        jack_log("JackPhysicalMidiOutput::Process (%d) - "
                                 "Sent.", current_frame);

                        midi_event_buffer++;
                        midi_event_size--;
                        if (current_frame >= midi_event_time) {

                            // The event we're processing must be a
                            // non-realtime event.  It has more than one
                            // byte.

                            goto buffer_non_realtime_data;
                        }
                    } while (midi_event_size > 1);
                }

                jack_log("JackPhysicalMidiOutput::Process (%d) - Advancing to "
                         ">= %d", current_frame, midi_event_time);

                current_frame = Advance(midi_event_time);

                jack_log("JackPhysicalMidiOutput::Process (%d) - Advanced.",
                         current_frame);

            }
        }

        // If the event is realtime, then we'll send the event now.
        // Otherwise, we attempt to put the rest of the event bytes in the
        // non-realtime buffer.

        if (datum >= 0xf8) {

            jack_log("JackPhysicalMidiOutput::Process (%d) - Sending "
                     "unbuffered realtime event.", current_frame);

            current_frame = Send(current_frame, datum);

            jack_log("JackPhysicalMidiOutput::Process (%d) - Sent.",
                     current_frame);

        } else if (jack_ringbuffer_write_space(output_ring) >=
                   (midi_event_size * datum_size)) {
        buffer_non_realtime_data:

            jack_log("JackPhysicalMidiOutput::Process (%d) - Buffering %d "
                     "byte(s) of non-realtime data.", current_frame,
                     midi_event_size);

            jack_ringbuffer_write(output_ring,
                                  (const char *) midi_event_buffer,
                                  midi_event_size);
        } else {
            HandleEventLoss(midi_event);
        }
    }

    if (current_frame < frames) {

        // If we have time left to send data, then we know that all of the
        // data in the realtime buffer has been sent, and that all of the
        // non-realtime messages have either been sent, or buffered.  We
        // use whatever time is left to send data in the non-realtime
        // buffer.

        if (jack_ringbuffer_read_space(output_ring)) {

            jack_log("JackPhysicalMidiOutput::Process (%d) - All events "
                     "processed.  Sending buffered non-realtime data.",
                     current_frame);

            current_frame = SendBufferedData(output_ring, current_frame,
                                             frames);

            jack_log("JackPhysicalMidiOutput::Process (%d) - Sent.",
                     current_frame);

        }
    } else {

        // Since we have no time left, we need to put all remaining midi
        // events in their appropriate buffers, and send them next period.

        for (; current_midi_event < midi_events; current_midi_event++) {
            midi_event = &(port_buffer->events[current_midi_event]);
            midi_event_buffer = midi_event->GetData(port_buffer);
            midi_event_size = midi_event->size;
            datum = ApplyRunningStatus(&midi_event_buffer, &midi_event_size);
            if (datum >= 0xf8) {

                // Realtime.

                if (jack_ringbuffer_write_space(rt_output_ring) >=
                    datum_size) {

                    jack_log("JackPhysicalMidiOutput::Process - Buffering "
                             "realtime event for next period.");

                    jack_ringbuffer_write(rt_output_ring,
                                          (const char *) &datum, datum_size);
                    continue;
                }
            } else {

                // Non-realtime.

                if (jack_ringbuffer_write_space(output_ring) >=
                    (midi_event_size * datum_size)) {

                    jack_log("JackPhysicalMidiOutput::Process - Buffering "
                             "non-realtime event for next period.");

                    jack_ringbuffer_write(output_ring,
                                          (const char *) midi_event_buffer,
                                          midi_event_size * datum_size);
                    continue;
                }
            }
            HandleEventLoss(midi_event);
        }
    }
}

jack_nframes_t
JackPhysicalMidiOutput::SendBufferedData(jack_ringbuffer_t *buffer,
                                         jack_nframes_t current_frame,
                                         jack_nframes_t boundary)
{
    assert(buffer);
    assert(current_frame < boundary);
    size_t datum_size = sizeof(jack_midi_data_t);
    size_t data_length = jack_ringbuffer_read_space(buffer) / datum_size;
    for (size_t i = 0; i < data_length; i++) {
        jack_midi_data_t datum;
        jack_ringbuffer_read(buffer, (char *) &datum, datum_size);
        current_frame = Send(current_frame, datum);
        if (current_frame >= boundary) {
            break;
        }
    }
    return current_frame;
}

}
