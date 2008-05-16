#ifndef	_JackRPCEngine_user_
#define	_JackRPCEngine_user_

/* Module JackRPCEngine */

#include <string.h>
#include <mach/ndr.h>
#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/notify.h>
#include <mach/mach_types.h>
#include <mach/message.h>
#include <mach/mig_errors.h>
#include <mach/port.h>

#ifdef AUTOTEST
#ifndef FUNCTION_PTR_T
#define FUNCTION_PTR_T
typedef void (*function_ptr_t)(mach_port_t, char *, mach_msg_type_number_t);
typedef struct {
        char            *name;
        function_ptr_t  function;
} function_table_entry;
typedef function_table_entry 	*function_table_t;
#endif /* FUNCTION_PTR_T */
#endif /* AUTOTEST */

#ifndef	JackRPCEngine_MSG_COUNT
#define	JackRPCEngine_MSG_COUNT	20
#endif	/* JackRPCEngine_MSG_COUNT */

#include <mach/std_types.h>
#include <mach/mig.h>
#include <mach/mig.h>
#include <mach/mach_types.h>
#include "Jackdefs.h"

#ifdef __BeforeMigUserHeader
__BeforeMigUserHeader
#endif /* __BeforeMigUserHeader */

#include <sys/cdefs.h>
__BEGIN_DECLS


/* Routine rpc_jack_client_open */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t rpc_jack_client_open
(
	mach_port_t server_port,
	client_name_t client_name,
	mach_port_t *private_port,
	int *shared_engine,
	int *shared_client,
	int *shared_graph,
	int *result
);

/* Routine rpc_jack_client_check */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t rpc_jack_client_check
(
	mach_port_t server_port,
	client_name_t client_name,
	client_name_t client_name_res,
	int protocol,
	int options,
	int *status,
	int *result
);

/* Routine rpc_jack_client_close */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t rpc_jack_client_close
(
	mach_port_t server_port,
	int refnum,
	int *result
);

/* Routine rpc_jack_client_activate */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t rpc_jack_client_activate
(
	mach_port_t server_port,
	int refnum,
	int state,
	int *result
);

/* Routine rpc_jack_client_deactivate */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t rpc_jack_client_deactivate
(
	mach_port_t server_port,
	int refnum,
	int *result
);

/* Routine rpc_jack_port_register */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t rpc_jack_port_register
(
	mach_port_t server_port,
	int refnum,
	client_port_name_t name,
	client_port_type_t port_type,
	unsigned flags,
	unsigned buffer_size,
	unsigned *port_index,
	int *result
);

/* Routine rpc_jack_port_unregister */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t rpc_jack_port_unregister
(
	mach_port_t server_port,
	int refnum,
	int port,
	int *result
);

/* Routine rpc_jack_port_connect */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t rpc_jack_port_connect
(
	mach_port_t server_port,
	int refnum,
	int src,
	int dst,
	int *result
);

/* Routine rpc_jack_port_disconnect */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t rpc_jack_port_disconnect
(
	mach_port_t server_port,
	int refnum,
	int src,
	int dst,
	int *result
);

/* Routine rpc_jack_port_connect_name */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t rpc_jack_port_connect_name
(
	mach_port_t server_port,
	int refnum,
	client_port_name_t src,
	client_port_name_t dst,
	int *result
);

/* Routine rpc_jack_port_disconnect_name */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t rpc_jack_port_disconnect_name
(
	mach_port_t server_port,
	int refnum,
	client_port_name_t src,
	client_port_name_t dst,
	int *result
);

/* Routine rpc_jack_set_buffer_size */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t rpc_jack_set_buffer_size
(
	mach_port_t server_port,
	int buffer_size,
	int *result
);

/* Routine rpc_jack_set_freewheel */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t rpc_jack_set_freewheel
(
	mach_port_t server_port,
	int onoff,
	int *result
);

/* Routine rpc_jack_release_timebase */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t rpc_jack_release_timebase
(
	mach_port_t server_port,
	int refnum,
	int *result
);

