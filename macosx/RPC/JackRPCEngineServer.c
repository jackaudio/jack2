/*
 * IDENTIFICATION:
 * stub generated Mon Aug 27 17:58:23 2007
 * with a MiG generated Mon Sep 11 19:11:05 PDT 2006 by root@b09.apple.com
 * OPTIONS: 
 */

/* Module JackRPCEngine */

#define	__MIG_check__Request__JackRPCEngine_subsystem__ 1
#define	__NDR_convert__Request__JackRPCEngine_subsystem__ 1

#include <string.h>
#include <mach/ndr.h>
#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/notify.h>
#include <mach/mach_types.h>
#include <mach/message.h>
#include <mach/mig_errors.h>
#include <mach/port.h>

#include <mach/std_types.h>
#include <mach/mig.h>
#include <mach/mig.h>
#include <mach/mach_types.h>
#include "Jackdefs.h"

#ifndef	mig_internal
#define	mig_internal	static __inline__
#endif	/* mig_internal */

#ifndef	mig_external
#define mig_external
#endif	/* mig_external */

#if	!defined(__MigTypeCheck) && defined(TypeCheck)
#define	__MigTypeCheck		TypeCheck	/* Legacy setting */
#endif	/* !defined(__MigTypeCheck) */

#if	!defined(__MigKernelSpecificCode) && defined(_MIG_KERNEL_SPECIFIC_CODE_)
#define	__MigKernelSpecificCode	_MIG_KERNEL_SPECIFIC_CODE_	/* Legacy setting */
#endif	/* !defined(__MigKernelSpecificCode) */

#ifndef	LimitCheck
#define	LimitCheck 0
#endif	/* LimitCheck */

#ifndef	min
#define	min(a,b)  ( ((a) < (b))? (a): (b) )
#endif	/* min */

#if !defined(_WALIGN_)
#define _WALIGN_(x) (((x) + 3) & ~3)
#endif /* !defined(_WALIGN_) */

#if !defined(_WALIGNSZ_)
#define _WALIGNSZ_(x) _WALIGN_(sizeof(x))
#endif /* !defined(_WALIGNSZ_) */

#ifndef	UseStaticTemplates
#define	UseStaticTemplates	0
#endif	/* UseStaticTemplates */

#ifndef	__DeclareRcvRpc
#define	__DeclareRcvRpc(_NUM_, _NAME_)
#endif	/* __DeclareRcvRpc */

#ifndef	__BeforeRcvRpc
#define	__BeforeRcvRpc(_NUM_, _NAME_)
#endif	/* __BeforeRcvRpc */

#ifndef	__AfterRcvRpc
#define	__AfterRcvRpc(_NUM_, _NAME_)
#endif	/* __AfterRcvRpc */

#ifndef	__DeclareRcvSimple
#define	__DeclareRcvSimple(_NUM_, _NAME_)
#endif	/* __DeclareRcvSimple */

#ifndef	__BeforeRcvSimple
#define	__BeforeRcvSimple(_NUM_, _NAME_)
#endif	/* __BeforeRcvSimple */

#ifndef	__AfterRcvSimple
#define	__AfterRcvSimple(_NUM_, _NAME_)
#endif	/* __AfterRcvSimple */

#define novalue void

#define msgh_request_port	msgh_local_port
#define MACH_MSGH_BITS_REQUEST(bits)	MACH_MSGH_BITS_LOCAL(bits)
#define msgh_reply_port		msgh_remote_port
#define MACH_MSGH_BITS_REPLY(bits)	MACH_MSGH_BITS_REMOTE(bits)

#define MIG_RETURN_ERROR(X, code)	{\
				((mig_reply_error_t *)X)->RetCode = code;\
				((mig_reply_error_t *)X)->NDR = NDR_record;\
				return;\
				}

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
		int notify;
		int value;
	} __Request__rpc_jack_client_rt_notify_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif
#endif /* !__Request__JackRPCEngine_subsystem__defined */

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
	} __Reply__rpc_jack_client_rt_notify_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif
#endif /* !__Reply__JackRPCEngine_subsystem__defined */


/* union of all replies */

#ifndef __ReplyUnion__server_JackRPCEngine_subsystem__defined
#define __ReplyUnion__server_JackRPCEngine_subsystem__defined
union __ReplyUnion__server_JackRPCEngine_subsystem {
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
	__Reply__rpc_jack_client_rt_notify_t Reply_rpc_jack_client_rt_notify;
};
#endif /* __RequestUnion__server_JackRPCEngine_subsystem__defined */
/* Forward Declarations */


mig_internal novalue _Xrpc_jack_client_open
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP);

mig_internal novalue _Xrpc_jack_client_check
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP);

mig_internal novalue _Xrpc_jack_client_close
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP);

mig_internal novalue _Xrpc_jack_client_activate
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP);

mig_internal novalue _Xrpc_jack_client_deactivate
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP);

mig_internal novalue _Xrpc_jack_port_register
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP);

mig_internal novalue _Xrpc_jack_port_unregister
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP);

mig_internal novalue _Xrpc_jack_port_connect
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP);

mig_internal novalue _Xrpc_jack_port_disconnect
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP);

mig_internal novalue _Xrpc_jack_port_connect_name
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP);

mig_internal novalue _Xrpc_jack_port_disconnect_name
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP);

mig_internal novalue _Xrpc_jack_set_buffer_size
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP);

mig_internal novalue _Xrpc_jack_set_freewheel
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP);

mig_internal novalue _Xrpc_jack_release_timebase
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP);

mig_internal novalue _Xrpc_jack_set_timebase_callback
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP);

mig_internal novalue _Xrpc_jack_client_rt_notify
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP);


