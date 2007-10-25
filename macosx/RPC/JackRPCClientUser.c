/*
 * IDENTIFICATION:
 * stub generated Thu Oct 25 10:49:38 2007
 * with a MiG generated Mon Sep 11 19:11:05 PDT 2006 by root@b09.apple.com
 * OPTIONS: 
 */
#define	__MIG_check__Reply__JackRPCClient_subsystem__ 1
#define	__NDR_convert__Reply__JackRPCClient_subsystem__ 1
#define	__NDR_convert__mig_reply_error_subsystem__ 1

#include "JackRPCClient.h"


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

#ifndef	__MachMsgErrorWithTimeout
#define	__MachMsgErrorWithTimeout(_R_) { \
	switch (_R_) { \
	case MACH_SEND_INVALID_REPLY: \
	case MACH_RCV_INVALID_NAME: \
	case MACH_RCV_PORT_DIED: \
	case MACH_RCV_PORT_CHANGED: \
	case MACH_RCV_TIMED_OUT: \
		mig_dealloc_reply_port(InP->Head.msgh_reply_port); \
		break; \
	default: \
		mig_put_reply_port(InP->Head.msgh_reply_port); \
	} \
}
#endif	/* __MachMsgErrorWithTimeout */

#ifndef	__MachMsgErrorWithoutTimeout
#define	__MachMsgErrorWithoutTimeout(_R_) { \
	switch (_R_) { \
	case MACH_SEND_INVALID_REPLY: \
	case MACH_RCV_INVALID_NAME: \
	case MACH_RCV_PORT_DIED: \
	case MACH_RCV_PORT_CHANGED: \
		mig_dealloc_reply_port(InP->Head.msgh_reply_port); \
		break; \
	default: \
		mig_put_reply_port(InP->Head.msgh_reply_port); \
	} \
}
#endif	/* __MachMsgErrorWithoutTimeout */

#ifndef	__DeclareSendRpc
#define	__DeclareSendRpc(_NUM_, _NAME_)
#endif	/* __DeclareSendRpc */

#ifndef	__BeforeSendRpc
#define	__BeforeSendRpc(_NUM_, _NAME_)
#endif	/* __BeforeSendRpc */

#ifndef	__AfterSendRpc
#define	__AfterSendRpc(_NUM_, _NAME_)
#endif	/* __AfterSendRpc */

#ifndef	__DeclareSendSimple
#define	__DeclareSendSimple(_NUM_, _NAME_)
#endif	/* __DeclareSendSimple */

#ifndef	__BeforeSendSimple
#define	__BeforeSendSimple(_NUM_, _NAME_)
#endif	/* __BeforeSendSimple */

#ifndef	__AfterSendSimple
#define	__AfterSendSimple(_NUM_, _NAME_)
#endif	/* __AfterSendSimple */

#define msgh_request_port	msgh_remote_port
#define msgh_reply_port		msgh_local_port



