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

#ifndef __JackIOAdapter__
#define __JackIOAdapter__

#include "ringbuffer.h"
#include "jack.h"
#include "JackError.h"
#include "JackResampler.h"
#include "JackFilters.h"
#include <samplerate.h>
#include <stdio.h>

namespace Jack
{

	class JackIOAdapterInterface
	{
    
		protected:
        
            int fCaptureChannels;
            int fPlaybackChannels;
            
            int fBufferSize;
            float fSampleRate;
            
            jack_time_t fLastCallbackTime;
            jack_time_t fCurCallbackTime;
            jack_time_t fDeltaTime;
            
            JackFilter fProducerFilter;
            JackFilter fConsumerFilter;
            
            // DLL
            JackDelayLockedLoop fProducerDLL;
            JackDelayLockedLoop fConsumerDLL;
            jack_time_t fCurFrames;
             
            JackResampler* fCaptureRingBuffer;
            JackResampler* fPlaybackRingBuffer;
            bool fRunning;
               
		public:
        
			JackIOAdapterInterface(int input, int output, int buffer_size, float sample_rate)
                :fCaptureChannels(input), 
                fPlaybackChannels(output), 
                fBufferSize(buffer_size), 
                fSampleRate(sample_rate),
                fLastCallbackTime(0),
                fCurCallbackTime(0),
                fDeltaTime(0),
                fProducerDLL(buffer_size, sample_rate),
                fConsumerDLL(buffer_size, sample_rate),
                fRunning(false)
            {}
			virtual ~JackIOAdapterInterface()
            {}
            
            void SetRingBuffers(JackResampler* input, JackResampler* output)
            {
                fCaptureRingBuffer = input;
                fPlaybackRingBuffer = output;
            }
             
            bool IsRunning() {return fRunning;}
            
            virtual int Open();
            virtual int Close();
            
            virtual void SetBufferSize(int buffer_size)
            {
                fBufferSize = buffer_size;
            }
            
            virtual void SetCallbackDeltaTime(jack_time_t delta_usec)
            {
                jack_log("SetCallbackDeltaTime %ld", delta_usec);
                //printf("SetCallbackDeltaTime %ld\n", delta_usec);
                fDeltaTime = delta_usec;
            }
            
            virtual void SetCallbackTime(jack_time_t callback_usec)
            {
                jack_log("SetCallbackTime %ld", callback_usec);
                //printf("SetCallbackDeltaTime %ld\n", delta_usec);
                fConsumerDLL.IncFrame(callback_usec);
            }
        
	};
}

#endif
