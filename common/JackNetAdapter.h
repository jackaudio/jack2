/*
Copyright (C) 2008-2011 Romain Moret at Grame

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

#ifndef __JackNetAdapter__
#define __JackNetAdapter__

#include "JackAudioAdapterInterface.h"
#include "JackNetInterface.h"

namespace Jack
{

    /*!
    \brief Net adapter.
    */

    class JackNetAdapter : public JackAudioAdapterInterface,
        public JackNetSlaveInterface,
        public JackRunnableInterface
    {

        private:

            //jack data
            jack_client_t* fClient;

            //transport data
            int fLastTransportState;
            int fLastTimebaseMaster;

            //sample buffers
            sample_t** fSoftCaptureBuffer;
            sample_t** fSoftPlaybackBuffer;

            //adapter thread
            JackThread fThread;

            //transport
            void EncodeTransportData();
            void DecodeTransportData();

        public:

            JackNetAdapter(jack_client_t* jack_client, jack_nframes_t buffer_size, jack_nframes_t sample_rate, const JSList* params);
            ~JackNetAdapter();

            int Open();
            int Close();

            int SetBufferSize(jack_nframes_t buffer_size);

            bool Init();
            bool Execute();

            int Read();
            int Write();

            int Process();
    };
}

#endif
