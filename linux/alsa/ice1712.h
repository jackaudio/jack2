/*
    Copyright (C) 2002 Anthony Van Groningen

    Parts based on source code taken from the
    "Env24 chipset (ICE1712) control utility" that is

    Copyright (C) 2000 by Jaroslav Kysela <perex@suse.cz>

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

#ifndef __jack_ice1712_h__
#define __jack_ice1712_h__

#define ICE1712_SUBDEVICE_DELTA44       0x121433d6
#define ICE1712_SUBDEVICE_DELTA66       0x121432d6
#define ICE1712_SUBDEVICE_DELTA1010     0x121430d6
#define ICE1712_SUBDEVICE_DELTADIO2496  0x121431d6
#define ICE1712_SUBDEVICE_AUDIOPHILE    0x121434d6

#define SPDIF_PLAYBACK_ROUTE_NAME       "IEC958 Playback Route"
#define ANALOG_PLAYBACK_ROUTE_NAME      "H/W Playback Route"
#define MULTITRACK_PEAK_NAME            "Multi Track Peak"

typedef struct
{
    unsigned int subvendor; /* PCI[2c-2f] */
    unsigned char size;     /* size of EEPROM image in bytes */
    unsigned char version;  /* must be 1 */
    unsigned char codec;    /* codec configuration PCI[60] */
    unsigned char aclink;   /* ACLink configuration PCI[61] */
    unsigned char i2sID;    /* PCI[62] */
    unsigned char spdif;    /* S/PDIF configuration PCI[63] */
    unsigned char gpiomask; /* GPIO initial mask, 0 = write, 1 = don't */
    unsigned char gpiostate; /* GPIO initial state */
    unsigned char gpiodir;  /* GPIO direction state */
    unsigned short ac97main;
    unsigned short ac97pcm;
    unsigned short ac97rec;
    unsigned char ac97recsrc;
    unsigned char dacID[4]; /* I2S IDs for DACs */
    unsigned char adcID[4]; /* I2S IDs for ADCs */
    unsigned char extra[4];
}
ice1712_eeprom_t;

typedef struct
{
    alsa_driver_t *driver;
    ice1712_eeprom_t *eeprom;
    unsigned long active_channels;
}
ice1712_t;

#ifdef __cplusplus
extern "C"
{
#endif

    jack_hardware_t *jack_alsa_ice1712_hw_new (alsa_driver_t *driver);

#ifdef __cplusplus
}
#endif

#endif /* __jack_ice1712_h__*/
