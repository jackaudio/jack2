/*
Copyright (C) 2007 Dmitry Baikov

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

#ifndef __JackPortType__
#define __JackPortType__

#include "types.h"
#include "JackConstants.h"
#include <stddef.h>

namespace Jack
{

struct JackPortType
{
    const char* name;
    void (*init)(void* buffer, size_t buffer_size, jack_nframes_t nframes);
    void (*mixdown)(void *mixbuffer, void** src_buffers, int src_count, jack_nframes_t nframes);
};

extern int GetPortTypeId(const char* port_type);
extern const JackPortType* GetPortType(int port_type_id);

extern const JackPortType gAudioPortType;
extern const JackPortType gMidiPortType;

} // namespace Jack

#endif
