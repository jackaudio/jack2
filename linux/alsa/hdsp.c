/*
    Copyright (C) 2001 Paul Davis 
    Copyright (C) 2002 Dave LaRose

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

    $Id: hdsp.c,v 1.3 2005/09/29 14:51:59 letz Exp $
*/

#include "hardware.h"
#include "alsa_driver.h"
#include "hdsp.h"
#include "JackError.h"

/* Constants to make working with the hdsp matrix mixer easier */
static const int HDSP_MINUS_INFINITY_GAIN = 0;
static const int HDSP_UNITY_GAIN = 32768;
static const int HDSP_MAX_GAIN = 65535;

/*
 * Use these two arrays to choose the value of the input_channel 
 * argument to hsdp_set_mixer_gain().  hdsp_physical_input_index[n] 
 * selects the nth optical/analog input.  audio_stream_index[n] 
 * selects the nth channel being received from the host via pci/pccard.
 */
static const int hdsp_num_input_channels = 52;
static const int hdsp_physical_input_index[] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
  12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25};
static const int hdsp_audio_stream_index[] = {
  26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37,
  38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};

/*
 * Use this array to choose the value of the output_channel 
 * argument to hsdp_set_mixer_gain().  hdsp_physical_output_index[26]
 * and hdsp_physical_output_index[27] refer to the two "line out"
 * channels (1/4" phone jack on the front of digiface/multiface).
 */
static const int hdsp_num_output_channels = 28;
static const int hdsp_physical_output_index[] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
  12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27};


/* Function for checking argument values */
static int clamp_int(int value, int lower_bound, int upper_bound)
{
  if(value < lower_bound) {
    return lower_bound;
  }
  if(value > upper_bound) {
    return upper_bound;
  }
  return value;
}

/* Note(XXX): Maybe should share this code with hammerfall.c? */
static void 
set_control_id (snd_ctl_elem_id_t *ctl, const char *name)
{
	snd_ctl_elem_id_set_name (ctl, name);
	snd_ctl_elem_id_set_numid (ctl, 0);
	snd_ctl_elem_id_set_interface (ctl, SND_CTL_ELEM_IFACE_HWDEP);
	snd_ctl_elem_id_set_device (ctl, 0);
	snd_ctl_elem_id_set_subdevice (ctl, 0);
	snd_ctl_elem_id_set_index (ctl, 0);
}

/* The hdsp matrix mixer lets you connect pretty much any input to */
/* any output with gain from -inf to about +2dB. Pretty slick. */
/* This routine makes a convenient way to set the gain from */
/* input_channel to output_channel (see hdsp_physical_input_index */
/* etc. above. */
/* gain is an int from 0 to 65535, with 0 being -inf gain, and */
/* 65535 being about +2dB. */

static int hdsp_set_mixer_gain(jack_hardware_t *hw, int input_channel,
			       int output_channel, int gain)
{
	hdsp_t *h = (hdsp_t *) hw->private_hw;
	snd_ctl_elem_value_t *ctl;
	snd_ctl_elem_id_t *ctl_id;
	int err;

	/* Check args */
	input_channel = clamp_int(input_channel, 0, hdsp_num_input_channels);
	output_channel = clamp_int(output_channel, 0, hdsp_num_output_channels);
	gain = clamp_int(gain, HDSP_MINUS_INFINITY_GAIN, HDSP_MAX_GAIN);

	/* Allocate control element and select "Mixer" control */
	snd_ctl_elem_value_alloca (&ctl);
	snd_ctl_elem_id_alloca (&ctl_id);
	set_control_id (ctl_id, "Mixer");
	snd_ctl_elem_value_set_id (ctl, ctl_id);

	/* Apparently non-standard and unstable interface for the */
        /* mixer control. */
	snd_ctl_elem_value_set_integer (ctl, 0, input_channel);
	snd_ctl_elem_value_set_integer (ctl, 1, output_channel);
	snd_ctl_elem_value_set_integer (ctl, 2, gain);

	/* Commit the mixer value and check for errors */
	if ((err = snd_ctl_elem_write (h->driver->ctl_handle, ctl)) != 0) {
	  jack_error ("ALSA/HDSP: cannot set mixer gain (%s)", snd_strerror (err));
	  return -1;
	}

	/* Note (XXX): Perhaps we should maintain a cache of the current */
	/* mixer values, since it's not clear how to query them from the */
	/* hdsp hardware.  We'll leave this out until a little later. */
	return 0;
}
  
