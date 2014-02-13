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
#include "types.h"

namespace Jack
{

#define DEFAULT_RB_SIZE 32768
#define DEFAULT_ADAPTATIVE_SIZE 2048

inline float Range(float min, float max, float val)
{
    return (val < min) ? min : ((val > max) ? max : val);
}

/*!
\brief Base class for RingBuffer in frames.
*/

class JackRingBuffer
{

    protected:

        jack_ringbuffer_t* fRingBuffer;
        unsigned int fRingBufferSize;

    public:

        JackRingBuffer(int size = DEFAULT_RB_SIZE);
        virtual ~JackRingBuffer();

        virtual void Reset(unsigned int new_size);

        // in frames
        virtual unsigned int Read(jack_default_audio_sample_t* buffer, unsigned int frames);
        virtual unsigned int Write(jack_default_audio_sample_t* buffer, unsigned int frames);

        // in bytes
        virtual unsigned int Read(void* buffer, unsigned int bytes);
        virtual unsigned int Write(void* buffer, unsigned int bytes);

        // in frames
        virtual unsigned int ReadSpace();
        virtual unsigned int WriteSpace();

        unsigned int GetError()
        {
            return (jack_ringbuffer_read_space(fRingBuffer) / sizeof(float)) - (fRingBufferSize / 2);
        }

};

/*!
\brief Base class for Resampler.
*/

class JackResampler : public JackRingBuffer
{

    protected:

        double fRatio;
  
    public:

        JackResampler():JackRingBuffer(),fRatio(1)
        {}
        virtual ~JackResampler()
        {}

        virtual unsigned int ReadResample(jack_default_audio_sample_t* buffer, unsigned int frames);
        virtual unsigned int WriteResample(jack_default_audio_sample_t* buffer, unsigned int frames);

        void SetRatio(double ratio)
        {
            fRatio = Range(0.25, 4.0, ratio);
        }

        double GetRatio()
        {
            return fRatio;
        }

    };
}

#endif
