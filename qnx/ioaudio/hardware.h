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

 $Id: hardware.h,v 1.3 2005/11/23 11:24:29 letz Exp $
 */

#ifndef __jack_hardware_h__
#define __jack_hardware_h__

#include "types.h"

enum SampleClockMode
{
    AutoSync,
    WordClock,
    ClockMaster
};

enum Capabilities
{
    Cap_None = 0x0,
    Cap_HardwareMonitoring = 0x1,
    Cap_AutoSync = 0x2,
    Cap_WordClock = 0x4,
    Cap_ClockMaster = 0x8,
    Cap_ClockLockReporting = 0x10,
    Cap_HardwareMetering = 0x20
};

struct jack_hardware_t
{
    Capabilities capabilities;
    unsigned long input_monitor_mask;
    void *private_hw;

    jack_hardware_t() :
            capabilities( Cap_None ),
            input_monitor_mask( 0 )
    {
    }

    virtual void release() = 0;

    virtual int set_input_monitor_mask(
        unsigned long ) = 0;

    virtual int change_sample_clock(
        SampleClockMode ) = 0;

    virtual double get_hardware_peak(
        jack_port_t *port,
        jack_nframes_t frames ) = 0;

    virtual double get_hardware_power(
        jack_port_t *port,
        jack_nframes_t frames ) = 0;

};

#endif /* __jack_hardware_h__ */
