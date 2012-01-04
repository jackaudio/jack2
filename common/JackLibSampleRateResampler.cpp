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

#include "JackLibSampleRateResampler.h"
#include "JackError.h"

namespace Jack
{

JackLibSampleRateResampler::JackLibSampleRateResampler()
    :JackResampler()
{
    int error;
    fResampler = src_new(SRC_LINEAR, 1, &error);
    if (error != 0)
        jack_error("JackLibSampleRateResampler::JackLibSampleRateResampler err = %s", src_strerror(error));
}

JackLibSampleRateResampler::JackLibSampleRateResampler(unsigned int quality)
    :JackResampler()
{
     switch (quality) {
       case 0:
            quality = SRC_LINEAR;
            break;
        case 1:
            quality = SRC_ZERO_ORDER_HOLD;
            break;
        case 2:
            quality = SRC_SINC_FASTEST;
            break;
        case 3:
            quality = SRC_SINC_MEDIUM_QUALITY;
            break;
        case 4:
            quality = SRC_SINC_BEST_QUALITY;
            break;
        default:
            quality = SRC_LINEAR;
            jack_error("Out of range resample quality");
            break;
    }

    int error;
    fResampler = src_new(quality, 1, &error);
    if (error != 0) {
        jack_error("JackLibSampleRateResampler::JackLibSampleRateResampler err = %s", src_strerror(error));
    }
}

JackLibSampleRateResampler::~JackLibSampleRateResampler()
{
    src_delete(fResampler);
}

void JackLibSampleRateResampler::Reset(unsigned int new_size)
{
    JackResampler::Reset(new_size);
    src_reset(fResampler);
}

unsigned int JackLibSampleRateResampler::ReadResample(jack_default_audio_sample_t* buffer, unsigned int frames)
{
    jack_ringbuffer_data_t ring_buffer_data[2];
    SRC_DATA src_data;
    unsigned int frames_to_write = frames;
    unsigned int written_frames = 0;
    int res;

    jack_ringbuffer_get_read_vector(fRingBuffer, ring_buffer_data);
    unsigned int available_frames = (ring_buffer_data[0].len + ring_buffer_data[1].len) / sizeof(jack_default_audio_sample_t);
    jack_log("Output available = %ld", available_frames);

    for (int j = 0; j < 2; j++) {

        if (ring_buffer_data[j].len > 0) {

            src_data.data_in = (jack_default_audio_sample_t*)ring_buffer_data[j].buf;
            src_data.data_out = &buffer[written_frames];
            src_data.input_frames = ring_buffer_data[j].len / sizeof(jack_default_audio_sample_t);
            src_data.output_frames = frames_to_write;
            src_data.end_of_input = 0;
            src_data.src_ratio = fRatio;

            res = src_process(fResampler, &src_data);
            if (res != 0) {
                jack_error("JackLibSampleRateResampler::ReadResample ratio = %f err = %s", fRatio, src_strerror(res));
                return 0;
            }

            frames_to_write -= src_data.output_frames_gen;
            written_frames += src_data.output_frames_gen;

            if ((src_data.input_frames_used == 0 || src_data.output_frames_gen == 0) && j == 0) {
                jack_log("Output : j = %d input_frames_used = %ld output_frames_gen = %ld frames1 = %lu frames2 = %lu"
                    , j, src_data.input_frames_used, src_data.output_frames_gen, ring_buffer_data[0].len, ring_buffer_data[1].len);
            }

            jack_log("Output : j = %d input_frames_used = %ld output_frames_gen = %ld", j, src_data.input_frames_used, src_data.output_frames_gen);
            jack_ringbuffer_read_advance(fRingBuffer, src_data.input_frames_used * sizeof(jack_default_audio_sample_t));
        }
    }

    if (written_frames < frames) {
        jack_error("Output available = %ld", available_frames);
        jack_error("JackLibSampleRateResampler::ReadResample error written_frames = %ld", written_frames);
    }

    return written_frames;
}

unsigned int JackLibSampleRateResampler::WriteResample(jack_default_audio_sample_t* buffer, unsigned int frames)
{
    jack_ringbuffer_data_t ring_buffer_data[2];
    SRC_DATA src_data;
    unsigned int frames_to_read = frames;
    unsigned int read_frames = 0;
    int res;

    jack_ringbuffer_get_write_vector(fRingBuffer, ring_buffer_data);
    unsigned int available_frames = (ring_buffer_data[0].len + ring_buffer_data[1].len) / sizeof(jack_default_audio_sample_t);
    jack_log("Input available = %ld", available_frames);

    for (int j = 0; j < 2; j++) {

        if (ring_buffer_data[j].len > 0) {

            src_data.data_in = &buffer[read_frames];
            src_data.data_out = (jack_default_audio_sample_t*)ring_buffer_data[j].buf;
            src_data.input_frames = frames_to_read;
            src_data.output_frames = (ring_buffer_data[j].len / sizeof(jack_default_audio_sample_t));
            src_data.end_of_input = 0;
            src_data.src_ratio = fRatio;

            res = src_process(fResampler, &src_data);
            if (res != 0) {
                jack_error("JackLibSampleRateResampler::WriteResample ratio = %f err = %s", fRatio, src_strerror(res));
                return 0;
            }

            frames_to_read -= src_data.input_frames_used;
            read_frames += src_data.input_frames_used;

            if ((src_data.input_frames_used == 0 || src_data.output_frames_gen == 0) && j == 0) {
                jack_log("Input : j = %d input_frames_used = %ld output_frames_gen = %ld frames1 = %lu frames2 = %lu"
                    , j, src_data.input_frames_used, src_data.output_frames_gen, ring_buffer_data[0].len, ring_buffer_data[1].len);
            }

            jack_log("Input : j = %d input_frames_used = %ld output_frames_gen = %ld", j, src_data.input_frames_used, src_data.output_frames_gen);
            jack_ringbuffer_write_advance(fRingBuffer, src_data.output_frames_gen * sizeof(jack_default_audio_sample_t));
        }
    }

    if (read_frames < frames) {
        jack_error("Input available = %ld", available_frames);
        jack_error("JackLibSampleRateResampler::WriteResample error read_frames = %ld", read_frames);
    }

    return read_frames;
}

}
