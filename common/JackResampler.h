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

#ifndef __JackResampler__
#define __JackResampler__

#include "ringbuffer.h"
#include "JackError.h"
#include <samplerate.h>

namespace Jack
{

    #define DEFAULT_RB_SIZE 16384	

	class JackResampler
	{
    
		protected:
        
            SRC_STATE* fResampler;
            jack_ringbuffer_t* fRingBuffer;
            double fRatio;
               
		public:
        
			JackResampler();
        	virtual ~JackResampler();
            
            int ReadResample(float* buffer, unsigned int frames);
            int WriteResample(float* buffer, unsigned int frames);
            
            int Read(float* buffer, unsigned int frames);
            int Write(float* buffer, unsigned int frames);
            
            void SetRatio(double ratio) 
            {
                fRatio = ratio;
            }
            double GetRatio() 
            {
                return fRatio;
            }
	};
}

#endif
