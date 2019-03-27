#ifndef _MRP_CL_IF_H_
#define _MRP_CL_IF_H_

#ifdef __cplusplus
extern "C"
{
#endif

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

#include "avb_definitions.h"
#include "mrp_client_control_socket.h"
#include "mrp_client_send_msg.h"


int mrp_client_getDomain_joinVLAN(FILE* filepointer, avb_driver_state_t **avb_ctx, mrp_ctx_t *mrp_ctx);

int mrp_client_listener_await_talker(FILE* filepointer,avb_driver_state_t **avb_ctx, mrp_ctx_t *mrp_ctx);
int mrp_client_listener_send_ready(FILE* filepointer,avb_driver_state_t **avb_ctx, mrp_ctx_t *mrp_ctx);
int mrp_client_listener_send_leave(FILE* filepointer,avb_driver_state_t **avb_ctx, mrp_ctx_t *mrp_ctx);


#ifdef __cplusplus
}
#endif
#endif /* _MRP_CL_IF_H_ */