static int hdsp_set_input_monitor_mask (jack_hardware_t *hw, unsigned long mask)
{
	int i;

	/* For each input channel */
	for (i = 0; i < 26; i++) {
		/* Monitoring requested for this channel? */
		if(mask & (1<<i)) {
			/* Yes.  Connect physical input to output */

			if(hdsp_set_mixer_gain (hw, hdsp_physical_input_index[i],
						hdsp_physical_output_index[i],
						HDSP_UNITY_GAIN) != 0) {
			  return -1;
			}

#ifdef CANNOT_HEAR_SOFTWARE_STREAM_WHEN_MONITORING
			/* ...and disconnect the corresponding software */
			/* channel */
			if(hdsp_set_mixer_gain (hw, hdsp_audio_stream_index[i],
						hdsp_physical_output_index[i],
						HDSP_MINUS_INFINITY_GAIN) != 0) {
			  return -1;
			}
#endif

		} else {
			/* No.  Disconnect physical input from output */
			if(hdsp_set_mixer_gain (hw, hdsp_physical_input_index[i],
						hdsp_physical_output_index[i],
						HDSP_MINUS_INFINITY_GAIN) != 0) {
			  return -1;
			}

#ifdef CANNOT_HEAR_SOFTWARE_STREAM_WHEN_MONITORING
			/* ...and connect the corresponding software */
			/* channel */
			if(hdsp_set_mixer_gain (hw, hdsp_audio_stream_index[i],
						hdsp_physical_output_index[i],
						HDSP_UNITY_GAIN) != 0) {
			  return -1;
			}
#endif
		}
	}
	/* Cache the monitor mask */
	hw->input_monitor_mask = mask;
	return 0;
}


static int hdsp_change_sample_clock (jack_hardware_t *hw, SampleClockMode mode) 
{
  // Empty for now, until Dave understands more about clock sync so
  // he can test.
  return -1;
}

static double hdsp_get_hardware_peak (jack_port_t *port, jack_nframes_t frame)
{
	return 0;
}

static double hdsp_get_hardware_power (jack_port_t *port, jack_nframes_t frame)
{
	return 0;
}

static void
hdsp_release (jack_hardware_t *hw)
{
	hdsp_t *h = (hdsp_t *) hw->private_hw;

	if (h != 0) {
	  free (h);
	}
}

/* Mostly copied directly from hammerfall.c */
jack_hardware_t *
jack_alsa_hdsp_hw_new (alsa_driver_t *driver)
{
	jack_hardware_t *hw;
	hdsp_t *h;

	hw = (jack_hardware_t *) malloc (sizeof (jack_hardware_t));

	/* Not using clock lock-sync-whatever in home hardware setup */
	/* yet.  Will write this code when can test it. */
	/* hw->capabilities = Cap_HardwareMonitoring|Cap_AutoSync|Cap_WordClock|Cap_ClockMaster|Cap_ClockLockReporting; */
	hw->capabilities = Cap_HardwareMonitoring | Cap_HardwareMetering;
	hw->input_monitor_mask = 0;
	hw->private_hw = 0;

	hw->set_input_monitor_mask = hdsp_set_input_monitor_mask;
	hw->change_sample_clock = hdsp_change_sample_clock;
	hw->release = hdsp_release;
	hw->get_hardware_peak = hdsp_get_hardware_peak;
	hw->get_hardware_power = hdsp_get_hardware_power;
	
	h = (hdsp_t *) malloc (sizeof (hdsp_t));
	h->driver = driver;
	hw->private_hw = h;

	return hw;
}
