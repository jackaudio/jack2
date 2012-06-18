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
    fPeriodUsecs = 0.0f;
    fFilterOmega = 0.0f; /* Initialised later */
}

jack_nframes_t JackTimer::Time2Frames(jack_time_t usecs, jack_nframes_t buffer_size)
{
    if (fInitialized) {
        /*
        Make sure we have signed differences. It would make a lot of sense
        to use the standard signed intNN_t types everywhere  instead of e.g.
        jack_nframes_t and jack_time_t. This would at least ensure that the
        types used below are the correct ones. There is no way to get a type
        that would be 'a signed version of jack_time_t' for example - the
        types below are inherently fragile and there is no automatic way to
        check they are the correct ones. The only way is to check manually
        against jack/types.h.  FA - 16/02/2012
        */
        int64_t du = usecs - fCurrentWakeup;
        int64_t dp = fNextWakeUp - fCurrentWakeup;
        return fFrames + (int32_t)rint((double)du / (double)dp * buffer_size);
    } else {
        return 0;
    }
}

jack_time_t JackTimer::Frames2Time(jack_nframes_t frames, jack_nframes_t buffer_size)
{
    if (fInitialized) {
        /*
        Make sure we have signed differences. It would make a lot of sense
        to use the standard signed intNN_t types everywhere  instead of e.g.
        jack_nframes_t and jack_time_t. This would at least ensure that the
        types used below are the correct ones. There is no way to get a type
        that would be 'a signed version of jack_time_t' for example - the
        types below are inherently fragile and there is no automatic way to
        check they are the correct ones. The only way is to check manually
        against jack/types.h.  FA - 16/02/2012
        */
        int32_t df = frames - fFrames;
        int64_t dp = fNextWakeUp - fCurrentWakeup;
        return fCurrentWakeup + (int64_t)rint((double) df * (double) dp / buffer_size);
    } else {
        return 0;
    }
}

int JackTimer::GetCycleTimes(jack_nframes_t* current_frames, jack_time_t* current_usecs, jack_time_t* next_usecs, float* period_usecs)
{
    if (fInitialized) {
        *current_frames  = fFrames;
        *current_usecs = fCurrentWakeup;
        *next_usecs = fNextWakeUp;
        *period_usecs = fPeriodUsecs;
        return 0;
    } else {
        return -1;
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
    }
    
    IncFrameTimeAux(buffer_size, callback_usecs, period_usecs);
}

void JackFrameTimer::ResetFrameTime(jack_time_t callback_usecs)
{
    if (!fFirstWakeUp) { // ResetFrameTime may be called by a xrun/delayed wakeup on the first cycle
        JackTimer* timer = WriteNextStateStart();
        timer->fCurrentWakeup = callback_usecs;
        timer->fCurrentCallback = callback_usecs;
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
    /* the first wakeup or post-freewheeling or post-xrun */

    /* There seems to be no significant difference between
       the two conditions OR-ed above. Incrementing the
       frame_time after an xrun shouldn't harm, as there 
       will be a discontinuity anyway. So the two are
       combined in this version.
       FA 16/03/2012 
    */
    /* Since the DLL *will* be run, next_wakeup should be the
       current wakeup time *without* adding the period time, as
       if it were computed in the previous period.
       FA 16/03/2012 
    */
    /* Added initialisation of timer->period_usecs, required
       due to the modified implementation of the DLL itself. 
       OTOH, this should maybe not be repeated after e.g.
       freewheeling or an xrun, as the current value would be
       more accurate than the nominal one. But it doesn't really
       harm either. Implementing this would require a new flag
       in the engine structure, to be used after freewheeling 
       or an xrun instead of first_wakeup. I don't know if this
       can be done without breaking compatibility, so I did not
       add this
       FA 13/02/2012
    */
    /* Added initialisation of timer->filter_omega. This makes 
       the DLL bandwidth independent of the actual period time.
       The bandwidth is now 1/8 Hz in all cases. The value of
       timer->filter_omega is 2 * pi * BW * Tperiod.
       FA 13/02/2012
    */
    
    JackTimer* timer = WriteNextStateStart();
    timer->fPeriodUsecs = (float)period_usecs;
    timer->fCurrentCallback = callback_usecs;
    timer->fNextWakeUp = callback_usecs;
    timer->fFilterOmega = period_usecs * 7.854e-7f;
    WriteNextStateStop();
    TrySwitchState(); // always succeed since there is only one writer
}

void JackFrameTimer::IncFrameTimeAux(jack_nframes_t buffer_size, jack_time_t callback_usecs, jack_time_t period_usecs)
{
    JackTimer* timer = WriteNextStateStart();
    
    /* Modified implementation (the actual result is the same).

    'fSecondOrderIntegrator' is renamed to 'fPeriodUsecs'
    and now represents the DLL's best estimate of the 
    period time in microseconds (before it was a scaled
    version of the difference w.r.t. the nominal value).
    This allows this value to be made available to clients
    that are interested in it (see jack_get_cycle_times).
    This change also means that 'fPeriodUsecs' must be
    initialised to the nominal period time instead of zero.
    This is done in the first cycle in jack_run_cycle().

   'fFilterCoefficient' is renamed to 'fFilterOmega'. It
    is now equal to the 'omega' value as defined in the
    'Using a DLL to filter time' paper (before it was a
    scaled version of this value). It is computed once in
    jack_run_cycle() rather than set to a fixed value. This
    makes the DLL bandwidth independent of the period time.

    FA 13/02/2012
    */
    
    float delta = (float)((int64_t)callback_usecs - (int64_t)timer->fNextWakeUp);
    delta *= timer->fFilterOmega;
    timer->fCurrentWakeup = timer->fNextWakeUp;
    timer->fCurrentCallback = callback_usecs;
    timer->fFrames += buffer_size;
    timer->fPeriodUsecs += timer->fFilterOmega * delta;	
    timer->fNextWakeUp += (int64_t)floorf(timer->fPeriodUsecs + 1.41f * delta + 0.5f);
    timer->fInitialized = true;
    
    WriteNextStateStop();
    TrySwitchState(); // always succeed since there is only one writer
}

} // end of namespace

