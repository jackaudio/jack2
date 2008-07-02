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

#ifndef __JackPortAudioIOAdapter__
#define __JackPortAudioIOAdapter__

#include "JackIOAdapter.h"
#include "portaudio.h"

namespace Jack
{

    #define MAX_SIZE 64
    
    struct  Filter {
    
        jack_time_t fTable[MAX_SIZE];
        
        Filter()
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

	class JackPortAudioIOAdapter : public JackIOAdapterInterface
	{
    
		private:
        
            PaStream* fStream;
            PaDeviceIndex fInputDevice;
            PaDeviceIndex fOutputDevice;
            
            Filter fProducerFilter;
            Filter fConsumerFilter;
            
            static int Render(const void* inputBuffer, void* outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void* userData);

		public:
        
			JackPortAudioIOAdapter(int input, int output, int buffer_size, float sample_rate)
                :JackIOAdapterInterface(input, output, buffer_size, sample_rate)
            {}
			~JackPortAudioIOAdapter()
            {}
            
            int Open();
            int Close();
            
           void SetBufferSize(int buffer_size);
            
   	};
}

#endif
