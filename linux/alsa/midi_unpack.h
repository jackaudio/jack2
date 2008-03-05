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

#ifndef __jack_midi_unpack_h__
#define __jack_midi_unpack_h__

enum {
    MIDI_UNPACK_MAX_MSG = 1024
};

typedef struct
{
    int pos, need, size;
    unsigned char data[MIDI_UNPACK_MAX_MSG];
}
midi_unpack_t;

static inline
void midi_unpack_init(midi_unpack_t *u)
{
    u->pos = 0;
    u->size = sizeof(u->data);
    u->need = u->size;
}

static inline
void midi_unpack_reset(midi_unpack_t *u)
{
    u->pos = 0;
    u->need = u->size;
}

static const unsigned char midi_voice_len[] =
    {
        3, /*0x80 Note Off*/
        3, /*0x90 Note On*/
        3, /*0xA0 Aftertouch*/
        3, /*0xB0 Control Change*/
        2, /*0xC0 Program Change*/
        2, /*0xD0 Channel Pressure*/
        3, /*0xE0 Pitch Wheel*/
        1  /*0xF0 System*/
    };

static const unsigned char midi_system_len[] =
    {
        0, /*0xF0 System Exclusive Start*/
        2, /*0xF1 MTC Quarter Frame*/
        3, /*0xF2 Song Postion*/
        2, /*0xF3 Song Select*/
        0, /*0xF4 undefined*/
        0, /*0xF5 undefined*/
        1, /*0xF6 Tune Request*/
        1  /*0xF7 System Exlusive End*/
    };

static
int midi_unpack_buf(midi_unpack_t *buf, const unsigned char *data, int len, void *jack_port_buf, jack_nframes_t time)
{
    int i;
    for (i = 0; i < len; ++i) {
        const unsigned char byte = data[i];
        if (byte >= 0xF8) // system realtime
        {
            jack_midi_event_write(jack_port_buf, time, &data[i], 1);
            //printf("midi_unpack: written system relatime event\n");
            //midi_input_write(in, &data[i], 1);
        } else if (byte < 0x80) // data
        {
            assert (buf->pos < buf->size);
            buf->data[buf->pos++] = byte;
        } else if (byte < 0xF0) // voice
        {
            assert (byte >= 0x80 && byte < 0xF0);
            //buf->need = ((byte|0x0F) == 0xCF || (byte|0x0F)==0xDF) ? 2 : 3;
            buf->need = midi_voice_len[(byte-0x80)>>4];
            buf->data[0] = byte;
            buf->pos = 1;
        } else if (byte == 0xF7) // sysex end
        {
            assert (buf->pos < buf->size);
            buf->data[buf->pos++] = byte;
            buf->need = buf->pos;
        } else {
            assert (byte >= 0xF0 && byte < 0xF8);
            buf->pos = 1;
            buf->data[0] = byte;
            buf->need = midi_system_len[byte - 0xF0];
            if (!buf->need)
                buf->need = buf->size;
        }
        if (buf->pos == buf->need) {
            // TODO: deal with big sysex'es (they are silently dropped for now)
            if (buf->data[0] >= 0x80 || (buf->data[0] == 0xF0 && buf->data[buf->pos-1] == 0xF7)) {
                /* convert Note On with velocity 0 to Note Off */
                if ((buf->data[0] & 0xF0) == 0x90 && buf->data[2] == 0) {
                    // we use temp array here to keep running status sync
                    jack_midi_data_t temp[3] = { 0x80, 0, 0x40 };
                    temp[0] |= buf->data[0] & 0x0F;
                    temp[1] = buf->data[1];
                    jack_midi_event_write(jack_port_buf, time, temp, 3);
                } else
                    jack_midi_event_write(jack_port_buf, time, &buf->data[0], buf->pos);
                //printf("midi_unpack: written %d-byte event\n", buf->pos);
                //midi_input_write(in, &buf->data[0], buf->pos);
            }
            /* keep running status */
            if (buf->data[0] >= 0x80 && buf->data[0] < 0xF0)
                buf->pos = 1;
            else {
                buf->pos = 0;
                buf->need = buf->size;
            }
        }
    }
    assert (i == len);
    return i;
}

#endif /* __jack_midi_unpack_h__ */