/* Routine rpc_jack_set_timebase_callback */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t rpc_jack_set_timebase_callback
(
	mach_port_t server_port,
	int refnum,
	int conditional,
	int *result
);

/* Routine rpc_jack_get_internal_clientname */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t rpc_jack_get_internal_clientname
(
	mach_port_t server_port,
	int refnum,
	int int_ref,
	client_name_t client_name_res,
	int *result
);

/* Routine rpc_jack_internal_clienthandle */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t rpc_jack_internal_clienthandle
(
	mach_port_t server_port,
	int refnum,
	client_name_t client_name,
	int *int_ref,
	int *status,
	int *result
);

/* Routine rpc_jack_internal_clientload */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t rpc_jack_internal_clientload
(
	mach_port_t server_port,
	int refnum,
	client_name_t client_name,
	so_name_t so_name,
	objet_data_t objet_data,
	int options,
	int *status,
	int *int_ref,
	int *result
);

/* Routine rpc_jack_internal_clientunload */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t rpc_jack_internal_clientunload
(
	mach_port_t server_port,
	int refnum,
	int int_ref,
	int *status,
	int *result
);

/* SimpleRoutine rpc_jack_client_rt_notify */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t rpc_jack_client_rt_notify
(
	mach_port_t client_port,
	int refnum,
	int notify,
	int value,
	int timeout
);

__END_DECLS

/********************** Caution **************************/
/* The following data types should be used to calculate  */
/* maximum message sizes only. The actual message may be */
/* smaller, and the position of the arguments within the */
/* message layout may vary from what is presented here.  */
/* For example, if any of the arguments are variable-    */
/* sized, and less than the maximum is sent, the data    */
/* will be packed tight in the actual message to reduce  */
/* the presence of holes.                                */
/********************** Caution **************************/

/* typedefs for all requests */

#ifndef __Request__JackRPCEngine_subsystem__defined
#define __Request__JackRPCEngine_subsystem__defined

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		client_name_t client_name;
	} __Request__rpc_jack_client_open_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		client_name_t client_name;
		int protocol;
		int options;
	} __Request__rpc_jack_client_check_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
	} __Request__rpc_jack_client_close_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
		int state;
	} __Request__rpc_jack_client_activate_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
	} __Request__rpc_jack_client_deactivate_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
		client_port_name_t name;
		client_port_type_t port_type;
		unsigned flags;
		unsigned buffer_size;
	} __Request__rpc_jack_port_register_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
		int port;
	} __Request__rpc_jack_port_unregister_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
		int src;
		int dst;
	} __Request__rpc_jack_port_connect_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
		int src;
		int dst;
	} __Request__rpc_jack_port_disconnect_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
		client_port_name_t src;
		client_port_name_t dst;
	} __Request__rpc_jack_port_connect_name_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
		client_port_name_t src;
		client_port_name_t dst;
	} __Request__rpc_jack_port_disconnect_name_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int buffer_size;
	} __Request__rpc_jack_set_buffer_size_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int onoff;
	} __Request__rpc_jack_set_freewheel_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
	} __Request__rpc_jack_release_timebase_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
		int conditional;
	} __Request__rpc_jack_set_timebase_callback_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
		int int_ref;
	} __Request__rpc_jack_get_internal_clientname_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
		client_name_t client_name;
	} __Request__rpc_jack_internal_clienthandle_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
		client_name_t client_name;
		so_name_t so_name;
		objet_data_t objet_data;
		int options;
	} __Request__rpc_jack_internal_clientload_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
		int int_ref;
	} __Request__rpc_jack_internal_clientunload_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
		int notify;
		int value;
	} __Request__rpc_jack_client_rt_notify_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif
#endif /* !__Request__JackRPCEngine_subsystem__defined */

/* union of all requests */

