/*
 * udpSocket.c
 *
 *  Created on: Nov 7, 2016
 *      Author: christoph
 */
#include "avb_sockets.h"

#define DUMMY_STREAMID (0xABCDEF)

/* IEEE 1722 AVTP Receive Socket */
int enable_1722avtp_filter( FILE* filepointer, int raw_transport_socket, unsigned char *destinationMacAddress)
{
    /*
	 	tcpdump -i enp9s0 "ether dst 91:e0:f0:00:c3:51" -dd
	 	{ 0x20, 0, 0, 0x00000002 },
		{ 0x15, 0, 3, 0xf000c351 },
		{ 0x28, 0, 0, 0x00000000 },
		{ 0x15, 0, 1, 0x000091e0 },
		{ 0x6, 0, 0, 0x00040000 },
		{ 0x6, 0, 0, 0x00000000 },
     */

    unsigned int low_mac=0, hi_mac=0;
    low_mac = (((destinationMacAddress[0] & 0xFF) & 0xFFFFFFFF) << 8 ) | (destinationMacAddress[1] & 0xFF );
    hi_mac = (((destinationMacAddress[2] & 0xFF) & 0xFFFFFFFF) << 24 ) | (((destinationMacAddress[3] & 0xFF ) & 0xFFFFFFFF) << 16 ) | (((destinationMacAddress[4] & 0xFF ) & 0xFFFFFFFF) << 8 ) | (destinationMacAddress[5] & 0xFF );

	struct sock_filter code[] = {
			{ 0x20, 0, 0, 0x00000002 },
			{ 0x15, 0, 3, hi_mac },
			{ 0x28, 0, 0, 0x00000000 },
			{ 0x15, 0, 1, low_mac },
			{ 0x6, 0, 0, 0x00040000 },
			{ 0x6, 0, 0, 0x00000000 },
	};

	struct sock_fprog bpf = {
		.len = ARRAYSIZE(code),
		.filter = code,
	};

    if(setsockopt(raw_transport_socket, SOL_SOCKET, SO_ATTACH_FILTER, &bpf, sizeof(bpf)) < 0) {
		fprintf(filepointer,  "setsockopt error: %s \n", strerror(errno));fflush(filepointer);
		return RETURN_VALUE_FAILURE;
    }

    return RETURN_VALUE_FAILURE;
}


int create_RAW_AVB_Transport_Socket( FILE* filepointer, int* raw_transport_socket, const char* eth_dev)
{
	struct ifreq ifr;
	memset((char*)&ifr, 0, sizeof(struct ifreq));
	int sockopt=0;

	struct ifreq ifopts;
	memset((char*)&ifopts, 0, sizeof(struct ifreq));
	int s;

	strncpy (ifr.ifr_name, eth_dev, IFNAMSIZ - 1);
	ifr.ifr_name[sizeof(ifr.ifr_name)-1] = '\0';

	if (( s = socket(PF_PACKET, SOCK_RAW, htons(AVB_ETHER_TYPE/*ETH_P_ALL*/))) < 0){
		fprintf(filepointer,  "[RAW_TRANSPORT] Error creating RAW Socket \n");fflush(filepointer);

		return RETURN_VALUE_FAILURE;
	}
	*raw_transport_socket = s;

	strncpy(ifopts.ifr_name, ifr.ifr_name, IFNAMSIZ-1);

	if( ioctl(*raw_transport_socket, SIOCGIFFLAGS, &ifopts) == -1) {
		fprintf(filepointer,  "[RAW_TRANSPORT] No such interface");fflush(filepointer);
		fprintf(filepointer,  "Zero \n");fflush(filepointer);
	    close(*raw_transport_socket);

		return RETURN_VALUE_FAILURE;
	}

	ifopts.ifr_flags |= IFF_PROMISC;

	if( ioctl(*raw_transport_socket, SIOCSIFFLAGS, &ifopts) == -1){
		fprintf(filepointer,  "[RAW_TRANSPORT] Interface is down. \n");fflush(filepointer);
	    close(*raw_transport_socket);

		return RETURN_VALUE_FAILURE;
	}

	if (setsockopt(*raw_transport_socket, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof( sockopt)) == -1) {
		fprintf(filepointer,  "[RAW_TRANSPORT] setsockopt failed \n");fflush(filepointer);
		close(*raw_transport_socket);

		return RETURN_VALUE_FAILURE;
	}

	/* Set Timestamping Option => requires recvmsg to be used => timestamp in ancillary data */
    int timestamp_flags = 0;

//	timestamp_flags |= SOF_TIMESTAMPING_TX_HARDWARE;
	timestamp_flags |= SOF_TIMESTAMPING_RX_HARDWARE;
	timestamp_flags |= SOF_TIMESTAMPING_SYS_HARDWARE;
	timestamp_flags |= SOF_TIMESTAMPING_RAW_HARDWARE;

	struct hwtstamp_config hwconfig;
	memset( &hwconfig, 0, sizeof( hwconfig ));
	hwconfig.rx_filter = HWTSTAMP_FILTER_ALL;
//	hwconfig.tx_type = HWTSTAMP_TX_OFF;
	hwconfig.tx_type = HWTSTAMP_TX_ON; /* NECESARRY FOR CMSGs TO WORK*/

	struct ifreq hwtstamp;
	memset((char*)&hwtstamp, 0, sizeof(struct ifreq));
	strncpy(hwtstamp.ifr_name, ifr.ifr_name, IFNAMSIZ-1);
	hwtstamp.ifr_data = (void *) &hwconfig;

	if( ioctl( *raw_transport_socket, SIOCSHWTSTAMP, &hwtstamp ) == -1 ) {
        fprintf(filepointer,  "[RAW TRANSPORT] ioctl timestamping failed %d %s \n", errno, strerror(errno));fflush(filepointer);
		close(*raw_transport_socket);
		return RETURN_VALUE_FAILURE;
	}

	if (setsockopt(*raw_transport_socket, SOL_SOCKET, SO_TIMESTAMPING, &timestamp_flags, sizeof(timestamp_flags) ) == -1) {
		fprintf(filepointer,  "[RAW TRANSPORT] setsockopt timestamping failed %d %s \n", errno, strerror(errno));fflush(filepointer);
		close(*raw_transport_socket);
		return RETURN_VALUE_FAILURE;
	} else {
		fprintf(filepointer,  "[RAW TRANSPORT] Timestamp Socket \n");fflush(filepointer);
	}

	if (setsockopt(*raw_transport_socket, SOL_SOCKET, SO_BINDTODEVICE, eth_dev, IFNAMSIZ-1) == -1)	{
		fprintf(filepointer,  "[RAW_TRANSPORT] SO_BINDTODEVICE failed \n");fflush(filepointer);
		close(*raw_transport_socket);

		return RETURN_VALUE_FAILURE;
	}
	return RETURN_VALUE_SUCCESS;
}

