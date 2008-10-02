/*
 * Copyright (c) 2007 Dmitry S. Baikov
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

#ifndef __jack_alsa_midi_impl_h__
#define __jack_alsa_midi_impl_h__

#include "JackConstants.h"

#ifdef JACKMP

#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    int JACK_is_realtime(jack_client_t *client);
    int JACK_client_create_thread(jack_client_t *client, pthread_t *thread, int priority, int realtime, void *(*start_routine)(void*), void *arg);

    jack_port_t* JACK_port_register(jack_client_t *client, const char *port_name, const char *port_type, unsigned long flags, unsigned long buffer_size);
    int JACK_port_unregister(jack_client_t *, jack_port_t*);
    void* JACK_port_get_buffer(jack_port_t*, jack_nframes_t);
    int JACK_port_set_alias(jack_port_t* port, const char* name);

    jack_nframes_t JACK_get_sample_rate(jack_client_t *);
    jack_nframes_t JACK_frame_time(jack_client_t *);
    jack_nframes_t JACK_last_frame_time(jack_client_t *);

#define jack_is_realtime JACK_is_realtime
#define jack_client_create_thread JACK_client_create_thread

#define jack_port_register JACK_port_register
#define jack_port_unregister JACK_port_unregister
#define jack_port_get_buffer JACK_port_get_buffer
#define jack_port_set_alias JACK_port_set_alias

#define jack_get_sample_rate JACK_get_sample_rate
#define jack_frame_time JACK_frame_time
#define jack_last_frame_time JACK_last_frame_time

#ifdef __cplusplus
} // extern "C"
#endif

#else // usual jack

#include "jack.h"
#include "thread.h"

#endif

#if defined(STANDALONE)
#define MESSAGE(...) fprintf(stderr, __VA_ARGS__)
#elif !defined(JACKMP)
#include <jack/messagebuffer.h>
#endif

#define info_log(...)  jack_info(__VA_ARGS__)
#define error_log(...) jack_error(__VA_ARGS__)

#ifdef ALSA_MIDI_DEBUG
#define debug_log(...) jack_info(__VA_ARGS__)
#else
#define debug_log(...)
#endif

#include "alsa_midi.h"

#endif /* __jack_alsa_midi_impl_h__ */
