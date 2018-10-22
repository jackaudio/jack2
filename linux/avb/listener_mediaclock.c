#include "listener_mediaclock.h"
extern int errno;

static uint64_t last_packet_time_ns = 0;


int create_avb_Mediaclock_Listener( FILE* filepointer, ieee1722_avtp_driver_state_t **ieee1722mc, char* avb_dev_name,
                                    char* stream_id, char* destination_mac,
                                    struct sockaddr_in **si_other_avb, struct pollfd **avtp_transport_socket_fds)
{
	fprintf(filepointer,  "Create Mediaclock Listener\n");fflush(filepointer);


	//00:22:97:00:41:2c:00:00  91:e0:f0:11:11:11
    memcpy((*ieee1722mc)->streamid8, stream_id, 8);
    memcpy((*ieee1722mc)->destination_mac_address, destination_mac, 6);

	fprintf(filepointer,  "create RAW AVTP Socket %s  \n", avb_dev_name);fflush(filepointer);
	(*avtp_transport_socket_fds) = (struct pollfd*)malloc(sizeof(struct pollfd));
	memset((*avtp_transport_socket_fds), 0, sizeof(struct sockaddr_in));
	(*si_other_avb) = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
	memset((*si_other_avb), 0, sizeof(struct sockaddr_in));

	if( create_RAW_AVB_Transport_Socket(filepointer, &((*avtp_transport_socket_fds)->fd), avb_dev_name) > RETURN_VALUE_FAILURE ){
        fprintf(filepointer,  "enable IEEE1722 AVTP MAC filter %x:%x:%x:%x:%x:%x  \n",
                (*ieee1722mc)->destination_mac_address[0],
                (*ieee1722mc)->destination_mac_address[1],
                (*ieee1722mc)->destination_mac_address[2],
                (*ieee1722mc)->destination_mac_address[3],
                (*ieee1722mc)->destination_mac_address[4],
                (*ieee1722mc)->destination_mac_address[5]);fflush(filepointer);

        enable_1722avtp_filter(filepointer, (*avtp_transport_socket_fds)->fd, (*ieee1722mc)->destination_mac_address);
        (*avtp_transport_socket_fds)->events = POLLIN;

	} else {
		fprintf(filepointer,  "Listener Creation failed\n");fflush(filepointer);
        return RETURN_VALUE_FAILURE;
	}

	fprintf(filepointer,  "Get Domain VLAN\n");fflush(filepointer);

    return RETURN_VALUE_SUCCESS;
}


void delete_avb_Mediaclock_Listener( FILE* filepointer, ieee1722_avtp_driver_state_t **ieee1722mc )
{
}

