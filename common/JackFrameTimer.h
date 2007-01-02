/*
Copyright (C) 2001 Paul Davis 
Copyright (C) 2004-2006 Grame

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

#ifndef __JackFrameTimer__
#define __JackFrameTimer__

#include "JackAtomicState.h"
#include "types.h"

namespace Jack
{

/*!
\brief A structure used for time management.
*/

struct JackTimer
{
    jack_nframes_t fFrames;
    jack_time_t	fCurrentWakeup;
    jack_time_t	fCurrentCallback;
    jack_time_t	fNextWakeUp;
    float fSecondOrderIntegrator;
    bool fInitialized;

    /* not accessed by clients */

    float fFilterCoefficient;	/* set once, never altered */

    JackTimer();
    ~JackTimer()
    {}
	
};

/*!
\brief A class using the JackAtomicState to manage jack time. 
*/

class JackFrameTimer : public JackAtomicState<JackTimer>
{
    private:

        bool fFirstWakeUp;
        void IncFrameTimeAux(jack_nframes_t nframes, jack_time_t callback_usecs, jack_time_t period_usecs);
		void InitFrameTimeAux(jack_time_t callback_usecs, jack_time_t period_usecs);

    public:

        JackFrameTimer(): fFirstWakeUp(true)
        {}
        ~JackFrameTimer()
        {}

        void InitFrameTime();
        void ResetFrameTime(jack_nframes_t frames_rate, jack_time_t callback_usecs, jack_time_t period_usecs);
        void IncFrameTime(jack_nframes_t nframes, jack_time_t callback_usecs, jack_time_t period_usecs);
        void ReadFrameTime(JackTimer* timer);
};


} // end of namespace

#endif
