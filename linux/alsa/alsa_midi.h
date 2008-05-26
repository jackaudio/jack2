/*
 * Copyright (c) 2006 Dmitry S. Baikov
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#ifndef __jack_alsa_midi_h__
#define __jack_alsa_midi_h__

#ifdef __cplusplus
extern "C"
{
#else
#include <stdbool.h>
#endif

    typedef struct alsa_midi_t alsa_midi_t;
    struct alsa_midi_t {
        void (*destroy)(alsa_midi_t *amidi);
        int (*attach)(alsa_midi_t *amidi);
        int (*detach)(alsa_midi_t *amidi);
        int (*start)(alsa_midi_t *amidi);
        int (*stop)(alsa_midi_t *amidi);
        void (*read)(alsa_midi_t *amidi, jack_nframes_t nframes);
        void (*write)(alsa_midi_t *amidi, jack_nframes_t nframes);
    };

    alsa_midi_t* alsa_rawmidi_new(jack_client_t *jack);
    alsa_midi_t* alsa_seqmidi_new(jack_client_t *jack, const char* alsa_name);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __jack_alsa_midi_h__ */
