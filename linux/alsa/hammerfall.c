/*
    Copyright (C) 2001 Paul Davis 

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

    $Id: hammerfall.c,v 1.3 2005/09/29 14:51:59 letz Exp $
*/

#include "hardware.h"
#include "alsa_driver.h"
#include "hammerfall.h"
#include "JackError.h"

#define FALSE 0
#define TRUE 1

/* Set this to 1 if you want this compile error:
 *   warning: `hammerfall_monitor_controls' defined but not used */
#define HAMMERFALL_MONITOR_CONTROLS 0

static void 
set_control_id (snd_ctl_elem_id_t *ctl, const char *name)
{
	snd_ctl_elem_id_set_name (ctl, name);
	snd_ctl_elem_id_set_numid (ctl, 0);
	snd_ctl_elem_id_set_interface (ctl, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_device (ctl, 0);
	snd_ctl_elem_id_set_subdevice (ctl, 0);
	snd_ctl_elem_id_set_index (ctl, 0);
}

#if HAMMERFALL_MONITOR_CONTROLS
static void
hammerfall_broadcast_channel_status_change (hammerfall_t *h, int lock, int sync, channel_t lowchn, channel_t highchn)

{
	channel_t chn;
	ClockSyncStatus status = 0;

	if (lock) {
		status |= Lock;
	} else {
		status |= NoLock;
	}

	if (sync) {
		status |= Sync;
	} else {
		status |= NoSync;
	}

	for (chn = lowchn; chn < highchn; chn++) {
		alsa_driver_set_clock_sync_status (h->driver, chn, status);
	}
}

static void
hammerfall_check_sync_state (hammerfall_t *h, int val, int adat_id)

{
	int lock;
	int sync;

	/* S/PDIF channel is always locked and synced, but we only
	   need tell people once that this is TRUE.

	   XXX - maybe need to make sure that the rate matches our
	   idea of the current rate ?
	*/

	if (!h->said_that_spdif_is_fine) {
		ClockSyncStatus status;
		
		status = Lock|Sync;

		/* XXX broken! fix for hammerfall light ! */

		alsa_driver_set_clock_sync_status (h->driver, 24, status);
		alsa_driver_set_clock_sync_status (h->driver, 25, status);

		h->said_that_spdif_is_fine = TRUE;
	}

	lock = (val & 0x1) ? TRUE : FALSE;
	sync = (val & 0x2) ? TRUE : FALSE;
	
	if (h->lock_status[adat_id] != lock ||
	    h->sync_status[adat_id] != sync) {
		hammerfall_broadcast_channel_status_change (h, lock, sync, adat_id*8, (adat_id*8)+8);
	}

	h->lock_status[adat_id] = lock;
	h->sync_status[adat_id] = sync;
}

static void
hammerfall_check_sync (hammerfall_t *h, snd_ctl_elem_value_t *ctl)

{
	const char *name;
	int val;
	snd_ctl_elem_id_t *ctl_id;
	
	jack_info ("check sync");

	snd_ctl_elem_id_alloca (&ctl_id);
	snd_ctl_elem_value_get_id (ctl, ctl_id);

	name = snd_ctl_elem_id_get_name (ctl_id);

	if (strcmp (name, "ADAT1 Sync Check") == 0) {
		val = snd_ctl_elem_value_get_enumerated (ctl, 0);
		hammerfall_check_sync_state (h, val, 0);
	} else if (strcmp (name, "ADAT2 Sync Check") == 0) {
		val = snd_ctl_elem_value_get_enumerated (ctl, 0);
		hammerfall_check_sync_state (h, val, 1);
	} else if (strcmp (name, "ADAT3 Sync Check") == 0) {
		val = snd_ctl_elem_value_get_enumerated (ctl, 0);
		hammerfall_check_sync_state (h, val, 2);
	} else {
		jack_error ("Hammerfall: unknown control \"%s\"", name);
	}
}
#endif /* HAMMERFALL_MONITOR_CONTROLS */

static int 
hammerfall_set_input_monitor_mask (jack_hardware_t *hw, unsigned long mask)
{
	hammerfall_t *h = (hammerfall_t *) hw->private_hw;
	snd_ctl_elem_value_t *ctl;
	snd_ctl_elem_id_t *ctl_id;
	int err;
	int i;
	
	snd_ctl_elem_value_alloca (&ctl);
	snd_ctl_elem_id_alloca (&ctl_id);
	set_control_id (ctl_id, "Channels Thru");
	snd_ctl_elem_value_set_id (ctl, ctl_id);
	
	for (i = 0; i < 26; i++) {
		snd_ctl_elem_value_set_integer (ctl, i, (mask & (1<<i)) ? 1 : 0);
	}
	
	if ((err = snd_ctl_elem_write (h->driver->ctl_handle, ctl)) != 0) {
		jack_error ("ALSA/Hammerfall: cannot set input monitoring (%s)", snd_strerror (err));
		return -1;
	}
	
	hw->input_monitor_mask = mask;

	return 0;
}

static int 
hammerfall_change_sample_clock (jack_hardware_t *hw, SampleClockMode mode) 
{
	hammerfall_t *h = (hammerfall_t *) hw->private_hw;
	snd_ctl_elem_value_t *ctl;
	snd_ctl_elem_id_t *ctl_id;
	int err;

	snd_ctl_elem_value_alloca (&ctl);
	snd_ctl_elem_id_alloca (&ctl_id);
	set_control_id (ctl_id, "Sync Mode");
	snd_ctl_elem_value_set_id (ctl, ctl_id);

	switch (mode) {
	case AutoSync:
		snd_ctl_elem_value_set_enumerated (ctl, 0, 0);
		break;
	case ClockMaster:
		snd_ctl_elem_value_set_enumerated (ctl, 0, 1);
		break;
	case WordClock:
		snd_ctl_elem_value_set_enumerated (ctl, 0, 2);
		break;
	}

	if ((err = snd_ctl_elem_write (h->driver->ctl_handle, ctl)) < 0) {
		jack_error ("ALSA-Hammerfall: cannot set clock mode");
	}

	return 0;
}

static void
hammerfall_release (jack_hardware_t *hw)

{
	hammerfall_t *h = (hammerfall_t *) hw->private_hw;
	void *status;

	if (h == 0) {
		return;
	}

#ifndef __ANDROID__
    if (h->monitor_thread) {
        pthread_cancel (h->monitor_thread);
        pthread_join (h->monitor_thread, &status);
    }
#endif

	free (h);
}

#if HAMMERFALL_MONITOR_CONTROLS
static void *
hammerfall_monitor_controls (void *arg)
{
	jack_hardware_t *hw = (jack_hardware_t *) arg;
	hammerfall_t *h = (hammerfall_t *) hw->private_hw;
	snd_ctl_elem_id_t *switch_id[3];
	snd_ctl_elem_value_t *sw[3];

	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	snd_ctl_elem_id_malloc (&switch_id[0]);
	snd_ctl_elem_id_malloc (&switch_id[1]);
	snd_ctl_elem_id_malloc (&switch_id[2]);

	snd_ctl_elem_value_malloc (&sw[0]);
	snd_ctl_elem_value_malloc (&sw[1]);
	snd_ctl_elem_value_malloc (&sw[2]);

	set_control_id (switch_id[0], "ADAT1 Sync Check");
	set_control_id (switch_id[1], "ADAT2 Sync Check");
	set_control_id (switch_id[2], "ADAT3 Sync Check");

	snd_ctl_elem_value_set_id (sw[0], switch_id[0]);
	snd_ctl_elem_value_set_id (sw[1], switch_id[1]);
	snd_ctl_elem_value_set_id (sw[2], switch_id[2]);

	while (1) {
		if (snd_ctl_elem_read (h->driver->ctl_handle, sw[0])) {
			jack_error ("cannot read control switch 0 ...");
		}
		hammerfall_check_sync (h, sw[0]);

		if (snd_ctl_elem_read (h->driver->ctl_handle, sw[1])) {
			jack_error ("cannot read control switch 0 ...");
		}
		hammerfall_check_sync (h, sw[1]);

		if (snd_ctl_elem_read (h->driver->ctl_handle, sw[2])) {
			jack_error ("cannot read control switch 0 ...");
		}
		hammerfall_check_sync (h, sw[2]);
		
		if (nanosleep (&h->monitor_interval, 0)) {
			break;
		}
	}

	pthread_exit (0);
}
#endif /* HAMMERFALL_MONITOR_CONTROLS */

jack_hardware_t *
jack_alsa_hammerfall_hw_new (alsa_driver_t *driver)
{
	jack_hardware_t *hw;
	hammerfall_t *h;

	hw = (jack_hardware_t *) malloc (sizeof (jack_hardware_t));

	hw->capabilities = Cap_HardwareMonitoring|Cap_AutoSync|Cap_WordClock|Cap_ClockMaster|Cap_ClockLockReporting;
	hw->input_monitor_mask = 0;
	hw->private_hw = 0;

	hw->set_input_monitor_mask = hammerfall_set_input_monitor_mask;
	hw->change_sample_clock = hammerfall_change_sample_clock;
	hw->release = hammerfall_release;

	h = (hammerfall_t *) malloc (sizeof (hammerfall_t));

	h->lock_status[0] = FALSE;
	h->sync_status[0] = FALSE;
	h->lock_status[1] = FALSE;
	h->sync_status[1] = FALSE;
	h->lock_status[2] = FALSE;
	h->sync_status[2] = FALSE;
	h->said_that_spdif_is_fine = FALSE;
	h->driver = driver;

	h->monitor_interval.tv_sec = 1;
	h->monitor_interval.tv_nsec = 0;

	hw->private_hw = h;

#if 0
	if (pthread_create (&h->monitor_thread, 0, hammerfall_monitor_controls, hw)) {
		jack_error ("ALSA/Hammerfall: cannot create sync monitor thread");
	}
#endif

	return hw;
}
