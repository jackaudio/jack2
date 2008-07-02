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
#include <samplerate.h>

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
            
            SRC_STATE** fCaptureResampler;
            SRC_STATE** fPlaybackResampler;
          
            jack_ringbuffer_t** fCaptureRingBuffer;
            jack_ringbuffer_t** fPlaybackRingBuffer;
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
                fRunning(false)
            {
                fCaptureResampler = new SRC_STATE*[fCaptureChannels];
                fPlaybackResampler = new SRC_STATE*[fPlaybackChannels];
            }
			virtual ~JackIOAdapterInterface()
            {
                delete[] fCaptureResampler;
                delete[] fPlaybackResampler;
            }
            
            void SetRingBuffers(jack_ringbuffer_t** input, jack_ringbuffer_t** output)
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
                fDeltaTime = delta_usec;
            }
        
	};
}

#endif