uint64_t mediaclock_listener_wait_recv_ts( FILE* filepointer, ieee1722_avtp_driver_state_t **ieee1722mc, struct sockaddr_in **si_other_avb, struct pollfd **avtp_transport_socket_fds, int packet_num )
{
    socklen_t slen_avb = sizeof(struct sockaddr_in);
    char stream_packet[BUFLEN];

//    struct cmsghdr {
//        socklen_t     cmsg_len;     /* data byte count, including hdr */
//        int           cmsg_level;   /* originating protocol */
//        int           cmsg_type;    /* protocol-specific type */
//        /* followed by unsigned char cmsg_data[]; */
//    };
//
//    struct msghdr {
//        void         *msg_name;       /* optional address */
//        socklen_t     msg_namelen;    /* size of address */
//        struct iovec *msg_iov;        /* scatter/gather array */
//        size_t        msg_iovlen;     /* # elements in msg_iov */
//        void         *msg_control;    /* ancillary data, see below */
//        size_t        msg_controllen; /* ancillary data buffer len */
//        int           msg_flags;      /* flags on received message */
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

	int status = recvmsg((*avtp_transport_socket_fds)->fd, &msg, NULL);

	if (status == 0) {
		fprintf(filepointer, "EOF\n");fflush(filepointer);
		return -1;
	} else if (status < 0) {
		fprintf(filepointer, "Error recvmsg: %d %d %s\n", status, errno, strerror(errno));fflush(filepointer);
		return -1;
	}

//	fprintf(filepointer, "Stream Packet: %02x %02x %02x %02x %02x %02x %02x %02x \n",
//                                            (uint8_t) stream_packet[18],
//                                            (uint8_t) stream_packet[19],
//                                            (uint8_t) stream_packet[20],
//                                            (uint8_t) stream_packet[21],
//                                            (uint8_t) stream_packet[22],
//                                            (uint8_t) stream_packet[23],
//                                            (uint8_t) stream_packet[24],
//                                            (uint8_t) stream_packet[25]);fflush(filepointer);
    if(
        ((*ieee1722mc)->streamid8[0] == (uint8_t) stream_packet[18]) &&
        ((*ieee1722mc)->streamid8[1] == (uint8_t) stream_packet[19]) &&
        ((*ieee1722mc)->streamid8[2] == (uint8_t) stream_packet[20]) &&
        ((*ieee1722mc)->streamid8[3] == (uint8_t) stream_packet[21]) &&
        ((*ieee1722mc)->streamid8[4] == (uint8_t) stream_packet[22]) &&
        ((*ieee1722mc)->streamid8[5] == (uint8_t) stream_packet[23]) &&
        ((*ieee1722mc)->streamid8[6] == (uint8_t) stream_packet[24]) &&
        ((*ieee1722mc)->streamid8[7] == (uint8_t) stream_packet[25])
    ){

        int samples_in_packet = 0;
        uint64_t adjust_packet_time_ns = 0;
        uint64_t packet_arrival_time_ns = 0;
        uint64_t ipg_to_last_packet_ns = 0;

        int bytes_per_stereo_channel = 12 /*CHANNEL_COUNT_STEREO * AVTP_SAMPLES_PER_CHANNEL_PER_PACKET = 2*6 */ * sizeof(uint32_t);
        int avtp_hdr_len = ETHERNET_HDR_LENGTH + 32 /*AVB_HEADER_LENGTH*/;




//        fprintf(filepointer, "stream packet!\n");fflush(filepointer);

        /* Packet Arrival Time */
        cmsg = CMSG_FIRSTHDR(&msg);
        while( cmsg != NULL ) {
            if( cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SO_TIMESTAMPING ) {
                struct timespec *ts_device, *ts_system;
                ts_system = ((struct timespec *) CMSG_DATA(cmsg)) + 1;
                ts_device = ts_system + 1;
//                fprintf(filepointer, "Device %lld sec %lld nanosec\n", ts_device->tv_sec, ts_device->tv_nsec);fflush(filepointer);

                packet_arrival_time_ns =  (ts_device->tv_sec*1000000000LL + ts_device->tv_nsec);

                break;
            }
            cmsg = CMSG_NXTHDR(&msg,cmsg);
        }


//        struct timeval sys_time;
//
//        if (clock_gettime(CLOCK_REALTIME, &sys_time)) {
//            fprintf(filepointer, " Clockrealtime Error\n");fflush(filepointer);
//        }
//        packet_arrival_time_ns = (sys_time.tv_sec*1000000000LL + sys_time.tv_usec);




        ipg_to_last_packet_ns = packet_arrival_time_ns - last_packet_time_ns;
        last_packet_time_ns = packet_arrival_time_ns;
		fprintf(filepointer, "packet arrival time %lld ns, ipg %lld ns\n", packet_arrival_time_ns, ipg_to_last_packet_ns);fflush(filepointer);



        /*
         *
         *      6 or less samples per packet? =>
         *
         */

        if( packet_num == (*ieee1722mc)->num_packets -1){

            adjust_packet_time_ns = (uint64_t) ( ( (float)((*ieee1722mc)->period_size % 6 ) / (float)(*ieee1722mc)->sample_rate ) * 1000000000LL);
//            fprintf(filepointer, "adjust time %lld ns\n", adjust_packet_time_ns);fflush(filepointer);
        }


//        for( int s = avtp_hdr_len; s < avtp_hdr_len + bytes_per_stereo_channel; s += sizeof(uint32_t) ){
//
//            if(stream_packet[ s ] != 0x00){
//                                    fprintf(filepointer,  "avb sample %d %x %x %x %x \n", s, avb_packet[ s ],
//                                                                                            avb_packet[ s + 1 ],
//                                                                                            avb_packet[ s + 2 ],
//                                                                                            avb_packet[ s + 3 ] );fflush(filepointer);
//                samples_in_packet++;
//            }
//        }
//
//        if( samples_in_packet < 6){
//            adjust_packet_time_ns = samples_in_packet / (*ieee1722mc)->sample_rate * 1000000000;
//        }







        return ipg_to_last_packet_ns - adjust_packet_time_ns;
    }
    return -1;

}

