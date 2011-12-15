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

#include <cassert>

#include "JackCoreMidiPort.h"
#include "JackCoreMidiUtil.h"
#include "JackError.h"

using Jack::JackCoreMidiPort;

JackCoreMidiPort::JackCoreMidiPort(double time_ratio)
{
    initialized = false;
    this->time_ratio = time_ratio;
}

JackCoreMidiPort::~JackCoreMidiPort()
{
    // Empty
}

const char *
JackCoreMidiPort::GetAlias()
{
    assert(initialized);
    return alias;
}

MIDIEndpointRef
JackCoreMidiPort::GetEndpoint()
{
    assert(initialized);
    return endpoint;
}

const char *
JackCoreMidiPort::GetName()
{
    assert(initialized);
    return name;
}

void
JackCoreMidiPort::Initialize(const char *alias_name, const char *client_name,
                             const char *driver_name, int index,
                             MIDIEndpointRef endpoint, bool is_output)
{
    char endpoint_name[REAL_JACK_PORT_NAME_SIZE];
    CFStringRef endpoint_name_ref;
    int num = index + 1;
    Boolean res;
    OSStatus result = MIDIObjectGetStringProperty(endpoint, kMIDIPropertyName,
                                                  &endpoint_name_ref);
    if (result != noErr) {
        WriteMacOSError("JackCoreMidiPort::Initialize",
                        "MIDIObjectGetStringProperty", result);
        goto get_basic_alias;
    }
    res = CFStringGetCString(endpoint_name_ref, endpoint_name,
                                sizeof(endpoint_name), 0);
    CFRelease(endpoint_name_ref);
    if (!res) {
        jack_error("JackCoreMidiPort::Initialize - failed to allocate memory "
                   "for endpoint name.");
    get_basic_alias:
        snprintf(alias, sizeof(alias), "%s:%s:%s%d", alias_name,
                 driver_name, is_output ? "in" : "out", num);
    } else {
        snprintf(alias, sizeof(alias), "%s:%s:%s%d", alias_name,
                 endpoint_name, is_output ? "in" : "out", num);
    }
    snprintf(name, sizeof(name), "%s:%s_%d", client_name,
             is_output ? "playback" : "capture", num);
    this->endpoint = endpoint;
    initialized = true;
}
