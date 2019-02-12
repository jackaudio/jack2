#include "listener_mediaclock.h"
extern int errno;

static uint64_t last_packet_time_ns = 0;

pthread_t writerThread;
mqd_t tsq_tx;

void *worker_thread_listener_fileWriter()
{
	struct timespec tim;
    FILE* filepointer;

	tim.tv_sec = 0;
	tim.tv_nsec = 300000;

	if( ! (filepointer = fopen("mcs_ts.log", "w")) ){
		printf("Error Opening file %d\n", errno);
		pthread_exit((void*)-1);
	}


    fprintf(filepointer, "Started Filewriter Thread %d\n", sizeof(uint64_t));fflush(filepointer);

	mqd_t tsq_rx = mq_open(Q_NAME, O_RDWR | O_NONBLOCK);
    char msg_recv[Q_MSG_SIZE];


    while(1){

        if ( mq_receive(tsq_rx, msg_recv, Q_MSG_SIZE, NULL) > 0) {
    		fprintf(filepointer, "%s\n",msg_recv);fflush(filepointer);
        } else {
            if(errno != EAGAIN){
                fprintf(filepointer, "recv error %d %s %s\n", errno, strerror(errno), msg_recv);fflush(filepointer);
            }
        }
        nanosleep(&tim , NULL);
    }
    fclose(filepointer);
}


int create_avb_Mediaclock_Listener( FILE* filepointer, ieee1722_avtp_driver_state_t **ieee1722mc, char* avb_dev_name,
                                    char* stream_id, char* destination_mac,
                                    struct sockaddr_in **si_other_avb, struct pollfd **avtp_transport_socket_fds)
{
	fprintf(filepointer,  "Create Mediaclock Listener\n");fflush(filepointer);

	struct mq_attr attr;
	attr.mq_flags = 0;
	attr.mq_maxmsg = 1000;
	attr.mq_msgsize = Q_MSG_SIZE;
	attr.mq_curmsgs = 0;


    if( mq_unlink(Q_NAME) < 0) {
        printf("unlink %s error %d %s\n", Q_NAME, errno, strerror(errno));fflush(stdout);
    } else {
         printf("unlink %s success\n", Q_NAME );fflush(stdout);
    }

	if ((tsq_tx = mq_open(Q_NAME, O_RDWR | O_CREAT | O_NONBLOCK | O_EXCL, 0666, &attr)) == -1)  {
		printf("create error %s %d %s\n", Q_NAME, errno, strerror(errno));fflush(stdout);
	} else {
        printf("create success %s\n", Q_NAME);fflush(stdout);
	}

    if( pthread_create( &writerThread, NULL, (&worker_thread_listener_fileWriter), NULL) != 0 ) {
        printf("Error creating thread\n");fflush(stdout);
    }



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

uint64_t mediaclock_listener_wait_recv_ts( FILE* filepointer, ieee1722_avtp_driver_state_t **ieee1722mc,
                                            struct sockaddr_in **si_other_avb, struct pollfd **avtp_transport_socket_fds,
                                            int packet_num )
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

        /* Packet Arrival Time from Device */
        cmsg = CMSG_FIRSTHDR(&msg);
        while( cmsg != NULL ) {
            if( cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SO_TIMESTAMPING ) {
                struct timespec *ts_device, *ts_system;
                ts_system = ((struct timespec *) CMSG_DATA(cmsg)) + 1;
                ts_device = ts_system + 1;
//                fprintf(filepointer, "Device %lld sec %lld nanosec\n", ts_device->tv_sec, ts_device->tv_nsec);fflush(filepointer);

                packet_arrival_time_ns =  (ts_device->tv_sec*1000000000LL + ts_device->tv_nsec);

//                char msg_send[Q_MSG_SIZE];
//                memset(msg_send, 0, Q_MSG_SIZE);
//                sprintf (msg_send, "%lld", packet_arrival_time_ns);
//
//                if (mq_send(tsq_tx, msg_send, Q_MSG_SIZE, 0) < 0) {
//            //		fprintf(filepointer, "send error %d %s %s\n", errno, strerror(errno), msg_send);fflush(filepointer);
//                }

                break;
            }
            cmsg = CMSG_NXTHDR(&msg,cmsg);
        }

        ipg_to_last_packet_ns = packet_arrival_time_ns - last_packet_time_ns;
        last_packet_time_ns = packet_arrival_time_ns;
//		fprintf(filepointer, "packet arrival time %lld ns, ipg %lld ns\n", packet_arrival_time_ns, ipg_to_last_packet_ns);fflush(filepointer);


        if( packet_num == (*ieee1722mc)->num_packets -1){

            adjust_packet_time_ns = (uint64_t) ( ( (float)((*ieee1722mc)->period_size % 6 ) / (float)(*ieee1722mc)->sample_rate ) * 1000000000LL);
//            fprintf(filepointer, "adjust time %lld ns\n", adjust_packet_time_ns);fflush(filepointer);
        } else {
            adjust_packet_time_ns = (*ieee1722mc)->adjust ? ipg_to_last_packet_ns : 125000;
        }
//        fprintf(filepointer, "adjust time %lld ns\n", adjust_packet_time_ns);fflush(filepointer);

        return adjust_packet_time_ns;
    }
    return -1;

}
