/*
Copyright (C) 2009 Grame

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

#ifndef __JackMidiDriver__
#define __JackMidiDriver__

#include "JackDriver.h"
#include "JackMidiPort.h"
#include "JackLockedEngine.h"
#include "ringbuffer.h"

namespace Jack
{

/*!
\brief The base class for MIDI drivers: drivers with MIDI ports.
*/

class SERVER_EXPORT JackMidiDriver : public JackDriver
{

     protected:

        int fCaptureChannels;
        int fPlaybackChannels;
        
        jack_ringbuffer_t* fRingBuffer[DRIVER_PORT_NUM];

        jack_port_id_t fCapturePortList[DRIVER_PORT_NUM];
        jack_port_id_t fPlaybackPortList[DRIVER_PORT_NUM];
   
        JackMidiBuffer* GetInputBuffer(int port_index);
        JackMidiBuffer* GetOutputBuffer(int port_index);
 
    public:

        JackMidiDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table);
        virtual ~JackMidiDriver();

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
        
        virtual int Read();
        virtual int Write();
 
};

} // end of namespace

#endif
