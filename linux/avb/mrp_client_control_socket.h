#ifndef _MRP_CONTROL_SOCKET_H_
#define _MRP_CONTROL_SOCKET_H_

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

#include <arpa/inet.h>

#include "avb_definitions.h"
#include "mrp_client_send_msg.h"

int mrp_client_get_Control_socket( );
int mrp_client_init_Control_socket( FILE* filepointer );


#ifdef __cplusplus
}
#endif
#endif /* _MRP_CONTROL_SOCKET_H_ */
