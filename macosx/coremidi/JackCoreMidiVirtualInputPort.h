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

#ifndef __JackCoreMidiVirtualInputPort__
#define __JackCoreMidiVirtualInputPort__

#include "JackCoreMidiInputPort.h"

namespace Jack {

    class JackCoreMidiVirtualInputPort: public JackCoreMidiInputPort {

    private:

        static void
        HandleInputEvent(const MIDIPacketList *packet_list, void *port,
                         void *src_ref);

    public:

        JackCoreMidiVirtualInputPort(const char *alias_name,
                                     const char *client_name,
                                     const char *driver_name, int index,
                                     MIDIClientRef client, double time_ratio,
                                     size_t max_bytes=4096,
                                     size_t max_messages=1024);

        ~JackCoreMidiVirtualInputPort();

    };

}

#endif
