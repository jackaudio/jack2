#ifndef JACK1722DRIVER_H
#define JACK1722DRIVER_H

#ifdef __cplusplus
extern "C"
{
#endif


#define _GNU_SOURCE

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/mman.h> // Needed for mlockall()
#include <sys/time.h> // needed for getrusage
#include <sys/resource.h> // needed for getrusage
#include <unistd.h> // needed for sysconf(int name);
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/errno.h>

#include <sched.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <linux/sched.h>

#include "global_definitions.h"
#include "listener_mediaclock.h"
#include "mrpClient_control_socket.h"
#include "mrpClient_send_msg.h"




int init_1722_driver(   ieee1722_avtp_driver_state_t *ieee1722mc, const char* name,
                        char* stream_id, char* destination_mac,
                        int sample_rate, int period_size, int num_periods);
int startup_1722_driver( ieee1722_avtp_driver_state_t *ieee1722mc);
int poll_recv_1722_mediaclockstream( ieee1722_avtp_driver_state_t *ieee1722mc );
int wait_recv_1722_mediaclockstream( ieee1722_avtp_driver_state_t *ieee1722mc );
int wait_recv_ts_1722_mediaclockstream( ieee1722_avtp_driver_state_t *ieee1722mc );
int shutdown_1722_driver( ieee1722_avtp_driver_state_t *ieee1722mc);


#ifdef __cplusplus
}
#endif
#endif //JACK1722DRIVER_H