#if ( __MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Reply__JackRPCClient_subsystem__
#if !defined(__MIG_check__Reply__rpc_jack_client_sync_notify_t__defined)
#define __MIG_check__Reply__rpc_jack_client_sync_notify_t__defined
#ifndef __NDR_convert__int_rep__Reply__rpc_jack_client_sync_notify_t__RetCode__defined
#if	defined(__NDR_convert__int_rep__JackRPCClient__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_sync_notify_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_sync_notify_t__RetCode(a, f) \
	__NDR_convert__int_rep__JackRPCClient__kern_return_t((kern_return_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_sync_notify_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_sync_notify_t__RetCode(a, f) \
	__NDR_convert__int_rep__kern_return_t((kern_return_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_sync_notify_t__RetCode__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_client_sync_notify_t__result__defined
#if	defined(__NDR_convert__int_rep__JackRPCClient__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_sync_notify_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_sync_notify_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCClient__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_sync_notify_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_sync_notify_t__result(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCClient__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_sync_notify_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_sync_notify_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCClient__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_sync_notify_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_sync_notify_t__result(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_sync_notify_t__result__defined */



#ifndef __NDR_convert__char_rep__Reply__rpc_jack_client_sync_notify_t__result__defined
#if	defined(__NDR_convert__char_rep__JackRPCClient__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_sync_notify_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_sync_notify_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCClient__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_sync_notify_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_sync_notify_t__result(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCClient__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_sync_notify_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_sync_notify_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCClient__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_sync_notify_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_sync_notify_t__result(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_client_sync_notify_t__result__defined */



#ifndef __NDR_convert__float_rep__Reply__rpc_jack_client_sync_notify_t__result__defined
#if	defined(__NDR_convert__float_rep__JackRPCClient__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_sync_notify_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_sync_notify_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCClient__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_sync_notify_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_sync_notify_t__result(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCClient__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_sync_notify_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_sync_notify_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCClient__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_sync_notify_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_sync_notify_t__result(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_client_sync_notify_t__result__defined */



mig_internal kern_return_t __MIG_check__Reply__rpc_jack_client_sync_notify_t(__Reply__rpc_jack_client_sync_notify_t *Out0P)
{

	typedef __Reply__rpc_jack_client_sync_notify_t __Reply;
#if	__MigTypeCheck
	unsigned int msgh_size;
#endif	/* __MigTypeCheck */
	if (Out0P->Head.msgh_id != 1100) {
	    if (Out0P->Head.msgh_id == MACH_NOTIFY_SEND_ONCE)
		{ return MIG_SERVER_DIED; }
	    else
		{ return MIG_REPLY_MISMATCH; }
	}

#if	__MigTypeCheck
	msgh_size = Out0P->Head.msgh_size;

	if ((Out0P->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX) ||
	    ((msgh_size != (mach_msg_size_t)sizeof(__Reply)) &&
	     (msgh_size != (mach_msg_size_t)sizeof(mig_reply_error_t) ||
	      Out0P->RetCode == KERN_SUCCESS)))
		{ return MIG_TYPE_ERROR ; }
#endif	/* __MigTypeCheck */

	if (Out0P->RetCode != KERN_SUCCESS) {
#ifdef	__NDR_convert__mig_reply_error_t__defined
		__NDR_convert__mig_reply_error_t((mig_reply_error_t *)Out0P);
#endif	/* __NDR_convert__mig_reply_error_t__defined */
		return ((mig_reply_error_t *)Out0P)->RetCode;
	}

#if	defined(__NDR_convert__int_rep__Reply__rpc_jack_client_sync_notify_t__RetCode__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_client_sync_notify_t__result__defined)
	if (Out0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_client_sync_notify_t__RetCode__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_client_sync_notify_t__RetCode(&Out0P->RetCode, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_sync_notify_t__RetCode__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_client_sync_notify_t__result__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_client_sync_notify_t__result(&Out0P->result, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_sync_notify_t__result__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	0 || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_client_sync_notify_t__result__defined)
	if (Out0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_client_sync_notify_t__result__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_client_sync_notify_t__result(&Out0P->result, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_client_sync_notify_t__result__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	0 || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_client_sync_notify_t__result__defined)
	if (Out0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_client_sync_notify_t__result__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_client_sync_notify_t__result(&Out0P->result, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_client_sync_notify_t__result__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Reply__rpc_jack_client_sync_notify_t__defined) */
#endif /* __MIG_check__Reply__JackRPCClient_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_client_sync_notify */
mig_external kern_return_t rpc_jack_client_sync_notify
(
	mach_port_t client_port,
	int refnum,
	client_name_t client_name,
	int notify,
	int value,
	int *result
)
{
    {

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
		client_name_t client_name;
		int notify;
		int value;
	} Request;
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
		mach_msg_trailer_t trailer;
	} Reply;
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
	} __Reply;
#ifdef  __MigPackStructs
#pragma pack()
#endif
	/*
	 * typedef struct {
	 * 	mach_msg_header_t Head;
	 * 	NDR_record_t NDR;
	 * 	kern_return_t RetCode;
	 * } mig_reply_error_t;
	 */

	union {
		Request In;
		Reply Out;
	} Mess;

	Request *InP = &Mess.In;
	Reply *Out0P = &Mess.Out;

	mach_msg_return_t msg_result;

#ifdef	__MIG_check__Reply__rpc_jack_client_sync_notify_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Reply__rpc_jack_client_sync_notify_t__defined */

	__DeclareSendRpc(1000, "rpc_jack_client_sync_notify")

	InP->NDR = NDR_record;

	InP->refnum = refnum;

	(void) mig_strncpy(InP->client_name, client_name, 128);

	InP->notify = notify;

	InP->value = value;

	InP->Head.msgh_bits =
		MACH_MSGH_BITS(19, MACH_MSG_TYPE_MAKE_SEND_ONCE);
	/* msgh_size passed as argument */
	InP->Head.msgh_request_port = client_port;
	InP->Head.msgh_reply_port = mig_get_reply_port();
	InP->Head.msgh_id = 1000;

	__BeforeSendRpc(1000, "rpc_jack_client_sync_notify")
	msg_result = mach_msg(&InP->Head, MACH_SEND_MSG|MACH_RCV_MSG|MACH_SEND_TIMEOUT|MACH_RCV_TIMEOUT|MACH_MSG_OPTION_NONE, (mach_msg_size_t)sizeof(Request), (mach_msg_size_t)sizeof(Reply), InP->Head.msgh_reply_port, 5000, MACH_PORT_NULL);
	__AfterSendRpc(1000, "rpc_jack_client_sync_notify")
	if (msg_result != MACH_MSG_SUCCESS) {
		__MachMsgErrorWithTimeout(msg_result);
		{ return msg_result; }
	}


#if	defined(__MIG_check__Reply__rpc_jack_client_sync_notify_t__defined)
	check_result = __MIG_check__Reply__rpc_jack_client_sync_notify_t((__Reply__rpc_jack_client_sync_notify_t *)Out0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ return check_result; }
#endif	/* defined(__MIG_check__Reply__rpc_jack_client_sync_notify_t__defined) */

	*result = Out0P->result;

	return KERN_SUCCESS;
    }
}

/* SimpleRoutine rpc_jack_client_async_notify */
mig_external kern_return_t rpc_jack_client_async_notify
(
	mach_port_t client_port,
	int refnum,
	client_name_t client_name,
	int notify,
	int value
)
{
    {

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int refnum;
		client_name_t client_name;
		int notify;
		int value;
	} Request;
#ifdef  __MigPackStructs
#pragma pack()
#endif
	/*
	 * typedef struct {
	 * 	mach_msg_header_t Head;
	 * 	NDR_record_t NDR;
	 * 	kern_return_t RetCode;
	 * } mig_reply_error_t;
	 */

	union {
		Request In;
	} Mess;

	Request *InP = &Mess.In;

	mach_msg_return_t msg_result;

#ifdef	__MIG_check__Reply__rpc_jack_client_async_notify_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Reply__rpc_jack_client_async_notify_t__defined */

	__DeclareSendSimple(1001, "rpc_jack_client_async_notify")

	InP->NDR = NDR_record;

	InP->refnum = refnum;

	(void) mig_strncpy(InP->client_name, client_name, 128);

	InP->notify = notify;

	InP->value = value;

	InP->Head.msgh_bits =
		MACH_MSGH_BITS(19, 0);
	/* msgh_size passed as argument */
	InP->Head.msgh_request_port = client_port;
	InP->Head.msgh_reply_port = MACH_PORT_NULL;
	InP->Head.msgh_id = 1001;

	__BeforeSendSimple(1001, "rpc_jack_client_async_notify")
	msg_result = mach_msg(&InP->Head, MACH_SEND_MSG|MACH_SEND_TIMEOUT|MACH_MSG_OPTION_NONE, (mach_msg_size_t)sizeof(Request), 0, MACH_PORT_NULL, 5000, MACH_PORT_NULL);
	__AfterSendSimple(1001, "rpc_jack_client_async_notify")
		return msg_result;
	return KERN_SUCCESS;
    }
}
