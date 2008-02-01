/*
Copyright (C) 2001 Paul Davis 
Copyright (C) 2004-2008 Grame

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

#include "JackFrameTimer.h"
#include "JackError.h"
#include <math.h>

namespace Jack
{

JackTimer::JackTimer()
{
    fInitialized = false;
    fFrames = 0;
    fCurrentWakeup = 0;
    fCurrentCallback = 0;
    fNextWakeUp = 0;
    fFilterCoefficient = 0.01f;
    fSecondOrderIntegrator = 0.0f;
}

void JackFrameTimer::InitFrameTime()
{
    fFirstWakeUp = true;
}

void JackFrameTimer::IncFrameTime(jack_nframes_t nframes, jack_time_t callback_usecs, jack_time_t period_usecs)
{
    if (fFirstWakeUp) {
        InitFrameTimeAux(callback_usecs, period_usecs);
        fFirstWakeUp = false;
    } else {
        IncFrameTimeAux(nframes, callback_usecs, period_usecs);
    }
}

void JackFrameTimer::ResetFrameTime(jack_nframes_t frames_rate, jack_time_t callback_usecs, jack_time_t period_usecs)
{
    if (!fFirstWakeUp) { // ResetFrameTime may be called by a xrun/delayed wakeup on the first cycle
        JackTimer* timer = WriteNextStateStart();
        jack_nframes_t period_size_guess = (jack_nframes_t)(frames_rate * ((timer->fNextWakeUp - timer->fCurrentWakeup) / 1000000.0));
        timer->fFrames += ((callback_usecs - timer->fNextWakeUp) / period_size_guess) * period_size_guess;
        timer->fCurrentWakeup = callback_usecs;
        timer->fCurrentCallback = callback_usecs;
        timer->fNextWakeUp = callback_usecs + period_usecs;
        WriteNextStateStop();
        TrySwitchState(); // always succeed since there is only one writer
    }
}

/*
	Use the state returned by ReadCurrentState and check that the state was not changed during the read operation.
	The operation is lock-free since there is no intermediate state in the write operation that could cause the
	read to loop forever.
*/
void JackFrameTimer::ReadFrameTime(JackTimer* timer)
{
    UInt16 next_index = GetCurrentIndex();
    UInt16 cur_index;
    do {
        cur_index = next_index;
        memcpy(timer, ReadCurrentState(), sizeof(JackTimer));
        next_index = GetCurrentIndex();
    } while (cur_index != next_index); // Until a coherent state has been read
}

// Internal

void JackFrameTimer::InitFrameTimeAux(jack_time_t callback_usecs, jack_time_t period_usecs)
{
    JackTimer* timer = WriteNextStateStart();
    timer->fSecondOrderIntegrator = 0.0f;
    timer->fCurrentCallback = callback_usecs;
    timer->fNextWakeUp = callback_usecs + period_usecs;
    WriteNextStateStop();
    TrySwitchState(); // always succeed since there is only one writer
}

void JackFrameTimer::IncFrameTimeAux(jack_nframes_t nframes, jack_time_t callback_usecs, jack_time_t period_usecs)
{
    JackTimer* timer = WriteNextStateStart();
    float delta = (int64_t)callback_usecs - (int64_t)timer->fNextWakeUp;
    timer->fCurrentWakeup = timer->fNextWakeUp;
    timer->fCurrentCallback = callback_usecs;
    timer->fFrames += nframes;
    timer->fSecondOrderIntegrator += 0.5f * timer->fFilterCoefficient * delta;
    timer->fNextWakeUp = timer->fCurrentWakeup + period_usecs + (int64_t) floorf((timer->fFilterCoefficient * (delta + timer->fSecondOrderIntegrator)));
    timer->fInitialized = true;
    WriteNextStateStop();
    TrySwitchState(); // always succeed since there is only one writer
}

} // end of namespace

