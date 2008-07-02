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

namespace Jack
{

	class JackIOAdapterInterface
	{
    
		protected:
        
            int fCaptureChannels;
            int fPlaybackChannels;
            int fBufferSize;
            float fSampleRate;
            jack_time_t fCallbackTime;
            jack_time_t fFirstCallbackTime;
          
            jack_ringbuffer_t* fCaptureRingBuffer;
            jack_ringbuffer_t* fPlaybackRingBuffer;
            bool fRunning;
               
		public:
        
			JackIOAdapterInterface(int input, int output, int buffer_size, float sample_rate)
                :fCaptureChannels(input), 
                fPlaybackChannels(output), 
                fBufferSize(buffer_size), 
                fSampleRate(sample_rate),
                fCallbackTime(0),
                fRunning(false)
            {}
			virtual ~JackIOAdapterInterface()
            {}
            
            void SetRingBuffers(jack_ringbuffer_t* input, jack_ringbuffer_t* output)
            {
                fCaptureRingBuffer = input;
                fPlaybackRingBuffer = output;
            }
             
            bool IsRunning() {return fRunning;}
            
            virtual int Open() = 0;
            virtual int Close() = 0;
            
            virtual void SetBufferSize(int buffer_size)
            {
                fBufferSize = buffer_size;
            }
            
            virtual void SetCallbackTime(jack_time_t usec)
            {
                fCallbackTime = usec;
            }
        
	};
}

#endif
