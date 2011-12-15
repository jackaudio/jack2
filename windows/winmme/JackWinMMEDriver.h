/*
Copyright (C) 2009 Grame
Copyright (C) 2011 Devin Anderson

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

#ifndef __JackWinMMEDriver__
#define __JackWinMMEDriver__

#include "JackMidiDriver.h"
#include "JackWinMMEInputPort.h"
#include "JackWinMMEOutputPort.h"

namespace Jack {

    class JackWinMMEDriver : public JackMidiDriver {

    private:

        JackWinMMEInputPort **input_ports;
        JackWinMMEOutputPort **output_ports;
        UINT period;

    public:

        JackWinMMEDriver(const char* name, const char* alias,
                         JackLockedEngine* engine, JackSynchro* table);

        ~JackWinMMEDriver();

        int
        Attach();

        int
        Close();

        int
        Open(bool capturing, bool playing, int num_inputs, int num_outputs,
             bool monitor, const char* capture_driver_name,
             const char* playback_driver_name, jack_nframes_t capture_latency,
             jack_nframes_t playback_latency);

        int
        Read();

        int
        Start();

        int
        Stop();

        int
        Write();

    };

}

#endif
