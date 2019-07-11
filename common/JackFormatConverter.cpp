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

#include <cstring>
#include <jack/jack.h>
#include <jack/format_converter.h>
#include "JackCompilerDeps.h"
#include "JackError.h"
#include "memops.h"


class BaseJackPortConverter : public IJackPortConverter {
    protected:
        jack_port_t* const port;
    public:
        BaseJackPortConverter(jack_port_t* pt) : port(pt) {}
};

class ForwardJackPortConverter : public BaseJackPortConverter {
    private:
        void* buffer = NULL;
    public:
        ForwardJackPortConverter(jack_port_t* pt) : BaseJackPortConverter(pt) {}

        virtual void* get( jack_nframes_t frames) {
            buffer = jack_port_get_buffer(port, frames);
            return buffer;
        }

        virtual void set(void* buf,  jack_nframes_t frames) {
            if (buf == buffer)
                return;
            std::memcpy(jack_port_get_buffer(port, frames), buf, frames*sizeof(jack_default_audio_sample_t));
        }
};

class IntegerJackPortConverter : public BaseJackPortConverter {
    typedef void (*ReadCopyFunction)  (jack_default_audio_sample_t *dst, char *src,
                                       unsigned long src_bytes,
                                       unsigned long src_skip_bytes);
    typedef void (*WriteCopyFunction) (char *dst, jack_default_audio_sample_t *src,
                                       unsigned long src_bytes,
                                       unsigned long dst_skip_bytes,
                                       dither_state_t *state);

    private:
        int32_t buffer[BUFFER_SIZE_MAX + 8];
        const ReadCopyFunction to_jack;
        const WriteCopyFunction from_jack;
        const size_t sample_size;

        int32_t* GetBuffer()
        {
            return (int32_t*)((uintptr_t)buffer & ~31L) + 8;
        }

    public:
        IntegerJackPortConverter(const ReadCopyFunction to_jack,
                                 const WriteCopyFunction from_jack,
                                 const size_t sample_size,
                                 jack_port_t* pt) : BaseJackPortConverter(pt),
        to_jack(to_jack), from_jack(from_jack), sample_size(sample_size) {}

        virtual void* get(jack_nframes_t frames) {
            int32_t * aligned_ptr = GetBuffer();
            jack_default_audio_sample_t* src = (jack_default_audio_sample_t*) jack_port_get_buffer(port, frames);
            /* error is already thrown in jack_port_get_buffer() */
            if (src == NULL)
                return NULL;
            from_jack ((char *)aligned_ptr, src, frames, sample_size, NULL);
            return aligned_ptr;
        }

        virtual void set(void* src, jack_nframes_t frames) {
            jack_default_audio_sample_t* dst = (jack_default_audio_sample_t*) jack_port_get_buffer(port, frames);
            /* error is already thrown in jack_port_get_buffer() */
            if (dst == NULL)
                return;
            to_jack (dst,(char *) src, frames, sample_size);
        }
};

LIB_EXPORT IJackPortConverter* jack_port_create_converter(jack_port_t* port, const std::type_info& dst_type, const bool init_output_silence)
{
    if(dst_type == (typeid(jack_default_audio_sample_t))) {
        return new ForwardJackPortConverter(port);
    }
    else if(dst_type == (typeid(int32_t))) {
        return new IntegerJackPortConverter(sample_move_dS_s32,
                                            sample_move_d32_sS, sizeof(int32_t),
                                            port);
    }
    else if(dst_type == (typeid(int16_t))) {
        return new IntegerJackPortConverter(sample_move_dS_s16,
                                            sample_move_d16_sS,  sizeof(int16_t),
                                            port);
    }
    else {
        jack_error("jack_port_create_converter called with dst_type that is not supported");
        return NULL;
    }
}