#ifndef __RequestUnion__JackRPCEngine_subsystem__defined
#define __RequestUnion__JackRPCEngine_subsystem__defined
union __RequestUnion__JackRPCEngine_subsystem {
	__Request__rpc_jack_client_open_t Request_rpc_jack_client_open;
	__Request__rpc_jack_client_check_t Request_rpc_jack_client_check;
	__Request__rpc_jack_client_close_t Request_rpc_jack_client_close;
	__Request__rpc_jack_client_activate_t Request_rpc_jack_client_activate;
	__Request__rpc_jack_client_deactivate_t Request_rpc_jack_client_deactivate;
	__Request__rpc_jack_port_register_t Request_rpc_jack_port_register;
	__Request__rpc_jack_port_unregister_t Request_rpc_jack_port_unregister;
	__Request__rpc_jack_port_connect_t Request_rpc_jack_port_connect;
	__Request__rpc_jack_port_disconnect_t Request_rpc_jack_port_disconnect;
	__Request__rpc_jack_port_connect_name_t Request_rpc_jack_port_connect_name;
	__Request__rpc_jack_port_disconnect_name_t Request_rpc_jack_port_disconnect_name;
	__Request__rpc_jack_set_buffer_size_t Request_rpc_jack_set_buffer_size;
	__Request__rpc_jack_set_freewheel_t Request_rpc_jack_set_freewheel;
	__Request__rpc_jack_release_timebase_t Request_rpc_jack_release_timebase;
	__Request__rpc_jack_set_timebase_callback_t Request_rpc_jack_set_timebase_callback;
	__Request__rpc_jack_get_internal_clientname_t Request_rpc_jack_get_internal_clientname;
	__Request__rpc_jack_internal_clienthandle_t Request_rpc_jack_internal_clienthandle;
	__Request__rpc_jack_internal_clientload_t Request_rpc_jack_internal_clientload;
	__Request__rpc_jack_internal_clientunload_t Request_rpc_jack_internal_clientunload;
	__Request__rpc_jack_client_rt_notify_t Request_rpc_jack_client_rt_notify;
};
#endif /* !__RequestUnion__JackRPCEngine_subsystem__defined */
/* typedefs for all replies */

#ifndef __Reply__JackRPCEngine_subsystem__defined
#define __Reply__JackRPCEngine_subsystem__defined

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		/* start of the kernel processed data */
		mach_msg_body_t msgh_body;
		mach_msg_port_descriptor_t private_port;
		/* end of the kernel processed data */
		NDR_record_t NDR;
		int shared_engine;
		int shared_client;
		int shared_graph;
		int result;
	} __Reply__rpc_jack_client_open_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
		client_name_t client_name_res;
		int status;
		int result;
	} __Reply__rpc_jack_client_check_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
		int result;
	} __Reply__rpc_jack_client_close_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
		int result;
	} __Reply__rpc_jack_client_activate_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
		int result;
	} __Reply__rpc_jack_client_deactivate_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
		unsigned port_index;
		int result;
	} __Reply__rpc_jack_port_register_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
		int result;
	} __Reply__rpc_jack_port_unregister_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
		int result;
	} __Reply__rpc_jack_port_connect_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
		int result;
	} __Reply__rpc_jack_port_disconnect_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
		int result;
	} __Reply__rpc_jack_port_connect_name_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
		int result;
	} __Reply__rpc_jack_port_disconnect_name_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
		int result;
	} __Reply__rpc_jack_set_buffer_size_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
		int result;
	} __Reply__rpc_jack_set_freewheel_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
		int result;
	} __Reply__rpc_jack_release_timebase_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
		int result;
	} __Reply__rpc_jack_set_timebase_callback_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
		client_name_t client_name_res;
		int result;
	} __Reply__rpc_jack_get_internal_clientname_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
		int int_ref;
		int status;
		int result;
	} __Reply__rpc_jack_internal_clienthandle_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
		int status;
		int int_ref;
		int result;
	} __Reply__rpc_jack_internal_clientload_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
		int status;
		int result;
	} __Reply__rpc_jack_internal_clientunload_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
	} __Reply__rpc_jack_client_rt_notify_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif
