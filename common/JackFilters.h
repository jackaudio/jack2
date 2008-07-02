/*
Copyright (C) 2008 Grame

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

#ifndef __JackFilters__
#define __JackFilters__

#include "jack.h"
#include <math.h>
#include <stdio.h>

namespace Jack
{

    #define MAX_SIZE 64
    
	struct JackFilter 
    {
    
        jack_time_t fTable[MAX_SIZE];
        
        JackFilter()
        {
            for (int i = 0; i < MAX_SIZE; i++)
                fTable[i] = 0;
        }
        
        void AddValue(jack_time_t val)
        {
            memcpy(&fTable[1], &fTable[0], sizeof(jack_time_t) * (MAX_SIZE - 1));
            fTable[0] = val;
        }
        
        jack_time_t GetVal()
        {
            jack_time_t mean = 0;
            for (int i = 0; i < MAX_SIZE; i++)
                mean += fTable[i];
            return mean / MAX_SIZE;
        }
    };
    
    class JackDelayLockedLoop
    {
    
        private:
        
            jack_nframes_t fFrames;
            jack_time_t	fCurrentWakeup;
            jack_time_t	fCurrentCallback;
            jack_time_t	fNextWakeUp;
            float fSecondOrderIntegrator;
            jack_nframes_t fBufferSize;
            jack_nframes_t fSampleRate;
            jack_time_t fPeriodUsecs;
            float fFilterCoefficient;	/* set once, never altered */
        
        public:
        
            JackDelayLockedLoop(jack_nframes_t buffer_size, jack_nframes_t sample_rate)
            {
                fFrames = 0;
                fCurrentWakeup = 0;
                fCurrentCallback = 0;
                fNextWakeUp = 0;
                fFilterCoefficient = 0.01f;
                fSecondOrderIntegrator = 0.0f;
                
                fBufferSize = buffer_size;
                fSampleRate = sample_rate;
                fPeriodUsecs = jack_time_t(1000000.f / fSampleRate * fBufferSize);	// in microsec
            }
        
            void Init(jack_time_t callback_usecs)
            {
                fSecondOrderIntegrator = 0.0f;
                fCurrentCallback = callback_usecs;
                fNextWakeUp = callback_usecs + fPeriodUsecs;
            }
            
            void IncFrame(jack_time_t callback_usecs)
            {
                float delta = (int64_t)callback_usecs - (int64_t)fNextWakeUp;
                fCurrentWakeup = fNextWakeUp;
                fCurrentCallback = callback_usecs;
                fFrames += fBufferSize;
                fSecondOrderIntegrator += 0.5f * fFilterCoefficient * delta;
                fNextWakeUp = fCurrentWakeup + fPeriodUsecs + (int64_t) floorf((fFilterCoefficient * (delta + fSecondOrderIntegrator)));
            }
            
            jack_nframes_t Time2Frames(jack_time_t time)
            {
                return fFrames + (long) rint(((double) (long(time - fCurrentWakeup)) / ((jack_time_t)(fNextWakeUp - fCurrentWakeup))) * fBufferSize);
            }
            
            jack_time_t Frames2Time(jack_nframes_t frames)
            {
                return fCurrentWakeup + (long) rint(((double) (long(frames - fFrames)) * ((jack_time_t)(fNextWakeUp - fCurrentWakeup))) / fBufferSize);
            }
            
            jack_time_t CurFrame2Time()
            {
                return fCurrentWakeup;
            }
            
            jack_nframes_t CurTime2Frame()
            {
                return fFrames;
            }

    };

    inline float Range(float min, float max, float val)
    {
        return (val < min) ? min : ((val > max) ? max : val);
    }
 
}

#endif
