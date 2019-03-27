
#include "mrp_client_send_msg.h"

int mrp_client_send_mrp_msg(FILE* filepointer, int control_socket, char *notify_data, int notify_len)
{
	struct sockaddr_in addr;

	if ( control_socket == -1 )
		return RETURN_VALUE_FAILURE;
	if (notify_data == NULL)
		return RETURN_VALUE_FAILURE;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(MRPD_PORT_DEFAULT);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	inet_aton("127.0.0.1", &addr.sin_addr);

	return sendto( control_socket, notify_data, notify_len, 0, (struct sockaddr*)&addr, (socklen_t)sizeof(addr));
}