#endif /* !__Reply__JackRPCEngine_subsystem__defined */

/* union of all replies */

#ifndef __ReplyUnion__JackRPCEngine_subsystem__defined
#define __ReplyUnion__JackRPCEngine_subsystem__defined
union __ReplyUnion__JackRPCEngine_subsystem {
	__Reply__rpc_jack_client_open_t Reply_rpc_jack_client_open;
	__Reply__rpc_jack_client_check_t Reply_rpc_jack_client_check;
	__Reply__rpc_jack_client_close_t Reply_rpc_jack_client_close;
	__Reply__rpc_jack_client_activate_t Reply_rpc_jack_client_activate;
	__Reply__rpc_jack_client_deactivate_t Reply_rpc_jack_client_deactivate;
	__Reply__rpc_jack_port_register_t Reply_rpc_jack_port_register;
	__Reply__rpc_jack_port_unregister_t Reply_rpc_jack_port_unregister;
	__Reply__rpc_jack_port_connect_t Reply_rpc_jack_port_connect;
	__Reply__rpc_jack_port_disconnect_t Reply_rpc_jack_port_disconnect;
	__Reply__rpc_jack_port_connect_name_t Reply_rpc_jack_port_connect_name;
	__Reply__rpc_jack_port_disconnect_name_t Reply_rpc_jack_port_disconnect_name;
	__Reply__rpc_jack_set_buffer_size_t Reply_rpc_jack_set_buffer_size;
	__Reply__rpc_jack_set_freewheel_t Reply_rpc_jack_set_freewheel;
	__Reply__rpc_jack_release_timebase_t Reply_rpc_jack_release_timebase;
	__Reply__rpc_jack_set_timebase_callback_t Reply_rpc_jack_set_timebase_callback;
	__Reply__rpc_jack_get_internal_clientname_t Reply_rpc_jack_get_internal_clientname;
	__Reply__rpc_jack_internal_clienthandle_t Reply_rpc_jack_internal_clienthandle;
	__Reply__rpc_jack_internal_clientload_t Reply_rpc_jack_internal_clientload;
	__Reply__rpc_jack_internal_clientunload_t Reply_rpc_jack_internal_clientunload;
	__Reply__rpc_jack_client_rt_notify_t Reply_rpc_jack_client_rt_notify;
};
#endif /* !__RequestUnion__JackRPCEngine_subsystem__defined */

#ifndef subsystem_to_name_map_JackRPCEngine
#define subsystem_to_name_map_JackRPCEngine \
    { "rpc_jack_client_open", 1000 },\
    { "rpc_jack_client_check", 1001 },\
    { "rpc_jack_client_close", 1002 },\
    { "rpc_jack_client_activate", 1003 },\
    { "rpc_jack_client_deactivate", 1004 },\
    { "rpc_jack_port_register", 1005 },\
    { "rpc_jack_port_unregister", 1006 },\
    { "rpc_jack_port_connect", 1007 },\
    { "rpc_jack_port_disconnect", 1008 },\
    { "rpc_jack_port_connect_name", 1009 },\
    { "rpc_jack_port_disconnect_name", 1010 },\
    { "rpc_jack_set_buffer_size", 1011 },\
    { "rpc_jack_set_freewheel", 1012 },\
    { "rpc_jack_release_timebase", 1013 },\
    { "rpc_jack_set_timebase_callback", 1014 },\
    { "rpc_jack_get_internal_clientname", 1015 },\
    { "rpc_jack_internal_clienthandle", 1016 },\
    { "rpc_jack_internal_clientload", 1017 },\
    { "rpc_jack_internal_clientunload", 1018 },\
    { "rpc_jack_client_rt_notify", 1019 }
#endif

#ifdef __AfterMigUserHeader
__AfterMigUserHeader
#endif /* __AfterMigUserHeader */

#endif	 /* _JackRPCEngine_user_ */
