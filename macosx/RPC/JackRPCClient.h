#ifndef	_JackRPCClient_user_
#define	_JackRPCClient_user_

/* Module JackRPCClient */

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

#ifndef	JackRPCClient_MSG_COUNT
#define	JackRPCClient_MSG_COUNT	2
#endif	/* JackRPCClient_MSG_COUNT */

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


/* Routine rpc_jack_client_sync_notify */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t rpc_jack_client_sync_notify
(
	mach_port_t client_port,
	int refnum,
	client_name_t client_name,
	int notify,
	int value1,
	int value2,
	int *result
);

/* SimpleRoutine rpc_jack_client_async_notify */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t rpc_jack_client_async_notify
(
	mach_port_t client_port,
	int refnum,
	client_name_t client_name,
	int notify,
	int value1,
	int value2
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

#ifndef __Request__JackRPCClient_subsystem__defined
#define __Request__JackRPCClient_subsystem__defined

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
		client_name_t client_name;
		int notify;
		int value1;
		int value2;
	} __Request__rpc_jack_client_sync_notify_t;
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
		int notify;
		int value1;
		int value2;
	} __Request__rpc_jack_client_async_notify_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif
#endif /* !__Request__JackRPCClient_subsystem__defined */

/* union of all requests */

#ifndef __RequestUnion__JackRPCClient_subsystem__defined
#define __RequestUnion__JackRPCClient_subsystem__defined
union __RequestUnion__JackRPCClient_subsystem {
	__Request__rpc_jack_client_sync_notify_t Request_rpc_jack_client_sync_notify;
	__Request__rpc_jack_client_async_notify_t Request_rpc_jack_client_async_notify;
};
#endif /* !__RequestUnion__JackRPCClient_subsystem__defined */
/* typedefs for all replies */

#ifndef __Reply__JackRPCClient_subsystem__defined
#define __Reply__JackRPCClient_subsystem__defined

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
		int result;
	} __Reply__rpc_jack_client_sync_notify_t;
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
	} __Reply__rpc_jack_client_async_notify_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif
#endif /* !__Reply__JackRPCClient_subsystem__defined */

/* union of all replies */

#ifndef __ReplyUnion__JackRPCClient_subsystem__defined
#define __ReplyUnion__JackRPCClient_subsystem__defined
union __ReplyUnion__JackRPCClient_subsystem {
	__Reply__rpc_jack_client_sync_notify_t Reply_rpc_jack_client_sync_notify;
	__Reply__rpc_jack_client_async_notify_t Reply_rpc_jack_client_async_notify;
};
#endif /* !__RequestUnion__JackRPCClient_subsystem__defined */

#ifndef subsystem_to_name_map_JackRPCClient
#define subsystem_to_name_map_JackRPCClient \
    { "rpc_jack_client_sync_notify", 1000 },\
    { "rpc_jack_client_async_notify", 1001 }
#endif

#ifdef __AfterMigUserHeader
__AfterMigUserHeader
#endif /* __AfterMigUserHeader */

#endif	 /* _JackRPCClient_user_ */
