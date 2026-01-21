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

#include "media_clock_listener.h"

#define NUM_TS 10000000

extern int errno;
static uint64_t last_packet_time_ns = 0;
uint64_t timestamps[NUM_TS];
int ts_cnt =0;

int avtp_mcl_create( FILE* filepointer, avb_driver_state_t **avb_ctx, const char* avb_dev_name,
                                    char* stream_id, char* destination_mac,
                                    struct sockaddr_in **si_other_avb, struct pollfd **avtp_transport_socket_fds)
{
    fprintf(filepointer,  "Create Mediaclock Listener\n");fflush(filepointer);
    memset( timestamps, 0, sizeof(uint64_t)*NUM_TS);
    //00:22:97:00:41:2c:00:00  91:e0:f0:11:11:11
    memcpy((*avb_ctx)->streamid8, stream_id, 8);
    memcpy((*avb_ctx)->destination_mac_address, destination_mac, 6);

    fprintf(filepointer,  "create RAW AVTP Socket %s  \n", avb_dev_name);fflush(filepointer);
    (*avtp_transport_socket_fds) = (struct pollfd*)malloc(sizeof(struct pollfd));
    memset((*avtp_transport_socket_fds), 0, sizeof(struct sockaddr_in));
    (*si_other_avb) = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
    memset((*si_other_avb), 0, sizeof(struct sockaddr_in));

    if( create_RAW_AVB_Transport_Socket(filepointer, &((*avtp_transport_socket_fds)->fd), avb_dev_name) > RETURN_VALUE_FAILURE ){
        fprintf(filepointer,  "enable IEEE1722 AVTP MAC filter %x:%x:%x:%x:%x:%x  \n",
                                            (*avb_ctx)->destination_mac_address[0],
                                            (*avb_ctx)->destination_mac_address[1],
                                            (*avb_ctx)->destination_mac_address[2],
                                            (*avb_ctx)->destination_mac_address[3],
                                            (*avb_ctx)->destination_mac_address[4],
                                            (*avb_ctx)->destination_mac_address[5]);fflush(filepointer);

        enable_1722avtp_filter(filepointer, (*avtp_transport_socket_fds)->fd, (*avb_ctx)->destination_mac_address);
        (*avtp_transport_socket_fds)->events = POLLIN;
    } else {
        fprintf(filepointer,  "Listener Creation failed\n");fflush(filepointer);
        return RETURN_VALUE_FAILURE;
    }

    fprintf(filepointer,  "Get Domain VLAN\n");fflush(filepointer);
    return RETURN_VALUE_SUCCESS;
}

void avtp_mcl_delete( FILE* filepointer, avb_driver_state_t **avb_ctx )
{
    FILE* filepointer2;
    int i = 0;

    if( ! (filepointer2 = fopen("mcs_ts.log", "w")) ){
        printf("Error Opening file %d\n", errno);
        return;
    }

    for(i = 0; i < NUM_TS; i++){
        if(timestamps[i] != 0 ){
            fprintf(filepointer2, "%lud\n",timestamps[i]);fflush(filepointer2);
        }
    }
    fclose(filepointer2);
}

