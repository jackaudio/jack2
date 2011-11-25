/*
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

#ifndef __JackCoreMidiPort__
#define __JackCoreMidiPort__

#include <CoreMIDI/CoreMIDI.h>

#include "JackConstants.h"

namespace Jack {

    class JackCoreMidiPort {

    private:

        char alias[REAL_JACK_PORT_NAME_SIZE];
        bool initialized;
        char name[REAL_JACK_PORT_NAME_SIZE];

    protected:

        MIDIEndpointRef
        GetEndpoint();

        void
        Initialize(const char *alias_name, const char *client_name,
                   const char *driver_name, int index,
                   MIDIEndpointRef endpoint, bool is_output);

        double time_ratio;
        MIDIEndpointRef endpoint;

    public:

        JackCoreMidiPort(double time_ratio);

        virtual
        ~JackCoreMidiPort();

        const char *
        GetAlias();

        const char *
        GetName();

    };

}

#endif
