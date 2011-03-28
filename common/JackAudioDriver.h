/*
Copyright (C) 2001 Paul Davis
Copyright (C) 2004-2008 Grame

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

#ifndef __JackAudioDriver__
#define __JackAudioDriver__

#include "JackDriver.h"

namespace Jack
{

/*!
\brief The base class for audio drivers: drivers with audio ports.
*/

class SERVER_EXPORT JackAudioDriver : public JackDriver
{

    protected:

        void ProcessGraphAsync();
        int ProcessGraphSync();
        void WaitUntilNextCycle();

        virtual int ProcessAsync();
        virtual int ProcessSync();

        int fCaptureChannels;
        int fPlaybackChannels;

        // Static tables since the actual number of ports may be changed by the real driver
        // thus dynamic allocation is more difficult to handle
        jack_port_id_t fCapturePortList[DRIVER_PORT_NUM];
        jack_port_id_t fPlaybackPortList[DRIVER_PORT_NUM];
        jack_port_id_t fMonitorPortList[DRIVER_PORT_NUM];

        bool fWithMonitorPorts;

        jack_default_audio_sample_t* GetInputBuffer(int port_index);
        jack_default_audio_sample_t* GetOutputBuffer(int port_index);
        jack_default_audio_sample_t* GetMonitorBuffer(int port_index);

        void HandleLatencyCallback(int status);

    public:

        JackAudioDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table);
        virtual ~JackAudioDriver();

        virtual int Open(jack_nframes_t buffer_size,
                        jack_nframes_t samplerate,
                        bool capturing,
                        bool playing,
                        int inchannels,
                        int outchannels,
                        bool monitor,
                        const char* capture_driver_name,
                        const char* playback_driver_name,
                        jack_nframes_t capture_latency,
                        jack_nframes_t playback_latency);

        virtual int Open(bool capturing,
                        bool playing,
                        int inchannels,
                        int outchannels,
                        bool monitor,
                        const char* capture_driver_name,
                        const char* playback_driver_name,
                        jack_nframes_t capture_latency,
                        jack_nframes_t playback_latency);

        virtual int Process();
        virtual int ProcessNull();

        virtual int Attach();
        virtual int Detach();

        virtual int Start();
        virtual int Stop();

        virtual int Write();

        virtual int SetBufferSize(jack_nframes_t buffer_size);
        virtual int SetSampleRate(jack_nframes_t sample_rate);

        virtual int ClientNotify(int refnum, const char* name, int notify, int sync, const char* message, int value1, int value2);

};

} // end of namespace

#endif
