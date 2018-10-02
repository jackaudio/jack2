#ifndef _MRP_CL_IF_H_
#define _MRP_CL_IF_H_

#ifdef __cplusplus
extern "C"
{
#endif


#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h> // needed for sysconf(int name);
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/errno.h>

#include "global_definitions.h"
#include "mrpClient_control_socket.h"
#include "mrpClient_send_msg.h"


int mrpClient_getDomain_joinVLAN(FILE* filepointer, ieee1722_avtp_driver_state_t **ieee1722mc, mrp_ctx_t *mrp_ctx);

int mrpClient_listener_await_talker(FILE* filepointer,ieee1722_avtp_driver_state_t **ieee1722mc, mrp_ctx_t *mrp_ctx);
int mrpClient_listener_send_ready(FILE* filepointer,ieee1722_avtp_driver_state_t **ieee1722mc, mrp_ctx_t *mrp_ctx);
int mrpClient_listener_send_leave(FILE* filepointer,ieee1722_avtp_driver_state_t **ieee1722mc, mrp_ctx_t *mrp_ctx);


#ifdef __cplusplus
}
#endif
#endif /* _MRP_CL_IF_H_ */
