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

#ifndef __JackCoreMidiDriver__
#define __JackCoreMidiDriver__

#include <CoreMIDI/CoreMIDI.h>
#include "JackMidiDriver.h"
#include "JackTime.h"

namespace Jack
{

/*!
\brief The CoreMidi driver.
*/

class JackCoreMidiDriver : public JackMidiDriver
{

    private:

        MIDIClientRef fMidiClient;
        MIDIPortRef fInputPort;
        MIDIPortRef fOutputPort;
        MIDIEndpointRef* fMidiDestination;
        MIDIEndpointRef* fMidiSource;

        char fMIDIBuffer[BUFFER_SIZE_MAX * sizeof(jack_default_audio_sample_t)];

        int fRealCaptureChannels;
        int fRealPlaybackChannels;

        static void ReadProcAux(const MIDIPacketList *pktlist, jack_ringbuffer_t* ringbuffer);
        static void ReadProc(const MIDIPacketList *pktlist, void *refCon, void *connRefCon);
        static void ReadVirtualProc(const MIDIPacketList *pktlist, void *refCon, void *connRefCon);
        static void NotifyProc(const MIDINotification *message, void *refCon);

    public:

        JackCoreMidiDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table);
        virtual ~JackCoreMidiDriver();

        int Open( bool capturing,
                 bool playing,
                 int chan_in,
                 int chan_out,
                 bool monitor,
                 const char* capture_driver_name,
                 const char* playback_driver_name,
                 jack_nframes_t capture_latency,
                 jack_nframes_t playback_latency);
        int Close();

        int Attach();

        int Read();
        int Write();

};

} // end of namespace

#endif
