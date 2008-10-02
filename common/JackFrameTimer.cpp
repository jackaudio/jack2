/*
Copyright (C) 2001 Paul Davis
Copyright (C) 2004-2008 Grame

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

#include "JackFrameTimer.h"
#include "JackError.h"
#include <math.h>
#include <stdio.h>

namespace Jack
{

#if defined(WIN32) && !defined(__MINGW32__)
/* missing on Windows : see http://bugs.mysql.com/bug.php?id=15936 */
inline double rint(double nr)
{
    double f = floor(nr);
    double c = ceil(nr);
    return (((c -nr) >= (nr - f)) ? f : c);
}
#endif

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

jack_nframes_t JackTimer::Time2Frames(jack_time_t time, jack_nframes_t buffer_size)
{
    if (fInitialized) {
        return fFrames + (long)rint(((double) ((long long)(time - fCurrentWakeup)) / ((long long)(fNextWakeUp - fCurrentWakeup))) * buffer_size);
    } else {
        return 0;
    }
}

jack_time_t JackTimer::Frames2Time(jack_nframes_t frames, jack_nframes_t buffer_size)
{
    if (fInitialized) {
        return fCurrentWakeup + (long)rint(((double) ((long long)(frames - fFrames)) * ((long long)(fNextWakeUp - fCurrentWakeup))) / buffer_size);
    } else {
        return 0;
    }
}

jack_nframes_t JackTimer::FramesSinceCycleStart(jack_time_t cur_time, jack_nframes_t frames_rate)
{
    return (jack_nframes_t) floor((((float)frames_rate) / 1000000.0f) * (cur_time - fCurrentCallback));
}

void JackFrameTimer::InitFrameTime()
{
    fFirstWakeUp = true;
}

void JackFrameTimer::IncFrameTime(jack_nframes_t buffer_size, jack_time_t callback_usecs, jack_time_t period_usecs)
{
    if (fFirstWakeUp) {
        InitFrameTimeAux(callback_usecs, period_usecs);
        fFirstWakeUp = false;
    } else {
        IncFrameTimeAux(buffer_size, callback_usecs, period_usecs);
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

void JackFrameTimer::IncFrameTimeAux(jack_nframes_t buffer_size, jack_time_t callback_usecs, jack_time_t period_usecs)
{
    JackTimer* timer = WriteNextStateStart();
    float delta = (int64_t)callback_usecs - (int64_t)timer->fNextWakeUp;
    timer->fCurrentWakeup = timer->fNextWakeUp;
    timer->fCurrentCallback = callback_usecs;
    timer->fFrames += buffer_size;
    timer->fSecondOrderIntegrator += 0.5f * timer->fFilterCoefficient * delta;
    timer->fNextWakeUp = timer->fCurrentWakeup + period_usecs + (int64_t) floorf((timer->fFilterCoefficient * (delta + timer->fSecondOrderIntegrator)));
    timer->fInitialized = true;
    WriteNextStateStop();
    TrySwitchState(); // always succeed since there is only one writer
}

} // end of namespace

