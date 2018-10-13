
#ifndef MEDIACLOCKLISTENER_H
#define MEDIACLOCKLISTENER_H

#ifdef __cplusplus
extern "C"
{
#endif



#define _GNU_SOURCE


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

#include "avb_sockets.h"

#include "mrpClient_interface.h"
#include "mrpClient_control_socket.h"
#include "global_definitions.h"


int create_avb_Mediaclock_Listener( FILE* filepointer, ieee1722_avtp_driver_state_t **ieee1722mc, char* avb_dev_name,
                                    char* stream_id, char* destination_mac,
                                    struct sockaddr_in **si_other_avb, struct pollfd **avtp_transport_socket_fds);
void delete_avb_Mediaclock_Listener( FILE* filepointer, ieee1722_avtp_driver_state_t **ieee1722mc);

uint64_t mediaclock_listener_wait_recv_ts( FILE* filepointer, ieee1722_avtp_driver_state_t **ieee1722mc,
                                        struct sockaddr_in **si_other_avb, struct pollfd **avtp_transport_socket_fds, int packet_num  );

uint64_t mediaclock_listener_wait_recv( FILE* filepointer, ieee1722_avtp_driver_state_t **ieee1722mc,
                                        struct sockaddr_in **si_other_avb, struct pollfd **avtp_transport_socket_fds, int packet_num  );

uint64_t mediaclock_listener_poll_recv( FILE* filepointer, ieee1722_avtp_driver_state_t **ieee1722mc,
                                        struct sockaddr_in **si_other_avb, struct pollfd **avtp_transport_socket_fds, int packet_num  );

#ifdef __cplusplus
}
#endif
#endif //MEDIACLOCKLISTENER_H
