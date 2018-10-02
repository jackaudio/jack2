#include "listener_mediaclock.h"
extern int errno;

//gPtpTimeData *ptpData;
uint64_t last_arrival_time;

//static inline uint64_t getCpuFrequency(void)
//{
//    uint64_t freq = 0;
//    std::string line;
//    std::ifstream cpuinfo("/proc/cpuinfo");
//    if (cpuinfo.is_open())
//    {
//        while ( getline (cpuinfo,line) )
//        {
//            if(line.find("MHz") != line.npos)
//                break;
//        }
//        cpuinfo.close();
//    }
//    else std::cout << "Unable to open file";
//
//    size_t pos = line.find(":");
//    if (pos != line.npos) {
//        std::string mhz_str = line.substr(pos+2, line.npos - pos);
//        double freq1 = strtod(mhz_str.c_str(), NULL);
//        freq = freq1 * 1000000ULL;
//    }
//    return freq;
//}


//int ptp_shm_open()
//{
//    int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
//
//    if( shm_fd < 0) {
//        fprintf(stderr, "shm_open(). %s\n", strerror(errno));
//        return -1;
//    }
//    char *addr = (char*)mmap(NULL, SHM_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
//
//    if( addr == MAP_FAILED ) {
//        fprintf(stderr, "Error on mmap. Aborting.\n");
//        return -1;
//    }
//    fprintf(filepointer, "--------------------------------------------\n");
//    int buf_offset = 0;
//    buf_offset += sizeof(pthread_mutex_t);
//    ptpData = (gPtpTimeData*)(addr+buf_offset);
//    /*TODO: Scale to ns*/
////    uint64_t freq = getCpuFrequency();
////    printf("Frequency %lu Hz\n", freq);
//
//    fprintf(filepointer, "ml phoffset %ld\n", ptpData->ml_phoffset);
//    fprintf(filepointer, "ml freq offset %Lf\n", ptpData->ml_freqoffset);
//    fprintf(filepointer, "ls phoffset %ld\n", ptpData->ls_phoffset);
//    fprintf(filepointer, "ls freq offset %Lf\n", ptpData->ls_freqoffset);
//    fprintf(filepointer, "local time %lu\n", ptpData->local_time);
//    fprintf(filepointer, "sync count %u\n", ptpData->sync_count);
//    fprintf(filepointer, "pdelay count %u\n", ptpData->pdelay_count);
//    fprintf(filepointer, "asCapable %s\n", ptpData->asCapable ? "True" : "False");
//    fprintf(filepointer, "Port State %d\n", (int)ptpData->port_state);
//
//
//
//    return 0;
//}
//
//uint64_t ptp_client_get_time( )
//{
//	uint64_t now_local = 0, now_8021as=0;
//	uint64_t update_8021as=0;
//	unsigned delta_8021as=0, delta_local=0;
//
//	update_8021as = ptpData->local_time - ptpData->ml_phoffset;
//	delta_local = (unsigned)(now_local - ptpData->local_time);
//	delta_8021as = (unsigned)(ptpData->ml_freqoffset * delta_local);
//	now_8021as = update_8021as + delta_8021as;
//
//	return now_8021as;
//}




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

//    last_arrival_time = ptp_client_get_time( );

    return RETURN_VALUE_SUCCESS;
}


void delete_avb_Mediaclock_Listener( FILE* filepointer, ieee1722_avtp_driver_state_t **ieee1722mc )
{
}

