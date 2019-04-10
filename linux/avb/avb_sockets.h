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

#ifndef AVBSOCKET_H_
#define AVBSOCKET_H_


#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>


#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <arpa/inet.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <netinet/udp.h>    //Provides declarations for udp header
#include <netinet/ip.h>    //Provides declarations for udp header
#include <netdb.h>
#include <ifaddrs.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/net_tstamp.h>
#include <linux/if.h>
#include <linux/sockios.h>
//#include <linux/ip.h>
#include <linux/ptp_clock.h>
#include <linux/filter.h>

#include <poll.h>
#include <sched.h>


#include "avb_definitions.h"


int enable_1722avtp_filter( FILE* filepointer, int raw_transport_socket, unsigned char *destinationMacAddress);
int create_RAW_AVB_Transport_Socket( FILE* filepointer, int* raw_transport_socket, const char* eth_dev);

#ifdef __cplusplus
}
#endif
#endif
