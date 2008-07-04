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

#ifndef __JackCallbackAudioAdapter__
#define __JackCallbackAudioAdapter__

#include "JackAudioAdapter.h"
#include "ringbuffer.h"

namespace Jack
{
    class JackCallbackAudioAdapter : public JackAudioAdapter
    {

        private:
        
            JackResampler** fCaptureRingBuffer;
            JackResampler** fPlaybackRingBuffer;

            static int Process(jack_nframes_t, void* arg);
            static int BufferSize(jack_nframes_t buffer_size, void *arg);
            static int SampleRate(jack_nframes_t sample_rate, void *arg);

            void Reset();

        public:

            JackCallbackAudioAdapter(jack_client_t* jack_client, 
                                    JackAudioAdapterInterface* audio_io, 
                                    int input, 
                                    int output);
            ~JackCallbackAudioAdapter();
    
    };
    
}

#endif
