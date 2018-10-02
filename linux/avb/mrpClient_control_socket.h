#ifndef _MRP_CONTROL_SOCKET_H_
#define _MRP_CONTROL_SOCKET_H_

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

#include "global_definitions.h"
#include "mrpClient_send_msg.h"

int mrpClient_get_Control_socket( );
int mrpClient_init_Control_socket( FILE* filepointer );


#endif /* _MRP_CONTROL_SOCKET_H_ */