uint64_t avtp_mcl_wait_for_rx_ts( FILE* filepointer, avb_driver_state_t **avb_ctx,
                                            struct sockaddr_in **si_other_avb,
                                            struct pollfd **avtp_transport_socket_fds,
                                            int packet_num )
{
    char stream_packet[BUFLEN];

//    struct cmsghdr {
//        socklen_t     cmsg_len;     // data byte count, including hdr
//        int           cmsg_level;   // originating protocol
//        int           cmsg_type;    // protocol-specific type
//        // followed by unsigned char cmsg_data[];
//    };
//
//    struct msghdr {
//        void         *msg_name;       // optional address
//        socklen_t     msg_namelen;    // size of address
//        struct iovec *msg_iov;        // scatter/gather array
//        size_t        msg_iovlen;     // # elements in msg_iov
//        void         *msg_control;    // ancillary data, see below
//        size_t        msg_controllen; // ancillary data buffer len
//        int           msg_flags;      // flags on received message
//    };

    struct msghdr msg;
    struct cmsghdr *cmsg;
    struct sockaddr_ll remote;
    struct iovec sgentry;
    struct {
        struct cmsghdr cm;
        char control[256];
    } control;

    memset( &msg, 0, sizeof( msg ));
    msg.msg_iov = &sgentry;
    msg.msg_iovlen = 1;
    sgentry.iov_base = stream_packet;
    sgentry.iov_len = BUFLEN;

    memset( &remote, 0, sizeof(remote));
    msg.msg_name = (caddr_t) &remote;
    msg.msg_namelen = sizeof( remote );
    msg.msg_control = &control;
    msg.msg_controllen = sizeof(control);

    int status = recvmsg((*avtp_transport_socket_fds)->fd, &msg, 0);//NULL);

    if (status == 0) {
        fprintf(filepointer, "EOF\n");fflush(filepointer);
        return -1;
    } else if (status < 0) {
        fprintf(filepointer, "Error recvmsg: %d %d %s\n", status, errno, strerror(errno));fflush(filepointer);
        return -1;
    }
    if( // Compare Stream IDs
        ((*avb_ctx)->streamid8[0] == (uint8_t) stream_packet[18]) &&
        ((*avb_ctx)->streamid8[1] == (uint8_t) stream_packet[19]) &&
        ((*avb_ctx)->streamid8[2] == (uint8_t) stream_packet[20]) &&
        ((*avb_ctx)->streamid8[3] == (uint8_t) stream_packet[21]) &&
        ((*avb_ctx)->streamid8[4] == (uint8_t) stream_packet[22]) &&
        ((*avb_ctx)->streamid8[5] == (uint8_t) stream_packet[23]) &&
        ((*avb_ctx)->streamid8[6] == (uint8_t) stream_packet[24]) &&
        ((*avb_ctx)->streamid8[7] == (uint8_t) stream_packet[25])
    ){
        uint64_t adjust_packet_time_ns = 0;
        uint64_t packet_arrival_time_ns = 0;
        uint64_t rx_int_to_last_packet_ns = 0;

        // Packet Arrival Time from Device
        cmsg = CMSG_FIRSTHDR(&msg);
        while( cmsg != NULL ) {
            if( cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SO_TIMESTAMPING ) {
                struct timespec *ts_device, *ts_system;
                ts_system = ((struct timespec *) CMSG_DATA(cmsg)) + 1;
                ts_device = ts_system + 1;
                packet_arrival_time_ns =  (ts_device->tv_sec*1000000000LL + ts_device->tv_nsec);
                if( ts_cnt < NUM_TS )
                    timestamps[ts_cnt++] = packet_arrival_time_ns;
                break;
            }
            cmsg = CMSG_NXTHDR(&msg,cmsg);
        }

        rx_int_to_last_packet_ns = packet_arrival_time_ns - last_packet_time_ns;
        last_packet_time_ns = packet_arrival_time_ns;

        if( packet_num == (*avb_ctx)->num_packets -1){

            adjust_packet_time_ns = (uint64_t) ( ( (float)((*avb_ctx)->period_size % 6 ) / (float)(*avb_ctx)->sample_rate ) * 1000000000LL);
        } else {
            adjust_packet_time_ns = (*avb_ctx)->adjust ? rx_int_to_last_packet_ns : 125000;
        }
        return adjust_packet_time_ns -1000;
    }
    return -1;
}




