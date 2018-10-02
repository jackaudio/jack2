/*
 * udpSocket.h
 *
 *  Created on: Nov 7, 2016
 *      Author: christoph
 */

#ifndef AVBSOCKET_H_
#define AVBSOCKET_H_

#define _GNU_SOURCE

#include <stdint.h>
#include <unistd.h>
#include <linux/if_packet.h>
#include <netinet/in.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <pci/pci.h>

//#include <linux/if.h>
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

#include <arpa/inet.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <netinet/udp.h>	//Provides declarations for udp header
#include <netinet/ip.h>	//Provides declarations for udp header

#include <netdb.h>
#include <ifaddrs.h>

#include "global_definitions.h"


int enable_1722avtp_filter( FILE* filepointer, int raw_transport_socket, unsigned char *destinationMacAddress);
int create_RAW_AVB_Transport_Socket( FILE* filepointer, int* raw_transport_socket, char* eth_dev);

#endif /* UDPSOCKET_H_ */
