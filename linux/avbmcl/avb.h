/*
Copyright (C) 2016-2019 Christoph Kuhr

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

#ifndef _JACK_AVTP_DRIVER_H_
#define _JACK_AVTP_DRIVER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "avb_definitions.h"
#include "media_clock_listener.h"

int init_avb_driver(   avb_driver_state_t *avb_ctx, const char* name,
                        char* stream_id, char* destination_mac,
                        int sample_rate, int period_size, int num_periods, int adjust, int capture_ports, int playback_ports);
int startup_avb_driver( avb_driver_state_t *avb_ctx);
uint64_t await_avtp_rx_ts( avb_driver_state_t *avb_ctx, int packet_num, uint64_t *lateness );
int shutdown_avb_driver( avb_driver_state_t *avb_ctx);

#ifdef __cplusplus
}
#endif

#endif //_JACK_AVTP_DRIVER_H_
