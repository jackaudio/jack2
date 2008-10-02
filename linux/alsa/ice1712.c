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

#include "hardware.h"
#include "alsa_driver.h"
#include "ice1712.h"
#include "JackError.h"

static int
ice1712_hw_monitor_toggle(jack_hardware_t *hw, int idx, int onoff)
{
        ice1712_t *h = (ice1712_t *) hw->private_hw;
	snd_ctl_elem_value_t *val;
	int err;
	
	snd_ctl_elem_value_alloca (&val);
	snd_ctl_elem_value_set_interface (val, SND_CTL_ELEM_IFACE_MIXER);
	if (idx >= 8) {
		snd_ctl_elem_value_set_name (val, SPDIF_PLAYBACK_ROUTE_NAME);
		snd_ctl_elem_value_set_index (val, idx - 8);
	} else {
		snd_ctl_elem_value_set_name (val, ANALOG_PLAYBACK_ROUTE_NAME);
		snd_ctl_elem_value_set_index (val, idx);
	}
	if (onoff) {
		snd_ctl_elem_value_set_enumerated (val, 0, idx + 1);
	} else {
		snd_ctl_elem_value_set_enumerated (val, 0, 0);
	}
	if ((err = snd_ctl_elem_write (h->driver->ctl_handle, val)) != 0) {
		jack_error ("ALSA/ICE1712: (%d) cannot set input monitoring (%s)",
			    idx,snd_strerror (err));
		return -1;
	}

	return 0;
}

static int 
ice1712_set_input_monitor_mask (jack_hardware_t *hw, unsigned long mask)    
{
	int idx;
	ice1712_t *h = (ice1712_t *) hw->private_hw;
	
	for (idx = 0; idx < 10; idx++) {
		if (h->active_channels & (1<<idx)) {
			ice1712_hw_monitor_toggle (hw, idx, mask & (1<<idx) ? 1 : 0);
		}
	}
	hw->input_monitor_mask = mask;
	
	return 0;
}

static int 
ice1712_change_sample_clock (jack_hardware_t *hw, SampleClockMode mode)      
{
	return -1;
}

static void
ice1712_release (jack_hardware_t *hw)
{
	ice1712_t *h = (ice1712_t *) hw->private_hw;
	
	if (h == 0)
	return;

	if (h->eeprom)
		free(h->eeprom);

	free(h);
}


jack_hardware_t *
jack_alsa_ice1712_hw_new (alsa_driver_t *driver)
{
	jack_hardware_t *hw;
	ice1712_t *h;
	snd_ctl_elem_value_t *val;	
	int err;

	hw = (jack_hardware_t *) malloc (sizeof (jack_hardware_t));

	hw->capabilities = Cap_HardwareMonitoring;
	hw->input_monitor_mask = 0;
	hw->private_hw = 0;

	hw->set_input_monitor_mask = ice1712_set_input_monitor_mask;
	hw->change_sample_clock = ice1712_change_sample_clock;
	hw->release = ice1712_release;

	h = (ice1712_t *) malloc (sizeof (ice1712_t));

	h->driver = driver;

	/* Get the EEPROM (adopted from envy24control) */
	h->eeprom = (ice1712_eeprom_t *) malloc (sizeof (ice1712_eeprom_t));
	snd_ctl_elem_value_alloca (&val);
	snd_ctl_elem_value_set_interface (val, SND_CTL_ELEM_IFACE_CARD);
        snd_ctl_elem_value_set_name (val, "ICE1712 EEPROM");
        if ((err = snd_ctl_elem_read (driver->ctl_handle, val)) < 0) {
                jack_error( "ALSA/ICE1712: Unable to read EEPROM contents (%s)\n", snd_strerror (err));
                /* Recover? */
        }
        memcpy(h->eeprom, snd_ctl_elem_value_get_bytes(val), 32);

	/* determine number of pro ADC's. We're asumming that there is at least one stereo pair. 
	   Should check this first, but how?  */
	switch((h->eeprom->codec & 0xCU) >> 2) {
	case 0:
	        h->active_channels = 0x3U;
	        break;
	case 1:
	        h->active_channels = 0xfU;
	        break;
	case 2:
	        h->active_channels = 0x3fU;
	        break;
	case 3:
	        h->active_channels = 0xffU;
	        break;
	}
	/* check for SPDIF In's */
	if (h->eeprom->spdif & 0x1U) {
	        h->active_channels |= 0x300U;
	}
	
	hw->private_hw = h;

	return hw;
}
