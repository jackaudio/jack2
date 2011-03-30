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
#include "JackCoreMidiVirtualOutputPort.h"

using Jack::JackCoreMidiVirtualOutputPort;

JackCoreMidiVirtualOutputPort::
JackCoreMidiVirtualOutputPort(const char *alias_name, const char *client_name,
                              const char *driver_name, int index,
                              MIDIClientRef client, double time_ratio,
                              size_t max_bytes,
                              size_t max_messages):
    JackCoreMidiOutputPort(time_ratio, max_bytes,
                           max_messages)
{
    std::stringstream stream;
    stream << "virtual" << (index + 1);
    CFStringRef name = CFStringCreateWithCString(0, stream.str().c_str(),
                                                 CFStringGetSystemEncoding());
    if (! name) {
        throw std::bad_alloc();
    }
    MIDIEndpointRef source;
    OSStatus status = MIDISourceCreate(client, name, &source);
    CFRelease(name);
    if (status != noErr) {
        throw std::runtime_error(GetMacOSErrorString(status));
    }
    Initialize(alias_name, client_name, driver_name, index, source, 0);
}

JackCoreMidiVirtualOutputPort::~JackCoreMidiVirtualOutputPort()
{
    OSStatus status = MIDIEndpointDispose(GetEndpoint());
    if (status != noErr) {
        WriteMacOSError("JackCoreMidiVirtualOutputPort [destructor]",
                        "MIDIEndpointDispose", status);
    }
}

bool
JackCoreMidiVirtualOutputPort::SendPacketList(MIDIPacketList *packet_list)
{
    OSStatus status = MIDIReceived(endpoint, packet_list);
    bool result = status == noErr;
    if (! result) {
        WriteMacOSError("JackCoreMidiVirtualOutputPort::SendPacketList",
                        "MIDIReceived", status);
    }
    return result;
}