int mediaclock_listener_wait_recv_ts( FILE* filepointer, ieee1722_avtp_driver_state_t **ieee1722mc, struct sockaddr_in **si_other_avb, struct pollfd **avtp_transport_socket_fds )
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


	struct iovec iov = { stream_packet, BUFLEN };
	struct msghdr msg = { (void*)((struct sockaddr *)(*si_other_avb)), slen_avb, &iov, 1, NULL, 0, 0 };

    fprintf(filepointer, "recvmsg...");fflush(filepointer);
	int status = recvmsg((*avtp_transport_socket_fds)->fd, &msg, NULL);
	fprintf(filepointer, "done\n");fflush(filepointer);

	if (status < 0) {
		fprintf(filepointer, "Error recvmsg: %d %d %s\n", status, errno, strerror(errno));fflush(filepointer);
		return -1;
	}

	if (status == 0) {
		fprintf(filepointer, "EOF\n");fflush(filepointer);
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
//        struct cmsghdr *cmsg = (struct cmsghdr *)malloc(sizeof(struct cmsghdr));
//        cmsg = CMSG_FIRSTHDR(&msg);
//        fprintf(filepointer, "stream packet! %d %d %d\n", cmsg->cmsg_len, cmsg->cmsg_level, cmsg->cmsg_type);fflush(filepointer);
//        for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)){
//            fprintf(filepointer, "stream packet!: %d %d\n", cmsg->cmsg_level, cmsg->cmsg_type);fflush(filepointer);
//            if (cmsg->cmsg_level != SOL_SOCKET)
//                continue;
//            switch (cmsg->cmsg_type){
//                case SO_TIMESTAMPING:{
//                        struct timespec* stamp = (struct timespec*)CMSG_DATA(cmsg); // timestamp is found
//                        fprintf(filepointer, "Timestamp %ld sec %ld nanosec\n", stamp->tv_sec, stamp->tv_nsec);fflush(filepointer);
//                        return 0;
//                    break;
//                }
//                default:
//                        fprintf(filepointer, "no timestamp\n");fflush(filepointer);
//                    break;
//            }
//        }
        fprintf(filepointer, "no timestamp\n");fflush(filepointer);
        return 0;

    }
    return -1;

}

int mediaclock_listener_wait_recv( FILE* filepointer, ieee1722_avtp_driver_state_t **ieee1722mc, struct sockaddr_in **si_other_avb, struct pollfd **avtp_transport_socket_fds )
{
	int recv_len=0;
	int rc;


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


            /* CMESG is better suited? */
            struct timeval tv_ioctl;
            tv_ioctl.tv_sec = 0;
            tv_ioctl.tv_usec = 0;
            int error = ioctl((*avtp_transport_socket_fds)->fd, SIOCGSTAMP, &tv_ioctl);



            /* inaccurate */
            uint64_t new_arrival_time = 0;//ptp_client_get_time( );
            uint64_t delta_arrival = new_arrival_time - last_arrival_time;


//                struct timeval sys_time;
//
//                if (clock_gettime(CLOCK_REALTIME, &sys_time)) {
//                    fprintf(filepointer, " Clockrealtime Error\n");fflush(filepointer);
////                    return RETURN_VALUE_FAILURE;
//                } else {
//                    fprintf(filepointer, " rx mc stream %lld \n", sys_time.tv_usec);fflush(filepointer);
////                    uint64_t ret = (uint64_t) sys_time.tv_usec;
////                    return ret;
//                }

            return delta_arrival;
        }
    }
    return -1;

}

int mediaclock_listener_poll_recv( FILE* filepointer, ieee1722_avtp_driver_state_t **ieee1722mc, struct sockaddr_in **si_other_avb, struct pollfd **avtp_transport_socket_fds )
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
             *      How many Channels and Samples per Packet => samplerate
             */




            uint64_t new_arrival_time = 0;//ptp_client_get_time( );
            uint64_t delta_arrival = new_arrival_time - last_arrival_time;





//                struct timeval sys_time;
//
//                if (clock_gettime(CLOCK_REALTIME, &sys_time)) {
//                    fprintf(filepointer, " Clockrealtime Error\n");fflush(filepointer);
////                    return RETURN_VALUE_FAILURE;
//                } else {
//                    fprintf(filepointer, " rx mc stream %lld \n", sys_time.tv_usec);fflush(filepointer);
////                    uint64_t ret = (uint64_t) sys_time.tv_usec;
////                    return ret;
//                }

                return delta_arrival;
            }
        }
    }
    return -1;

}
