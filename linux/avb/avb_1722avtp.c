#include "avb_1722avtp.h"


#define SHM_SIZE (sizeof(gPtpTimeData) + sizeof(pthread_mutex_t))   /*!< Shared memory size*/
#define SHM_NAME  "/ptp"                                            /*!< Shared memory name*/
#define SHM_NAME_LENGTH 100


extern int errno;
FILE* filepointer;

int shm_fd;
mrp_ctx_t *mrp_ctx;
volatile int mrp_running = 1;

struct sockaddr_in *si_other_avb = NULL;
struct pollfd *avtp_transport_socket_fds = NULL;


/*
 * POSIX Shared Memory for Listener Context
 */
int mrp_shm_open(int _init)
{
	char shm_name[SHM_NAME_LENGTH];
	memset(shm_name, 0, SHM_NAME_LENGTH);

    sprintf(shm_name, "/mediaclock_jack_mrp_ctx");
    fprintf(filepointer,  "Open listener shm %s\n", shm_name);fflush(filepointer);


    if( _init == 1){
        if ((shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR)) == -1) {
            fprintf(filepointer,  "Open listener shm failed %s\n", strerror( errno ) );fflush(filepointer);
            return RETURN_VALUE_FAILURE;
        }
        fprintf(filepointer,  "Setting size of listener shm\n");fflush(filepointer);
        if( ftruncate( shm_fd, sizeof(mrp_ctx_t) ) == -1 ){
            fprintf(filepointer,  "Setting size of listener shm failed %s\n", strerror( errno ) );fflush(filepointer);
            return RETURN_VALUE_FAILURE;
        }


        if( MAP_FAILED == (mrp_ctx = (mrp_ctx_t*) mmap( 0, sizeof(mrp_ctx_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0 ) ) ){
            fprintf(filepointer,  "Get listener shm address failed %s\n", strerror( errno ) );fflush(filepointer);
            return RETURN_VALUE_FAILURE;
        }

        memset(mrp_ctx, 0, sizeof(mrp_ctx_t) );

    } else {
        if ((shm_fd = shm_open(shm_name, O_RDWR, S_IRUSR | S_IWUSR)) == -1) {
            fprintf(filepointer,  "Open listener shm failed %s\n", strerror( errno ) );fflush(filepointer);
            return RETURN_VALUE_FAILURE;
        }

        if( MAP_FAILED == (mrp_ctx = (mrp_ctx_t*) mmap( 0, sizeof(mrp_ctx_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0 ) ) ){
            fprintf(filepointer,  "Get listener shm address failed %s\n", strerror( errno ) );fflush(filepointer);
            return RETURN_VALUE_FAILURE;
        }
    }

    fprintf(filepointer,  "Segment listener shm pointer address key: %x\n", mrp_ctx );fflush(filepointer);

	return RETURN_VALUE_SUCCESS;
}




int mrp_shm_close(int _remove)
{
	char shm_name[SHM_NAME_LENGTH];
	memset(shm_name, 0, SHM_NAME_LENGTH);

    if( close( shm_fd ) == -1){
        fprintf(filepointer,  "Close listener shm failed %s\n", strerror( errno ) );fflush(filepointer);
        return RETURN_VALUE_FAILURE;
    }

    if( _remove ){
        sprintf(shm_name, "/mediaclock_jack");
        if( shm_unlink( shm_name ) == -1 ){
            fprintf(filepointer,  "Unlink listener shm failed %s\n", strerror( errno ) );fflush(filepointer);
            return RETURN_VALUE_FAILURE;
        }
    }


    return RETURN_VALUE_SUCCESS;
}



/*
 *       MRP Client Functions
 */
int check_stream_id(FILE* filepointer2, ieee1722_avtp_driver_state_t **ieee1722mc, int *buf_offset, char *buf)
{
	unsigned int streamid[8];
	int hit;

    fprintf(filepointer2,  "Event on Stream Id: ");fflush(filepointer2);
    for(int i = 0; i < 8 ; (*buf_offset)+=2, i++)		{
        sscanf(&buf[*buf_offset],"%02x",&streamid[i]);
        fprintf(filepointer2,  "%02x ", streamid[i]);fflush(filepointer2);
    }
    fprintf(filepointer2,  "\n");fflush(filepointer2);

    hit = 0;
    for(int i = 0; i < 8 ; i++)		{
        if( streamid[i] == (*ieee1722mc)->streamid8[i]){
            hit++;
        } else {
            break;
        }
    }
    if( hit == 8 ){
        fprintf(filepointer2,  " Message for Mediaclock Listener %llx with local Id %02x:%02x%02x:%02x%02x:%02x%02x:%02x\n",
                                (*ieee1722mc)->streamid8[0], (*ieee1722mc)->streamid8[1], (*ieee1722mc)->streamid8[2], (*ieee1722mc)->streamid8[3] ,
                                (*ieee1722mc)->streamid8[4], (*ieee1722mc)->streamid8[5], (*ieee1722mc)->streamid8[6], (*ieee1722mc)->streamid8[7]  );
        fflush(filepointer2);
        return RETURN_VALUE_SUCCESS;
    }

    return RETURN_VALUE_FAILURE;
}


int check_listener_dst_mac(FILE* filepointer2, ieee1722_avtp_driver_state_t **ieee1722mc, int *buf_offset, char *buf)
{
    unsigned int mac_addr[6];
    int hit = 0;

    fprintf(filepointer2,  "Check destination MAC... ");fflush(filepointer2);
    for(int i = 0; i < 6 ; (*buf_offset)+=2, i++)		{
        sscanf(&buf[*buf_offset],"%02x",&mac_addr[i]);
//        fprintf(filepointer2,  "%02x == %02x ?\n", mac_addr[i], (*ieee1722mc)->destination_mac_address[i]);fflush(filepointer2);
        if( mac_addr[i] == (*ieee1722mc)->destination_mac_address[i]){
            if( ++hit == 6 ){
                return RETURN_VALUE_SUCCESS;
            }
        } else {
            return RETURN_VALUE_FAILURE;
        }
    }
}


int find_next_line(FILE* filepointer2, char* buf, int* buf_offset, int buflen, int* buf_pos)
{
    fprintf(filepointer2,  " try to find a newline buflen %d bufpos %d  bufoffset %d\n", buflen, *buf_pos, *buf_offset);fflush(filepointer2);
    while (((*buf_offset) < buflen) && (buf[*buf_offset] != '\n') && (buf[*buf_offset] != '\0')){
//        fprintf(filepointer2,  " bufoffset %d buf %s\n", *buf_offset, &buf[*buf_offset]);fflush(filepointer2);
        (*buf_offset)++;
    }
    if ((*buf_offset) == buflen || buf[*buf_offset] == '\0'){
        fprintf(filepointer2,  " end of message buflen %d bufoffset %d\n", buflen, *buf_offset);fflush(filepointer2);
        return RETURN_VALUE_FAILURE;
    }
    *buf_pos = ++(*buf_offset);
    fprintf(filepointer2,  " found new line bufpos %d  bufoffset %d\n", *buf_pos, *buf_offset);fflush(filepointer2);

    return RETURN_VALUE_SUCCESS;
}


int process_mrp_msg(FILE* filepointer2, ieee1722_avtp_driver_state_t **ieee1722mc, char *buf, int buflen)
{
	/*
	 * 1st character indicates application
	 * [MVS] - MAC, VLAN or STREAM
	 */
	unsigned int id=0;
	unsigned int priority=0;
	unsigned int vid=0;
	int buf_offset=0;
	int buf_pos = 0;
	unsigned int substate=0;
	int dst_mac_match = 0;

next_line_listener:

	fprintf(filepointer2,  "%s", buf);fflush(filepointer2);

    fprintf(filepointer2,  "mrp status = %d\n", mrp_ctx->mrp_status);fflush(filepointer2);
	if (strncmp(buf, "SNE T:", 6) == 0 || strncmp(buf, "SJO T:", 6) == 0)	{
        fprintf(filepointer2,  " SNE T or SJO T: %s\n", buf);fflush(filepointer2);
		buf_offset = 6; /* skip "Sxx T:" */
		while ((buf_offset < buflen) && ('S' != buf[buf_offset++]));
		if (buf_offset == buflen)
			return RETURN_VALUE_FAILURE;
		buf_offset++;

		if( RETURN_VALUE_SUCCESS == check_stream_id(filepointer2, ieee1722mc, &buf_offset, buf)){
            buf_offset+=3;
            if( check_listener_dst_mac(filepointer2, ieee1722mc, &buf_offset, buf)
                        && mrp_ctx->mrp_status == LISTENER_WAITING ){
                mrp_ctx->mrp_status = LISTENER_READY;
//                return RETURN_VALUE_SUCCESS;
            }
		}
	} else
	if (strncmp(buf, "SJO D:", 6) == 0)	{
        fprintf(filepointer2,  " SJO D: %s\n", buf);fflush(filepointer2);

//		buf_offset=8;
		sscanf(&(buf[8]), "%d", &id);
//		buf_offset+=4;
		sscanf(&(buf[12]), "%d", &priority);
//		buf_offset+=4;
		sscanf(&(buf[16]), "%x", &vid);

		if( vid == 0 || priority == 0 || id == 0){
            fprintf(filepointer2,  " found 0-mvrp message ... skipping line\n");fflush(filepointer2);

            char* msgbuf2= malloc(1500);
            int rc=0;

            if (NULL == msgbuf2)		return RETURN_VALUE_FAILURE;

            memset(msgbuf2, 0, 1500);
            sprintf(msgbuf2, "V--:I=%04x",vid);
            fprintf(filepointer2, "Leave VLAN %s\n",msgbuf2);fflush(filepointer2);
        //	fclose(fp);

            rc = mrpClient_send_mrp_msg( filepointer2, mrpClient_get_Control_socket(), msgbuf2, 1500);
            free(msgbuf2);

            if( find_next_line( filepointer2, buf, &buf_offset, buflen, &buf_pos ) == RETURN_VALUE_FAILURE ){
                return RETURN_VALUE_SUCCESS;
            } else {
                goto next_line_listener;
            }
		}

        if (id == 6 && mrp_ctx->domain_a_valid == 0 ){
            // Class A
            mrp_ctx->domain_class_a_id = id;
            mrp_ctx->domain_class_a_priority = priority;
            mrp_ctx->domain_class_a_vid = vid;
            mrp_ctx->domain_a_valid = 1;
            fprintf(filepointer2,  " Domain A for Mediaclock Listener valid %x %x %x %x\n",
                                                        mrp_ctx->domain_class_a_id,
                                                        mrp_ctx->domain_class_a_priority,
                                                        mrp_ctx->domain_class_a_vid,
                                                        mrp_ctx->domain_a_valid);fflush(filepointer2);
        } else if (id == 5 && mrp_ctx->domain_b_valid == 0 ){
            // Class B
            mrp_ctx->domain_class_b_id = id;
            mrp_ctx->domain_class_b_priority = priority;
            mrp_ctx->domain_class_b_vid = vid;
            mrp_ctx->domain_b_valid = 1;
            fprintf(filepointer2,  " Domain B for Mediaclock Listener valid %x %x %x %x\n",
                                                        mrp_ctx->domain_class_b_id,
                                                        mrp_ctx->domain_class_b_priority,
                                                        mrp_ctx->domain_class_b_vid,
                                                        mrp_ctx->domain_b_valid);fflush(filepointer2);
        }

//		buf_offset+=4;
		return RETURN_VALUE_SUCCESS;

	}
	return RETURN_VALUE_SUCCESS;
}



int mrp_thread(ieee1722_avtp_driver_state_t **ieee1722mc)
{
	char *msgbuf;
	struct sockaddr_in client_addr;
	struct msghdr msg;
	struct iovec iov;
	int bytes = 0;
	struct pollfd fds;
	int rc;
	FILE* filepointer2;
	int argsCnt=0;
	struct timespec tim, tim2;

    /*
     *      Create Logfile
     */

	if( ! (filepointer2 = fopen("mrp.log", "w"))){
		printf("Error Opening file %d\n", errno);
		return RETURN_VALUE_FAILURE;
	}

	fprintf(filepointer2,  "%s malloc msgbuf\n");fflush(filepointer2);

	msgbuf = (char *)malloc(MAX_MRPD_CMDSZ);

	if (msgbuf == NULL ){
		fprintf(filepointer2,  "mrp_talker_monitor_thread - NULL == msgbuf\n");fflush(filepointer2);
		return RETURN_VALUE_FAILURE;
	}


    if(RETURN_VALUE_FAILURE == mrp_shm_open( 0 ) ){
        return RETURN_VALUE_FAILURE;
    }


    tim.tv_sec = 0;
    tim.tv_nsec = 10 * MILISLEEP_TIME;
    tim2.tv_sec = 0;
    tim2.tv_nsec = 0;

    fds.fd = mrpClient_get_Control_socket();
    fds.events = POLLIN;
    fds.revents = 0;

    /*
     *
     *
     *      Register this Client with MRP Daemon
     *
     */
	bool connect_to_daemon = true;

    /*
     *      Set RT Scheduling
     */
	pid_t own_pid = getpid();
    fprintf(filepointer2,  "PROCESS ID >>>>>>>>>>>> %d\n", own_pid);fflush(filepointer2);
#ifdef EDF_TASK_SCHEDULING
    if( set_rt_scheduling( filepointer2, own_pid, SCHED_DEADLINE, 10000/*10us*/, 49000/*49us*/, 50000/*50us*/ ) == RETURN_VALUE_FAILURE){
        set_rt_scheduling( filepointer2, own_pid, SCHED_RR, -9, 67, 0 );
    }
#else
//    set_rt_scheduling( filepointer2, own_pid, SCHED_RR, -9, 67, 0 );
#endif // EDF_TASK_SCHEDULING



	fprintf(filepointer2,  "loop\n");fflush(filepointer2);
	while ( mrp_running ) {

        if( connect_to_daemon ){
            fprintf(filepointer2,  "connect to MRP daemon...");fflush(filepointer2);
            memset(&msg, 0, sizeof(msg));
            memset(&client_addr, 0, sizeof(client_addr));
            memset(msgbuf, 0, MAX_MRPD_CMDSZ);
            iov.iov_len = MAX_MRPD_CMDSZ;
            iov.iov_base = msgbuf;
            msg.msg_name = &client_addr;
            msg.msg_namelen = sizeof(client_addr);
            msg.msg_iov = &iov;
            msg.msg_iovlen = 1;

            sprintf(msgbuf, "S??");
            rc = mrpClient_send_mrp_msg(filepointer2, mrpClient_get_Control_socket(), msgbuf, 1500);
            connect_to_daemon = false;
            fprintf(filepointer2,  "successfully.\n");fflush(filepointer2);

        }

		if( (rc = poll(&fds, 1, 100) ) == 0) continue;
		if( (rc < 0) || ( (fds.revents & POLLIN) == 0 ) ) {
			fprintf(filepointer2,  "mrp client process: rc = poll(&fds, 1, 100) failed\n");fflush(filepointer2);
			break;
		}

		memset(&msg, 0, sizeof(msg));
		memset(&client_addr, 0, sizeof(client_addr));
		memset(msgbuf, 0, MAX_MRPD_CMDSZ);
		iov.iov_len = MAX_MRPD_CMDSZ;
		iov.iov_base = msgbuf;
		msg.msg_name = &client_addr;
		msg.msg_namelen = sizeof(client_addr);
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;

		if( (bytes = recvmsg(mrpClient_get_Control_socket(), &msg, 0)) <=0) continue;

		fprintf( filepointer2,  "\nprocess_mrp_msg %d:\n", bytes);fflush(filepointer2);

        int ret = process_mrp_msg(filepointer2, ieee1722mc, msgbuf, bytes);
		fprintf( filepointer2,  "\n\n");fflush(filepointer2);


#ifndef EDF_TASK_SCHEDULING
        nanosleep(&tim , &tim2);
#else
			sched_yield();
#endif // EDF_TASK_SCHEDULING
	}

    fprintf(filepointer2,  "quit_mrp: quitting\n");fflush(filepointer2);

	if (NULL == msgbuf){
		fprintf(filepointer2,  "%s NULL == msgbuf - rc != 1500\t LISTENER_FAILED\n");fflush(filepointer2);
		return RETURN_VALUE_FAILURE;
	}

	memset(msgbuf, 0, 1500);
	sprintf(msgbuf, "BYE");

	rc = mrpClient_send_mrp_msg(filepointer2, mrpClient_get_Control_socket(), msgbuf, 1500);
	free(msgbuf);

	if (rc != 1500)
		return RETURN_VALUE_FAILURE;
	else
		return RETURN_VALUE_SUCCESS;

	free(msgbuf);
	fclose(filepointer2);
	mrp_shm_close(0);

    return RETURN_VALUE_SUCCESS;
}

void *worker_thread_mrp(void* v_ieee1722mc)
{
	ieee1722_avtp_driver_state_t *t_ieee1722mc = (ieee1722_avtp_driver_state_t *) v_ieee1722mc;
    mrp_thread( &t_ieee1722mc );

}

/*
 *      IEEE1722 AVTP Mediaclock Listener Backend
 */







int init_1722_driver( ieee1722_avtp_driver_state_t *ieee1722mc, const char* name,
                        char* stream_id, char* destination_mac,
                        int sample_rate, int period_size, int num_periods, int capture_ports, int playback_ports)
{
	char filename[100];
    sprintf(filename, "jack1722driver.log");
    if( ! (filepointer = fopen(filename, "w"))){
        printf("Error Opening file %d\n", errno);
        fclose(filepointer);
        return /*EXIT_FAILURE*/ -1;
    }

    if(RETURN_VALUE_FAILURE == mrp_shm_open( 1 ) ){
        return RETURN_VALUE_FAILURE;
    }


    if( mrpClient_init_Control_socket( filepointer ) == RETURN_VALUE_FAILURE ) {
        fprintf(filepointer,  "Error initializing MRP socket\n");fflush(filepointer);
        fclose(filepointer);
        return /*EXIT_FAILURE*/ -1;
    }

    if( pthread_create( &ieee1722mc->thread, NULL, (&worker_thread_mrp), (void*) ieee1722mc ) != 0 ) {
        fprintf(filepointer,  "Error creating thread\n");fflush(filepointer);
        fclose(filepointer);
        return /*EXIT_FAILURE*/ -1;
    } else {
        fprintf(filepointer,  "Success creating thread\n");fflush(filepointer);
    }

    fprintf(filepointer, "JackAVBPDriver::JackAVBPDriver Ethernet Device %s\n", name);fflush(filepointer);

    fprintf(filepointer, "Stream ID: %02x %02x %02x %02x %02x %02x %02x %02x\n",
                                                                (uint8_t) stream_id[0],
                                                                (uint8_t) stream_id[1],
                                                                (uint8_t) stream_id[2],
                                                                (uint8_t) stream_id[3],
                                                                (uint8_t) stream_id[4],
                                                                (uint8_t) stream_id[5],
                                                                (uint8_t) stream_id[6],
                                                                (uint8_t) stream_id[7]);fflush(filepointer);

    fprintf(filepointer, "Destination MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
                                                                (uint8_t) destination_mac[0],
                                                                (uint8_t) destination_mac[1],
                                                                (uint8_t) destination_mac[2],
                                                                (uint8_t) destination_mac[3],
                                                                (uint8_t) destination_mac[4],
                                                                (uint8_t) destination_mac[5]);fflush(filepointer);

    ieee1722mc->playback_channels = playback_ports;
    ieee1722mc->capture_channels = capture_ports;
    ieee1722mc->sample_rate = sample_rate;
    ieee1722mc->period_size = period_size;
    ieee1722mc->period_usecs = (uint64_t) ((float)period_size / (float)sample_rate * 1000000);

    ieee1722mc->num_packets = (int)( ieee1722mc->period_size / 6 ) + 1;

    fprintf(filepointer,"sample_rate: %d, period size: %d, period usec: %lld, num_packets: %d\n",ieee1722mc->sample_rate, ieee1722mc->period_size, ieee1722mc->period_usecs, ieee1722mc->num_packets);fflush(filepointer);

    if( RETURN_VALUE_FAILURE == create_avb_Mediaclock_Listener(filepointer, &ieee1722mc, name,
                                   stream_id, destination_mac,
                                   &si_other_avb, &avtp_transport_socket_fds)){
        fprintf(filepointer,  "Creation failed!\n");fflush(filepointer);
        fclose(filepointer);
        return /*EXIT_FAILURE*/ -1;
	}



    if( RETURN_VALUE_FAILURE == mrpClient_listener_send_leave(filepointer, &ieee1722mc, mrp_ctx )){
        fprintf(filepointer,  "send leave failed\n");fflush(filepointer);
    } else {
        fprintf(filepointer,  "send leave success\n");fflush(filepointer);
    }


	if( RETURN_VALUE_FAILURE == mrpClient_getDomain_joinVLAN( filepointer,  ieee1722mc, mrp_ctx) ){
        fprintf(filepointer,  "mrpClient_getDomain_joinVLAN failed\n");fflush(filepointer);
        fclose(filepointer);
        return /*EXIT_FAILURE*/ -1;
	}

    return 0;
}

int startup_1722_driver( ieee1722_avtp_driver_state_t *ieee1722mc )
{
    if( RETURN_VALUE_FAILURE == mrpClient_listener_await_talker( filepointer, &ieee1722mc, mrp_ctx)){
        fprintf(filepointer,  "mrpClient_listener_await_talker failed\n");fflush(filepointer);
        fclose(filepointer);
        return /*EXIT_FAILURE*/ -1;
    } else {
        return 0;
    }
}

uint64_t poll_recv_1722_mediaclockstream( ieee1722_avtp_driver_state_t *ieee1722mc, int packet_num  )
{
    return mediaclock_listener_poll_recv( filepointer, &ieee1722mc, &si_other_avb, &avtp_transport_socket_fds, packet_num  );
}

uint64_t wait_recv_1722_mediaclockstream( ieee1722_avtp_driver_state_t *ieee1722mc, int packet_num  )
{
    return mediaclock_listener_wait_recv( filepointer, &ieee1722mc, &si_other_avb, &avtp_transport_socket_fds, packet_num  );
}

uint64_t wait_recv_ts_1722_mediaclockstream( ieee1722_avtp_driver_state_t *ieee1722mc, int packet_num  )
{
    return mediaclock_listener_wait_recv_ts( filepointer, &ieee1722mc, &si_other_avb, &avtp_transport_socket_fds, packet_num  );
}


int shutdown_1722_driver( ieee1722_avtp_driver_state_t *ieee1722mc )
{

    if( RETURN_VALUE_FAILURE == mrpClient_listener_send_leave(filepointer, &ieee1722mc, mrp_ctx )){
        fprintf(filepointer,  "send leave failed\n");fflush(filepointer);
    } else {
        fprintf(filepointer,  "send leave success\n");fflush(filepointer);
    }

    // stop mrp thread
    mrp_running = 0;
    pthread_join(ieee1722mc->thread, NULL);

    mrp_shm_close( 1 );

    delete_avb_Mediaclock_Listener( filepointer, &ieee1722mc );
    fclose(filepointer);
    return 0;
}
