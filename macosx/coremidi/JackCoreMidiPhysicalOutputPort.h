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

#ifndef __JackCoreMidiPhysicalOutputPort__
#define __JackCoreMidiPhysicalOutputPort__

#include "JackCoreMidiOutputPort.h"

namespace Jack {

    class JackCoreMidiPhysicalOutputPort: public JackCoreMidiOutputPort {

    private:

        MIDIPortRef internal_output;

    protected:

        bool
        SendPacketList(MIDIPacketList *packet_list);

    public:

        JackCoreMidiPhysicalOutputPort(const char *alias_name,
                                       const char *client_name,
                                       const char *driver_name, int index,
                                       MIDIClientRef client,
                                       MIDIPortRef internal_output,
                                       double time_ratio,
                                       size_t max_bytes=4096,
                                       size_t max_messages=1024);

        ~JackCoreMidiPhysicalOutputPort();

    };

}

#endif
