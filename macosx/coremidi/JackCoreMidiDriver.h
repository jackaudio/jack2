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

#ifndef __JackCoreMidiDriver__
#define __JackCoreMidiDriver__

#include "JackCoreMidiPhysicalInputPort.h"
#include "JackCoreMidiPhysicalOutputPort.h"
#include "JackCoreMidiVirtualInputPort.h"
#include "JackCoreMidiVirtualOutputPort.h"
#include "JackMidiDriver.h"
#include "JackThread.h"

namespace Jack {

    class JackCoreMidiDriver: public JackMidiDriver, public JackRunnableInterface, public JackLockAble {

    private:

        static void
        HandleInputEvent(const MIDIPacketList *packet_list, void *driver,
                         void *port);

        static void
        HandleNotificationEvent(const MIDINotification *message, void *driver);

        void
        HandleNotification(const MIDINotification *message);

        MIDIClientRef client;
        MIDIPortRef internal_input;
        MIDIPortRef internal_output;
        int num_physical_inputs;
        int num_physical_outputs;
        int num_virtual_inputs;
        int num_virtual_outputs;
        JackCoreMidiPhysicalInputPort **physical_input_ports;
        JackCoreMidiPhysicalOutputPort **physical_output_ports;
        double time_ratio;
        JackCoreMidiVirtualInputPort **virtual_input_ports;
        JackCoreMidiVirtualOutputPort **virtual_output_ports;

        bool OpenAux();
        int CloseAux();

        void Restart();

        JackThread fThread;    /*! Thread to execute the Process function */

    public:

        JackCoreMidiDriver(const char* name, const char* alias,
                           JackLockedEngine* engine, JackSynchro* table);

        ~JackCoreMidiDriver();

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

        int ProcessRead();
        int ProcessWrite();

        // JackRunnableInterface interface
        bool Init();
        bool Execute();

    };

}

#define WAIT_COUNTER 100

#endif
