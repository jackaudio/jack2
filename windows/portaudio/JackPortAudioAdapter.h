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

#ifndef __JackPortAudioAdapter__
#define __JackPortAudioAdapter__

#include "JackAudioAdapter.h"
#include "JackPortAudioDevices.h"
#include "jslist.h"

namespace Jack
{

    /*!
    \brief Audio adapter using PortAudio API.
    */

    class JackPortAudioAdapter : public JackAudioAdapterInterface
    {

    private:

        PortAudioDevices fPaDevices;
        PaStream* fStream;
        PaDeviceIndex fInputDevice;
        PaDeviceIndex fOutputDevice;

        static int Render(const void* inputBuffer, void* outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void* userData);

    public:

        JackPortAudioAdapter(jack_nframes_t buffer_size, jack_nframes_t sample_rate, const JSList* params);
        ~JackPortAudioAdapter()
        {}

        int Open();
        int Close();

        int SetSampleRate(jack_nframes_t sample_rate);
        int SetBufferSize(jack_nframes_t buffer_size);

    };

}

#ifdef __cplusplus
extern "C"
{
#endif

#include "JackCompilerDeps.h"
#include "driver_interface.h"

SERVER_EXPORT jack_driver_desc_t* jack_get_descriptor();

#ifdef __cplusplus
}
#endif

#endif