#if (__MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Request__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Request__rpc_jack_client_open_t__defined)
#define __MIG_check__Request__rpc_jack_client_open_t__defined
#ifndef __NDR_convert__int_rep__Request__rpc_jack_client_open_t__client_name__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__client_name_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_open_t__client_name__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_open_t__client_name(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__client_name_t((client_name_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__client_name_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_open_t__client_name__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_open_t__client_name(a, f) \
	__NDR_convert__int_rep__client_name_t((client_name_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__string__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_open_t__client_name__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_open_t__client_name(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__string(a, f, 128)
#elif	defined(__NDR_convert__int_rep__string__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_open_t__client_name__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_open_t__client_name(a, f) \
	__NDR_convert__int_rep__string(a, f, 128)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_client_open_t__client_name__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_client_open_t__client_name__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__client_name_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_open_t__client_name__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_open_t__client_name(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__client_name_t((client_name_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__client_name_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_open_t__client_name__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_open_t__client_name(a, f) \
	__NDR_convert__char_rep__client_name_t((client_name_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__string__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_open_t__client_name__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_open_t__client_name(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__string(a, f, 128)
#elif	defined(__NDR_convert__char_rep__string__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_open_t__client_name__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_open_t__client_name(a, f) \
	__NDR_convert__char_rep__string(a, f, 128)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_client_open_t__client_name__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_client_open_t__client_name__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__client_name_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_open_t__client_name__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_open_t__client_name(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__client_name_t((client_name_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__client_name_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_open_t__client_name__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_open_t__client_name(a, f) \
	__NDR_convert__float_rep__client_name_t((client_name_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__string__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_open_t__client_name__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_open_t__client_name(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__string(a, f, 128)
#elif	defined(__NDR_convert__float_rep__string__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_open_t__client_name__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_open_t__client_name(a, f) \
	__NDR_convert__float_rep__string(a, f, 128)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_client_open_t__client_name__defined */


mig_internal kern_return_t __MIG_check__Request__rpc_jack_client_open_t(__Request__rpc_jack_client_open_t *In0P)
{

	typedef __Request__rpc_jack_client_open_t __Request;
#if	__MigTypeCheck
	if ((In0P->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX) ||
	    (In0P->Head.msgh_size != (mach_msg_size_t)sizeof(__Request)))
		return MIG_BAD_ARGUMENTS;
#endif	/* __MigTypeCheck */

#if	defined(__NDR_convert__int_rep__Request__rpc_jack_client_open_t__client_name__defined)
	if (In0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Request__rpc_jack_client_open_t__client_name__defined)
		__NDR_convert__int_rep__Request__rpc_jack_client_open_t__client_name(&In0P->client_name, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_client_open_t__client_name__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	defined(__NDR_convert__char_rep__Request__rpc_jack_client_open_t__client_name__defined)
	if (In0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Request__rpc_jack_client_open_t__client_name__defined)
		__NDR_convert__char_rep__Request__rpc_jack_client_open_t__client_name(&In0P->client_name, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_client_open_t__client_name__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	defined(__NDR_convert__float_rep__Request__rpc_jack_client_open_t__client_name__defined)
	if (In0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Request__rpc_jack_client_open_t__client_name__defined)
		__NDR_convert__float_rep__Request__rpc_jack_client_open_t__client_name(&In0P->client_name, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_client_open_t__client_name__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Request__rpc_jack_client_open_t__defined) */
#endif /* __MIG_check__Request__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_client_open */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t server_rpc_jack_client_open
(
	mach_port_t server_port,
	client_name_t client_name,
	mach_port_t *private_port,
	int *shared_engine,
	int *shared_client,
	int *shared_graph,
	int *result
);

/* Routine rpc_jack_client_open */
mig_internal novalue _Xrpc_jack_client_open
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP)
{

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		client_name_t client_name;
		mach_msg_trailer_t trailer;
	} Request;
#ifdef  __MigPackStructs
#pragma pack()
#endif
	typedef __Request__rpc_jack_client_open_t __Request;
	typedef __Reply__rpc_jack_client_open_t Reply;

	/*
	 * typedef struct {
	 * 	mach_msg_header_t Head;
	 * 	NDR_record_t NDR;
	 * 	kern_return_t RetCode;
	 * } mig_reply_error_t;
	 */

	Request *In0P = (Request *) InHeadP;
	Reply *OutP = (Reply *) OutHeadP;
#ifdef	__MIG_check__Request__rpc_jack_client_open_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Request__rpc_jack_client_open_t__defined */

#if	UseStaticTemplates
	const static mach_msg_port_descriptor_t private_portTemplate = {
		/* name = */		MACH_PORT_NULL,
		/* pad1 = */		0,
		/* pad2 = */		0,
		/* disp = */		20,
		/* type = */		MACH_MSG_PORT_DESCRIPTOR,
	};
#endif	/* UseStaticTemplates */

	kern_return_t RetCode;
	__DeclareRcvRpc(1000, "rpc_jack_client_open")
	__BeforeRcvRpc(1000, "rpc_jack_client_open")

#if	defined(__MIG_check__Request__rpc_jack_client_open_t__defined)
	check_result = __MIG_check__Request__rpc_jack_client_open_t((__Request *)In0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ MIG_RETURN_ERROR(OutP, check_result); }
#endif	/* defined(__MIG_check__Request__rpc_jack_client_open_t__defined) */

#if	UseStaticTemplates
	OutP->private_port = private_portTemplate;
#else	/* UseStaticTemplates */
	OutP->private_port.disposition = 20;
	OutP->private_port.type = MACH_MSG_PORT_DESCRIPTOR;
#endif	/* UseStaticTemplates */


	RetCode = server_rpc_jack_client_open(In0P->Head.msgh_request_port, In0P->client_name, &OutP->private_port.name, &OutP->shared_engine, &OutP->shared_client, &OutP->shared_graph, &OutP->result);
	if (RetCode != KERN_SUCCESS) {
		MIG_RETURN_ERROR(OutP, RetCode);
	}

	OutP->NDR = NDR_record;


	OutP->Head.msgh_bits |= MACH_MSGH_BITS_COMPLEX;
	OutP->Head.msgh_size = (mach_msg_size_t)(sizeof(Reply));
	OutP->msgh_body.msgh_descriptor_count = 1;
	__AfterRcvRpc(1000, "rpc_jack_client_open")
}

#if (__MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Request__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Request__rpc_jack_client_check_t__defined)
#define __MIG_check__Request__rpc_jack_client_check_t__defined
#ifndef __NDR_convert__int_rep__Request__rpc_jack_client_check_t__client_name__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__client_name_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_check_t__client_name__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_check_t__client_name(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__client_name_t((client_name_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__client_name_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_check_t__client_name__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_check_t__client_name(a, f) \
	__NDR_convert__int_rep__client_name_t((client_name_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__string__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_check_t__client_name__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_check_t__client_name(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__string(a, f, 128)
#elif	defined(__NDR_convert__int_rep__string__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_check_t__client_name__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_check_t__client_name(a, f) \
	__NDR_convert__int_rep__string(a, f, 128)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_client_check_t__client_name__defined */

#ifndef __NDR_convert__int_rep__Request__rpc_jack_client_check_t__protocol__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_check_t__protocol__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_check_t__protocol(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_check_t__protocol__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_check_t__protocol(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_check_t__protocol__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_check_t__protocol(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_check_t__protocol__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_check_t__protocol(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_client_check_t__protocol__defined */

#ifndef __NDR_convert__int_rep__Request__rpc_jack_client_check_t__options__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_check_t__options__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_check_t__options(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_check_t__options__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_check_t__options(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_check_t__options__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_check_t__options(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_check_t__options__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_check_t__options(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_client_check_t__options__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_client_check_t__client_name__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__client_name_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_check_t__client_name__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_check_t__client_name(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__client_name_t((client_name_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__client_name_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_check_t__client_name__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_check_t__client_name(a, f) \
	__NDR_convert__char_rep__client_name_t((client_name_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__string__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_check_t__client_name__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_check_t__client_name(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__string(a, f, 128)
#elif	defined(__NDR_convert__char_rep__string__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_check_t__client_name__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_check_t__client_name(a, f) \
	__NDR_convert__char_rep__string(a, f, 128)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_client_check_t__client_name__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_client_check_t__protocol__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_check_t__protocol__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_check_t__protocol(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_check_t__protocol__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_check_t__protocol(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_check_t__protocol__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_check_t__protocol(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_check_t__protocol__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_check_t__protocol(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_client_check_t__protocol__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_client_check_t__options__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_check_t__options__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_check_t__options(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_check_t__options__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_check_t__options(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_check_t__options__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_check_t__options(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_check_t__options__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_check_t__options(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_client_check_t__options__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_client_check_t__client_name__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__client_name_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_check_t__client_name__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_check_t__client_name(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__client_name_t((client_name_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__client_name_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_check_t__client_name__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_check_t__client_name(a, f) \
	__NDR_convert__float_rep__client_name_t((client_name_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__string__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_check_t__client_name__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_check_t__client_name(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__string(a, f, 128)
#elif	defined(__NDR_convert__float_rep__string__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_check_t__client_name__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_check_t__client_name(a, f) \
	__NDR_convert__float_rep__string(a, f, 128)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_client_check_t__client_name__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_client_check_t__protocol__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_check_t__protocol__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_check_t__protocol(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_check_t__protocol__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_check_t__protocol(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_check_t__protocol__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_check_t__protocol(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_check_t__protocol__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_check_t__protocol(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_client_check_t__protocol__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_client_check_t__options__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_check_t__options__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_check_t__options(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_check_t__options__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_check_t__options(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_check_t__options__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_check_t__options(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_check_t__options__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_check_t__options(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_client_check_t__options__defined */


mig_internal kern_return_t __MIG_check__Request__rpc_jack_client_check_t(__Request__rpc_jack_client_check_t *In0P)
{

	typedef __Request__rpc_jack_client_check_t __Request;
#if	__MigTypeCheck
	if ((In0P->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX) ||
	    (In0P->Head.msgh_size != (mach_msg_size_t)sizeof(__Request)))
		return MIG_BAD_ARGUMENTS;
#endif	/* __MigTypeCheck */

#if	defined(__NDR_convert__int_rep__Request__rpc_jack_client_check_t__client_name__defined) || \
	defined(__NDR_convert__int_rep__Request__rpc_jack_client_check_t__protocol__defined) || \
	defined(__NDR_convert__int_rep__Request__rpc_jack_client_check_t__options__defined)
	if (In0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Request__rpc_jack_client_check_t__client_name__defined)
		__NDR_convert__int_rep__Request__rpc_jack_client_check_t__client_name(&In0P->client_name, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_client_check_t__client_name__defined */
#if defined(__NDR_convert__int_rep__Request__rpc_jack_client_check_t__protocol__defined)
		__NDR_convert__int_rep__Request__rpc_jack_client_check_t__protocol(&In0P->protocol, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_client_check_t__protocol__defined */
#if defined(__NDR_convert__int_rep__Request__rpc_jack_client_check_t__options__defined)
		__NDR_convert__int_rep__Request__rpc_jack_client_check_t__options(&In0P->options, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_client_check_t__options__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	defined(__NDR_convert__char_rep__Request__rpc_jack_client_check_t__client_name__defined) || \
	defined(__NDR_convert__char_rep__Request__rpc_jack_client_check_t__protocol__defined) || \
	defined(__NDR_convert__char_rep__Request__rpc_jack_client_check_t__options__defined)
	if (In0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Request__rpc_jack_client_check_t__client_name__defined)
		__NDR_convert__char_rep__Request__rpc_jack_client_check_t__client_name(&In0P->client_name, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_client_check_t__client_name__defined */
#if defined(__NDR_convert__char_rep__Request__rpc_jack_client_check_t__protocol__defined)
		__NDR_convert__char_rep__Request__rpc_jack_client_check_t__protocol(&In0P->protocol, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_client_check_t__protocol__defined */
#if defined(__NDR_convert__char_rep__Request__rpc_jack_client_check_t__options__defined)
		__NDR_convert__char_rep__Request__rpc_jack_client_check_t__options(&In0P->options, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_client_check_t__options__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	defined(__NDR_convert__float_rep__Request__rpc_jack_client_check_t__client_name__defined) || \
	defined(__NDR_convert__float_rep__Request__rpc_jack_client_check_t__protocol__defined) || \
	defined(__NDR_convert__float_rep__Request__rpc_jack_client_check_t__options__defined)
	if (In0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Request__rpc_jack_client_check_t__client_name__defined)
		__NDR_convert__float_rep__Request__rpc_jack_client_check_t__client_name(&In0P->client_name, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_client_check_t__client_name__defined */
#if defined(__NDR_convert__float_rep__Request__rpc_jack_client_check_t__protocol__defined)
		__NDR_convert__float_rep__Request__rpc_jack_client_check_t__protocol(&In0P->protocol, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_client_check_t__protocol__defined */
#if defined(__NDR_convert__float_rep__Request__rpc_jack_client_check_t__options__defined)
		__NDR_convert__float_rep__Request__rpc_jack_client_check_t__options(&In0P->options, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_client_check_t__options__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Request__rpc_jack_client_check_t__defined) */
#endif /* __MIG_check__Request__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_client_check */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t server_rpc_jack_client_check
(
	mach_port_t server_port,
	client_name_t client_name,
	client_name_t client_name_res,
	int protocol,
	int options,
	int *status,
	int *result
);

/* Routine rpc_jack_client_check */
mig_internal novalue _Xrpc_jack_client_check
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP)
{

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		client_name_t client_name;
		int protocol;
		int options;
		mach_msg_trailer_t trailer;
	} Request;
#ifdef  __MigPackStructs
#pragma pack()
#endif
	typedef __Request__rpc_jack_client_check_t __Request;
	typedef __Reply__rpc_jack_client_check_t Reply;

	/*
	 * typedef struct {
	 * 	mach_msg_header_t Head;
	 * 	NDR_record_t NDR;
	 * 	kern_return_t RetCode;
	 * } mig_reply_error_t;
	 */

	Request *In0P = (Request *) InHeadP;
	Reply *OutP = (Reply *) OutHeadP;
#ifdef	__MIG_check__Request__rpc_jack_client_check_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Request__rpc_jack_client_check_t__defined */

	__DeclareRcvRpc(1001, "rpc_jack_client_check")
	__BeforeRcvRpc(1001, "rpc_jack_client_check")

#if	defined(__MIG_check__Request__rpc_jack_client_check_t__defined)
	check_result = __MIG_check__Request__rpc_jack_client_check_t((__Request *)In0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ MIG_RETURN_ERROR(OutP, check_result); }
#endif	/* defined(__MIG_check__Request__rpc_jack_client_check_t__defined) */

	OutP->RetCode = server_rpc_jack_client_check(In0P->Head.msgh_request_port, In0P->client_name, OutP->client_name_res, In0P->protocol, In0P->options, &OutP->status, &OutP->result);
	if (OutP->RetCode != KERN_SUCCESS) {
		MIG_RETURN_ERROR(OutP, OutP->RetCode);
	}

	OutP->NDR = NDR_record;


	OutP->Head.msgh_size = (mach_msg_size_t)(sizeof(Reply));
	__AfterRcvRpc(1001, "rpc_jack_client_check")
}

#if (__MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Request__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Request__rpc_jack_client_close_t__defined)
#define __MIG_check__Request__rpc_jack_client_close_t__defined
#ifndef __NDR_convert__int_rep__Request__rpc_jack_client_close_t__refnum__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_close_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_close_t__refnum(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_close_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_close_t__refnum(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_close_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_close_t__refnum(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_close_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_close_t__refnum(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_client_close_t__refnum__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_client_close_t__refnum__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_close_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_close_t__refnum(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_close_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_close_t__refnum(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_close_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_close_t__refnum(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_close_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_close_t__refnum(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_client_close_t__refnum__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_client_close_t__refnum__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_close_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_close_t__refnum(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_close_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_close_t__refnum(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_close_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_close_t__refnum(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_close_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_close_t__refnum(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_client_close_t__refnum__defined */


mig_internal kern_return_t __MIG_check__Request__rpc_jack_client_close_t(__Request__rpc_jack_client_close_t *In0P)
{

	typedef __Request__rpc_jack_client_close_t __Request;
#if	__MigTypeCheck
	if ((In0P->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX) ||
	    (In0P->Head.msgh_size != (mach_msg_size_t)sizeof(__Request)))
		return MIG_BAD_ARGUMENTS;
#endif	/* __MigTypeCheck */

#if	defined(__NDR_convert__int_rep__Request__rpc_jack_client_close_t__refnum__defined)
	if (In0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Request__rpc_jack_client_close_t__refnum__defined)
		__NDR_convert__int_rep__Request__rpc_jack_client_close_t__refnum(&In0P->refnum, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_client_close_t__refnum__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	defined(__NDR_convert__char_rep__Request__rpc_jack_client_close_t__refnum__defined)
	if (In0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Request__rpc_jack_client_close_t__refnum__defined)
		__NDR_convert__char_rep__Request__rpc_jack_client_close_t__refnum(&In0P->refnum, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_client_close_t__refnum__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	defined(__NDR_convert__float_rep__Request__rpc_jack_client_close_t__refnum__defined)
	if (In0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Request__rpc_jack_client_close_t__refnum__defined)
		__NDR_convert__float_rep__Request__rpc_jack_client_close_t__refnum(&In0P->refnum, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_client_close_t__refnum__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Request__rpc_jack_client_close_t__defined) */
#endif /* __MIG_check__Request__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_client_close */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t server_rpc_jack_client_close
(
	mach_port_t server_port,
	int refnum,
	int *result
);

/* Routine rpc_jack_client_close */
mig_internal novalue _Xrpc_jack_client_close
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP)
{

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
		mach_msg_trailer_t trailer;
	} Request;
#ifdef  __MigPackStructs
#pragma pack()
#endif
	typedef __Request__rpc_jack_client_close_t __Request;
	typedef __Reply__rpc_jack_client_close_t Reply;

	/*
	 * typedef struct {
	 * 	mach_msg_header_t Head;
	 * 	NDR_record_t NDR;
	 * 	kern_return_t RetCode;
	 * } mig_reply_error_t;
	 */

	Request *In0P = (Request *) InHeadP;
	Reply *OutP = (Reply *) OutHeadP;
#ifdef	__MIG_check__Request__rpc_jack_client_close_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Request__rpc_jack_client_close_t__defined */

	__DeclareRcvRpc(1002, "rpc_jack_client_close")
	__BeforeRcvRpc(1002, "rpc_jack_client_close")

#if	defined(__MIG_check__Request__rpc_jack_client_close_t__defined)
	check_result = __MIG_check__Request__rpc_jack_client_close_t((__Request *)In0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ MIG_RETURN_ERROR(OutP, check_result); }
#endif	/* defined(__MIG_check__Request__rpc_jack_client_close_t__defined) */

	OutP->RetCode = server_rpc_jack_client_close(In0P->Head.msgh_request_port, In0P->refnum, &OutP->result);
	if (OutP->RetCode != KERN_SUCCESS) {
		MIG_RETURN_ERROR(OutP, OutP->RetCode);
	}

	OutP->NDR = NDR_record;


	OutP->Head.msgh_size = (mach_msg_size_t)(sizeof(Reply));
	__AfterRcvRpc(1002, "rpc_jack_client_close")
}

#if (__MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Request__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Request__rpc_jack_client_activate_t__defined)
#define __MIG_check__Request__rpc_jack_client_activate_t__defined
#ifndef __NDR_convert__int_rep__Request__rpc_jack_client_activate_t__refnum__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_activate_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_activate_t__refnum(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_activate_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_activate_t__refnum(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_activate_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_activate_t__refnum(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_activate_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_activate_t__refnum(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_client_activate_t__refnum__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_client_activate_t__refnum__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_activate_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_activate_t__refnum(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_activate_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_activate_t__refnum(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_activate_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_activate_t__refnum(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_activate_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_activate_t__refnum(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_client_activate_t__refnum__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_client_activate_t__refnum__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_activate_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_activate_t__refnum(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_activate_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_activate_t__refnum(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_activate_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_activate_t__refnum(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_activate_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_activate_t__refnum(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_client_activate_t__refnum__defined */


mig_internal kern_return_t __MIG_check__Request__rpc_jack_client_activate_t(__Request__rpc_jack_client_activate_t *In0P)
{

	typedef __Request__rpc_jack_client_activate_t __Request;
#if	__MigTypeCheck
	if ((In0P->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX) ||
	    (In0P->Head.msgh_size != (mach_msg_size_t)sizeof(__Request)))
		return MIG_BAD_ARGUMENTS;
#endif	/* __MigTypeCheck */

#if	defined(__NDR_convert__int_rep__Request__rpc_jack_client_activate_t__refnum__defined)
	if (In0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Request__rpc_jack_client_activate_t__refnum__defined)
		__NDR_convert__int_rep__Request__rpc_jack_client_activate_t__refnum(&In0P->refnum, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_client_activate_t__refnum__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	defined(__NDR_convert__char_rep__Request__rpc_jack_client_activate_t__refnum__defined)
	if (In0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Request__rpc_jack_client_activate_t__refnum__defined)
		__NDR_convert__char_rep__Request__rpc_jack_client_activate_t__refnum(&In0P->refnum, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_client_activate_t__refnum__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	defined(__NDR_convert__float_rep__Request__rpc_jack_client_activate_t__refnum__defined)
	if (In0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Request__rpc_jack_client_activate_t__refnum__defined)
		__NDR_convert__float_rep__Request__rpc_jack_client_activate_t__refnum(&In0P->refnum, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_client_activate_t__refnum__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Request__rpc_jack_client_activate_t__defined) */
#endif /* __MIG_check__Request__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_client_activate */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t server_rpc_jack_client_activate
(
	mach_port_t server_port,
	int refnum,
	int *result
);

/* Routine rpc_jack_client_activate */
mig_internal novalue _Xrpc_jack_client_activate
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP)
{

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
		mach_msg_trailer_t trailer;
	} Request;
#ifdef  __MigPackStructs
#pragma pack()
#endif
	typedef __Request__rpc_jack_client_activate_t __Request;
	typedef __Reply__rpc_jack_client_activate_t Reply;

	/*
	 * typedef struct {
	 * 	mach_msg_header_t Head;
	 * 	NDR_record_t NDR;
	 * 	kern_return_t RetCode;
	 * } mig_reply_error_t;
	 */

	Request *In0P = (Request *) InHeadP;
	Reply *OutP = (Reply *) OutHeadP;
#ifdef	__MIG_check__Request__rpc_jack_client_activate_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Request__rpc_jack_client_activate_t__defined */

	__DeclareRcvRpc(1003, "rpc_jack_client_activate")
	__BeforeRcvRpc(1003, "rpc_jack_client_activate")

#if	defined(__MIG_check__Request__rpc_jack_client_activate_t__defined)
	check_result = __MIG_check__Request__rpc_jack_client_activate_t((__Request *)In0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ MIG_RETURN_ERROR(OutP, check_result); }
#endif	/* defined(__MIG_check__Request__rpc_jack_client_activate_t__defined) */

	OutP->RetCode = server_rpc_jack_client_activate(In0P->Head.msgh_request_port, In0P->refnum, &OutP->result);
	if (OutP->RetCode != KERN_SUCCESS) {
		MIG_RETURN_ERROR(OutP, OutP->RetCode);
	}

	OutP->NDR = NDR_record;


	OutP->Head.msgh_size = (mach_msg_size_t)(sizeof(Reply));
	__AfterRcvRpc(1003, "rpc_jack_client_activate")
}

#if (__MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Request__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Request__rpc_jack_client_deactivate_t__defined)
#define __MIG_check__Request__rpc_jack_client_deactivate_t__defined
#ifndef __NDR_convert__int_rep__Request__rpc_jack_client_deactivate_t__refnum__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_deactivate_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_deactivate_t__refnum(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_deactivate_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_deactivate_t__refnum(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_deactivate_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_deactivate_t__refnum(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_deactivate_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_deactivate_t__refnum(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_client_deactivate_t__refnum__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_client_deactivate_t__refnum__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_deactivate_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_deactivate_t__refnum(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_deactivate_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_deactivate_t__refnum(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_deactivate_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_deactivate_t__refnum(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_deactivate_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_deactivate_t__refnum(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_client_deactivate_t__refnum__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_client_deactivate_t__refnum__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_deactivate_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_deactivate_t__refnum(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_deactivate_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_deactivate_t__refnum(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_deactivate_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_deactivate_t__refnum(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_deactivate_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_deactivate_t__refnum(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_client_deactivate_t__refnum__defined */


mig_internal kern_return_t __MIG_check__Request__rpc_jack_client_deactivate_t(__Request__rpc_jack_client_deactivate_t *In0P)
{

	typedef __Request__rpc_jack_client_deactivate_t __Request;
#if	__MigTypeCheck
	if ((In0P->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX) ||
	    (In0P->Head.msgh_size != (mach_msg_size_t)sizeof(__Request)))
		return MIG_BAD_ARGUMENTS;
#endif	/* __MigTypeCheck */

#if	defined(__NDR_convert__int_rep__Request__rpc_jack_client_deactivate_t__refnum__defined)
	if (In0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Request__rpc_jack_client_deactivate_t__refnum__defined)
		__NDR_convert__int_rep__Request__rpc_jack_client_deactivate_t__refnum(&In0P->refnum, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_client_deactivate_t__refnum__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	defined(__NDR_convert__char_rep__Request__rpc_jack_client_deactivate_t__refnum__defined)
	if (In0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Request__rpc_jack_client_deactivate_t__refnum__defined)
		__NDR_convert__char_rep__Request__rpc_jack_client_deactivate_t__refnum(&In0P->refnum, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_client_deactivate_t__refnum__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	defined(__NDR_convert__float_rep__Request__rpc_jack_client_deactivate_t__refnum__defined)
	if (In0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Request__rpc_jack_client_deactivate_t__refnum__defined)
		__NDR_convert__float_rep__Request__rpc_jack_client_deactivate_t__refnum(&In0P->refnum, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_client_deactivate_t__refnum__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Request__rpc_jack_client_deactivate_t__defined) */
#endif /* __MIG_check__Request__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_client_deactivate */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t server_rpc_jack_client_deactivate
(
	mach_port_t server_port,
	int refnum,
	int *result
);

/* Routine rpc_jack_client_deactivate */
mig_internal novalue _Xrpc_jack_client_deactivate
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP)
{

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
		mach_msg_trailer_t trailer;
	} Request;
#ifdef  __MigPackStructs
#pragma pack()
#endif
	typedef __Request__rpc_jack_client_deactivate_t __Request;
	typedef __Reply__rpc_jack_client_deactivate_t Reply;

	/*
	 * typedef struct {
	 * 	mach_msg_header_t Head;
	 * 	NDR_record_t NDR;
	 * 	kern_return_t RetCode;
	 * } mig_reply_error_t;
	 */

	Request *In0P = (Request *) InHeadP;
	Reply *OutP = (Reply *) OutHeadP;
#ifdef	__MIG_check__Request__rpc_jack_client_deactivate_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Request__rpc_jack_client_deactivate_t__defined */

	__DeclareRcvRpc(1004, "rpc_jack_client_deactivate")
	__BeforeRcvRpc(1004, "rpc_jack_client_deactivate")

#if	defined(__MIG_check__Request__rpc_jack_client_deactivate_t__defined)
	check_result = __MIG_check__Request__rpc_jack_client_deactivate_t((__Request *)In0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ MIG_RETURN_ERROR(OutP, check_result); }
#endif	/* defined(__MIG_check__Request__rpc_jack_client_deactivate_t__defined) */

	OutP->RetCode = server_rpc_jack_client_deactivate(In0P->Head.msgh_request_port, In0P->refnum, &OutP->result);
	if (OutP->RetCode != KERN_SUCCESS) {
		MIG_RETURN_ERROR(OutP, OutP->RetCode);
	}

	OutP->NDR = NDR_record;


	OutP->Head.msgh_size = (mach_msg_size_t)(sizeof(Reply));
	__AfterRcvRpc(1004, "rpc_jack_client_deactivate")
}

#if (__MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Request__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Request__rpc_jack_port_register_t__defined)
#define __MIG_check__Request__rpc_jack_port_register_t__defined
#ifndef __NDR_convert__int_rep__Request__rpc_jack_port_register_t__refnum__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__refnum(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__refnum(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__refnum(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__refnum(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_port_register_t__refnum__defined */

#ifndef __NDR_convert__int_rep__Request__rpc_jack_port_register_t__name__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__client_port_name_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__name__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__name(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__client_port_name_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__name__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__name(a, f) \
	__NDR_convert__int_rep__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__string__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__name__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__name(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__string(a, f, 128)
#elif	defined(__NDR_convert__int_rep__string__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__name__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__name(a, f) \
	__NDR_convert__int_rep__string(a, f, 128)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_port_register_t__name__defined */

#ifndef __NDR_convert__int_rep__Request__rpc_jack_port_register_t__flags__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__unsigned__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__flags__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__flags(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__unsigned((unsigned *)(a), f)
#elif	defined(__NDR_convert__int_rep__unsigned__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__flags__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__flags(a, f) \
	__NDR_convert__int_rep__unsigned((unsigned *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__uint32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__flags__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__flags(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__uint32_t((uint32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__uint32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__flags__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__flags(a, f) \
	__NDR_convert__int_rep__uint32_t((uint32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_port_register_t__flags__defined */

#ifndef __NDR_convert__int_rep__Request__rpc_jack_port_register_t__buffer_size__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__unsigned__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__buffer_size__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__buffer_size(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__unsigned((unsigned *)(a), f)
#elif	defined(__NDR_convert__int_rep__unsigned__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__buffer_size__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__buffer_size(a, f) \
	__NDR_convert__int_rep__unsigned((unsigned *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__uint32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__buffer_size__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__buffer_size(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__uint32_t((uint32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__uint32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__buffer_size__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_register_t__buffer_size(a, f) \
	__NDR_convert__int_rep__uint32_t((uint32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_port_register_t__buffer_size__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_port_register_t__refnum__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__refnum(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__refnum(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__refnum(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__refnum(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_port_register_t__refnum__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_port_register_t__name__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__client_port_name_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__name__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__name(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__client_port_name_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__name__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__name(a, f) \
	__NDR_convert__char_rep__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__string__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__name__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__name(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__string(a, f, 128)
#elif	defined(__NDR_convert__char_rep__string__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__name__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__name(a, f) \
	__NDR_convert__char_rep__string(a, f, 128)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_port_register_t__name__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_port_register_t__flags__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__unsigned__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__flags__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__flags(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__unsigned((unsigned *)(a), f)
#elif	defined(__NDR_convert__char_rep__unsigned__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__flags__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__flags(a, f) \
	__NDR_convert__char_rep__unsigned((unsigned *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__uint32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__flags__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__flags(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__uint32_t((uint32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__uint32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__flags__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__flags(a, f) \
	__NDR_convert__char_rep__uint32_t((uint32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_port_register_t__flags__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_port_register_t__buffer_size__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__unsigned__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__buffer_size__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__buffer_size(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__unsigned((unsigned *)(a), f)
#elif	defined(__NDR_convert__char_rep__unsigned__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__buffer_size__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__buffer_size(a, f) \
	__NDR_convert__char_rep__unsigned((unsigned *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__uint32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__buffer_size__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__buffer_size(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__uint32_t((uint32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__uint32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__buffer_size__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_register_t__buffer_size(a, f) \
	__NDR_convert__char_rep__uint32_t((uint32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_port_register_t__buffer_size__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_port_register_t__refnum__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__refnum(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__refnum(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__refnum(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__refnum(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_port_register_t__refnum__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_port_register_t__name__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__client_port_name_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__name__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__name(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__client_port_name_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__name__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__name(a, f) \
	__NDR_convert__float_rep__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__string__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__name__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__name(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__string(a, f, 128)
#elif	defined(__NDR_convert__float_rep__string__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__name__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__name(a, f) \
	__NDR_convert__float_rep__string(a, f, 128)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_port_register_t__name__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_port_register_t__flags__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__unsigned__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__flags__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__flags(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__unsigned((unsigned *)(a), f)
#elif	defined(__NDR_convert__float_rep__unsigned__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__flags__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__flags(a, f) \
	__NDR_convert__float_rep__unsigned((unsigned *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__uint32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__flags__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__flags(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__uint32_t((uint32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__uint32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__flags__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__flags(a, f) \
	__NDR_convert__float_rep__uint32_t((uint32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_port_register_t__flags__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_port_register_t__buffer_size__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__unsigned__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__buffer_size__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__buffer_size(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__unsigned((unsigned *)(a), f)
#elif	defined(__NDR_convert__float_rep__unsigned__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__buffer_size__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__buffer_size(a, f) \
	__NDR_convert__float_rep__unsigned((unsigned *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__uint32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__buffer_size__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__buffer_size(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__uint32_t((uint32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__uint32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__buffer_size__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_register_t__buffer_size(a, f) \
	__NDR_convert__float_rep__uint32_t((uint32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_port_register_t__buffer_size__defined */


mig_internal kern_return_t __MIG_check__Request__rpc_jack_port_register_t(__Request__rpc_jack_port_register_t *In0P)
{

	typedef __Request__rpc_jack_port_register_t __Request;
#if	__MigTypeCheck
	if ((In0P->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX) ||
	    (In0P->Head.msgh_size != (mach_msg_size_t)sizeof(__Request)))
		return MIG_BAD_ARGUMENTS;
#endif	/* __MigTypeCheck */

#if	defined(__NDR_convert__int_rep__Request__rpc_jack_port_register_t__refnum__defined) || \
	defined(__NDR_convert__int_rep__Request__rpc_jack_port_register_t__name__defined) || \
	defined(__NDR_convert__int_rep__Request__rpc_jack_port_register_t__flags__defined) || \
	defined(__NDR_convert__int_rep__Request__rpc_jack_port_register_t__buffer_size__defined)
	if (In0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Request__rpc_jack_port_register_t__refnum__defined)
		__NDR_convert__int_rep__Request__rpc_jack_port_register_t__refnum(&In0P->refnum, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_port_register_t__refnum__defined */
#if defined(__NDR_convert__int_rep__Request__rpc_jack_port_register_t__name__defined)
		__NDR_convert__int_rep__Request__rpc_jack_port_register_t__name(&In0P->name, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_port_register_t__name__defined */
#if defined(__NDR_convert__int_rep__Request__rpc_jack_port_register_t__flags__defined)
		__NDR_convert__int_rep__Request__rpc_jack_port_register_t__flags(&In0P->flags, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_port_register_t__flags__defined */
#if defined(__NDR_convert__int_rep__Request__rpc_jack_port_register_t__buffer_size__defined)
		__NDR_convert__int_rep__Request__rpc_jack_port_register_t__buffer_size(&In0P->buffer_size, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_port_register_t__buffer_size__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	defined(__NDR_convert__char_rep__Request__rpc_jack_port_register_t__refnum__defined) || \
	defined(__NDR_convert__char_rep__Request__rpc_jack_port_register_t__name__defined) || \
	defined(__NDR_convert__char_rep__Request__rpc_jack_port_register_t__flags__defined) || \
	defined(__NDR_convert__char_rep__Request__rpc_jack_port_register_t__buffer_size__defined)
	if (In0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Request__rpc_jack_port_register_t__refnum__defined)
		__NDR_convert__char_rep__Request__rpc_jack_port_register_t__refnum(&In0P->refnum, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_port_register_t__refnum__defined */
#if defined(__NDR_convert__char_rep__Request__rpc_jack_port_register_t__name__defined)
		__NDR_convert__char_rep__Request__rpc_jack_port_register_t__name(&In0P->name, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_port_register_t__name__defined */
#if defined(__NDR_convert__char_rep__Request__rpc_jack_port_register_t__flags__defined)
		__NDR_convert__char_rep__Request__rpc_jack_port_register_t__flags(&In0P->flags, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_port_register_t__flags__defined */
#if defined(__NDR_convert__char_rep__Request__rpc_jack_port_register_t__buffer_size__defined)
		__NDR_convert__char_rep__Request__rpc_jack_port_register_t__buffer_size(&In0P->buffer_size, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_port_register_t__buffer_size__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	defined(__NDR_convert__float_rep__Request__rpc_jack_port_register_t__refnum__defined) || \
	defined(__NDR_convert__float_rep__Request__rpc_jack_port_register_t__name__defined) || \
	defined(__NDR_convert__float_rep__Request__rpc_jack_port_register_t__flags__defined) || \
	defined(__NDR_convert__float_rep__Request__rpc_jack_port_register_t__buffer_size__defined)
	if (In0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Request__rpc_jack_port_register_t__refnum__defined)
		__NDR_convert__float_rep__Request__rpc_jack_port_register_t__refnum(&In0P->refnum, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_port_register_t__refnum__defined */
#if defined(__NDR_convert__float_rep__Request__rpc_jack_port_register_t__name__defined)
		__NDR_convert__float_rep__Request__rpc_jack_port_register_t__name(&In0P->name, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_port_register_t__name__defined */
#if defined(__NDR_convert__float_rep__Request__rpc_jack_port_register_t__flags__defined)
		__NDR_convert__float_rep__Request__rpc_jack_port_register_t__flags(&In0P->flags, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_port_register_t__flags__defined */
#if defined(__NDR_convert__float_rep__Request__rpc_jack_port_register_t__buffer_size__defined)
		__NDR_convert__float_rep__Request__rpc_jack_port_register_t__buffer_size(&In0P->buffer_size, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_port_register_t__buffer_size__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Request__rpc_jack_port_register_t__defined) */
#endif /* __MIG_check__Request__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_port_register */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t server_rpc_jack_port_register
(
	mach_port_t server_port,
	int refnum,
	client_port_name_t name,
	unsigned flags,
	unsigned buffer_size,
	unsigned *port_index,
	int *result
);

/* Routine rpc_jack_port_register */
mig_internal novalue _Xrpc_jack_port_register
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP)
{

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
		client_port_name_t name;
		unsigned flags;
		unsigned buffer_size;
		mach_msg_trailer_t trailer;
	} Request;
#ifdef  __MigPackStructs
#pragma pack()
#endif
	typedef __Request__rpc_jack_port_register_t __Request;
	typedef __Reply__rpc_jack_port_register_t Reply;

	/*
	 * typedef struct {
	 * 	mach_msg_header_t Head;
	 * 	NDR_record_t NDR;
	 * 	kern_return_t RetCode;
	 * } mig_reply_error_t;
	 */

	Request *In0P = (Request *) InHeadP;
	Reply *OutP = (Reply *) OutHeadP;
#ifdef	__MIG_check__Request__rpc_jack_port_register_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Request__rpc_jack_port_register_t__defined */

	__DeclareRcvRpc(1005, "rpc_jack_port_register")
	__BeforeRcvRpc(1005, "rpc_jack_port_register")

#if	defined(__MIG_check__Request__rpc_jack_port_register_t__defined)
	check_result = __MIG_check__Request__rpc_jack_port_register_t((__Request *)In0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ MIG_RETURN_ERROR(OutP, check_result); }
#endif	/* defined(__MIG_check__Request__rpc_jack_port_register_t__defined) */

	OutP->RetCode = server_rpc_jack_port_register(In0P->Head.msgh_request_port, In0P->refnum, In0P->name, In0P->flags, In0P->buffer_size, &OutP->port_index, &OutP->result);
	if (OutP->RetCode != KERN_SUCCESS) {
		MIG_RETURN_ERROR(OutP, OutP->RetCode);
	}

	OutP->NDR = NDR_record;


	OutP->Head.msgh_size = (mach_msg_size_t)(sizeof(Reply));
	__AfterRcvRpc(1005, "rpc_jack_port_register")
}

#if (__MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Request__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Request__rpc_jack_port_unregister_t__defined)
#define __MIG_check__Request__rpc_jack_port_unregister_t__defined
#ifndef __NDR_convert__int_rep__Request__rpc_jack_port_unregister_t__refnum__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_unregister_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_unregister_t__refnum(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_unregister_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_unregister_t__refnum(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_unregister_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_unregister_t__refnum(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_unregister_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_unregister_t__refnum(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_port_unregister_t__refnum__defined */

#ifndef __NDR_convert__int_rep__Request__rpc_jack_port_unregister_t__port__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_unregister_t__port__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_unregister_t__port(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_unregister_t__port__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_unregister_t__port(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_unregister_t__port__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_unregister_t__port(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_unregister_t__port__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_unregister_t__port(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_port_unregister_t__port__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_port_unregister_t__refnum__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_unregister_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_unregister_t__refnum(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_unregister_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_unregister_t__refnum(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_unregister_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_unregister_t__refnum(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_unregister_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_unregister_t__refnum(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_port_unregister_t__refnum__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_port_unregister_t__port__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_unregister_t__port__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_unregister_t__port(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_unregister_t__port__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_unregister_t__port(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_unregister_t__port__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_unregister_t__port(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_unregister_t__port__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_unregister_t__port(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_port_unregister_t__port__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_port_unregister_t__refnum__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_unregister_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_unregister_t__refnum(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_unregister_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_unregister_t__refnum(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_unregister_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_unregister_t__refnum(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_unregister_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_unregister_t__refnum(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_port_unregister_t__refnum__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_port_unregister_t__port__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_unregister_t__port__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_unregister_t__port(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_unregister_t__port__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_unregister_t__port(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_unregister_t__port__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_unregister_t__port(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_unregister_t__port__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_unregister_t__port(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_port_unregister_t__port__defined */


mig_internal kern_return_t __MIG_check__Request__rpc_jack_port_unregister_t(__Request__rpc_jack_port_unregister_t *In0P)
{

	typedef __Request__rpc_jack_port_unregister_t __Request;
#if	__MigTypeCheck
	if ((In0P->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX) ||
	    (In0P->Head.msgh_size != (mach_msg_size_t)sizeof(__Request)))
		return MIG_BAD_ARGUMENTS;
#endif	/* __MigTypeCheck */

#if	defined(__NDR_convert__int_rep__Request__rpc_jack_port_unregister_t__refnum__defined) || \
	defined(__NDR_convert__int_rep__Request__rpc_jack_port_unregister_t__port__defined)
	if (In0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Request__rpc_jack_port_unregister_t__refnum__defined)
		__NDR_convert__int_rep__Request__rpc_jack_port_unregister_t__refnum(&In0P->refnum, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_port_unregister_t__refnum__defined */
#if defined(__NDR_convert__int_rep__Request__rpc_jack_port_unregister_t__port__defined)
		__NDR_convert__int_rep__Request__rpc_jack_port_unregister_t__port(&In0P->port, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_port_unregister_t__port__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	defined(__NDR_convert__char_rep__Request__rpc_jack_port_unregister_t__refnum__defined) || \
	defined(__NDR_convert__char_rep__Request__rpc_jack_port_unregister_t__port__defined)
	if (In0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Request__rpc_jack_port_unregister_t__refnum__defined)
		__NDR_convert__char_rep__Request__rpc_jack_port_unregister_t__refnum(&In0P->refnum, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_port_unregister_t__refnum__defined */
#if defined(__NDR_convert__char_rep__Request__rpc_jack_port_unregister_t__port__defined)
		__NDR_convert__char_rep__Request__rpc_jack_port_unregister_t__port(&In0P->port, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_port_unregister_t__port__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	defined(__NDR_convert__float_rep__Request__rpc_jack_port_unregister_t__refnum__defined) || \
	defined(__NDR_convert__float_rep__Request__rpc_jack_port_unregister_t__port__defined)
	if (In0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Request__rpc_jack_port_unregister_t__refnum__defined)
		__NDR_convert__float_rep__Request__rpc_jack_port_unregister_t__refnum(&In0P->refnum, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_port_unregister_t__refnum__defined */
#if defined(__NDR_convert__float_rep__Request__rpc_jack_port_unregister_t__port__defined)
		__NDR_convert__float_rep__Request__rpc_jack_port_unregister_t__port(&In0P->port, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_port_unregister_t__port__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Request__rpc_jack_port_unregister_t__defined) */
#endif /* __MIG_check__Request__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_port_unregister */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t server_rpc_jack_port_unregister
(
	mach_port_t server_port,
	int refnum,
	int port,
	int *result
);

/* Routine rpc_jack_port_unregister */
mig_internal novalue _Xrpc_jack_port_unregister
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP)
{

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
		int port;
		mach_msg_trailer_t trailer;
	} Request;
#ifdef  __MigPackStructs
#pragma pack()
#endif
	typedef __Request__rpc_jack_port_unregister_t __Request;
	typedef __Reply__rpc_jack_port_unregister_t Reply;

	/*
	 * typedef struct {
	 * 	mach_msg_header_t Head;
	 * 	NDR_record_t NDR;
	 * 	kern_return_t RetCode;
	 * } mig_reply_error_t;
	 */

	Request *In0P = (Request *) InHeadP;
	Reply *OutP = (Reply *) OutHeadP;
#ifdef	__MIG_check__Request__rpc_jack_port_unregister_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Request__rpc_jack_port_unregister_t__defined */

	__DeclareRcvRpc(1006, "rpc_jack_port_unregister")
	__BeforeRcvRpc(1006, "rpc_jack_port_unregister")

#if	defined(__MIG_check__Request__rpc_jack_port_unregister_t__defined)
	check_result = __MIG_check__Request__rpc_jack_port_unregister_t((__Request *)In0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ MIG_RETURN_ERROR(OutP, check_result); }
#endif	/* defined(__MIG_check__Request__rpc_jack_port_unregister_t__defined) */

	OutP->RetCode = server_rpc_jack_port_unregister(In0P->Head.msgh_request_port, In0P->refnum, In0P->port, &OutP->result);
	if (OutP->RetCode != KERN_SUCCESS) {
		MIG_RETURN_ERROR(OutP, OutP->RetCode);
	}

	OutP->NDR = NDR_record;


	OutP->Head.msgh_size = (mach_msg_size_t)(sizeof(Reply));
	__AfterRcvRpc(1006, "rpc_jack_port_unregister")
}

#if (__MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Request__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Request__rpc_jack_port_connect_t__defined)
#define __MIG_check__Request__rpc_jack_port_connect_t__defined
#ifndef __NDR_convert__int_rep__Request__rpc_jack_port_connect_t__refnum__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__refnum(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__refnum(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__refnum(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__refnum(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_port_connect_t__refnum__defined */

#ifndef __NDR_convert__int_rep__Request__rpc_jack_port_connect_t__src__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__src__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__src(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__src__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__src(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__src__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__src(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__src__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__src(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_port_connect_t__src__defined */

#ifndef __NDR_convert__int_rep__Request__rpc_jack_port_connect_t__dst__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__dst__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__dst(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__dst__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__dst(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__dst__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__dst(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__dst__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__dst(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_port_connect_t__dst__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_port_connect_t__refnum__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__refnum(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__refnum(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__refnum(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__refnum(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_port_connect_t__refnum__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_port_connect_t__src__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__src__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__src(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__src__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__src(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__src__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__src(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__src__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__src(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_port_connect_t__src__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_port_connect_t__dst__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__dst__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__dst(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__dst__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__dst(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__dst__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__dst(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__dst__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__dst(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_port_connect_t__dst__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_port_connect_t__refnum__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__refnum(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__refnum(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__refnum(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__refnum(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_port_connect_t__refnum__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_port_connect_t__src__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__src__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__src(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__src__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__src(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__src__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__src(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__src__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__src(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_port_connect_t__src__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_port_connect_t__dst__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__dst__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__dst(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__dst__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__dst(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__dst__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__dst(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__dst__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__dst(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_port_connect_t__dst__defined */


mig_internal kern_return_t __MIG_check__Request__rpc_jack_port_connect_t(__Request__rpc_jack_port_connect_t *In0P)
{

	typedef __Request__rpc_jack_port_connect_t __Request;
#if	__MigTypeCheck
	if ((In0P->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX) ||
	    (In0P->Head.msgh_size != (mach_msg_size_t)sizeof(__Request)))
		return MIG_BAD_ARGUMENTS;
#endif	/* __MigTypeCheck */

#if	defined(__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__refnum__defined) || \
	defined(__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__src__defined) || \
	defined(__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__dst__defined)
	if (In0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__refnum__defined)
		__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__refnum(&In0P->refnum, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_port_connect_t__refnum__defined */
#if defined(__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__src__defined)
		__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__src(&In0P->src, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_port_connect_t__src__defined */
#if defined(__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__dst__defined)
		__NDR_convert__int_rep__Request__rpc_jack_port_connect_t__dst(&In0P->dst, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_port_connect_t__dst__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	defined(__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__refnum__defined) || \
	defined(__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__src__defined) || \
	defined(__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__dst__defined)
	if (In0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__refnum__defined)
		__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__refnum(&In0P->refnum, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_port_connect_t__refnum__defined */
#if defined(__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__src__defined)
		__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__src(&In0P->src, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_port_connect_t__src__defined */
#if defined(__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__dst__defined)
		__NDR_convert__char_rep__Request__rpc_jack_port_connect_t__dst(&In0P->dst, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_port_connect_t__dst__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	defined(__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__refnum__defined) || \
	defined(__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__src__defined) || \
	defined(__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__dst__defined)
	if (In0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__refnum__defined)
		__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__refnum(&In0P->refnum, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_port_connect_t__refnum__defined */
#if defined(__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__src__defined)
		__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__src(&In0P->src, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_port_connect_t__src__defined */
#if defined(__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__dst__defined)
		__NDR_convert__float_rep__Request__rpc_jack_port_connect_t__dst(&In0P->dst, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_port_connect_t__dst__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Request__rpc_jack_port_connect_t__defined) */
#endif /* __MIG_check__Request__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_port_connect */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t server_rpc_jack_port_connect
(
	mach_port_t server_port,
	int refnum,
	int src,
	int dst,
	int *result
);

/* Routine rpc_jack_port_connect */
mig_internal novalue _Xrpc_jack_port_connect
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP)
{

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
		int src;
		int dst;
		mach_msg_trailer_t trailer;
	} Request;
#ifdef  __MigPackStructs
#pragma pack()
#endif
	typedef __Request__rpc_jack_port_connect_t __Request;
	typedef __Reply__rpc_jack_port_connect_t Reply;

	/*
	 * typedef struct {
	 * 	mach_msg_header_t Head;
	 * 	NDR_record_t NDR;
	 * 	kern_return_t RetCode;
	 * } mig_reply_error_t;
	 */

	Request *In0P = (Request *) InHeadP;
	Reply *OutP = (Reply *) OutHeadP;
#ifdef	__MIG_check__Request__rpc_jack_port_connect_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Request__rpc_jack_port_connect_t__defined */

	__DeclareRcvRpc(1007, "rpc_jack_port_connect")
	__BeforeRcvRpc(1007, "rpc_jack_port_connect")

#if	defined(__MIG_check__Request__rpc_jack_port_connect_t__defined)
	check_result = __MIG_check__Request__rpc_jack_port_connect_t((__Request *)In0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ MIG_RETURN_ERROR(OutP, check_result); }
#endif	/* defined(__MIG_check__Request__rpc_jack_port_connect_t__defined) */

	OutP->RetCode = server_rpc_jack_port_connect(In0P->Head.msgh_request_port, In0P->refnum, In0P->src, In0P->dst, &OutP->result);
	if (OutP->RetCode != KERN_SUCCESS) {
		MIG_RETURN_ERROR(OutP, OutP->RetCode);
	}

	OutP->NDR = NDR_record;


	OutP->Head.msgh_size = (mach_msg_size_t)(sizeof(Reply));
	__AfterRcvRpc(1007, "rpc_jack_port_connect")
}

#if (__MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Request__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Request__rpc_jack_port_disconnect_t__defined)
#define __MIG_check__Request__rpc_jack_port_disconnect_t__defined
#ifndef __NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__refnum__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__refnum(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__refnum(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__refnum(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__refnum(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__refnum__defined */

#ifndef __NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__src__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__src__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__src(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__src__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__src(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__src__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__src(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__src__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__src(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__src__defined */

#ifndef __NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__dst__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__dst__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__dst(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__dst__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__dst(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__dst__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__dst(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__dst__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__dst(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__dst__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__refnum__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__refnum(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__refnum(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__refnum(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__refnum(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__refnum__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__src__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__src__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__src(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__src__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__src(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__src__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__src(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__src__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__src(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__src__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__dst__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__dst__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__dst(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__dst__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__dst(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__dst__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__dst(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__dst__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__dst(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__dst__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__refnum__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__refnum(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__refnum(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__refnum(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__refnum(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__refnum__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__src__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__src__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__src(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__src__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__src(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__src__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__src(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__src__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__src(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__src__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__dst__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__dst__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__dst(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__dst__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__dst(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__dst__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__dst(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__dst__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__dst(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__dst__defined */


mig_internal kern_return_t __MIG_check__Request__rpc_jack_port_disconnect_t(__Request__rpc_jack_port_disconnect_t *In0P)
{

	typedef __Request__rpc_jack_port_disconnect_t __Request;
#if	__MigTypeCheck
	if ((In0P->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX) ||
	    (In0P->Head.msgh_size != (mach_msg_size_t)sizeof(__Request)))
		return MIG_BAD_ARGUMENTS;
#endif	/* __MigTypeCheck */

#if	defined(__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__refnum__defined) || \
	defined(__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__src__defined) || \
	defined(__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__dst__defined)
	if (In0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__refnum__defined)
		__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__refnum(&In0P->refnum, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__refnum__defined */
#if defined(__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__src__defined)
		__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__src(&In0P->src, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__src__defined */
#if defined(__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__dst__defined)
		__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__dst(&In0P->dst, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_port_disconnect_t__dst__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	defined(__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__refnum__defined) || \
	defined(__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__src__defined) || \
	defined(__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__dst__defined)
	if (In0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__refnum__defined)
		__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__refnum(&In0P->refnum, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__refnum__defined */
#if defined(__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__src__defined)
		__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__src(&In0P->src, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__src__defined */
#if defined(__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__dst__defined)
		__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__dst(&In0P->dst, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_port_disconnect_t__dst__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	defined(__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__refnum__defined) || \
	defined(__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__src__defined) || \
	defined(__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__dst__defined)
	if (In0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__refnum__defined)
		__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__refnum(&In0P->refnum, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__refnum__defined */
#if defined(__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__src__defined)
		__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__src(&In0P->src, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__src__defined */
#if defined(__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__dst__defined)
		__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__dst(&In0P->dst, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_port_disconnect_t__dst__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Request__rpc_jack_port_disconnect_t__defined) */
#endif /* __MIG_check__Request__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_port_disconnect */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t server_rpc_jack_port_disconnect
(
	mach_port_t server_port,
	int refnum,
	int src,
	int dst,
	int *result
);

/* Routine rpc_jack_port_disconnect */
mig_internal novalue _Xrpc_jack_port_disconnect
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP)
{

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
		int src;
		int dst;
		mach_msg_trailer_t trailer;
	} Request;
#ifdef  __MigPackStructs
#pragma pack()
#endif
	typedef __Request__rpc_jack_port_disconnect_t __Request;
	typedef __Reply__rpc_jack_port_disconnect_t Reply;

	/*
	 * typedef struct {
	 * 	mach_msg_header_t Head;
	 * 	NDR_record_t NDR;
	 * 	kern_return_t RetCode;
	 * } mig_reply_error_t;
	 */

	Request *In0P = (Request *) InHeadP;
	Reply *OutP = (Reply *) OutHeadP;
#ifdef	__MIG_check__Request__rpc_jack_port_disconnect_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Request__rpc_jack_port_disconnect_t__defined */

	__DeclareRcvRpc(1008, "rpc_jack_port_disconnect")
	__BeforeRcvRpc(1008, "rpc_jack_port_disconnect")

#if	defined(__MIG_check__Request__rpc_jack_port_disconnect_t__defined)
	check_result = __MIG_check__Request__rpc_jack_port_disconnect_t((__Request *)In0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ MIG_RETURN_ERROR(OutP, check_result); }
#endif	/* defined(__MIG_check__Request__rpc_jack_port_disconnect_t__defined) */

	OutP->RetCode = server_rpc_jack_port_disconnect(In0P->Head.msgh_request_port, In0P->refnum, In0P->src, In0P->dst, &OutP->result);
	if (OutP->RetCode != KERN_SUCCESS) {
		MIG_RETURN_ERROR(OutP, OutP->RetCode);
	}

	OutP->NDR = NDR_record;


	OutP->Head.msgh_size = (mach_msg_size_t)(sizeof(Reply));
	__AfterRcvRpc(1008, "rpc_jack_port_disconnect")
}

#if (__MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Request__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Request__rpc_jack_port_connect_name_t__defined)
#define __MIG_check__Request__rpc_jack_port_connect_name_t__defined
#ifndef __NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__refnum__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__refnum(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__refnum(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__refnum(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__refnum(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__refnum__defined */

#ifndef __NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__src__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__client_port_name_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__src__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__src(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__client_port_name_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__src__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__src(a, f) \
	__NDR_convert__int_rep__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__string__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__src__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__src(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__string(a, f, 128)
#elif	defined(__NDR_convert__int_rep__string__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__src__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__src(a, f) \
	__NDR_convert__int_rep__string(a, f, 128)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__src__defined */

#ifndef __NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__dst__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__client_port_name_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__dst__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__dst(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__client_port_name_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__dst__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__dst(a, f) \
	__NDR_convert__int_rep__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__string__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__dst__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__dst(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__string(a, f, 128)
#elif	defined(__NDR_convert__int_rep__string__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__dst__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__dst(a, f) \
	__NDR_convert__int_rep__string(a, f, 128)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__dst__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__refnum__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__refnum(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__refnum(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__refnum(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__refnum(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__refnum__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__src__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__client_port_name_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__src__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__src(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__client_port_name_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__src__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__src(a, f) \
	__NDR_convert__char_rep__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__string__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__src__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__src(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__string(a, f, 128)
#elif	defined(__NDR_convert__char_rep__string__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__src__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__src(a, f) \
	__NDR_convert__char_rep__string(a, f, 128)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__src__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__dst__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__client_port_name_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__dst__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__dst(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__client_port_name_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__dst__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__dst(a, f) \
	__NDR_convert__char_rep__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__string__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__dst__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__dst(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__string(a, f, 128)
#elif	defined(__NDR_convert__char_rep__string__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__dst__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__dst(a, f) \
	__NDR_convert__char_rep__string(a, f, 128)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__dst__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__refnum__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__refnum(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__refnum(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__refnum(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__refnum(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__refnum__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__src__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__client_port_name_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__src__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__src(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__client_port_name_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__src__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__src(a, f) \
	__NDR_convert__float_rep__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__string__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__src__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__src(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__string(a, f, 128)
#elif	defined(__NDR_convert__float_rep__string__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__src__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__src(a, f) \
	__NDR_convert__float_rep__string(a, f, 128)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__src__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__dst__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__client_port_name_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__dst__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__dst(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__client_port_name_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__dst__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__dst(a, f) \
	__NDR_convert__float_rep__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__string__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__dst__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__dst(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__string(a, f, 128)
#elif	defined(__NDR_convert__float_rep__string__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__dst__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__dst(a, f) \
	__NDR_convert__float_rep__string(a, f, 128)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__dst__defined */


mig_internal kern_return_t __MIG_check__Request__rpc_jack_port_connect_name_t(__Request__rpc_jack_port_connect_name_t *In0P)
{

	typedef __Request__rpc_jack_port_connect_name_t __Request;
#if	__MigTypeCheck
	if ((In0P->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX) ||
	    (In0P->Head.msgh_size != (mach_msg_size_t)sizeof(__Request)))
		return MIG_BAD_ARGUMENTS;
#endif	/* __MigTypeCheck */

#if	defined(__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__refnum__defined) || \
	defined(__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__src__defined) || \
	defined(__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__dst__defined)
	if (In0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__refnum__defined)
		__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__refnum(&In0P->refnum, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__refnum__defined */
#if defined(__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__src__defined)
		__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__src(&In0P->src, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__src__defined */
#if defined(__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__dst__defined)
		__NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__dst(&In0P->dst, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_port_connect_name_t__dst__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	defined(__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__refnum__defined) || \
	defined(__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__src__defined) || \
	defined(__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__dst__defined)
	if (In0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__refnum__defined)
		__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__refnum(&In0P->refnum, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__refnum__defined */
#if defined(__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__src__defined)
		__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__src(&In0P->src, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__src__defined */
#if defined(__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__dst__defined)
		__NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__dst(&In0P->dst, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_port_connect_name_t__dst__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	defined(__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__refnum__defined) || \
	defined(__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__src__defined) || \
	defined(__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__dst__defined)
	if (In0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__refnum__defined)
		__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__refnum(&In0P->refnum, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__refnum__defined */
#if defined(__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__src__defined)
		__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__src(&In0P->src, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__src__defined */
#if defined(__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__dst__defined)
		__NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__dst(&In0P->dst, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_port_connect_name_t__dst__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Request__rpc_jack_port_connect_name_t__defined) */
#endif /* __MIG_check__Request__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_port_connect_name */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t server_rpc_jack_port_connect_name
(
	mach_port_t server_port,
	int refnum,
	client_port_name_t src,
	client_port_name_t dst,
	int *result
);

/* Routine rpc_jack_port_connect_name */
mig_internal novalue _Xrpc_jack_port_connect_name
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP)
{

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
		client_port_name_t src;
		client_port_name_t dst;
		mach_msg_trailer_t trailer;
	} Request;
#ifdef  __MigPackStructs
#pragma pack()
#endif
	typedef __Request__rpc_jack_port_connect_name_t __Request;
	typedef __Reply__rpc_jack_port_connect_name_t Reply;

	/*
	 * typedef struct {
	 * 	mach_msg_header_t Head;
	 * 	NDR_record_t NDR;
	 * 	kern_return_t RetCode;
	 * } mig_reply_error_t;
	 */

	Request *In0P = (Request *) InHeadP;
	Reply *OutP = (Reply *) OutHeadP;
#ifdef	__MIG_check__Request__rpc_jack_port_connect_name_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Request__rpc_jack_port_connect_name_t__defined */

	__DeclareRcvRpc(1009, "rpc_jack_port_connect_name")
	__BeforeRcvRpc(1009, "rpc_jack_port_connect_name")

#if	defined(__MIG_check__Request__rpc_jack_port_connect_name_t__defined)
	check_result = __MIG_check__Request__rpc_jack_port_connect_name_t((__Request *)In0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ MIG_RETURN_ERROR(OutP, check_result); }
#endif	/* defined(__MIG_check__Request__rpc_jack_port_connect_name_t__defined) */

	OutP->RetCode = server_rpc_jack_port_connect_name(In0P->Head.msgh_request_port, In0P->refnum, In0P->src, In0P->dst, &OutP->result);
	if (OutP->RetCode != KERN_SUCCESS) {
		MIG_RETURN_ERROR(OutP, OutP->RetCode);
	}

	OutP->NDR = NDR_record;


	OutP->Head.msgh_size = (mach_msg_size_t)(sizeof(Reply));
	__AfterRcvRpc(1009, "rpc_jack_port_connect_name")
}

#if (__MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Request__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Request__rpc_jack_port_disconnect_name_t__defined)
#define __MIG_check__Request__rpc_jack_port_disconnect_name_t__defined
#ifndef __NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__refnum__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__refnum(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__refnum(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__refnum(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__refnum(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__refnum__defined */

#ifndef __NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__src__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__client_port_name_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__src__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__src(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__client_port_name_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__src__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__src(a, f) \
	__NDR_convert__int_rep__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__string__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__src__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__src(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__string(a, f, 128)
#elif	defined(__NDR_convert__int_rep__string__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__src__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__src(a, f) \
	__NDR_convert__int_rep__string(a, f, 128)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__src__defined */

#ifndef __NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__dst__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__client_port_name_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__dst__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__dst(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__client_port_name_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__dst__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__dst(a, f) \
	__NDR_convert__int_rep__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__string__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__dst__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__dst(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__string(a, f, 128)
#elif	defined(__NDR_convert__int_rep__string__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__dst__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__dst(a, f) \
	__NDR_convert__int_rep__string(a, f, 128)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__dst__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__refnum__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__refnum(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__refnum(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__refnum(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__refnum(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__refnum__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__src__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__client_port_name_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__src__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__src(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__client_port_name_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__src__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__src(a, f) \
	__NDR_convert__char_rep__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__string__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__src__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__src(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__string(a, f, 128)
#elif	defined(__NDR_convert__char_rep__string__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__src__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__src(a, f) \
	__NDR_convert__char_rep__string(a, f, 128)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__src__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__dst__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__client_port_name_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__dst__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__dst(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__client_port_name_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__dst__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__dst(a, f) \
	__NDR_convert__char_rep__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__string__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__dst__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__dst(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__string(a, f, 128)
#elif	defined(__NDR_convert__char_rep__string__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__dst__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__dst(a, f) \
	__NDR_convert__char_rep__string(a, f, 128)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__dst__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__refnum__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__refnum(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__refnum(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__refnum(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__refnum(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__refnum__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__src__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__client_port_name_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__src__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__src(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__client_port_name_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__src__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__src(a, f) \
	__NDR_convert__float_rep__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__string__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__src__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__src(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__string(a, f, 128)
#elif	defined(__NDR_convert__float_rep__string__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__src__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__src(a, f) \
	__NDR_convert__float_rep__string(a, f, 128)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__src__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__dst__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__client_port_name_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__dst__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__dst(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__client_port_name_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__dst__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__dst(a, f) \
	__NDR_convert__float_rep__client_port_name_t((client_port_name_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__string__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__dst__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__dst(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__string(a, f, 128)
#elif	defined(__NDR_convert__float_rep__string__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__dst__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__dst(a, f) \
	__NDR_convert__float_rep__string(a, f, 128)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__dst__defined */


mig_internal kern_return_t __MIG_check__Request__rpc_jack_port_disconnect_name_t(__Request__rpc_jack_port_disconnect_name_t *In0P)
{

	typedef __Request__rpc_jack_port_disconnect_name_t __Request;
#if	__MigTypeCheck
	if ((In0P->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX) ||
	    (In0P->Head.msgh_size != (mach_msg_size_t)sizeof(__Request)))
		return MIG_BAD_ARGUMENTS;
#endif	/* __MigTypeCheck */

#if	defined(__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__refnum__defined) || \
	defined(__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__src__defined) || \
	defined(__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__dst__defined)
	if (In0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__refnum__defined)
		__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__refnum(&In0P->refnum, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__refnum__defined */
#if defined(__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__src__defined)
		__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__src(&In0P->src, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__src__defined */
#if defined(__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__dst__defined)
		__NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__dst(&In0P->dst, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_port_disconnect_name_t__dst__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	defined(__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__refnum__defined) || \
	defined(__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__src__defined) || \
	defined(__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__dst__defined)
	if (In0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__refnum__defined)
		__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__refnum(&In0P->refnum, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__refnum__defined */
#if defined(__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__src__defined)
		__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__src(&In0P->src, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__src__defined */
#if defined(__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__dst__defined)
		__NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__dst(&In0P->dst, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_port_disconnect_name_t__dst__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	defined(__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__refnum__defined) || \
	defined(__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__src__defined) || \
	defined(__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__dst__defined)
	if (In0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__refnum__defined)
		__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__refnum(&In0P->refnum, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__refnum__defined */
#if defined(__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__src__defined)
		__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__src(&In0P->src, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__src__defined */
#if defined(__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__dst__defined)
		__NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__dst(&In0P->dst, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_port_disconnect_name_t__dst__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Request__rpc_jack_port_disconnect_name_t__defined) */
#endif /* __MIG_check__Request__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_port_disconnect_name */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t server_rpc_jack_port_disconnect_name
(
	mach_port_t server_port,
	int refnum,
	client_port_name_t src,
	client_port_name_t dst,
	int *result
);

/* Routine rpc_jack_port_disconnect_name */
mig_internal novalue _Xrpc_jack_port_disconnect_name
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP)
{

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
		client_port_name_t src;
		client_port_name_t dst;
		mach_msg_trailer_t trailer;
	} Request;
#ifdef  __MigPackStructs
#pragma pack()
#endif
	typedef __Request__rpc_jack_port_disconnect_name_t __Request;
	typedef __Reply__rpc_jack_port_disconnect_name_t Reply;

	/*
	 * typedef struct {
	 * 	mach_msg_header_t Head;
	 * 	NDR_record_t NDR;
	 * 	kern_return_t RetCode;
	 * } mig_reply_error_t;
	 */

	Request *In0P = (Request *) InHeadP;
	Reply *OutP = (Reply *) OutHeadP;
#ifdef	__MIG_check__Request__rpc_jack_port_disconnect_name_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Request__rpc_jack_port_disconnect_name_t__defined */

	__DeclareRcvRpc(1010, "rpc_jack_port_disconnect_name")
	__BeforeRcvRpc(1010, "rpc_jack_port_disconnect_name")

#if	defined(__MIG_check__Request__rpc_jack_port_disconnect_name_t__defined)
	check_result = __MIG_check__Request__rpc_jack_port_disconnect_name_t((__Request *)In0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ MIG_RETURN_ERROR(OutP, check_result); }
#endif	/* defined(__MIG_check__Request__rpc_jack_port_disconnect_name_t__defined) */

	OutP->RetCode = server_rpc_jack_port_disconnect_name(In0P->Head.msgh_request_port, In0P->refnum, In0P->src, In0P->dst, &OutP->result);
	if (OutP->RetCode != KERN_SUCCESS) {
		MIG_RETURN_ERROR(OutP, OutP->RetCode);
	}

	OutP->NDR = NDR_record;


	OutP->Head.msgh_size = (mach_msg_size_t)(sizeof(Reply));
	__AfterRcvRpc(1010, "rpc_jack_port_disconnect_name")
}

#if (__MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Request__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Request__rpc_jack_set_buffer_size_t__defined)
#define __MIG_check__Request__rpc_jack_set_buffer_size_t__defined
#ifndef __NDR_convert__int_rep__Request__rpc_jack_set_buffer_size_t__buffer_size__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_set_buffer_size_t__buffer_size__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_set_buffer_size_t__buffer_size(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_set_buffer_size_t__buffer_size__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_set_buffer_size_t__buffer_size(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_set_buffer_size_t__buffer_size__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_set_buffer_size_t__buffer_size(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_set_buffer_size_t__buffer_size__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_set_buffer_size_t__buffer_size(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_set_buffer_size_t__buffer_size__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_set_buffer_size_t__buffer_size__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_set_buffer_size_t__buffer_size__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_set_buffer_size_t__buffer_size(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_set_buffer_size_t__buffer_size__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_set_buffer_size_t__buffer_size(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_set_buffer_size_t__buffer_size__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_set_buffer_size_t__buffer_size(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_set_buffer_size_t__buffer_size__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_set_buffer_size_t__buffer_size(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_set_buffer_size_t__buffer_size__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_set_buffer_size_t__buffer_size__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_set_buffer_size_t__buffer_size__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_set_buffer_size_t__buffer_size(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_set_buffer_size_t__buffer_size__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_set_buffer_size_t__buffer_size(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_set_buffer_size_t__buffer_size__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_set_buffer_size_t__buffer_size(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_set_buffer_size_t__buffer_size__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_set_buffer_size_t__buffer_size(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_set_buffer_size_t__buffer_size__defined */


mig_internal kern_return_t __MIG_check__Request__rpc_jack_set_buffer_size_t(__Request__rpc_jack_set_buffer_size_t *In0P)
{

	typedef __Request__rpc_jack_set_buffer_size_t __Request;
#if	__MigTypeCheck
	if ((In0P->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX) ||
	    (In0P->Head.msgh_size != (mach_msg_size_t)sizeof(__Request)))
		return MIG_BAD_ARGUMENTS;
#endif	/* __MigTypeCheck */

#if	defined(__NDR_convert__int_rep__Request__rpc_jack_set_buffer_size_t__buffer_size__defined)
	if (In0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Request__rpc_jack_set_buffer_size_t__buffer_size__defined)
		__NDR_convert__int_rep__Request__rpc_jack_set_buffer_size_t__buffer_size(&In0P->buffer_size, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_set_buffer_size_t__buffer_size__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	defined(__NDR_convert__char_rep__Request__rpc_jack_set_buffer_size_t__buffer_size__defined)
	if (In0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Request__rpc_jack_set_buffer_size_t__buffer_size__defined)
		__NDR_convert__char_rep__Request__rpc_jack_set_buffer_size_t__buffer_size(&In0P->buffer_size, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_set_buffer_size_t__buffer_size__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	defined(__NDR_convert__float_rep__Request__rpc_jack_set_buffer_size_t__buffer_size__defined)
	if (In0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Request__rpc_jack_set_buffer_size_t__buffer_size__defined)
		__NDR_convert__float_rep__Request__rpc_jack_set_buffer_size_t__buffer_size(&In0P->buffer_size, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_set_buffer_size_t__buffer_size__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Request__rpc_jack_set_buffer_size_t__defined) */
#endif /* __MIG_check__Request__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_set_buffer_size */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t server_rpc_jack_set_buffer_size
(
	mach_port_t server_port,
	int buffer_size,
	int *result
);

/* Routine rpc_jack_set_buffer_size */
mig_internal novalue _Xrpc_jack_set_buffer_size
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP)
{

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int buffer_size;
		mach_msg_trailer_t trailer;
	} Request;
#ifdef  __MigPackStructs
#pragma pack()
#endif
	typedef __Request__rpc_jack_set_buffer_size_t __Request;
	typedef __Reply__rpc_jack_set_buffer_size_t Reply;

	/*
	 * typedef struct {
	 * 	mach_msg_header_t Head;
	 * 	NDR_record_t NDR;
	 * 	kern_return_t RetCode;
	 * } mig_reply_error_t;
	 */

	Request *In0P = (Request *) InHeadP;
	Reply *OutP = (Reply *) OutHeadP;
#ifdef	__MIG_check__Request__rpc_jack_set_buffer_size_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Request__rpc_jack_set_buffer_size_t__defined */

	__DeclareRcvRpc(1011, "rpc_jack_set_buffer_size")
	__BeforeRcvRpc(1011, "rpc_jack_set_buffer_size")

#if	defined(__MIG_check__Request__rpc_jack_set_buffer_size_t__defined)
	check_result = __MIG_check__Request__rpc_jack_set_buffer_size_t((__Request *)In0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ MIG_RETURN_ERROR(OutP, check_result); }
#endif	/* defined(__MIG_check__Request__rpc_jack_set_buffer_size_t__defined) */

	OutP->RetCode = server_rpc_jack_set_buffer_size(In0P->Head.msgh_request_port, In0P->buffer_size, &OutP->result);
	if (OutP->RetCode != KERN_SUCCESS) {
		MIG_RETURN_ERROR(OutP, OutP->RetCode);
	}

	OutP->NDR = NDR_record;


	OutP->Head.msgh_size = (mach_msg_size_t)(sizeof(Reply));
	__AfterRcvRpc(1011, "rpc_jack_set_buffer_size")
}

#if (__MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Request__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Request__rpc_jack_set_freewheel_t__defined)
#define __MIG_check__Request__rpc_jack_set_freewheel_t__defined
#ifndef __NDR_convert__int_rep__Request__rpc_jack_set_freewheel_t__onoff__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_set_freewheel_t__onoff__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_set_freewheel_t__onoff(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_set_freewheel_t__onoff__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_set_freewheel_t__onoff(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_set_freewheel_t__onoff__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_set_freewheel_t__onoff(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_set_freewheel_t__onoff__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_set_freewheel_t__onoff(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_set_freewheel_t__onoff__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_set_freewheel_t__onoff__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_set_freewheel_t__onoff__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_set_freewheel_t__onoff(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_set_freewheel_t__onoff__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_set_freewheel_t__onoff(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_set_freewheel_t__onoff__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_set_freewheel_t__onoff(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_set_freewheel_t__onoff__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_set_freewheel_t__onoff(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_set_freewheel_t__onoff__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_set_freewheel_t__onoff__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_set_freewheel_t__onoff__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_set_freewheel_t__onoff(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_set_freewheel_t__onoff__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_set_freewheel_t__onoff(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_set_freewheel_t__onoff__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_set_freewheel_t__onoff(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_set_freewheel_t__onoff__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_set_freewheel_t__onoff(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_set_freewheel_t__onoff__defined */


mig_internal kern_return_t __MIG_check__Request__rpc_jack_set_freewheel_t(__Request__rpc_jack_set_freewheel_t *In0P)
{

	typedef __Request__rpc_jack_set_freewheel_t __Request;
#if	__MigTypeCheck
	if ((In0P->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX) ||
	    (In0P->Head.msgh_size != (mach_msg_size_t)sizeof(__Request)))
		return MIG_BAD_ARGUMENTS;
#endif	/* __MigTypeCheck */

#if	defined(__NDR_convert__int_rep__Request__rpc_jack_set_freewheel_t__onoff__defined)
	if (In0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Request__rpc_jack_set_freewheel_t__onoff__defined)
		__NDR_convert__int_rep__Request__rpc_jack_set_freewheel_t__onoff(&In0P->onoff, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_set_freewheel_t__onoff__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	defined(__NDR_convert__char_rep__Request__rpc_jack_set_freewheel_t__onoff__defined)
	if (In0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Request__rpc_jack_set_freewheel_t__onoff__defined)
		__NDR_convert__char_rep__Request__rpc_jack_set_freewheel_t__onoff(&In0P->onoff, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_set_freewheel_t__onoff__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	defined(__NDR_convert__float_rep__Request__rpc_jack_set_freewheel_t__onoff__defined)
	if (In0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Request__rpc_jack_set_freewheel_t__onoff__defined)
		__NDR_convert__float_rep__Request__rpc_jack_set_freewheel_t__onoff(&In0P->onoff, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_set_freewheel_t__onoff__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Request__rpc_jack_set_freewheel_t__defined) */
#endif /* __MIG_check__Request__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_set_freewheel */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t server_rpc_jack_set_freewheel
(
	mach_port_t server_port,
	int onoff,
	int *result
);

/* Routine rpc_jack_set_freewheel */
mig_internal novalue _Xrpc_jack_set_freewheel
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP)
{

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int onoff;
		mach_msg_trailer_t trailer;
	} Request;
#ifdef  __MigPackStructs
#pragma pack()
#endif
	typedef __Request__rpc_jack_set_freewheel_t __Request;
	typedef __Reply__rpc_jack_set_freewheel_t Reply;

	/*
	 * typedef struct {
	 * 	mach_msg_header_t Head;
	 * 	NDR_record_t NDR;
	 * 	kern_return_t RetCode;
	 * } mig_reply_error_t;
	 */

	Request *In0P = (Request *) InHeadP;
	Reply *OutP = (Reply *) OutHeadP;
#ifdef	__MIG_check__Request__rpc_jack_set_freewheel_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Request__rpc_jack_set_freewheel_t__defined */

	__DeclareRcvRpc(1012, "rpc_jack_set_freewheel")
	__BeforeRcvRpc(1012, "rpc_jack_set_freewheel")

#if	defined(__MIG_check__Request__rpc_jack_set_freewheel_t__defined)
	check_result = __MIG_check__Request__rpc_jack_set_freewheel_t((__Request *)In0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ MIG_RETURN_ERROR(OutP, check_result); }
#endif	/* defined(__MIG_check__Request__rpc_jack_set_freewheel_t__defined) */

	OutP->RetCode = server_rpc_jack_set_freewheel(In0P->Head.msgh_request_port, In0P->onoff, &OutP->result);
	if (OutP->RetCode != KERN_SUCCESS) {
		MIG_RETURN_ERROR(OutP, OutP->RetCode);
	}

	OutP->NDR = NDR_record;


	OutP->Head.msgh_size = (mach_msg_size_t)(sizeof(Reply));
	__AfterRcvRpc(1012, "rpc_jack_set_freewheel")
}

#if (__MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Request__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Request__rpc_jack_release_timebase_t__defined)
#define __MIG_check__Request__rpc_jack_release_timebase_t__defined
#ifndef __NDR_convert__int_rep__Request__rpc_jack_release_timebase_t__refnum__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_release_timebase_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_release_timebase_t__refnum(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_release_timebase_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_release_timebase_t__refnum(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_release_timebase_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_release_timebase_t__refnum(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_release_timebase_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_release_timebase_t__refnum(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_release_timebase_t__refnum__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_release_timebase_t__refnum__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_release_timebase_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_release_timebase_t__refnum(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_release_timebase_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_release_timebase_t__refnum(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_release_timebase_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_release_timebase_t__refnum(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_release_timebase_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_release_timebase_t__refnum(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_release_timebase_t__refnum__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_release_timebase_t__refnum__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_release_timebase_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_release_timebase_t__refnum(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_release_timebase_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_release_timebase_t__refnum(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_release_timebase_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_release_timebase_t__refnum(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_release_timebase_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_release_timebase_t__refnum(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_release_timebase_t__refnum__defined */


mig_internal kern_return_t __MIG_check__Request__rpc_jack_release_timebase_t(__Request__rpc_jack_release_timebase_t *In0P)
{

	typedef __Request__rpc_jack_release_timebase_t __Request;
#if	__MigTypeCheck
	if ((In0P->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX) ||
	    (In0P->Head.msgh_size != (mach_msg_size_t)sizeof(__Request)))
		return MIG_BAD_ARGUMENTS;
#endif	/* __MigTypeCheck */

#if	defined(__NDR_convert__int_rep__Request__rpc_jack_release_timebase_t__refnum__defined)
	if (In0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Request__rpc_jack_release_timebase_t__refnum__defined)
		__NDR_convert__int_rep__Request__rpc_jack_release_timebase_t__refnum(&In0P->refnum, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_release_timebase_t__refnum__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	defined(__NDR_convert__char_rep__Request__rpc_jack_release_timebase_t__refnum__defined)
	if (In0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Request__rpc_jack_release_timebase_t__refnum__defined)
		__NDR_convert__char_rep__Request__rpc_jack_release_timebase_t__refnum(&In0P->refnum, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_release_timebase_t__refnum__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	defined(__NDR_convert__float_rep__Request__rpc_jack_release_timebase_t__refnum__defined)
	if (In0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Request__rpc_jack_release_timebase_t__refnum__defined)
		__NDR_convert__float_rep__Request__rpc_jack_release_timebase_t__refnum(&In0P->refnum, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_release_timebase_t__refnum__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Request__rpc_jack_release_timebase_t__defined) */
#endif /* __MIG_check__Request__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_release_timebase */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t server_rpc_jack_release_timebase
(
	mach_port_t server_port,
	int refnum,
	int *result
);

/* Routine rpc_jack_release_timebase */
mig_internal novalue _Xrpc_jack_release_timebase
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP)
{

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
		mach_msg_trailer_t trailer;
	} Request;
#ifdef  __MigPackStructs
#pragma pack()
#endif
	typedef __Request__rpc_jack_release_timebase_t __Request;
	typedef __Reply__rpc_jack_release_timebase_t Reply;

	/*
	 * typedef struct {
	 * 	mach_msg_header_t Head;
	 * 	NDR_record_t NDR;
	 * 	kern_return_t RetCode;
	 * } mig_reply_error_t;
	 */

	Request *In0P = (Request *) InHeadP;
	Reply *OutP = (Reply *) OutHeadP;
#ifdef	__MIG_check__Request__rpc_jack_release_timebase_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Request__rpc_jack_release_timebase_t__defined */

	__DeclareRcvRpc(1013, "rpc_jack_release_timebase")
	__BeforeRcvRpc(1013, "rpc_jack_release_timebase")

#if	defined(__MIG_check__Request__rpc_jack_release_timebase_t__defined)
	check_result = __MIG_check__Request__rpc_jack_release_timebase_t((__Request *)In0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ MIG_RETURN_ERROR(OutP, check_result); }
#endif	/* defined(__MIG_check__Request__rpc_jack_release_timebase_t__defined) */

	OutP->RetCode = server_rpc_jack_release_timebase(In0P->Head.msgh_request_port, In0P->refnum, &OutP->result);
	if (OutP->RetCode != KERN_SUCCESS) {
		MIG_RETURN_ERROR(OutP, OutP->RetCode);
	}

	OutP->NDR = NDR_record;


	OutP->Head.msgh_size = (mach_msg_size_t)(sizeof(Reply));
	__AfterRcvRpc(1013, "rpc_jack_release_timebase")
}

#if (__MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Request__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Request__rpc_jack_set_timebase_callback_t__defined)
#define __MIG_check__Request__rpc_jack_set_timebase_callback_t__defined
#ifndef __NDR_convert__int_rep__Request__rpc_jack_set_timebase_callback_t__refnum__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_set_timebase_callback_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_set_timebase_callback_t__refnum(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_set_timebase_callback_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_set_timebase_callback_t__refnum(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_set_timebase_callback_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_set_timebase_callback_t__refnum(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_set_timebase_callback_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_set_timebase_callback_t__refnum(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_set_timebase_callback_t__refnum__defined */

#ifndef __NDR_convert__int_rep__Request__rpc_jack_set_timebase_callback_t__conditional__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_set_timebase_callback_t__conditional__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_set_timebase_callback_t__conditional(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_set_timebase_callback_t__conditional__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_set_timebase_callback_t__conditional(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_set_timebase_callback_t__conditional__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_set_timebase_callback_t__conditional(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_set_timebase_callback_t__conditional__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_set_timebase_callback_t__conditional(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_set_timebase_callback_t__conditional__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_set_timebase_callback_t__refnum__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_set_timebase_callback_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_set_timebase_callback_t__refnum(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_set_timebase_callback_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_set_timebase_callback_t__refnum(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_set_timebase_callback_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_set_timebase_callback_t__refnum(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_set_timebase_callback_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_set_timebase_callback_t__refnum(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_set_timebase_callback_t__refnum__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_set_timebase_callback_t__conditional__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_set_timebase_callback_t__conditional__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_set_timebase_callback_t__conditional(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_set_timebase_callback_t__conditional__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_set_timebase_callback_t__conditional(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_set_timebase_callback_t__conditional__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_set_timebase_callback_t__conditional(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_set_timebase_callback_t__conditional__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_set_timebase_callback_t__conditional(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_set_timebase_callback_t__conditional__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_set_timebase_callback_t__refnum__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_set_timebase_callback_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_set_timebase_callback_t__refnum(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_set_timebase_callback_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_set_timebase_callback_t__refnum(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_set_timebase_callback_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_set_timebase_callback_t__refnum(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_set_timebase_callback_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_set_timebase_callback_t__refnum(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_set_timebase_callback_t__refnum__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_set_timebase_callback_t__conditional__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_set_timebase_callback_t__conditional__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_set_timebase_callback_t__conditional(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_set_timebase_callback_t__conditional__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_set_timebase_callback_t__conditional(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_set_timebase_callback_t__conditional__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_set_timebase_callback_t__conditional(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_set_timebase_callback_t__conditional__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_set_timebase_callback_t__conditional(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_set_timebase_callback_t__conditional__defined */


mig_internal kern_return_t __MIG_check__Request__rpc_jack_set_timebase_callback_t(__Request__rpc_jack_set_timebase_callback_t *In0P)
{

	typedef __Request__rpc_jack_set_timebase_callback_t __Request;
#if	__MigTypeCheck
	if ((In0P->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX) ||
	    (In0P->Head.msgh_size != (mach_msg_size_t)sizeof(__Request)))
		return MIG_BAD_ARGUMENTS;
#endif	/* __MigTypeCheck */

#if	defined(__NDR_convert__int_rep__Request__rpc_jack_set_timebase_callback_t__refnum__defined) || \
	defined(__NDR_convert__int_rep__Request__rpc_jack_set_timebase_callback_t__conditional__defined)
	if (In0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Request__rpc_jack_set_timebase_callback_t__refnum__defined)
		__NDR_convert__int_rep__Request__rpc_jack_set_timebase_callback_t__refnum(&In0P->refnum, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_set_timebase_callback_t__refnum__defined */
#if defined(__NDR_convert__int_rep__Request__rpc_jack_set_timebase_callback_t__conditional__defined)
		__NDR_convert__int_rep__Request__rpc_jack_set_timebase_callback_t__conditional(&In0P->conditional, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_set_timebase_callback_t__conditional__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	defined(__NDR_convert__char_rep__Request__rpc_jack_set_timebase_callback_t__refnum__defined) || \
	defined(__NDR_convert__char_rep__Request__rpc_jack_set_timebase_callback_t__conditional__defined)
	if (In0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Request__rpc_jack_set_timebase_callback_t__refnum__defined)
		__NDR_convert__char_rep__Request__rpc_jack_set_timebase_callback_t__refnum(&In0P->refnum, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_set_timebase_callback_t__refnum__defined */
#if defined(__NDR_convert__char_rep__Request__rpc_jack_set_timebase_callback_t__conditional__defined)
		__NDR_convert__char_rep__Request__rpc_jack_set_timebase_callback_t__conditional(&In0P->conditional, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_set_timebase_callback_t__conditional__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	defined(__NDR_convert__float_rep__Request__rpc_jack_set_timebase_callback_t__refnum__defined) || \
	defined(__NDR_convert__float_rep__Request__rpc_jack_set_timebase_callback_t__conditional__defined)
	if (In0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Request__rpc_jack_set_timebase_callback_t__refnum__defined)
		__NDR_convert__float_rep__Request__rpc_jack_set_timebase_callback_t__refnum(&In0P->refnum, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_set_timebase_callback_t__refnum__defined */
#if defined(__NDR_convert__float_rep__Request__rpc_jack_set_timebase_callback_t__conditional__defined)
		__NDR_convert__float_rep__Request__rpc_jack_set_timebase_callback_t__conditional(&In0P->conditional, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_set_timebase_callback_t__conditional__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Request__rpc_jack_set_timebase_callback_t__defined) */
#endif /* __MIG_check__Request__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_set_timebase_callback */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t server_rpc_jack_set_timebase_callback
(
	mach_port_t server_port,
	int refnum,
	int conditional,
	int *result
);

/* Routine rpc_jack_set_timebase_callback */
mig_internal novalue _Xrpc_jack_set_timebase_callback
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP)
{

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
		int conditional;
		mach_msg_trailer_t trailer;
	} Request;
#ifdef  __MigPackStructs
#pragma pack()
#endif
	typedef __Request__rpc_jack_set_timebase_callback_t __Request;
	typedef __Reply__rpc_jack_set_timebase_callback_t Reply;

	/*
	 * typedef struct {
	 * 	mach_msg_header_t Head;
	 * 	NDR_record_t NDR;
	 * 	kern_return_t RetCode;
	 * } mig_reply_error_t;
	 */

	Request *In0P = (Request *) InHeadP;
	Reply *OutP = (Reply *) OutHeadP;
#ifdef	__MIG_check__Request__rpc_jack_set_timebase_callback_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Request__rpc_jack_set_timebase_callback_t__defined */

	__DeclareRcvRpc(1014, "rpc_jack_set_timebase_callback")
	__BeforeRcvRpc(1014, "rpc_jack_set_timebase_callback")

#if	defined(__MIG_check__Request__rpc_jack_set_timebase_callback_t__defined)
	check_result = __MIG_check__Request__rpc_jack_set_timebase_callback_t((__Request *)In0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ MIG_RETURN_ERROR(OutP, check_result); }
#endif	/* defined(__MIG_check__Request__rpc_jack_set_timebase_callback_t__defined) */

	OutP->RetCode = server_rpc_jack_set_timebase_callback(In0P->Head.msgh_request_port, In0P->refnum, In0P->conditional, &OutP->result);
	if (OutP->RetCode != KERN_SUCCESS) {
		MIG_RETURN_ERROR(OutP, OutP->RetCode);
	}

	OutP->NDR = NDR_record;


	OutP->Head.msgh_size = (mach_msg_size_t)(sizeof(Reply));
	__AfterRcvRpc(1014, "rpc_jack_set_timebase_callback")
}

#if (__MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Request__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Request__rpc_jack_client_rt_notify_t__defined)
#define __MIG_check__Request__rpc_jack_client_rt_notify_t__defined
#ifndef __NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__refnum__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__refnum(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__refnum(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__refnum(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__refnum__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__refnum(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__refnum__defined */

#ifndef __NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__notify__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__notify__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__notify(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__notify__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__notify(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__notify__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__notify(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__notify__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__notify(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__notify__defined */

#ifndef __NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__value__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__value__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__value(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__value__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__value(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__value__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__value(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__value__defined
#define	__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__value(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__value__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__refnum__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__refnum(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__refnum(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__refnum(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__refnum__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__refnum(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__refnum__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__notify__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__notify__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__notify(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__notify__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__notify(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__notify__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__notify(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__notify__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__notify(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__notify__defined */

#ifndef __NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__value__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__value__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__value(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__value__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__value(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__value__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__value(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__value__defined
#define	__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__value(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__value__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__refnum__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__refnum(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__refnum(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__refnum(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__refnum__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__refnum(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__refnum__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__notify__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__notify__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__notify(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__notify__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__notify(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__notify__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__notify(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__notify__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__notify(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__notify__defined */

#ifndef __NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__value__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__value__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__value(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__value__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__value(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__value__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__value(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__value__defined
#define	__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__value(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__value__defined */


mig_internal kern_return_t __MIG_check__Request__rpc_jack_client_rt_notify_t(__Request__rpc_jack_client_rt_notify_t *In0P)
{

	typedef __Request__rpc_jack_client_rt_notify_t __Request;
#if	__MigTypeCheck
	if ((In0P->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX) ||
	    (In0P->Head.msgh_size != (mach_msg_size_t)sizeof(__Request)))
		return MIG_BAD_ARGUMENTS;
#endif	/* __MigTypeCheck */

#if	defined(__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__refnum__defined) || \
	defined(__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__notify__defined) || \
	defined(__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__value__defined)
	if (In0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__refnum__defined)
		__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__refnum(&In0P->refnum, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__refnum__defined */
#if defined(__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__notify__defined)
		__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__notify(&In0P->notify, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__notify__defined */
#if defined(__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__value__defined)
		__NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__value(&In0P->value, In0P->NDR.int_rep);
#endif	/* __NDR_convert__int_rep__Request__rpc_jack_client_rt_notify_t__value__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	defined(__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__refnum__defined) || \
	defined(__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__notify__defined) || \
	defined(__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__value__defined)
	if (In0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__refnum__defined)
		__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__refnum(&In0P->refnum, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__refnum__defined */
#if defined(__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__notify__defined)
		__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__notify(&In0P->notify, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__notify__defined */
#if defined(__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__value__defined)
		__NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__value(&In0P->value, In0P->NDR.char_rep);
#endif	/* __NDR_convert__char_rep__Request__rpc_jack_client_rt_notify_t__value__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	defined(__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__refnum__defined) || \
	defined(__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__notify__defined) || \
	defined(__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__value__defined)
	if (In0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__refnum__defined)
		__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__refnum(&In0P->refnum, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__refnum__defined */
#if defined(__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__notify__defined)
		__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__notify(&In0P->notify, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__notify__defined */
#if defined(__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__value__defined)
		__NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__value(&In0P->value, In0P->NDR.float_rep);
#endif	/* __NDR_convert__float_rep__Request__rpc_jack_client_rt_notify_t__value__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Request__rpc_jack_client_rt_notify_t__defined) */
#endif /* __MIG_check__Request__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* SimpleRoutine rpc_jack_client_rt_notify */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t server_rpc_jack_client_rt_notify
(
	mach_port_t client_port,
	int refnum,
	int notify,
	int value
);

/* SimpleRoutine rpc_jack_client_rt_notify */
mig_internal novalue _Xrpc_jack_client_rt_notify
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP)
{

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
		int notify;
		int value;
		mach_msg_trailer_t trailer;
	} Request;
#ifdef  __MigPackStructs
#pragma pack()
#endif
	typedef __Request__rpc_jack_client_rt_notify_t __Request;
	typedef __Reply__rpc_jack_client_rt_notify_t Reply;

	/*
	 * typedef struct {
	 * 	mach_msg_header_t Head;
	 * 	NDR_record_t NDR;
	 * 	kern_return_t RetCode;
	 * } mig_reply_error_t;
	 */

	Request *In0P = (Request *) InHeadP;
	Reply *OutP = (Reply *) OutHeadP;
#ifdef	__MIG_check__Request__rpc_jack_client_rt_notify_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Request__rpc_jack_client_rt_notify_t__defined */

	__DeclareRcvSimple(1015, "rpc_jack_client_rt_notify")
	__BeforeRcvSimple(1015, "rpc_jack_client_rt_notify")

#if	defined(__MIG_check__Request__rpc_jack_client_rt_notify_t__defined)
	check_result = __MIG_check__Request__rpc_jack_client_rt_notify_t((__Request *)In0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ MIG_RETURN_ERROR(OutP, check_result); }
#endif	/* defined(__MIG_check__Request__rpc_jack_client_rt_notify_t__defined) */

	OutP->RetCode = server_rpc_jack_client_rt_notify(In0P->Head.msgh_request_port, In0P->refnum, In0P->notify, In0P->value);
	__AfterRcvSimple(1015, "rpc_jack_client_rt_notify")
}


extern boolean_t JackRPCEngine_server(
		mach_msg_header_t *InHeadP,
		mach_msg_header_t *OutHeadP);

extern mig_routine_t JackRPCEngine_server_routine(
		mach_msg_header_t *InHeadP);


/* Description of this subsystem, for use in direct RPC */
const struct server_JackRPCEngine_subsystem {
	mig_server_routine_t 	server;	/* Server routine */
	mach_msg_id_t	start;	/* Min routine number */
	mach_msg_id_t	end;	/* Max routine number + 1 */
	unsigned int	maxsize;	/* Max msg size */
	vm_address_t	reserved;	/* Reserved */
	struct routine_descriptor	/*Array of routine descriptors */
		routine[16];
} server_JackRPCEngine_subsystem = {
	JackRPCEngine_server_routine,
	1000,
	1016,
	(mach_msg_size_t)sizeof(union __ReplyUnion__server_JackRPCEngine_subsystem),
	(vm_address_t)0,
	{
          { (mig_impl_routine_t) 0,
            (mig_stub_routine_t) _Xrpc_jack_client_open, 7, 0, (routine_arg_descriptor_t)0, (mach_msg_size_t)sizeof(__Reply__rpc_jack_client_open_t)},
          { (mig_impl_routine_t) 0,
            (mig_stub_routine_t) _Xrpc_jack_client_check, 7, 0, (routine_arg_descriptor_t)0, (mach_msg_size_t)sizeof(__Reply__rpc_jack_client_check_t)},
          { (mig_impl_routine_t) 0,
            (mig_stub_routine_t) _Xrpc_jack_client_close, 3, 0, (routine_arg_descriptor_t)0, (mach_msg_size_t)sizeof(__Reply__rpc_jack_client_close_t)},
          { (mig_impl_routine_t) 0,
            (mig_stub_routine_t) _Xrpc_jack_client_activate, 3, 0, (routine_arg_descriptor_t)0, (mach_msg_size_t)sizeof(__Reply__rpc_jack_client_activate_t)},
          { (mig_impl_routine_t) 0,
            (mig_stub_routine_t) _Xrpc_jack_client_deactivate, 3, 0, (routine_arg_descriptor_t)0, (mach_msg_size_t)sizeof(__Reply__rpc_jack_client_deactivate_t)},
          { (mig_impl_routine_t) 0,
            (mig_stub_routine_t) _Xrpc_jack_port_register, 7, 0, (routine_arg_descriptor_t)0, (mach_msg_size_t)sizeof(__Reply__rpc_jack_port_register_t)},
          { (mig_impl_routine_t) 0,
            (mig_stub_routine_t) _Xrpc_jack_port_unregister, 4, 0, (routine_arg_descriptor_t)0, (mach_msg_size_t)sizeof(__Reply__rpc_jack_port_unregister_t)},
          { (mig_impl_routine_t) 0,
            (mig_stub_routine_t) _Xrpc_jack_port_connect, 5, 0, (routine_arg_descriptor_t)0, (mach_msg_size_t)sizeof(__Reply__rpc_jack_port_connect_t)},
          { (mig_impl_routine_t) 0,
            (mig_stub_routine_t) _Xrpc_jack_port_disconnect, 5, 0, (routine_arg_descriptor_t)0, (mach_msg_size_t)sizeof(__Reply__rpc_jack_port_disconnect_t)},
          { (mig_impl_routine_t) 0,
            (mig_stub_routine_t) _Xrpc_jack_port_connect_name, 5, 0, (routine_arg_descriptor_t)0, (mach_msg_size_t)sizeof(__Reply__rpc_jack_port_connect_name_t)},
          { (mig_impl_routine_t) 0,
            (mig_stub_routine_t) _Xrpc_jack_port_disconnect_name, 5, 0, (routine_arg_descriptor_t)0, (mach_msg_size_t)sizeof(__Reply__rpc_jack_port_disconnect_name_t)},
          { (mig_impl_routine_t) 0,
            (mig_stub_routine_t) _Xrpc_jack_set_buffer_size, 3, 0, (routine_arg_descriptor_t)0, (mach_msg_size_t)sizeof(__Reply__rpc_jack_set_buffer_size_t)},
          { (mig_impl_routine_t) 0,
            (mig_stub_routine_t) _Xrpc_jack_set_freewheel, 3, 0, (routine_arg_descriptor_t)0, (mach_msg_size_t)sizeof(__Reply__rpc_jack_set_freewheel_t)},
          { (mig_impl_routine_t) 0,
            (mig_stub_routine_t) _Xrpc_jack_release_timebase, 3, 0, (routine_arg_descriptor_t)0, (mach_msg_size_t)sizeof(__Reply__rpc_jack_release_timebase_t)},
          { (mig_impl_routine_t) 0,
            (mig_stub_routine_t) _Xrpc_jack_set_timebase_callback, 4, 0, (routine_arg_descriptor_t)0, (mach_msg_size_t)sizeof(__Reply__rpc_jack_set_timebase_callback_t)},
          { (mig_impl_routine_t) 0,
            (mig_stub_routine_t) _Xrpc_jack_client_rt_notify, 4, 0, (routine_arg_descriptor_t)0, (mach_msg_size_t)sizeof(__Reply__rpc_jack_client_rt_notify_t)},
	}
};

mig_external boolean_t JackRPCEngine_server
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP)
{
	/*
	 * typedef struct {
	 * 	mach_msg_header_t Head;
	 * 	NDR_record_t NDR;
	 * 	kern_return_t RetCode;
	 * } mig_reply_error_t;
	 */

	register mig_routine_t routine;

	OutHeadP->msgh_bits = MACH_MSGH_BITS(MACH_MSGH_BITS_REPLY(InHeadP->msgh_bits), 0);
	OutHeadP->msgh_remote_port = InHeadP->msgh_reply_port;
	/* Minimal size: routine() will update it if different */
	OutHeadP->msgh_size = (mach_msg_size_t)sizeof(mig_reply_error_t);
	OutHeadP->msgh_local_port = MACH_PORT_NULL;
	OutHeadP->msgh_id = InHeadP->msgh_id + 100;

	if ((InHeadP->msgh_id > 1015) || (InHeadP->msgh_id < 1000) ||
	    ((routine = server_JackRPCEngine_subsystem.routine[InHeadP->msgh_id - 1000].stub_routine) == 0)) {
		((mig_reply_error_t *)OutHeadP)->NDR = NDR_record;
		((mig_reply_error_t *)OutHeadP)->RetCode = MIG_BAD_ID;
		return FALSE;
	}
	(*routine) (InHeadP, OutHeadP);
	return TRUE;
}

mig_external mig_routine_t JackRPCEngine_server_routine
	(mach_msg_header_t *InHeadP)
{
	register int msgh_id;

	msgh_id = InHeadP->msgh_id - 1000;

	if ((msgh_id > 15) || (msgh_id < 0))
		return 0;

	return server_JackRPCEngine_subsystem.routine[msgh_id].stub_routine;
}
