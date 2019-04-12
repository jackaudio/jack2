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

#ifndef _AVB_DEFINITIONS_H_
#define _AVB_DEFINITIONS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define _GNU_SOURCE

#include <netinet/in.h>
#include <linux/if.h>
#include <jack/transport.h>
#include "jack/jslist.h"

#include "OpenAvnu/daemons/mrpd/mrpd.h"
#include "OpenAvnu/daemons/mrpd/mrp.h"

#define RETURN_VALUE_FAILURE 0
#define RETURN_VALUE_SUCCESS 1

#define MILISLEEP_TIME 1000000
#define USLEEP_TIME 1000

#define MAX_DEV_STR_LEN               32
#define BUFLEN 1500
#define ETHERNET_Q_HDR_LENGTH 18
#define ETHERNET_HDR_LENGTH 14
#define IP_HDR_LENGTH 20
#define UDP_HDR_LENGTH 8
#define AVB_ETHER_TYPE    0x22f0
#define ARRAYSIZE(arr) (sizeof(arr) / sizeof(arr[0]))

typedef struct etherheader_q
{
    unsigned char  ether_dhost[6];    // destination eth addr
    unsigned char  ether_shost[6];    // source ether addr
    unsigned int vlan_id;            // VLAN ID field
    unsigned short int ether_type;    // packet type ID field
} etherheader_q_t;

typedef struct etherheader
{
    unsigned char  ether_dhost[6];    // destination eth addr
    unsigned char  ether_shost[6];    // source ether addr
    unsigned short int ether_type;    // packet type ID field
} etherheader_t;


typedef struct mrp_ctx{
    volatile int mrp_status;
    volatile int domain_a_valid;
    volatile int domain_b_valid;
    int domain_class_a_id;
    int domain_class_a_priority;
    u_int16_t domain_class_a_vid;
    int domain_class_b_id;
    int domain_class_b_priority;
    u_int16_t domain_class_b_vid;
}mrp_ctx_t;

typedef enum mrpStatus{
    TL_UNDEFINED,
    TALKER_IDLE,
    TALKER_ADVERTISE,
    TALKER_ASKFAILED,
    TALKER_READYFAILED,
    TALKER_CONNECTING,
    TALKER_CONNECTED,
    TALKER_ERROR,
    LISTENER_IDLE,
    LISTENER_WAITING,
    LISTENER_READY,
    LISTENER_CONNECTED,
    LISTENER_ERROR,
    LISTENER_FAILED
} mrpStatus_t;


typedef struct _avb_driver_state avb_driver_state_t;

struct _avb_driver_state {

    uint8_t streamid8[8];
    uint8_t destination_mac_address[6];
    unsigned char serverMACAddress[6];
    char avbdev[MAX_DEV_STR_LEN];
    struct sockaddr_in si_other_avb;
    int raw_transport_socket;
    struct ifreq if_idx;
    struct ifreq if_mac;

    pthread_t thread;
    pthread_mutex_t threadLock;
    pthread_cond_t dataReady;

    jack_nframes_t  sample_rate;

    jack_nframes_t  period_size;
    jack_time_t        period_usecs;
    int             num_packets;
    int             adjust;

    unsigned int    capture_channels;
    unsigned int    playback_channels;

    JSList        *capture_ports;
    JSList        *playback_ports;
    JSList        *playback_srcs;
    JSList        *capture_srcs;

    jack_client_t   *client;
};

#ifdef __cplusplus
}
#endif

#endif // _AVB_DEFINITIONS_H_
