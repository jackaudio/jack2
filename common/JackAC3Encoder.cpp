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

#include "JackAC3Encoder.h"
#include "JackError.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define max(x,y) (((x)>(y)) ? (x) : (y))
#define min(x,y) (((x)<(y)) ? (x) : (y))

namespace Jack
{
    
#ifndef __ppc__

JackAC3Encoder::JackAC3Encoder(const JackAC3EncoderParams& params)
{
    aften_set_defaults(&fAftenContext);
    
    fAftenContext.channels = params.channels;
	fAftenContext.samplerate = params.sample_rate;
	fAftenContext.params.bitrate = params.bitrate;

	int acmod = A52_ACMOD_MONO;
	int lfe = params.lfe;
   
	switch (params.channels) {
    
        case 1: acmod = A52_ACMOD_MONO; break;
        case 2: acmod = A52_ACMOD_STEREO; break;
        case 3: acmod = A52_ACMOD_3_0; break;
        case 4: acmod = A52_ACMOD_2_2; break;
        case 5: acmod = A52_ACMOD_3_2; break;
            break;
            
        default: 
            break;
	}

	if (lfe) {
		fAftenContext.channels += 1;
	}
	
	fAftenContext.acmod = acmod;
	fAftenContext.lfe = lfe;
	fAftenContext.sample_format = A52_SAMPLE_FMT_FLT;
	fAftenContext.verbose = 1;

	fAftenContext.system.n_threads = 1;
    
    // create interleaved framebuffer for MAX_AC3_CHANNELS
	fSampleBuffer = new float[MAX_AC3_CHANNELS * A52_SAMPLES_PER_FRAME];
    
    // create AC3 buffer
	fAC3Buffer = new unsigned char[A52_MAX_CODED_FRAME_SIZE];
	memset(fAC3Buffer, 0, A52_MAX_CODED_FRAME_SIZE);
    
    fZeroBuffer = new unsigned char[SPDIF_FRAME_SIZE];
	memset(fZeroBuffer, 0, SPDIF_FRAME_SIZE);
    
    fRingBuffer = jack_ringbuffer_create(32768);
  	
    fOutSizeByte = 0;
    fFramePos = 0;
    
    fSampleRate = 0;
    fByteRate = 0;
}

bool JackAC3Encoder::Init(jack_nframes_t sample_rate)
{
    fSampleRate = sample_rate;
	fByteRate = fSampleRate * sizeof(short) * 2;
    return (aften_encode_init(&fAftenContext) == 0);
}
        
JackAC3Encoder::~JackAC3Encoder()
{
    aften_encode_close(&fAftenContext);
    
    delete [] fSampleBuffer;
    delete [] fAC3Buffer;
    delete [] fZeroBuffer;
    
    if (fRingBuffer) {
        jack_ringbuffer_free(fRingBuffer);
    }
}

void JackAC3Encoder::Process(float** inputs_buffer, float** outputs_buffer, int nframes)
{
    // fill and process frame buffers as appropriate
	jack_nframes_t frames_left = A52_SAMPLES_PER_FRAME - fFramePos;
	jack_nframes_t offset = 0;

	while (offset < nframes)
	{
		if ((nframes - offset) >= frames_left) {
        
			// copy only frames_left more data
			jack_nframes_t pos = fFramePos * fAftenContext.channels;
			for (jack_nframes_t spos = offset; spos < offset + frames_left; ++spos) {
				for (size_t i = 0; i < fAftenContext.channels; ++i) {
					fSampleBuffer[pos + i] = inputs_buffer[i][spos];
				}
				pos += fAftenContext.channels;
			}  
			
			// use interleaved version
            int res = aften_encode_frame(&fAftenContext, fAC3Buffer + SPDIF_HEADER_SIZE, fSampleBuffer);
            if (res < 0) {
                jack_error("aften_encode_frame error !!");
                return;
            }
            
            fOutSizeByte = res;
	
			FillSpdifHeader(fAC3Buffer, fOutSizeByte + SPDIF_HEADER_SIZE);
			
			// push AC3 output to SPDIF ring buffer
			float calc_ac3byterate = (fOutSizeByte * fSampleRate / (float) A52_SAMPLES_PER_FRAME);  
			jack_nframes_t silencebytes = (jack_nframes_t) (fOutSizeByte * (fByteRate / calc_ac3byterate)) - fOutSizeByte - SPDIF_HEADER_SIZE;
			
            jack_ringbuffer_write(fRingBuffer, (const char *)fAC3Buffer, fOutSizeByte + SPDIF_HEADER_SIZE);

            // write the proper remainder of zero padding (inefficient, should be memsetting)
            jack_ringbuffer_write(fRingBuffer, (const char *)fZeroBuffer, silencebytes);
			
			offset += frames_left;
			frames_left = A52_SAMPLES_PER_FRAME;
			fFramePos = 0;
            
		} else {
        
			// copy incoming data into frame buffers without processing
			jack_nframes_t pos = fFramePos * fAftenContext.channels;
			for (jack_nframes_t spos = offset; spos < nframes; ++spos) {
				for (size_t i = 0; i < fAftenContext.channels; ++i) {
					fSampleBuffer[pos + i] = inputs_buffer[i][spos];
				}
				pos += fAftenContext.channels;
			}  

			fFramePos += (nframes - offset);
			offset += (nframes-offset);
		}
	}

	Output2Driver(outputs_buffer, nframes);
}

void JackAC3Encoder::FillSpdifHeader(unsigned char* buf, int outsize)
{
	// todo, use outsize and not assume the fixed frame size?
	int ac3outsize = outsize - SPDIF_HEADER_SIZE;
	
	buf[0] = 0x72; buf[1] = 0xf8;	/* spdif syncword */
	buf[2] = 0x1f; buf[3] = 0x4e;	/* .............. */
	buf[4] = 0x01;                  /* AC3 data */
	buf[5] = buf[13] & 7;           /* bsmod, stream = 0 */
	buf[6] = (ac3outsize << 3) & 0xff;
	buf[7] = (ac3outsize >> 5) & 0xff;

#if !IS_BIGENDIAN
	swab(buf+SPDIF_HEADER_SIZE, buf + SPDIF_HEADER_SIZE, ac3outsize);
#endif
}

int JackAC3Encoder::Output2Driver(float** outputs, jack_nframes_t nframes)
{
	int wrotebytes = 0;
	jack_nframes_t nframes_left = nframes;
	
	if (jack_ringbuffer_read_space(fRingBuffer) == 0) {
    
		// just write silence
		memset(outputs[0], 0, nframes * sizeof(jack_default_audio_sample_t));
		memset(outputs[1], 0, nframes * sizeof(jack_default_audio_sample_t));	
        	
	} else {
    
	  jack_ringbuffer_data_t rb_data[2];

		jack_ringbuffer_get_read_vector(fRingBuffer, rb_data);
		
		while (nframes_left > 0 && rb_data[0].len > 4) {
	
			jack_nframes_t towrite_frames = (rb_data[0].len) / (sizeof(short) * 2);
			towrite_frames = min(towrite_frames, nframes_left);
			
			// write and deinterleave into the two channels
#if 1
			sample_move_dS_s16(outputs[0] + (nframes - nframes_left), (char *) rb_data[0].buf, towrite_frames, sizeof(short) * 2);
			sample_move_dS_s16(outputs[1] + (nframes - nframes_left), (char *) rb_data[0].buf + sizeof(short), towrite_frames, sizeof(short) * 2);
#else
			sample_move_dS_s16_24ph(outputs[0] + (nframes - nframes_left), (char *) rb_data[0].buf, towrite_frames, sizeof(short) * 2);
			sample_move_dS_s16_24ph(outputs[1] + (nframes - nframes_left), (char *) rb_data[0].buf + sizeof(short), towrite_frames, sizeof(short) * 2);
#endif			
			wrotebytes = towrite_frames * sizeof(short) * 2;
			nframes_left -= towrite_frames;
			
			jack_ringbuffer_read_advance(fRingBuffer, wrotebytes);
			jack_ringbuffer_get_read_vector(fRingBuffer, rb_data);
		}

		if (nframes_left > 0) {
			// write silence
			memset(outputs[0] + (nframes - nframes_left), 0, (nframes_left) * sizeof(jack_default_audio_sample_t));
			memset(outputs[1] + (nframes - nframes_left), 0, (nframes_left) * sizeof(jack_default_audio_sample_t));		
		}
	}

	return wrotebytes;
}

void JackAC3Encoder::sample_move_dS_s16(jack_default_audio_sample_t* dst, char *src, jack_nframes_t nsamples, unsigned long src_skip) 
{
	/* ALERT: signed sign-extension portability !!! */
	while (nsamples--) {
		*dst = (*((short *) src)) / SAMPLE_MAX_16BIT;
		dst++;
		src += src_skip;
	}
}	

void JackAC3Encoder::sample_move_dS_s16_24ph(jack_default_audio_sample_t* dst, char *src, jack_nframes_t nsamples, unsigned long src_skip) 
{
	/* ALERT: signed sign-extension portability !!! */
	while (nsamples--) {
		*dst = (((int)(*((short *) src))) << 8) / SAMPLE_MAX_24BIT;
		dst++;
		src += src_skip;
	}
}

void JackAC3Encoder::GetChannelName(const char* name, const char* alias, char* portname, int channel)
{
    /*
	 * 2 channels = L, R
	 * 3 channels = L, C, R
	 * 4 channels = L, R, LS, RS
	 * 5 ch       = L, C, R,  LS, RS
	 * 6 ch       = L, C, R,  LS, RS, LFE
	 */
     
     const char* AC3_name = "";
     
     switch (channel) {
     
        case 0:
            AC3_name = "AC3_1_Left";
            break;
            
        case 1:
            if (fAftenContext.channels == 2 || fAftenContext.channels == 4) {
                AC3_name = "AC3_2_Right";
            } else {
                AC3_name = "AC3_2_Center";
            }
            break;
            
        case 2:
            if (fAftenContext.channels == 4) {
                AC3_name = "AC3_3_LeftSurround";
            } else {
                AC3_name = "AC3_3_Right";
            }
            break;
        
        case 3:
            if (fAftenContext.channels == 4) {
                AC3_name = "AC3_4_RightSurround";
            } else {
                AC3_name = "AC3_4_LeftSurround";
            }
            break;
            
        case 4:
            if (fAftenContext.channels > 4) {
               AC3_name = "AC3_5_RightSurround";
			}
            break;
            
        default:
            break;
     }
     
     // Last channel
     if (fAftenContext.lfe && (channel == fAftenContext.channels - 1)) {
        sprintf(portname, "%s:%s:AC3_%d_LFE", name, alias, fAftenContext.channels);
     } else {
        sprintf(portname, "%s:%s:%s", name, alias, AC3_name);
     }
}
    
#endif

} // end of namespace