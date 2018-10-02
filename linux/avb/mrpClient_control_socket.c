#include "mrpClient_control_socket.h"

static int control_socket = -1;



int mrpClient_get_Control_socket()
{
    return control_socket;
}

int mrpClient_init_Control_socket( FILE* filepointer )
{

	/** in POSIX fd 0,1,2 are reserved */
//	if (2 > (*ieee1722mc)->mrp_ctx.control_socket)	{
//		if (-1 > (*ieee1722mc)->mrp_ctx.control_socket)
//			close((*ieee1722mc)->mrp_ctx.control_socket);
//		return RETURN_VALUE_FAILURE;
//	}
	struct sockaddr_in addr;
	int sockopt=0;

    fprintf(filepointer,  "Create MRP control socket.\n");fflush(filepointer);

	memset((char*)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;

	//
	//      Listener... why 0?
	//
    addr.sin_port = htons(0);
//    addr.sin_port = htons(MRPD_PORT_DEFAULT);
	inet_aton("127.0.0.1", &addr.sin_addr);

    if( (control_socket = socket(addr.sin_family, SOCK_DGRAM, IPPROTO_UDP)) < 0 ){
        fprintf(filepointer,  "Failed to create socket. %d %s\n", errno, strerror(errno));fflush(filepointer);
        fclose(filepointer);
        return RETURN_VALUE_FAILURE;
    }

	/* Allow the socket to be reused - incase connection is closed prematurely */
	if (setsockopt(control_socket, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof( sockopt)) == -1) {
		fprintf(filepointer,  "setsockopt failed %d %s\n", errno, strerror(errno));fflush(filepointer);
		close(control_socket);
        fclose(filepointer);

		return RETURN_VALUE_FAILURE;
	}

    if( bind(control_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0)	{
        fprintf(filepointer,  "Could not bind socket. %d %s\n", errno, strerror(errno));fflush(filepointer);
        close(control_socket);
        fclose(filepointer);
        return RETURN_VALUE_FAILURE;
    } else {
        return RETURN_VALUE_SUCCESS;
    }

}