uint64_t avtp_mcl_wait_for_rx_ts_const( FILE* filepointer, avb_driver_state_t **avb_ctx,
                                            struct sockaddr_in **si_other_avb,
                                            struct pollfd **avtp_transport_socket_fds,
                                            int packet_num, uint64_t *lateness )
{
    char stream_packet[BUFLEN];

//    struct cmsghdr {
//        socklen_t     cmsg_len;     // data byte count, including hdr
//        int           cmsg_level;   // originating protocol
//        int           cmsg_type;    // protocol-specific type
//        // followed by unsigned char cmsg_data[];
//    };
//
//    struct msghdr {
//        void         *msg_name;       // optional address
//        socklen_t     msg_namelen;    // size of address
//        struct iovec *msg_iov;        // scatter/gather array
//        size_t        msg_iovlen;     // # elements in msg_iov
//        void         *msg_control;    // ancillary data, see below
//        size_t        msg_controllen; // ancillary data buffer len
//        int           msg_flags;      // flags on received message
//    };

    struct msghdr msg;
    struct cmsghdr *cmsg;
    struct sockaddr_ll remote;
    struct iovec sgentry;
    struct {
        struct cmsghdr cm;
        char control[256];
    } control;

    memset( &msg, 0, sizeof( msg ));
    msg.msg_iov = &sgentry;
    msg.msg_iovlen = 1;
    sgentry.iov_base = stream_packet;
    sgentry.iov_len = BUFLEN;

    memset( &remote, 0, sizeof(remote));
    msg.msg_name = (caddr_t) &remote;
    msg.msg_namelen = sizeof( remote );
    msg.msg_control = &control;
    msg.msg_controllen = sizeof(control);

    int status = recvmsg((*avtp_transport_socket_fds)->fd, &msg, 0);//NULL);

    if (status == 0) {
        fprintf(filepointer, "EOF\n");fflush(filepointer);
        return -1;
    } else if (status < 0) {
        fprintf(filepointer, "Error recvmsg: %d %d %s\n", status, errno, strerror(errno));fflush(filepointer);
        return -1;
    }
    if( // Compare Stream IDs
        ((*avb_ctx)->streamid8[0] == (uint8_t) stream_packet[18]) &&
        ((*avb_ctx)->streamid8[1] == (uint8_t) stream_packet[19]) &&
        ((*avb_ctx)->streamid8[2] == (uint8_t) stream_packet[20]) &&
        ((*avb_ctx)->streamid8[3] == (uint8_t) stream_packet[21]) &&
        ((*avb_ctx)->streamid8[4] == (uint8_t) stream_packet[22]) &&
        ((*avb_ctx)->streamid8[5] == (uint8_t) stream_packet[23]) &&
        ((*avb_ctx)->streamid8[6] == (uint8_t) stream_packet[24]) &&
        ((*avb_ctx)->streamid8[7] == (uint8_t) stream_packet[25])
    ){
        uint64_t adjust_packet_time_ns = 0;
        uint64_t packet_arrival_time_ns = 0;

        // Packet Arrival Time from Device
        cmsg = CMSG_FIRSTHDR(&msg);
        while( cmsg != NULL ) {
            if( cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SO_TIMESTAMPING ) {
                struct timespec *ts_device, *ts_system;
                ts_system = ((struct timespec *) CMSG_DATA(cmsg)) + 1;
                ts_device = ts_system + 1;
                packet_arrival_time_ns =  (ts_device->tv_sec*1000000000LL + ts_device->tv_nsec);
                if( ts_cnt < NUM_TS )
                    timestamps[ts_cnt++] = packet_arrival_time_ns;
                break;
            }
            cmsg = CMSG_NXTHDR(&msg,cmsg);
        }

        (*lateness) += ( packet_arrival_time_ns - last_packet_time_ns ) - 125000;
        last_packet_time_ns = packet_arrival_time_ns;

        if( packet_num == (*avb_ctx)->num_packets -1){
            adjust_packet_time_ns = (uint64_t) ( ( (float)((*avb_ctx)->period_size % 6 ) / (float)(*avb_ctx)->sample_rate ) * 1000000000LL);
        } else {
            adjust_packet_time_ns = 125000;
        }
        return adjust_packet_time_ns -1000;
    }
    return -1;
}

