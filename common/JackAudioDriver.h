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

A concrete derived class will have to be defined with a real audio driver API, 
either callback based one (like CoreAudio, PortAudio..) ones or blocking ones (like ALSA).

Most of the generic audio handing code is part of this class :
    - concrete callback basedd derived subclasses typically have to Open/Close the underlying audio API, 
        setup the audio callback and implement the Read/Write methods
    - concrete blocking based derived subclasses typically have to Open/Close the underlying audio API, 
        implement the Read/Write methods and "wraps" the driver with the JackThreadDriver class.
*/

class SERVER_EXPORT JackAudioDriver : public JackDriver
{

    protected:

        jack_default_audio_sample_t* GetInputBuffer(int port_index);
        jack_default_audio_sample_t* GetOutputBuffer(int port_index);
        jack_default_audio_sample_t* GetMonitorBuffer(int port_index);

        void HandleLatencyCallback(int status);
        virtual void UpdateLatencies();

        int ProcessAsync();
        void ProcessGraphAsync();
        void ProcessGraphAsyncMaster();
        void ProcessGraphAsyncSlave();

        int ProcessSync();
        void ProcessGraphSync();
        void ProcessGraphSyncMaster();
        void ProcessGraphSyncSlave();

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

        /*
            To be called by the underlying driver audio callback, or possibly by a RT thread (using JackThreadedDriver decorator) 
            when a blocking read/write underlying API is used (like ALSA)
        */
        virtual int Process();

        virtual int Attach();
        virtual int Detach();

        virtual int Write();

        virtual int SetBufferSize(jack_nframes_t buffer_size);
        virtual int SetSampleRate(jack_nframes_t sample_rate);

        virtual int ClientNotify(int refnum, const char* name, int notify, int sync, const char* message, int value1, int value2);

};

} // end of namespace

#endif
