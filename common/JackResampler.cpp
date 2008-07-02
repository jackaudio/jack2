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

#include "JackResampler.h"

namespace Jack
{

JackResampler::JackResampler():fRatio(0)
{
    int error;
    fResampler = src_new(SRC_LINEAR, 1, &error);
    if (error != 0) 
        jack_error("JackResampler::JackResampler err = %s", src_strerror(error));
    fRingBuffer = jack_ringbuffer_create(sizeof(float) * DEFAULT_RB_SIZE);
}

JackResampler::~JackResampler()
{
    src_delete(fResampler);
    if (fRingBuffer)
        jack_ringbuffer_free(fRingBuffer);
}

int JackResampler::Read(float* buffer, unsigned int frames)
{
    size_t len = jack_ringbuffer_read_space(fRingBuffer);
    jack_log("JackResampler::Read INPUT available = %ld", len / sizeof(float));
        
    if (len < frames * sizeof(float)) {
        jack_error("JackResampler::Read : producer too slow, missing frames = %d", frames);
        //jack_ringbuffer_read(fRingBuffer, buffer, len);
        return 0;
    } else {
        jack_ringbuffer_read(fRingBuffer, (char*)buffer, frames * sizeof(float));
        return frames;
    }
}

int JackResampler::Write(float* buffer, unsigned int frames)
{
    size_t len = jack_ringbuffer_write_space(fRingBuffer);
    jack_log("JackResampler::Write OUTPUT available = %ld", len / sizeof(float));
        
    if (len < frames * sizeof(float)) {
        jack_error("JackResampler::Write : consumer too slow, skip frames = %d", frames);
        //jack_ringbuffer_write(fRingBuffer, buffer, len);
        return 0;
    } else {
        jack_ringbuffer_write(fRingBuffer, (char*)buffer, frames * sizeof(float));
        return frames;
    }
}

int JackResampler::ReadResample(float* buffer, unsigned int frames)
{
    jack_ringbuffer_data_t ring_buffer_data[2];
    SRC_DATA src_data;
    unsigned int frames_to_write = frames;
    unsigned int written_frames = 0;
    int res;
    
    jack_ringbuffer_get_read_vector(fRingBuffer, ring_buffer_data);
    unsigned int available_frames = (ring_buffer_data[0].len + ring_buffer_data[1].len) / sizeof(float);
    jack_log("OUPUT available = %ld", available_frames);
      
    for (int j = 0; j < 2; j++) {
    
        if (ring_buffer_data[j].len > 0) {
            
            src_data.data_in = (float*)ring_buffer_data[j].buf;
            src_data.data_out = &buffer[written_frames];
            src_data.input_frames = ring_buffer_data[j].len / sizeof(float);
            src_data.output_frames = frames_to_write;
            src_data.end_of_input = 0;
            src_data.src_ratio = fRatio;
             
            res = src_process(fResampler, &src_data);
            if (res != 0)
                jack_error("JackPortAudioIOAdapter::Render err = %s", src_strerror(res));
                
            frames_to_write -= src_data.output_frames_gen;
            written_frames += src_data.output_frames_gen;
            
            jack_log("OUTPUT : j = %d input_frames_used = %ld output_frames_gen = %ld", j, src_data.input_frames_used, src_data.output_frames_gen);
            jack_ringbuffer_read_advance(fRingBuffer, src_data.input_frames_used * sizeof(float));
        }
    }
    
    if (written_frames < frames)
        jack_error("JackPortAudioIOAdapter::Render error written_frames = %ld", written_frames);
        
    return written_frames;
}

int JackResampler::WriteResample(float* buffer, unsigned int frames)
{
    jack_ringbuffer_data_t ring_buffer_data[2];
    SRC_DATA src_data;
    unsigned int frames_to_read = frames;
    unsigned int read_frames = 0;
    int res;
    
    jack_ringbuffer_get_write_vector(fRingBuffer, ring_buffer_data);
    unsigned int available_frames = (ring_buffer_data[0].len + ring_buffer_data[1].len) / sizeof(float);
    jack_log("INPUT available = %ld", available_frames);
    
    for (int j = 0; j < 2; j++) {
            
        if (ring_buffer_data[j].len > 0) {
        
            src_data.data_in = &buffer[read_frames];
            src_data.data_out = (float*)ring_buffer_data[j].buf;
            src_data.input_frames = frames_to_read;
            src_data.output_frames = (ring_buffer_data[j].len / sizeof(float));
            src_data.end_of_input = 0;
            src_data.src_ratio = fRatio;
         
            res = src_process(fResampler, &src_data);
            if (res != 0)
                jack_error("JackPortAudioIOAdapter::Render err = %s", src_strerror(res));
                
            frames_to_read -= src_data.input_frames_used;
            read_frames += src_data.input_frames_used;
        
            jack_log("INPUT : j = %d input_frames_used = %ld output_frames_gen = %ld", j, src_data.input_frames_used, src_data.output_frames_gen);
            jack_ringbuffer_write_advance(fRingBuffer, src_data.output_frames_gen * sizeof(float));
        }
    }
    
    if (read_frames < frames) 
        jack_error("JackPortAudioIOAdapter::Render error read_frames = %ld", read_frames);
        
    return read_frames;
}

}
