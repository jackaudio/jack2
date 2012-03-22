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

#ifndef __JackFrameTimer__
#define __JackFrameTimer__

#include "JackAtomicState.h"
#include "JackCompilerDeps.h"
#include "types.h"

namespace Jack
{

/*!
\brief A structure used for time management.
*/

PRE_PACKED_STRUCTURE
class SERVER_EXPORT JackTimer
{

    friend class JackFrameTimer;

    private:

        jack_nframes_t fFrames;
        jack_time_t	fCurrentWakeup;
        jack_time_t	fCurrentCallback;
        jack_time_t	fNextWakeUp;
        float fPeriodUsecs;
        float fFilterOmega; /* set once, never altered */
        bool fInitialized;

    public:

        JackTimer();
        ~JackTimer()
        {}

        jack_nframes_t Time2Frames(jack_time_t time, jack_nframes_t buffer_size);
        jack_time_t Frames2Time(jack_nframes_t frames, jack_nframes_t buffer_size);
        jack_nframes_t FramesSinceCycleStart(jack_time_t cur_time, jack_nframes_t frames_rate);
        int GetCycleTimes(jack_nframes_t* current_frames, jack_time_t* current_usecs, jack_time_t* next_usecs, float* period_usecs);

        jack_nframes_t CurFrame()
        {
            return fFrames;
        }

        jack_time_t CurTime()
        {
            return fCurrentWakeup;
        }

} POST_PACKED_STRUCTURE;

/*!
\brief A class using the JackAtomicState to manage jack time.
*/

PRE_PACKED_STRUCTURE
class SERVER_EXPORT JackFrameTimer : public JackAtomicState<JackTimer>
{

    private:

        bool fFirstWakeUp;
        void IncFrameTimeAux(jack_nframes_t buffer_size, jack_time_t callback_usecs, jack_time_t period_usecs);
        void InitFrameTimeAux(jack_time_t callback_usecs, jack_time_t period_usecs);

    public:

        JackFrameTimer(): fFirstWakeUp(true)
        {}
        ~JackFrameTimer()
        {}

        void InitFrameTime();
        void ResetFrameTime(jack_time_t callback_usecs);
        void IncFrameTime(jack_nframes_t buffer_size, jack_time_t callback_usecs, jack_time_t period_usecs);
        void ReadFrameTime(JackTimer* timer);

} POST_PACKED_STRUCTURE;


} // end of namespace

#endif
