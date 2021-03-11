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

#include "avb.h"
#include "mrp_client_control_socket.h"
#include "mrp_client_interface.h"
#include "mrp_client_send_msg.h"

#define SHM_SIZE (sizeof(gPtpTimeData) + sizeof(pthread_mutex_t))   // Shared memory size
#define SHM_NAME  "/ptp"                                            // Shared memory name
#define SHM_NAME_LENGTH 100

extern int errno;
FILE* filepointer;
int shm_fd;
mrp_ctx_t *mrp_ctx;
volatile int mrp_running = 1;
struct sockaddr_in *si_other_avb = NULL;
struct pollfd *avtp_transport_socket_fds = NULL;

// POSIX Shared Memory for Listener Context
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

// MRP Client Functions
int check_stream_id(FILE* filepointer2, avb_driver_state_t **avb_ctx, int *buf_offset, char *buf)
{
    unsigned int streamid[8];
    int hit = 0;
    int i = 0;

    fprintf(filepointer2,  "Event on Stream Id: ");fflush(filepointer2);
    for(i = 0; i < 8 ; (*buf_offset)+=2, i++)        {
        sscanf(&buf[*buf_offset],"%02x",&streamid[i]);
        fprintf(filepointer2,  "%02x ", streamid[i]);fflush(filepointer2);
    }
    fprintf(filepointer2,  "\n");fflush(filepointer2);

    hit = 0;
    for(i = 0; i < 8 ; i++)        {
        if( streamid[i] == (*avb_ctx)->streamid8[i]){
            hit++;
        } else {
            break;
        }
    }
    if( hit == 8 ){
        fprintf(filepointer2,  " Message for media clock listener stream Id %02x:%02x%02x:%02x%02x:%02x%02x:%02x\n",
                                                        (*avb_ctx)->streamid8[0], (*avb_ctx)->streamid8[1],
                                                        (*avb_ctx)->streamid8[2], (*avb_ctx)->streamid8[3],
                                                        (*avb_ctx)->streamid8[4], (*avb_ctx)->streamid8[5],
                                                        (*avb_ctx)->streamid8[6], (*avb_ctx)->streamid8[7]);
        fflush(filepointer2);
        return RETURN_VALUE_SUCCESS;
    }
    return RETURN_VALUE_FAILURE;
}

int check_listener_dst_mac(FILE* filepointer2, avb_driver_state_t **avb_ctx, int *buf_offset, char *buf)
{
    unsigned int mac_addr[6];
    int hit = 0;
    int i = 0;

    fprintf(filepointer2,  "Check destination MAC... ");fflush(filepointer2);
    for(i = 0; i < 6 ; (*buf_offset)+=2, i++)        {
        sscanf(&buf[*buf_offset],"%02x",&mac_addr[i]);
//        fprintf(filepointer2,  "%02x == %02x ?\n", mac_addr[i], (*avb_ctx)->destination_mac_address[i]);fflush(filepointer2);
        if( mac_addr[i] == (*avb_ctx)->destination_mac_address[i]){
            if( ++hit == 6 ){
                return RETURN_VALUE_SUCCESS;
            }
        } else {
            return RETURN_VALUE_FAILURE;
        }
    }
    return RETURN_VALUE_FAILURE;
}

