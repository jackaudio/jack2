/*
 * global_definitions.h
 *
 *  Created on: Jan 24, 2017
 *      Author: christoph
 */

#ifndef SRC_GLOBAL_DEFINITIONS_H_
#define SRC_GLOBAL_DEFINITIONS_H_

#define _GNU_SOURCE

#include <jack/types.h>
#include <jack/jack.h>
#include <jack/transport.h>
#include "jack/jslist.h"


#include "Open-AVB/daemons/mrpd/mrpd.h"
#include "Open-AVB/daemons/mrpd/mrp.h"
#include "Open-AVB/daemons/mrpd/msrp.h" // spurious dep daemons/mrpd/msrp.h:50:#define MSRP_LISTENER_ASKFAILED

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <poll.h>
#include <netinet/in.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if.h>
#include <linux/sockios.h>

#define RETURN_VALUE_FAILURE 0
#define RETURN_VALUE_SUCCESS 1

#define MILISLEEP_TIME 1000000
#define USLEEP_TIME 1000

#define MAX_DEV_STR_LEN               32


#define BUFLEN 1500
#define ETHERNET_Q_HDR_LENGTH 18
#define ETHERNET_HDR_LENGTH 14
#define IP_HDR_LENGTH 20
#define UDP_HDR_LENGTH 8
#define AVB_ETHER_TYPE	0x22f0

#define ARRAYSIZE(arr) (sizeof(arr) / sizeof(arr[0]))

typedef struct etherheader_q
{
	unsigned char  ether_dhost[6];	// destination eth addr
	unsigned char  ether_shost[6];	// source ether addr
	unsigned int vlan_id;			// VLAN ID field
	unsigned short int ether_type;	// packet type ID field
} etherheader_q_t;

typedef struct etherheader
{
	unsigned char  ether_dhost[6];	// destination eth addr
	unsigned char  ether_shost[6];	// source ether addr
	unsigned short int ether_type;	// packet type ID field
} etherheader_t;


typedef struct mrp_ctx{
	volatile int mrp_status;
	volatile int domain_a_valid;
	volatile int domain_b_valid;
	int domain_class_a_id;
	int domain_class_a_priority;
	u_int16_t domain_class_a_vid;
	int domain_class_b_id;
	int domain_class_b_priority;
	u_int16_t domain_class_b_vid;
}mrp_ctx_t;


//typedef struct {
//    int64_t ml_phoffset;			//!< Master to local phase offset
//    int64_t ls_phoffset;			//!< Local to system phase offset
//    FrequencyRatio ml_freqoffset;	//!< Master to local frequency offset
//    FrequencyRatio ls_freqoffset;	//!< Local to system frequency offset
//    uint64_t local_time;			//!< Local time of last update
//    uint32_t sync_count;			//!< Sync messages count
//    uint32_t pdelay_count;			//!< pdelay messages count
//    bool asCapable;                 //!< asCapable flag: true = device is AS Capable; false otherwise
//    PortState port_state;			//!< gPTP port state. It can assume values defined at ::PortState
//    PID_TYPE process_id;			//!< Process id number
//} gPtpTimeData;
/*

   Integer64  <master-local phase offset>
   Integer64  <local-system phase offset>
   LongDouble <master-local frequency offset>
   LongDouble <local-system frequency offset>
   UInteger64 <local time of last update>

 * Meaning of IPC provided values:

 master  ~= local   - <master-local phase offset>
 local   ~= system  - <local-system phase offset>
 Dmaster ~= Dlocal  * <master-local frequency offset>
 Dlocal  ~= Dsystem * <local-system freq offset>        (where D denotes a delta)

*/

typedef enum mrpStatus{
    TL_UNDEFINED,
	TALKER_IDLE,
	TALKER_ADVERTISE,
	TALKER_ASKFAILED,
	TALKER_READYFAILED,
	TALKER_CONNECTING,
	TALKER_CONNECTED,
	TALKER_ERROR,
	LISTENER_IDLE,
	LISTENER_WAITING,
	LISTENER_READY,
	LISTENER_CONNECTED,
	LISTENER_ERROR,
	LISTENER_FAILED
} mrpStatus_t;

/*
 *
 *          Merge Data Structures
 *
 */

typedef struct _ieee1722_avtp_driver_state ieee1722_avtp_driver_state;

struct _ieee1722_avtp_driver_state {


	uint8_t streamid8[8];
	uint8_t destination_mac_address[6];
	unsigned char serverMACAddress[6];
	char avbdev[MAX_DEV_STR_LEN];
	struct sockaddr_in si_other_avb;
	int raw_transport_socket;
	struct ifreq if_idx;
	struct ifreq if_mac;

	pthread_t thread;
    pthread_mutex_t threadLock;
    pthread_cond_t dataReady;

    jack_nframes_t  net_period_up;
    jack_nframes_t  net_period_down;

    jack_nframes_t  sample_rate;

    jack_nframes_t  period_size;
    jack_time_t	    period_usecs;
    int		    dont_htonl_floats;
    int		    always_deadline;

    unsigned int    capture_channels;
    unsigned int    capture_channels_audio;

    JSList	    *capture_ports;
    JSList	    *playback_ports;
    JSList	    *playback_srcs;
    JSList	    *capture_srcs;

    jack_client_t   *client;

    jack_nframes_t expected_framecnt;
    int		   expected_framecnt_valid;
    unsigned int   num_lost_packets;
    jack_time_t	   next_deadline;
    jack_time_t	   deadline_offset;
    int		   next_deadline_valid;
    int		   packet_data_valid;
    int		   resync_threshold;
    int		   running_free;
    int		   deadline_goodness;
    jack_time_t	   time_to_deadline;
    struct _packet_cache * packcache;
};

/*
 *
 *          Merge Data Structures
 *
 */



/*
 *
 * 			CONDITIONAL DEFINES!!!
 *
 *
 *
 */








#endif /* SRC_GLOBAL_DEFINITIONS_H_ */
