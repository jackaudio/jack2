/*
Copyright (C) 2006 Grame

Portable Audio I/O Library for ASIO Drivers
Author: Stephane Letz
Based on the Open Source API proposed by Ross Bencina
Copyright (c) 2000-2002 Stephane Letz, Phil Burk, Ross Bencina

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

#ifndef __JackASIODriver__
#define __JackASIODriver__

#include "JackAudioDriver.h"
#include "portaudio.h"

namespace Jack
{

/*!
\brief The ASIO driver.
*/

class JackASIODriver : public JackAudioDriver
{

    private:

        PaStream* fStream;
        float** fInputBuffer;
        float** fOutputBuffer;
        PaDeviceIndex fInputDevice;
        PaDeviceIndex fOutputDevice;

        static int Render(const void* inputBuffer, void* outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void* userData);

    public:

        JackASIODriver(const char* name, JackEngine* engine, JackSynchro** table)
                : JackAudioDriver(name, engine, table), fStream(NULL), fInputBuffer(NULL), fOutputBuffer(NULL),
                fInputDevice(paNoDevice), fOutputDevice(paNoDevice)
        {}

        virtual ~JackASIODriver()
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

        int SetBufferSize(jack_nframes_t nframes);

        void PrintState();
};

} // end of namespace

#endif
