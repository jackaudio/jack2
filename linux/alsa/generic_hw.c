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

    $Id: generic_hw.c,v 1.2 2005/08/29 10:36:28 letz Exp $
*/

#include "hardware.h"
#include "alsa_driver.h"

static int generic_set_input_monitor_mask (jack_hardware_t *hw, unsigned long mask)
{
	return -1;
}

static int generic_change_sample_clock (jack_hardware_t *hw, SampleClockMode mode) 
{
	return -1;
}

static void
generic_release (jack_hardware_t *hw)
{
	return;
}

jack_hardware_t *
jack_alsa_generic_hw_new (alsa_driver_t *driver)

{
	jack_hardware_t *hw;

	hw = (jack_hardware_t *) malloc (sizeof (jack_hardware_t));

	hw->capabilities = 0;
	hw->input_monitor_mask = 0;
	
	hw->set_input_monitor_mask = generic_set_input_monitor_mask;
	hw->change_sample_clock = generic_change_sample_clock;
	hw->release = generic_release;

	return hw;
}
