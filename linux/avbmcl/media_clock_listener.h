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

#ifndef _MEDIA_CLOCK_LISTENER_H_
#define _MEDIA_CLOCK_LISTENER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <mqueue.h>
#include "avb_definitions.h"
#include "avb_sockets.h"
#include "mrp_client_interface.h"

int avtp_mcl_create( FILE* filepointer, avb_driver_state_t **avb_ctx, const char* avb_dev_name,
                                    char* stream_id, char* destination_mac,
                                    struct sockaddr_in **si_other_avb,
                                    struct pollfd **avtp_transport_socket_fds);
void avtp_mcl_delete( FILE* filepointer, avb_driver_state_t **avb_ctx);
uint64_t avtp_mcl_wait_for_rx_ts( FILE* filepointer, avb_driver_state_t **avb_ctx,
                                        struct sockaddr_in **si_other_avb,
                                        struct pollfd **avtp_transport_socket_fds,
                                        int packet_num );
uint64_t avtp_mcl_wait_for_rx_ts_const( FILE* filepointer, avb_driver_state_t **avb_ctx,
                                            struct sockaddr_in **si_other_avb,
                                            struct pollfd **avtp_transport_socket_fds,
                                            int packet_num, uint64_t *lateness );
#ifdef __cplusplus
}
#endif

#endif //_MEDIA_CLOCK_LISTENER_H_
