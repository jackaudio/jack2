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

#include "JackCoreMidiPhysicalInputPort.h"
#include "JackCoreMidiUtil.h"

using Jack::JackCoreMidiPhysicalInputPort;

JackCoreMidiPhysicalInputPort::
JackCoreMidiPhysicalInputPort(const char *alias_name, const char *client_name,
                              const char *driver_name, int index,
                              MIDIClientRef client, MIDIPortRef internal_input,
                              double time_ratio, size_t max_bytes,
                              size_t max_messages):
    JackCoreMidiInputPort(time_ratio, max_bytes, max_messages)
{
    MIDIEndpointRef source = MIDIGetSource(index);
    if (! source) {
        // X: Is there a way to get a better error message?
        std::stringstream stream;
        stream << "The source at index '" << index << "' is not available";
        throw std::runtime_error(stream.str().c_str());
    }
    OSStatus status = MIDIPortConnectSource(internal_input, source, this);
    if (status != noErr) {
        throw std::runtime_error(GetMacOSErrorString(status));
    }
    Initialize(alias_name, client_name, driver_name, index, source);
}

JackCoreMidiPhysicalInputPort::~JackCoreMidiPhysicalInputPort()
{
    // Empty
}
