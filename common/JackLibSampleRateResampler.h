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

#ifndef __JackLibSampleRateResampler__
#define __JackLibSampleRateResampler__

#include "JackResampler.h"
#include <samplerate.h>

namespace Jack
{

/*!
\brief Resampler using "libsamplerate" (http://www.mega-nerd.com/SRC/).
*/

class JackLibSampleRateResampler : public JackResampler
{

    private:

        SRC_STATE* fResampler;

    public:

        JackLibSampleRateResampler();
        JackLibSampleRateResampler(unsigned int quality);
        virtual ~JackLibSampleRateResampler();

        unsigned int ReadResample(jack_default_audio_sample_t* buffer, unsigned int frames);
        unsigned int WriteResample(jack_default_audio_sample_t* buffer, unsigned int frames);

        void Reset(unsigned int new_size);

    };
}

#endif
