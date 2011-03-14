/*
Copyright (C) 2001-2003 Paul Davis
Copyright (C) 2004-2008 Grame

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#include "JackGlobals.h"
#include "JackEngineControl.h"
#include "JackPortType.h"

#include <string.h>

#if defined (__APPLE__)
#include <Accelerate/Accelerate.h>
#elif defined (__SSE__) && !defined (__sun__)
#include <xmmintrin.h>
#endif

namespace Jack
{

static void AudioBufferInit(void* buffer, size_t buffer_size, jack_nframes_t)
{
    memset(buffer, 0, buffer_size);
}

static inline void MixAudioBuffer(jack_default_audio_sample_t* mixbuffer, jack_default_audio_sample_t* buffer, jack_nframes_t frames)
{
#ifdef __APPLE__
    // It seems that a vector mult only operation does not exist...
    jack_default_audio_sample_t gain = jack_default_audio_sample_t(1.0);
    vDSP_vsma(buffer, 1, &gain, mixbuffer, 1, mixbuffer, 1, frames);
#else
    jack_nframes_t frames_group = frames / 4;
    frames = frames % 4;

    while (frames_group > 0) {
#if defined (__SSE__) && !defined (__sun__)
        __m128 vec = _mm_add_ps(_mm_load_ps(mixbuffer), _mm_load_ps(buffer));
        _mm_store_ps(mixbuffer, vec);

        mixbuffer += 4;
        buffer += 4;
        frames_group--;
#else
    register jack_default_audio_sample_t mixFloat1 = *mixbuffer;
    register jack_default_audio_sample_t sourceFloat1 = *buffer;
    register jack_default_audio_sample_t mixFloat2 = *(mixbuffer + 1);
    register jack_default_audio_sample_t sourceFloat2 = *(buffer + 1);
    register jack_default_audio_sample_t mixFloat3 = *(mixbuffer + 2);
    register jack_default_audio_sample_t sourceFloat3 = *(buffer + 2);
    register jack_default_audio_sample_t mixFloat4 = *(mixbuffer + 3);
    register jack_default_audio_sample_t sourceFloat4 = *(buffer + 3);

    buffer += 4;
    frames_group--;

    mixFloat1 += sourceFloat1;
    mixFloat2 += sourceFloat2;
    mixFloat3 += sourceFloat3;
    mixFloat4 += sourceFloat4;

    *mixbuffer = mixFloat1;
    *(mixbuffer + 1) = mixFloat2;
    *(mixbuffer + 2) = mixFloat3;
    *(mixbuffer + 3) = mixFloat4;

    mixbuffer += 4;
#endif
    }

    while (frames > 0) {
        register jack_default_audio_sample_t mixFloat1 = *mixbuffer;
        register jack_default_audio_sample_t sourceFloat1 = *buffer;
        buffer++;
        frames--;
        mixFloat1 += sourceFloat1;
        *mixbuffer = mixFloat1;
        mixbuffer++;
    }
#endif
}

static void AudioBufferMixdown(void* mixbuffer, void** src_buffers, int src_count, jack_nframes_t nframes)
{
    void* buffer;

    // Copy first buffer
#if defined (__SSE__) && !defined (__sun__)
    jack_nframes_t frames_group = nframes / 4;
    jack_nframes_t remaining_frames = nframes % 4;

    jack_default_audio_sample_t * source = static_cast<jack_default_audio_sample_t*>(src_buffers[0]);
    jack_default_audio_sample_t * target = static_cast<jack_default_audio_sample_t*>(mixbuffer);

    while (frames_group > 0)
    {
        __m128 vec = _mm_load_ps(source);
        _mm_store_ps(target, vec);
        source += 4;
        target += 4;
        --frames_group;
    }

    for (jack_nframes_t i = 0; i != remaining_frames; ++i)
        target[i] = source[i];

#else
    memcpy(mixbuffer, src_buffers[0], nframes * sizeof(jack_default_audio_sample_t));
#endif

    // Mix remaining buffers
    for (int i = 1; i < src_count; ++i) {
        buffer = src_buffers[i];
        MixAudioBuffer(static_cast<jack_default_audio_sample_t*>(mixbuffer), static_cast<jack_default_audio_sample_t*>(buffer), nframes);
    }
}

static size_t AudioBufferSize()
{
    return GetEngineControl()->fBufferSize * sizeof(jack_default_audio_sample_t);
}

const JackPortType gAudioPortType =
{
    JACK_DEFAULT_AUDIO_TYPE,
    AudioBufferSize,
    AudioBufferInit,
    AudioBufferMixdown
};

} // namespace Jack

