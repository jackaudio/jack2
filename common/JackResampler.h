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


namespace Jack
{

    #define DEFAULT_RB_SIZE 16384 * 1	

	class JackResampler
	{
    
		protected:
        
            jack_ringbuffer_t* fRingBuffer;
            unsigned int fNum;
            unsigned int fDenom;
               
		public:
        
			JackResampler();
        	virtual ~JackResampler();
            
            virtual void Reset();
            
            virtual int ReadResample(float* buffer, unsigned int frames);
            virtual int WriteResample(float* buffer, unsigned int frames);
            
            virtual int Read(float* buffer, unsigned int frames);
            virtual int Write(float* buffer, unsigned int frames);
            
            virtual unsigned int ReadSpace();
            virtual unsigned int WriteSpace();
    
            virtual void SetRatio(unsigned int num, unsigned int denom)
            {
                fNum = num;
                fDenom = denom;
            }
            virtual void GetRatio(unsigned int& num, unsigned int& denom)
            {
                num = fNum;
                denom = fDenom;
            }
     
        };
}

#endif
