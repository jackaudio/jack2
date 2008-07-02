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
            
            jack_nframes_t fBufferSize;
            jack_nframes_t fSampleRate;
             
            // DLL
            JackAtomicDelayLockedLoop fProducerDLL;
            JackAtomicDelayLockedLoop fConsumerDLL;
             
            JackResampler** fCaptureRingBuffer;
            JackResampler** fPlaybackRingBuffer;

            bool fRunning;
               
		public:
        
			JackIOAdapterInterface(int input, int output, jack_nframes_t buffer_size, jack_nframes_t sample_rate)
                :fCaptureChannels(input), 
                fPlaybackChannels(output), 
                fBufferSize(buffer_size), 
                fSampleRate(sample_rate),
                fProducerDLL(buffer_size, sample_rate),
                fConsumerDLL(buffer_size, sample_rate),
                fRunning(false)
            {}
			virtual ~JackIOAdapterInterface()
            {}
            
            void SetRingBuffers(JackResampler** input, JackResampler** output)
            {
                fCaptureRingBuffer = input;
                fPlaybackRingBuffer = output;
            }
             
            bool IsRunning() {return fRunning;}
            
            virtual void Reset() {fRunning = false;}
            
            virtual int Open();
            virtual int Close();
            
            virtual int SetBufferSize(jack_nframes_t buffer_size)
            {
                fBufferSize = buffer_size;
                return 0;
            }
            
            virtual void SetCallbackTime(jack_time_t callback_usec)
            {
                fConsumerDLL.IncFrame(callback_usec);
            }
        
	};
}

#endif
