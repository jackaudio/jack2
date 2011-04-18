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

#include "JackEngineControl.h"
#include "JackFrameTimer.h"
#include "JackGlobals.h"
#include "JackMidiUtil.h"
#include "JackTime.h"

jack_midi_data_t
Jack::ApplyRunningStatus(size_t *size, jack_midi_data_t **buffer,
                         jack_midi_data_t running_status)
{

    // Stolen and modified from alsa/midi_pack.h

    jack_midi_data_t status = **buffer;
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
    return running_status;
}

jack_midi_data_t
Jack::ApplyRunningStatus(jack_midi_event_t *event,
                         jack_midi_data_t running_status)
{
    return ApplyRunningStatus(&(event->size), &(event->buffer),
                              running_status);
}

jack_nframes_t
Jack::GetCurrentFrame()
{
    jack_time_t time = GetMicroSeconds();
    JackEngineControl *control = GetEngineControl();
    JackTimer timer;
    control->ReadFrameTime(&timer);
    return timer.Time2Frames(time, control->fBufferSize);
}

jack_nframes_t
Jack::GetFramesFromTime(jack_time_t time)
{
    JackEngineControl* control = GetEngineControl();
    JackTimer timer;
    control->ReadFrameTime(&timer);
    return timer.Time2Frames(time, control->fBufferSize);
}

jack_nframes_t
Jack::GetLastFrame()
{
    return GetEngineControl()->fFrameTimer.ReadCurrentState()->CurFrame();
}

int
Jack::GetMessageLength(jack_midi_data_t status_byte)
{
    switch (status_byte & 0xf0) {
    case 0x80:
    case 0x90:
    case 0xa0:
    case 0xb0:
    case 0xe0:
        return 3;
    case 0xc0:
    case 0xd0:
        return 2;
    case 0xf0:
        switch (status_byte) {
        case 0xf0:
            return 0;
        case 0xf1:
        case 0xf3:
            return 2;
        case 0xf2:
            return 3;
        case 0xf4:
        case 0xf5:
        case 0xf7:
        case 0xfd:
            break;
        default:
            return 1;
        }
    }
    return -1;
}

jack_time_t
Jack::GetTimeFromFrames(jack_nframes_t frames)
{
    JackEngineControl* control = GetEngineControl();
    JackTimer timer;
    control->ReadFrameTime(&timer);
    return timer.Frames2Time(frames, control->fBufferSize);
}