uint64_t mediaclock_listener_wait_recv( FILE* filepointer, ieee1722_avtp_driver_state_t **ieee1722mc, struct sockaddr_in **si_other_avb, struct pollfd **avtp_transport_socket_fds, int packet_num  )
{
	int recv_len=0;
	int rc;


    socklen_t slen_avb;
    char stream_packet[BUFLEN];
    if ((recv_len = recvfrom((*avtp_transport_socket_fds)->fd, stream_packet, BUFLEN, 0, (struct sockaddr *)(*si_other_avb), &slen_avb )) == -1){
        fprintf(filepointer, "recvfrom %s\n", strerror(errno));fflush(filepointer);
    } else {
        if(
                ((*ieee1722mc)->streamid8[0] == (uint8_t) stream_packet[18]) &&
                ((*ieee1722mc)->streamid8[1] == (uint8_t) stream_packet[19]) &&
                ((*ieee1722mc)->streamid8[2] == (uint8_t) stream_packet[20]) &&
                ((*ieee1722mc)->streamid8[3] == (uint8_t) stream_packet[21]) &&
                ((*ieee1722mc)->streamid8[4] == (uint8_t) stream_packet[22]) &&
                ((*ieee1722mc)->streamid8[5] == (uint8_t) stream_packet[23]) &&
                ((*ieee1722mc)->streamid8[6] == (uint8_t) stream_packet[24]) &&
                ((*ieee1722mc)->streamid8[7] == (uint8_t) stream_packet[25])
        ){


            /*
             *
             *      6 or less samples per packet? =>
             *
             */
            int samples_in_packet = 0;
            uint64_t adjust_packet_time_ns = 0;

            int bytes_per_stereo_channel = 12 /*CHANNEL_COUNT_STEREO * AVTP_SAMPLES_PER_CHANNEL_PER_PACKET = 2*6 */ * sizeof(uint32_t);
            int avtp_hdr_len = ETHERNET_HDR_LENGTH + 32 /*AVB_HEADER_LENGTH*/;

            for( int s = avtp_hdr_len; s < avtp_hdr_len + bytes_per_stereo_channel; s += sizeof(uint32_t) ){

                if(stream_packet[ s ] != 0x00){
        //                                fprintf(filepointer,  "avb sample %d %x %x %x %x \n", s, avb_packet[ s ],
        //                                                                                        avb_packet[ s + 1 ],
        //                                                                                        avb_packet[ s + 2 ],
        //                                                                                        avb_packet[ s + 3 ] );fflush(filepointer);
                    samples_in_packet++;
                }
            }




            if( samples_in_packet < 6 ){
                adjust_packet_time_ns = samples_in_packet / (*ieee1722mc)->sample_rate * 1000000000;
            }


            /* inaccurate */
            struct timeval sys_time;

            if (clock_gettime(CLOCK_REALTIME, &sys_time)) {
                fprintf(filepointer, " Clockrealtime Error\n");fflush(filepointer);
            }


            return (uint64_t)(sys_time.tv_usec * 1000) - adjust_packet_time_ns;
        }
    }
    return -1;

}

uint64_t mediaclock_listener_poll_recv( FILE* filepointer, ieee1722_avtp_driver_state_t **ieee1722mc, struct sockaddr_in **si_other_avb, struct pollfd **avtp_transport_socket_fds, int packet_num  )
{
	int recv_len=0;
	int rc;

    if( (rc = poll((*avtp_transport_socket_fds), 1, 0)) > 0 ){

        socklen_t slen_avb;
        char stream_packet[BUFLEN];
        if ((recv_len = recvfrom((*avtp_transport_socket_fds)->fd, stream_packet, BUFLEN, 0, (struct sockaddr *)(*si_other_avb), &slen_avb )) == -1){
            fprintf(filepointer, "recvfrom %s", strerror(errno));fflush(filepointer);

        } else {
            if(
                    ((*ieee1722mc)->streamid8[0] == (uint8_t) stream_packet[18]) &&
                    ((*ieee1722mc)->streamid8[1] == (uint8_t) stream_packet[19]) &&
                    ((*ieee1722mc)->streamid8[2] == (uint8_t) stream_packet[20]) &&
                    ((*ieee1722mc)->streamid8[3] == (uint8_t) stream_packet[21]) &&
                    ((*ieee1722mc)->streamid8[4] == (uint8_t) stream_packet[22]) &&
                    ((*ieee1722mc)->streamid8[5] == (uint8_t) stream_packet[23]) &&
                    ((*ieee1722mc)->streamid8[6] == (uint8_t) stream_packet[24]) &&
                    ((*ieee1722mc)->streamid8[7] == (uint8_t) stream_packet[25])
            ){
                /*
                 *
                 *      6 or less samples per packet? =>
                 *
                 */
                int samples_in_packet = 0;
                uint64_t adjust_packet_time_ns = 0;

                int bytes_per_stereo_channel = 12 /*CHANNEL_COUNT_STEREO * AVTP_SAMPLES_PER_CHANNEL_PER_PACKET = 2*6 */ * sizeof(uint32_t);
                int avtp_hdr_len = ETHERNET_HDR_LENGTH + 32 /*AVB_HEADER_LENGTH*/;

                for( int s = avtp_hdr_len; s < avtp_hdr_len + bytes_per_stereo_channel; s += sizeof(uint32_t) ){

                    if(stream_packet[ s ] != 0x00){
            //                                fprintf(filepointer,  "avb sample %d %x %x %x %x \n", s, avb_packet[ s ],
            //                                                                                        avb_packet[ s + 1 ],
            //                                                                                        avb_packet[ s + 2 ],
            //                                                                                        avb_packet[ s + 3 ] );fflush(filepointer);
                        samples_in_packet++;
                    }
                }




                if( samples_in_packet < 6 ){
                    adjust_packet_time_ns = samples_in_packet / (*ieee1722mc)->sample_rate * 1000000000;
                }


                /* inaccurate */
                struct timeval sys_time;

                if (clock_gettime(CLOCK_REALTIME, &sys_time)) {
                    fprintf(filepointer, " Clockrealtime Error\n");fflush(filepointer);
                }

                return (uint64_t)(sys_time.tv_usec * 1000) - adjust_packet_time_ns;
            }
        }
    }
    return -1;

}
