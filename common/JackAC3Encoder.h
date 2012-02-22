/*
Copyright (C) 2006 Jesse Chappell <jesse@essej.net> (AC3Jack)
Copyright (C) 2012 Grame

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

#ifndef __JackAC3Encoder__
#define __JackAC3Encoder__

#include <aften/aften.h>
#include <aften/aften-types.h>

#include "ringbuffer.h"
#include "types.h"

#define MAX_AC3_CHANNELS 6

#define SPDIF_HEADER_SIZE 8
#define SPDIF_FRAME_SIZE 6144

#define SAMPLE_MAX_16BIT  32768.0f
#define SAMPLE_MAX_24BIT  8388608.0f

namespace Jack
{

struct JackAC3EncoderParams
{
	int64_t duration;
	unsigned int channels;
	int bitdepth;
	int bitrate;
	unsigned int sample_rate;
	bool lfe;
};

class JackAC3Encoder
{

    private:

		AftenContext fAftenContext;
        jack_ringbuffer_t* fRingBuffer;
        
        float* fSampleBuffer;
        unsigned char* fAC3Buffer;
        unsigned char* fZeroBuffer;
        
        int fOutSizeByte; 
        
        jack_nframes_t fFramePos;
        jack_nframes_t fSampleRate;
        jack_nframes_t fByteRate;
        
        void FillSpdifHeader(unsigned char* buf, int outsize);
        int Output2Driver(float** outputs, jack_nframes_t nframes);
        
        void sample_move_dS_s16(jack_default_audio_sample_t* dst, char *src, jack_nframes_t nsamples, unsigned long src_skip);
        void sample_move_dS_s16_24ph(jack_default_audio_sample_t* dst, char *src, jack_nframes_t nsamples, unsigned long src_skip);

    public:
    
    #ifdef __ppc__
        JackAC3Encoder(const JackAC3EncoderParams& params) {}
        virtual ~JackAC3Encoder() {}
  
        bool Init(jack_nframes_t sample_rate) {return false;}
  
        void Process(float** inputs, float** outputs, int nframes) {}
        void GetChannelName(const char* name, const char* alias, char* portname, int channel) {}
    #else
        JackAC3Encoder(const JackAC3EncoderParams& params);
        virtual ~JackAC3Encoder();
    
        bool Init(jack_nframes_t sample_rate);
    
        void Process(float** inputs, float** outputs, int nframes);
        void GetChannelName(const char* name, const char* alias, char* portname, int channel);
    #endif
};

typedef JackAC3Encoder * JackAC3EncoderPtr;

} // end of namespace

#endif
