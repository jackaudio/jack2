
#ifndef _MRP_SEND_MSG_H_
#define _MRP_SEND_MSG_H_

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

int mrpClient_send_mrp_msg(FILE* filepointer, int control_socket, char *notify_data, int notify_len);

#ifdef __cplusplus
}
#endif

#endif /* _MRP_SEND_MSG_H_ */
