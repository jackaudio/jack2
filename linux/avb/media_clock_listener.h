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

#ifndef MEDIACLOCKLISTENER_H
#define MEDIACLOCKLISTENER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <sched.h>
#include <linux/types.h>
#include <sys/syscall.h>
#include <linux/sched.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <mqueue.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/resource.h>

#include "avb_definitions.h"
#include "avb_sockets.h"
#include "mrp_client_interface.h"
#include "mrp_client_control_socket.h"


int avtp_mcl_create( FILE* filepointer, avb_driver_state_t **avb_ctx, const char* avb_dev_name,
                                    char* stream_id, char* destination_mac,
                                    struct sockaddr_in **si_other_avb, struct pollfd **avtp_transport_socket_fds);
void avtp_mcl_delete( FILE* filepointer, avb_driver_state_t **avb_ctx);

uint64_t avtp_mcl_wait_for_rx_ts( FILE* filepointer, avb_driver_state_t **avb_ctx,
                                        struct sockaddr_in **si_other_avb, struct pollfd **avtp_transport_socket_fds, int packet_num );

#ifdef __cplusplus
}
#endif
#endif //MEDIACLOCKLISTENER_H
