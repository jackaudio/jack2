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

#ifndef __JackLoopbackDriver__
#define __JackLoopbackDriver__

#include "JackAudioDriver.h"
#include "JackTime.h"

namespace Jack
{

/*!
\brief The loopback driver : to be used to "pipeline" applications connected in sequence.
*/

class JackLoopbackDriver : public JackAudioDriver
{

    public:

        JackLoopbackDriver(const char* name, JackEngine* engine, JackSynchro** table)
                : JackAudioDriver(name, engine, table)
        {}
        virtual ~JackLoopbackDriver()
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

        int Process();
        void PrintState();
};

} // end of namespace

#endif
