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

#ifndef __JackPortType__
#define __JackPortType__

#include "types.h"
#include "JackConstants.h"
#include <stddef.h>

namespace Jack
{

extern jack_port_type_id_t PORT_TYPES_MAX;

struct JackPortType
{
    const char* fName;
    size_t (*size)();
    void (*init)(void* buffer, size_t buffer_size, jack_nframes_t nframes);
    void (*mixdown)(void *mixbuffer, void** src_buffers, int src_count, jack_nframes_t nframes);
};

extern jack_port_type_id_t GetPortTypeId(const char* port_type);
extern const struct JackPortType* GetPortType(jack_port_type_id_t port_type_id);

extern const struct JackPortType gAudioPortType;
extern const struct JackPortType gMidiPortType;

} // namespace Jack

#endif
