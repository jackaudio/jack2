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

#include <sstream>
#include <stdexcept>

#include "JackCoreMidiUtil.h"
#include "JackCoreMidiVirtualInputPort.h"

using Jack::JackCoreMidiVirtualInputPort;

///////////////////////////////////////////////////////////////////////////////
// Static callbacks
///////////////////////////////////////////////////////////////////////////////

void
JackCoreMidiVirtualInputPort::
HandleInputEvent(const MIDIPacketList *packet_list, void *port,
                 void */*src_ref*/)
{
    ((JackCoreMidiVirtualInputPort *) port)->ProcessCoreMidi(packet_list);
}

///////////////////////////////////////////////////////////////////////////////
// Class
///////////////////////////////////////////////////////////////////////////////

JackCoreMidiVirtualInputPort::
JackCoreMidiVirtualInputPort(const char *alias_name, const char *client_name,
                             const char *driver_name, int index,
                             MIDIClientRef client, double time_ratio,
                             size_t max_bytes, size_t max_messages):
    JackCoreMidiInputPort(time_ratio, max_bytes, max_messages)
{
    std::stringstream stream;
    stream << "virtual" << (index + 1);
    CFStringRef name = CFStringCreateWithCString(0, stream.str().c_str(),
                                                CFStringGetSystemEncoding());
    if (! name) {
        throw std::bad_alloc();
    }
    MIDIEndpointRef destination;
    OSStatus status = MIDIDestinationCreate(client, name, HandleInputEvent,
                                            this, &destination);
    CFRelease(name);
    if (status != noErr) {
        throw std::runtime_error(GetMacOSErrorString(status));
    }
    Initialize(alias_name, client_name, driver_name, index, destination);
}

JackCoreMidiVirtualInputPort::~JackCoreMidiVirtualInputPort()
{
    OSStatus status = MIDIEndpointDispose(GetEndpoint());
    if (status != noErr) {
        WriteMacOSError("JackCoreMidiVirtualInputPort [destructor]",
                        "MIDIEndpointDispose", status);
    }
}
