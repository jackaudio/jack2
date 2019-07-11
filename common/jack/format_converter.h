/*
  Copyright (C) 2019 Laxmi Devi <laxmi.devi@in.bosch.com>
  Copyright (C) 2019 Timo Wischer <twischer@de.adit-jv.com>


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

#ifndef __jack_format_converter_h__
#define __jack_format_converter_h__

#include <typeinfo>

#ifdef __cplusplus
extern "C"
{
#endif

#include <jack/weakmacros.h>


class IJackPortConverter {

    public:

        virtual void* get(jack_nframes_t frames) = 0;
        virtual void set(void* buf, jack_nframes_t frames) = 0;
};

/**
 * This returns a pointer to the instance of the object IJackPortConverter based
 * on the dst_type. Applications can use the get() and set()
 * of this object to get and set the pointers to the memory area associated with the specified port.
 * Currently Jack only supports Float, int32_t and int16_t.
 *
 * @param port jack_port_t pointer.
 * @param dst_type type required by client.
 * @param init_output_silence if true, jack will initialize the output port with silence
 *
 * @return ptr to IJackPortConverter on success, otherwise NULL if dst_type is not supported.
 */

IJackPortConverter* jack_port_create_converter(jack_port_t* port, const std::type_info& dst_type, const bool init_output_silence=true) JACK_OPTIONAL_WEAK_EXPORT;

#ifdef __cplusplus
}
#endif

#endif // __jack_format_converter_h__
