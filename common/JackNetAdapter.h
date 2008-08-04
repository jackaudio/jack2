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

#ifndef __JackNetAdapter__
#define __JackNetAdapter__

#include <math.h>
#include <limits.h>
#include <assert.h>
#include "JackAudioAdapterInterface.h"
#include "JackPlatformThread.h"
#include "jack.h"
#include "jslist.h"
#include "JackNetSlaveInterface.h"

namespace Jack
{

    /*!
    \brief Net adapter.
    */

    class JackNetAdapter : public JackAudioAdapterInterface, public JackNetSlaveInterface, public JackRunnableInterface
    {

    private:
		//jack data
        net_transport_data_t fTransportData;

        //sample buffers
        sample_t** fSoftCaptureBuffer;
        sample_t** fSoftPlaybackBuffer;

        int SetSyncPacket();
        int TransportSync();

        bool ProcessSync();
        bool ProcessAsync();

        JackThread fThread;

    public:

        JackNetAdapter ( jack_nframes_t buffer_size, jack_nframes_t sample_rate, const JSList* params );
        ~JackNetAdapter();

        virtual int Open();
        virtual int Close();

        virtual int SetBufferSize ( jack_nframes_t buffer_size );

        virtual bool Init();
        virtual bool Execute();
    };
}

#endif