int find_next_line(FILE* filepointer2, char* buf, int* buf_offset, int buflen, int* buf_pos)
{
    fprintf(filepointer2,  " try to find a newline buflen %d bufpos %d  bufoffset %d\n",
                                        buflen, *buf_pos, *buf_offset);fflush(filepointer2);
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

int process_mrp_msg(FILE* filepointer2, avb_driver_state_t **avb_ctx, char *buf, int buflen)
{
    unsigned int id=0;
    unsigned int priority=0;
    unsigned int vid=0;
    int buf_offset=0;
    int buf_pos = 0;

next_line_listener:

    fprintf(filepointer2,  "%s", buf);fflush(filepointer2);
    fprintf(filepointer2,  "mrp status = %d\n", mrp_ctx->mrp_status);fflush(filepointer2);

    // 1st character indicates application
    // [MVS] - MAC, VLAN or STREAM
    if (strncmp(buf, "SNE T:", 6) == 0 || strncmp(buf, "SJO T:", 6) == 0)    {
        fprintf(filepointer2,  " SNE T or SJO T: %s\n", buf);fflush(filepointer2);
        buf_offset = 6; // skip "Sxx T:"
        while ((buf_offset < buflen) && ('S' != buf[buf_offset++]));
        if (buf_offset == buflen)
            return RETURN_VALUE_FAILURE;
        buf_offset++;

        if( RETURN_VALUE_SUCCESS == check_stream_id(filepointer2, avb_ctx, &buf_offset, buf)){
            buf_offset+=3;
            if( check_listener_dst_mac(filepointer2, avb_ctx, &buf_offset, buf)
                        && mrp_ctx->mrp_status == LISTENER_WAITING ){
                mrp_ctx->mrp_status = LISTENER_READY;
//                return RETURN_VALUE_SUCCESS;
            }
        }
    } else
    if (strncmp(buf, "SJO D:", 6) == 0)    {
        fprintf(filepointer2,  " SJO D: %s\n", buf);fflush(filepointer2);

//        buf_offset=8;
        sscanf(&(buf[8]), "%d", &id);
//        buf_offset+=4;
        sscanf(&(buf[12]), "%d", &priority);
//        buf_offset+=4;
        sscanf(&(buf[16]), "%x", &vid);

        if( vid == 0 || priority == 0 || id == 0){
            fprintf(filepointer2,  " found 0-mvrp message ... skipping line\n");fflush(filepointer2);

            char* msgbuf2= malloc(1500);

            if (NULL == msgbuf2)        return RETURN_VALUE_FAILURE;

            memset(msgbuf2, 0, 1500);
            sprintf(msgbuf2, "V--:I=%04x",vid);
            fprintf(filepointer2, "Leave VLAN %s\n",msgbuf2);fflush(filepointer2);

            mrp_client_send_mrp_msg( filepointer2, mrp_client_get_Control_socket(), msgbuf2, 1500);
            free(msgbuf2);

            if( find_next_line( filepointer2, buf, &buf_offset, buflen, &buf_pos ) == RETURN_VALUE_FAILURE ){
                return RETURN_VALUE_SUCCESS;
            } else {
                goto next_line_listener;
            }
        }

        if (id == 6 && mrp_ctx->domain_a_valid == 0 ){ // Class A
            mrp_ctx->domain_class_a_id = id;
            mrp_ctx->domain_class_a_priority = priority;
            mrp_ctx->domain_class_a_vid = vid;
            mrp_ctx->domain_a_valid = 1;
            fprintf(filepointer2,  " Domain A for Mediaclock Listener valid %x %x %x %x\n",
                                                        mrp_ctx->domain_class_a_id,
                                                        mrp_ctx->domain_class_a_priority,
                                                        mrp_ctx->domain_class_a_vid,
                                                        mrp_ctx->domain_a_valid);fflush(filepointer2);
        } else if (id == 5 && mrp_ctx->domain_b_valid == 0 ){ // Class B
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
//        buf_offset+=4;
        return RETURN_VALUE_SUCCESS;
    }
    return RETURN_VALUE_SUCCESS;
}

int mrp_thread(avb_driver_state_t **avb_ctx)
{
    char *msgbuf;
    struct sockaddr_in client_addr;
    struct msghdr msg;
    struct iovec iov;
    int bytes = 0;
    struct pollfd fds;
    int rc;
    FILE* filepointer2;
    struct timespec tim, tim2;
    int cnt_sec_to_listener_ready = 0;

    if( ! (filepointer2 = fopen("mrp.log", "w"))){
        printf("Error Opening file %d\n", errno);
        return RETURN_VALUE_FAILURE;
    }

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

    fds.fd = mrp_client_get_Control_socket();
    fds.events = POLLIN;
    fds.revents = 0;

    // Register this Client with MRP Daemon
    bool connect_to_daemon = true;

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
            rc = mrp_client_send_mrp_msg(filepointer2, mrp_client_get_Control_socket(), msgbuf, 1500);
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

        if( (bytes = recvmsg(mrp_client_get_Control_socket(), &msg, 0)) <=0) continue;

        fprintf( filepointer2,  "\nprocess_mrp_msg %d:\n", bytes);fflush(filepointer2);

        process_mrp_msg(filepointer2, avb_ctx, msgbuf, bytes);
        fprintf( filepointer2,  "\n\n");fflush(filepointer2);




        if( 900 == cnt_sec_to_listener_ready++) {
            if ( mrp_client_listener_send_ready( filepointer2, avb_ctx, mrp_ctx ) > RETURN_VALUE_FAILURE) {
                fprintf(filepointer2,  "send_ready success\n");fflush(filepointer2);
            }
        }











        nanosleep(&tim , &tim2);
    }

    fprintf(filepointer2,  "quit_mrp: quitting\n");fflush(filepointer2);

    if (NULL == msgbuf){
        fprintf(filepointer2,  "LISTENER_FAILED\n");fflush(filepointer2);
        return RETURN_VALUE_FAILURE;
    }

    memset(msgbuf, 0, 1500);
    sprintf(msgbuf, "BYE");

    rc = mrp_client_send_mrp_msg(filepointer2, mrp_client_get_Control_socket(), msgbuf, 1500);
    free(msgbuf);
    fclose(filepointer2);
    mrp_shm_close(0);

    if (rc != 1500)
        return RETURN_VALUE_FAILURE;
    else
        return RETURN_VALUE_SUCCESS;
}

void *worker_thread_mrp(void* v_avb_ctx)
{
    avb_driver_state_t *t_avb_ctx = (avb_driver_state_t *) v_avb_ctx;
    mrp_thread( &t_avb_ctx );
    pthread_exit(0);
}

// AVB Backend
int init_avb_driver( avb_driver_state_t *avb_ctx, const char* name,
                        char* stream_id, char* destination_mac,
                        int sample_rate, int period_size, int num_periods, int adjust, int capture_ports, int playback_ports)
{
    char filename[100];
    sprintf(filename, "jackAVBdriver.log");
    if( ! (filepointer = fopen(filename, "w"))){
        printf("Error Opening file %d\n", errno);
        fclose(filepointer);
        return -1; // EXIT_FAILURE
    }

    if(RETURN_VALUE_FAILURE == mrp_shm_open( 1 ) ){
        return -1; // EXIT_FAILURE
    }

    if( mrp_client_init_Control_socket( filepointer ) == RETURN_VALUE_FAILURE ) {
        fprintf(filepointer,  "Error initializing MRP socket\n");fflush(filepointer);
        fclose(filepointer);
        return -1; // EXIT_FAILURE
    }

    if( pthread_create( &avb_ctx->thread, NULL, (&worker_thread_mrp), (void*) avb_ctx ) != 0 ) {
        fprintf(filepointer,  "Error creating thread\n");fflush(filepointer);
        fclose(filepointer);
        return -1; // EXIT_FAILURE
    } else {
        fprintf(filepointer,  "Success creating thread\n");fflush(filepointer);
    }

    fprintf(filepointer, "JackAVBDriver::JackAVBPDriver Ethernet Device %s\n", name);fflush(filepointer);
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
    avb_ctx->playback_channels = playback_ports;
    avb_ctx->capture_channels = capture_ports;
    avb_ctx->adjust = adjust;
    avb_ctx->sample_rate = sample_rate;
    avb_ctx->period_size = period_size;
    avb_ctx->period_usecs = (uint64_t) ((float)period_size / (float)sample_rate * 1000000);
    avb_ctx->num_packets = (int)( avb_ctx->period_size / 6 ) + 1;

    fprintf(filepointer,"sample_rate: %d, period size: %d, period usec: %lud, num_packets: %d\n",
                                                avb_ctx->sample_rate, avb_ctx->period_size,
                                                avb_ctx->period_usecs, avb_ctx->num_packets);fflush(filepointer);

    if( RETURN_VALUE_FAILURE == avtp_mcl_create(filepointer, &avb_ctx, name,
                                   stream_id, destination_mac,
                                   &si_other_avb, &avtp_transport_socket_fds)){
        fprintf(filepointer,  "Creation failed!\n");fflush(filepointer);
        fclose(filepointer);
        return -1; // EXIT_FAILURE
    }

    if( RETURN_VALUE_FAILURE == mrp_client_listener_send_leave(filepointer, &avb_ctx, mrp_ctx )){
        fprintf(filepointer,  "send leave failed\n");fflush(filepointer);
    } else {
        fprintf(filepointer,  "send leave success\n");fflush(filepointer);
    }

    if( RETURN_VALUE_FAILURE == mrp_client_getDomain_joinVLAN( filepointer, &avb_ctx, mrp_ctx) ){
        fprintf(filepointer,  "mrp_client_getDomain_joinVLAN failed\n");fflush(filepointer);
        fclose(filepointer);
        return -1; // EXIT_FAILURE
    }
    return 0; // EXIT_SUCCESS
}


int startup_avb_driver( avb_driver_state_t *avb_ctx )
{
    if( RETURN_VALUE_FAILURE == mrp_client_listener_await_talker( filepointer, &avb_ctx, mrp_ctx)){
        fprintf(filepointer,  "mrp_client_listener_await_talker failed\n");fflush(filepointer);
        fclose(filepointer);
        return -1; // EXIT_FAILURE
    } else {
        return 0; // EXIT_SUCCESS
    }
}

uint64_t await_avtp_rx_ts( avb_driver_state_t *avb_ctx, int packet_num, uint64_t *lateness )
{
//    return avtp_mcl_wait_for_rx_ts( filepointer, &avb_ctx, &si_other_avb, &avtp_transport_socket_fds, packet_num );
    return avtp_mcl_wait_for_rx_ts_const( filepointer, &avb_ctx, &si_other_avb, &avtp_transport_socket_fds, packet_num, lateness );
}

int shutdown_avb_driver( avb_driver_state_t *avb_ctx )
{
    if( RETURN_VALUE_FAILURE == mrp_client_listener_send_leave(filepointer, &avb_ctx, mrp_ctx )){
        fprintf(filepointer,  "send leave failed\n");fflush(filepointer);
    } else {
        fprintf(filepointer,  "send leave success\n");fflush(filepointer);
    }

    mrp_running = 0;
    pthread_join(avb_ctx->thread, NULL);
    mrp_shm_close( 1 );
    avtp_mcl_delete( filepointer, &avb_ctx );
    fclose(filepointer);
    return 0; // EXIT_SUCCESS
}
