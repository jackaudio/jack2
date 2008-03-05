/*
 * Copyright (c) 2006,2007 Dmitry S. Baikov
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

#ifndef __jack_midi_pack_h__
#define __jack_midi_pack_h__

typedef struct
{
    int running_status;
}
midi_pack_t;

static inline
void midi_pack_reset(midi_pack_t *p)
{
    p->running_status = 0;
}

static
void midi_pack_event(midi_pack_t *p, jack_midi_event_t *e)
{
    if (e->buffer[0] >= 0x80 && e->buffer[0] < 0xF0) { // Voice Message
        if (e->buffer[0] == p->running_status) {
            e->buffer++;
            e->size--;
        } else
            p->running_status = e->buffer[0];
    } else if (e->buffer[0] < 0xF8) { // not System Realtime
        p->running_status = 0;
    }
}

#endif /* __jack_midi_pack_h__ */
