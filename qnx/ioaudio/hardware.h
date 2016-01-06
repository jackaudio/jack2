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

typedef	enum {
    AutoSync,
    WordClock,
    ClockMaster
} SampleClockMode;

typedef enum {
    Cap_HardwareMonitoring = 0x1,
    Cap_AutoSync = 0x2,
    Cap_WordClock = 0x4,
    Cap_ClockMaster = 0x8,
    Cap_ClockLockReporting = 0x10,
    Cap_HardwareMetering = 0x20
} Capabilities;

struct _jack_hardware;

typedef void (*JackHardwareReleaseFunction)(struct _jack_hardware *);
typedef int (*JackHardwareSetInputMonitorMaskFunction)(struct _jack_hardware *, unsigned long);
typedef int (*JackHardwareChangeSampleClockFunction)(struct _jack_hardware *, SampleClockMode);
typedef double (*JackHardwareGetHardwarePeak)(jack_port_t *port, jack_nframes_t frames);
typedef double (*JackHardwareGetHardwarePower)(jack_port_t *port, jack_nframes_t frames);

typedef struct _jack_hardware
{
    unsigned long capabilities;
    unsigned long input_monitor_mask;

    JackHardwareChangeSampleClockFunction change_sample_clock;
    JackHardwareSetInputMonitorMaskFunction set_input_monitor_mask;
    JackHardwareReleaseFunction release;
    JackHardwareGetHardwarePeak get_hardware_peak;
    JackHardwareGetHardwarePower get_hardware_power;
    void *private_hw;
}
jack_hardware_t;

#ifdef __cplusplus
extern "C"
{
#endif

    jack_hardware_t * jack_hardware_new ();

#ifdef __cplusplus
}
#endif


#endif /* __jack_hardware_h__ */
