/*
Copyright (C) 2007 Dmitry Baikov

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#include "JackPortType.h"
#include <string.h>
#include <assert.h>

namespace Jack
{

static const JackPortType* gPortTypes[] =
{
    &gAudioPortType,
    &gMidiPortType,
};

jack_port_type_id_t PORT_TYPES_MAX = sizeof(gPortTypes) / sizeof(gPortTypes[0]);

jack_port_type_id_t GetPortTypeId(const char* port_type)
{
    for (jack_port_type_id_t i = 0; i < PORT_TYPES_MAX; ++i) {
        const JackPortType* type = gPortTypes[i];
        assert(type != 0);
        if (strcmp(port_type, type->fName) == 0)
            return i;
    }
    return PORT_TYPES_MAX;
}

const JackPortType* GetPortType(jack_port_type_id_t type_id)
{
    assert(type_id >= 0 && type_id <= PORT_TYPES_MAX);
    const JackPortType* type = gPortTypes[type_id];
    assert(type != 0);
    return type;
}

} // namespace Jack
