/*
Copyright (C) 2001 Paul Davis 
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

#ifndef __JackAudioDriver__
#define __JackAudioDriver__

#include "JackDriver.h"


namespace Jack
{

/*!
\brief The base class for audio drivers: drivers with audio ports.
*/

class EXPORT JackAudioDriver : public JackDriver
{

    protected:

        int fCaptureChannels;
        int fPlaybackChannels;

        // static tables since the actual number of ports may be changed by the real driver
        // thus dynamic allocation is more difficult to handle
        jack_port_id_t fCapturePortList[PORT_NUM];
        jack_port_id_t fPlaybackPortList[PORT_NUM];
        jack_port_id_t fMonitorPortList[PORT_NUM];

        bool fWithMonitorPorts;

        jack_default_audio_sample_t* GetInputBuffer(int port_index);
        jack_default_audio_sample_t* GetOutputBuffer(int port_index);
        jack_default_audio_sample_t* GetMonitorBuffer(int port_index);

    private:

        int ProcessAsync();
        int ProcessSync();

    public:

        JackAudioDriver(const char* name, JackEngine* engine, JackSynchro** table);
        virtual ~JackAudioDriver();

        virtual int Process();

        virtual int Open(jack_nframes_t nframes,
                         jack_nframes_t samplerate,
                         int capturing,
                         int playing,
                         int inchannels,
                         int outchannels,
                         bool monitor,
                         const char* capture_driver_name,
                         const char* playback_driver_name,
                         jack_nframes_t capture_latency,
                         jack_nframes_t playback_latency);

        virtual int Attach();
        virtual int Detach();
        virtual int Write();

        virtual void NotifyXRun(jack_time_t callback_usecs); // XRun notification sent by the driver

};

} // end of namespace

#endif
