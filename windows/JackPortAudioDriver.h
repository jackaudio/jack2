/*
Copyright (C) 2004-2006 Grame

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

#ifndef __JackPortAudioDriver__
#define __JackPortAudioDriver__

#include "JackAudioDriver.h"
#include "portaudio.h"

namespace Jack
{

/*!
\brief The PortAudio driver.
*/

class JackPortAudioDriver : public JackAudioDriver
{

    private:

        PaStream* fStream;
        float** fInputBuffer;
        float** fOutputBuffer;
        PaDeviceIndex fInputDevice;
        PaDeviceIndex fOutputDevice;

        void PrintSupportedStandardSampleRates(const PaStreamParameters* inputParameters, const PaStreamParameters* outputParameters);
        bool GetInputDeviceFromName(const char* name, PaDeviceIndex* device, int* in_max);
        bool GetOutputDeviceFromName(const char* name, PaDeviceIndex* device, int* out_max);

        static int Render(const void* inputBuffer, void* outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void* userData);

    public:

        JackPortAudioDriver(const char* name, JackEngine* engine, JackSynchro** table)
                : JackAudioDriver(name, engine, table), fStream(NULL), fInputBuffer(NULL), fOutputBuffer(NULL),
                fInputDevice(paNoDevice), fOutputDevice(paNoDevice)
        {}

        virtual ~JackPortAudioDriver()
        {}

        int Open(jack_nframes_t frames_per_cycle,
                 jack_nframes_t rate,
                 int capturing,
                 int playing,
                 int chan_in,
                 int chan_out,
                 bool monitor,
                 const char* capture_driver_name,
                 const char* playback_driver_name,
                 jack_nframes_t capture_latency,
                 jack_nframes_t playback_latency);

        int Close();

        int Start();
        int Stop();

        int Read();
        int Write();

        int SetBufferSize(jack_nframes_t buffer_size);

        void PrintState();
};

} // end of namespace

#endif
