/*
    Copyright (C) 2001 Paul Davis
    Copyright (C) 2004 Karsten Wiese, Rui Nuno Capela

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

    $Id: usx2y.h 855 2004-12-28 05:50:18Z joq $
*/

#ifndef __jack_usx2y_h__
#define __jack_usx2y_h__

#define USX2Y_MAXPACK		50
#define USX2Y_MAXBUFFERMS	100
#define USX2Y_MAXSTRIDE	3

#define USX2Y_SSS (((USX2Y_MAXPACK * USX2Y_MAXBUFFERMS * USX2Y_MAXSTRIDE + 4096) / 4096) * 4096)

struct snd_usX2Y_hwdep_pcm_shm
{
    char playback[USX2Y_SSS];
    char capture0x8[USX2Y_SSS];
    char capture0xA[USX2Y_SSS];
    volatile int playback_iso_head;
    int playback_iso_start;
    struct
    {
        int	frame,
        offset,
        length;
    }
    captured_iso[128];
    volatile int captured_iso_head;
    volatile unsigned captured_iso_frames;
    int capture_iso_start;
};
typedef struct snd_usX2Y_hwdep_pcm_shm snd_usX2Y_hwdep_pcm_shm_t;

typedef struct
{
    alsa_driver_t *driver;
    snd_hwdep_t *hwdep_handle;
    struct pollfd pfds;
    struct snd_usX2Y_hwdep_pcm_shm *hwdep_pcm_shm;
    int playback_iso_start;
    int playback_iso_bytes_done;
    int capture_iso_start;
    int capture_iso_bytes_done;
}
usx2y_t;

jack_hardware_t *
jack_alsa_usx2y_hw_new (alsa_driver_t *driver);

#endif /* __jack_usx2y_h__*/
