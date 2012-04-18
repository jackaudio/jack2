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

#ifndef __JackAudioAdapter__
#define __JackAudioAdapter__

#include "JackAudioAdapterInterface.h"
#include "driver_interface.h"

namespace Jack
{

    /*!
    \brief Audio adapter : Jack client side.
    */

    class JackAudioAdapter
    {
        private:

            static int Process(jack_nframes_t, void* arg);
            static int BufferSize(jack_nframes_t buffer_size, void* arg);
            static int SampleRate(jack_nframes_t sample_rate, void* arg);
            static void Latency(jack_latency_callback_mode_t mode, void* arg);

            jack_port_t** fCapturePortList;
            jack_port_t** fPlaybackPortList;

            jack_default_audio_sample_t** fInputBufferList;
            jack_default_audio_sample_t** fOutputBufferList;

            jack_client_t* fClient;
            JackAudioAdapterInterface* fAudioAdapter;
            bool fAutoConnect;

            void FreePorts();
            void ConnectPorts();
            void Reset();
            int ProcessAux(jack_nframes_t frames);

        public:

            JackAudioAdapter(jack_client_t* client, JackAudioAdapterInterface* audio_io, const JSList* params = NULL);
            ~JackAudioAdapter();

            int Open();
            int Close();
    };

}

#define CaptureDriverFlags  static_cast<JackPortFlags>(JackPortIsOutput | JackPortIsPhysical | JackPortIsTerminal)
#define PlaybackDriverFlags static_cast<JackPortFlags>(JackPortIsInput | JackPortIsPhysical | JackPortIsTerminal)

#endif
