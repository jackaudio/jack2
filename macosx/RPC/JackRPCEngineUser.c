/*
 * IDENTIFICATION:
 * stub generated Thu Oct 11 16:40:18 2007
 * with a MiG generated Mon Sep 11 19:11:05 PDT 2006 by root@b09.apple.com
 * OPTIONS: 
 */
#define	__MIG_check__Reply__JackRPCEngine_subsystem__ 1
#define	__NDR_convert__Reply__JackRPCEngine_subsystem__ 1
#define	__NDR_convert__mig_reply_error_subsystem__ 1

#include "JackRPCEngine.h"


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
#if __MIG_check__Reply__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Reply__rpc_jack_client_open_t__defined)
#define __MIG_check__Reply__rpc_jack_client_open_t__defined
#ifndef __NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_engine__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_engine__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_engine(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_engine__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_engine(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_engine__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_engine(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_engine__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_engine(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_engine__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_client__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_client__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_client(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_client__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_client(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_client__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_client(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_client__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_client(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_client__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_graph__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_graph__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_graph(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_graph__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_graph(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_graph__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_graph(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_graph__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_graph(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_graph__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_client_open_t__result__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__result(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__result(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_open_t__result__defined */


#ifndef __NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_engine__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_engine__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_engine(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_engine__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_engine(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_engine__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_engine(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_engine__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_engine(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_engine__defined */


#ifndef __NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_client__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_client__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_client(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_client__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_client(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_client__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_client(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_client__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_client(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_client__defined */


#ifndef __NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_graph__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_graph__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_graph(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_graph__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_graph(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_graph__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_graph(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_graph__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_graph(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_graph__defined */


#ifndef __NDR_convert__char_rep__Reply__rpc_jack_client_open_t__result__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__result(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__result(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_client_open_t__result__defined */


#ifndef __NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_engine__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_engine__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_engine(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_engine__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_engine(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_engine__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_engine(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_engine__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_engine(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_engine__defined */


#ifndef __NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_client__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_client__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_client(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_client__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_client(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_client__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_client(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_client__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_client(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_client__defined */


#ifndef __NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_graph__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_graph__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_graph(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_graph__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_graph(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_graph__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_graph(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_graph__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_graph(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_graph__defined */


#ifndef __NDR_convert__float_rep__Reply__rpc_jack_client_open_t__result__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__result(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__result(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_client_open_t__result__defined */



mig_internal kern_return_t __MIG_check__Reply__rpc_jack_client_open_t(__Reply__rpc_jack_client_open_t *Out0P)
{

	typedef __Reply__rpc_jack_client_open_t __Reply;
	boolean_t msgh_simple;
#if	__MigTypeCheck
	unsigned int msgh_size;
#endif	/* __MigTypeCheck */
	if (Out0P->Head.msgh_id != 1100) {
	    if (Out0P->Head.msgh_id == MACH_NOTIFY_SEND_ONCE)
		{ return MIG_SERVER_DIED; }
	    else
		{ return MIG_REPLY_MISMATCH; }
	}

	msgh_simple = !(Out0P->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX);
#if	__MigTypeCheck
	msgh_size = Out0P->Head.msgh_size;

	if ((msgh_simple || Out0P->msgh_body.msgh_descriptor_count != 1 ||
	    msgh_size != (mach_msg_size_t)sizeof(__Reply)) &&
	    (!msgh_simple || msgh_size != (mach_msg_size_t)sizeof(mig_reply_error_t) ||
	    ((mig_reply_error_t *)Out0P)->RetCode == KERN_SUCCESS))
		{ return MIG_TYPE_ERROR ; }
#endif	/* __MigTypeCheck */

	if (msgh_simple) {
#ifdef	__NDR_convert__mig_reply_error_t__defined
		__NDR_convert__mig_reply_error_t((mig_reply_error_t *)Out0P);
#endif	/* __NDR_convert__mig_reply_error_t__defined */
		return ((mig_reply_error_t *)Out0P)->RetCode;
	}

#if	__MigTypeCheck
	if (Out0P->private_port.type != MACH_MSG_PORT_DESCRIPTOR ||
	    Out0P->private_port.disposition != 17)
		{ return MIG_TYPE_ERROR; }
#endif	/* __MigTypeCheck */

#if	defined(__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_engine__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_client__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_graph__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__result__defined)
	if (Out0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_engine__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_engine(&Out0P->shared_engine, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_engine__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_client__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_client(&Out0P->shared_client, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_client__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_graph__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_graph(&Out0P->shared_graph, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_open_t__shared_graph__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__result__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_client_open_t__result(&Out0P->result, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_open_t__result__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	defined(__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_engine__defined) || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_client__defined) || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_graph__defined) || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__result__defined)
	if (Out0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_engine__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_engine(&Out0P->shared_engine, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_engine__defined */
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_client__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_client(&Out0P->shared_client, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_client__defined */
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_graph__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_graph(&Out0P->shared_graph, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_client_open_t__shared_graph__defined */
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__result__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_client_open_t__result(&Out0P->result, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_client_open_t__result__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	defined(__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_engine__defined) || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_client__defined) || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_graph__defined) || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__result__defined)
	if (Out0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_engine__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_engine(&Out0P->shared_engine, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_engine__defined */
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_client__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_client(&Out0P->shared_client, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_client__defined */
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_graph__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_graph(&Out0P->shared_graph, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_client_open_t__shared_graph__defined */
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__result__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_client_open_t__result(&Out0P->result, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_client_open_t__result__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Reply__rpc_jack_client_open_t__defined) */
#endif /* __MIG_check__Reply__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_client_open */
mig_external kern_return_t rpc_jack_client_open
(
	mach_port_t server_port,
	client_name_t client_name,
	mach_port_t *private_port,
	int *shared_engine,
	int *shared_client,
	int *shared_graph,
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
		client_name_t client_name;
	} Request;
#ifdef  __MigPackStructs
#pragma pack()
#endif

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
		/* start of the kernel processed data */
		mach_msg_body_t msgh_body;
		mach_msg_port_descriptor_t private_port;
		/* end of the kernel processed data */
		NDR_record_t NDR;
		int shared_engine;
		int shared_client;
		int shared_graph;
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

#ifdef	__MIG_check__Reply__rpc_jack_client_open_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Reply__rpc_jack_client_open_t__defined */

	__DeclareSendRpc(1000, "rpc_jack_client_open")

	InP->NDR = NDR_record;

	(void) mig_strncpy(InP->client_name, client_name, 128);

	InP->Head.msgh_bits =
		MACH_MSGH_BITS(19, MACH_MSG_TYPE_MAKE_SEND_ONCE);
	/* msgh_size passed as argument */
	InP->Head.msgh_request_port = server_port;
	InP->Head.msgh_reply_port = mig_get_reply_port();
	InP->Head.msgh_id = 1000;

	__BeforeSendRpc(1000, "rpc_jack_client_open")
	msg_result = mach_msg(&InP->Head, MACH_SEND_MSG|MACH_RCV_MSG|MACH_MSG_OPTION_NONE, (mach_msg_size_t)sizeof(Request), (mach_msg_size_t)sizeof(Reply), InP->Head.msgh_reply_port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	__AfterSendRpc(1000, "rpc_jack_client_open")
	if (msg_result != MACH_MSG_SUCCESS) {
		__MachMsgErrorWithoutTimeout(msg_result);
		{ return msg_result; }
	}


#if	defined(__MIG_check__Reply__rpc_jack_client_open_t__defined)
	check_result = __MIG_check__Reply__rpc_jack_client_open_t((__Reply__rpc_jack_client_open_t *)Out0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ return check_result; }
#endif	/* defined(__MIG_check__Reply__rpc_jack_client_open_t__defined) */

	*private_port = Out0P->private_port.name;
	*shared_engine = Out0P->shared_engine;

	*shared_client = Out0P->shared_client;

	*shared_graph = Out0P->shared_graph;

	*result = Out0P->result;

	return KERN_SUCCESS;
    }
}

#if ( __MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Reply__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Reply__rpc_jack_client_check_t__defined)
#define __MIG_check__Reply__rpc_jack_client_check_t__defined
#ifndef __NDR_convert__int_rep__Reply__rpc_jack_client_check_t__RetCode__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__RetCode(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__kern_return_t((kern_return_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__RetCode(a, f) \
	__NDR_convert__int_rep__kern_return_t((kern_return_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_check_t__RetCode__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_client_check_t__client_name_res__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__client_name_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__client_name_res__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__client_name_res(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__client_name_t((client_name_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__client_name_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__client_name_res__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__client_name_res(a, f) \
	__NDR_convert__int_rep__client_name_t((client_name_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__string__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__client_name_res__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__client_name_res(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__string(a, f, 128)
#elif	defined(__NDR_convert__int_rep__string__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__client_name_res__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__client_name_res(a, f) \
	__NDR_convert__int_rep__string(a, f, 128)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_check_t__client_name_res__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_client_check_t__status__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__status__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__status(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__status__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__status(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__status__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__status(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__status__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__status(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_check_t__status__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_client_check_t__result__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__result(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__result(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_check_t__result__defined */



#ifndef __NDR_convert__char_rep__Reply__rpc_jack_client_check_t__client_name_res__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__client_name_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__client_name_res__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__client_name_res(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__client_name_t((client_name_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__client_name_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__client_name_res__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__client_name_res(a, f) \
	__NDR_convert__char_rep__client_name_t((client_name_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__string__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__client_name_res__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__client_name_res(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__string(a, f, 128)
#elif	defined(__NDR_convert__char_rep__string__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__client_name_res__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__client_name_res(a, f) \
	__NDR_convert__char_rep__string(a, f, 128)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_client_check_t__client_name_res__defined */


#ifndef __NDR_convert__char_rep__Reply__rpc_jack_client_check_t__status__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__status__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__status(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__status__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__status(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__status__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__status(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__status__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__status(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_client_check_t__status__defined */


#ifndef __NDR_convert__char_rep__Reply__rpc_jack_client_check_t__result__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__result(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__result(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_client_check_t__result__defined */



#ifndef __NDR_convert__float_rep__Reply__rpc_jack_client_check_t__client_name_res__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__client_name_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__client_name_res__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__client_name_res(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__client_name_t((client_name_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__client_name_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__client_name_res__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__client_name_res(a, f) \
	__NDR_convert__float_rep__client_name_t((client_name_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__string__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__client_name_res__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__client_name_res(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__string(a, f, 128)
#elif	defined(__NDR_convert__float_rep__string__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__client_name_res__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__client_name_res(a, f) \
	__NDR_convert__float_rep__string(a, f, 128)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_client_check_t__client_name_res__defined */


#ifndef __NDR_convert__float_rep__Reply__rpc_jack_client_check_t__status__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__status__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__status(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__status__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__status(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__status__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__status(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__status__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__status(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_client_check_t__status__defined */


#ifndef __NDR_convert__float_rep__Reply__rpc_jack_client_check_t__result__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__result(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__result(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_client_check_t__result__defined */



mig_internal kern_return_t __MIG_check__Reply__rpc_jack_client_check_t(__Reply__rpc_jack_client_check_t *Out0P)
{

	typedef __Reply__rpc_jack_client_check_t __Reply;
#if	__MigTypeCheck
	unsigned int msgh_size;
#endif	/* __MigTypeCheck */
	if (Out0P->Head.msgh_id != 1101) {
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

#if	defined(__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__RetCode__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__client_name_res__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__status__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__result__defined)
	if (Out0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__RetCode__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__RetCode(&Out0P->RetCode, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_check_t__RetCode__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__client_name_res__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__client_name_res(&Out0P->client_name_res, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_check_t__client_name_res__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__status__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__status(&Out0P->status, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_check_t__status__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__result__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_client_check_t__result(&Out0P->result, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_check_t__result__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	0 || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__client_name_res__defined) || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__status__defined) || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__result__defined)
	if (Out0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__client_name_res__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__client_name_res(&Out0P->client_name_res, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_client_check_t__client_name_res__defined */
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__status__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__status(&Out0P->status, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_client_check_t__status__defined */
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__result__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_client_check_t__result(&Out0P->result, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_client_check_t__result__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	0 || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__client_name_res__defined) || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__status__defined) || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__result__defined)
	if (Out0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__client_name_res__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__client_name_res(&Out0P->client_name_res, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_client_check_t__client_name_res__defined */
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__status__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__status(&Out0P->status, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_client_check_t__status__defined */
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__result__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_client_check_t__result(&Out0P->result, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_client_check_t__result__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Reply__rpc_jack_client_check_t__defined) */
#endif /* __MIG_check__Reply__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_client_check */
mig_external kern_return_t rpc_jack_client_check
(
	mach_port_t server_port,
	client_name_t client_name,
	client_name_t client_name_res,
	int protocol,
	int options,
	int *status,
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
		client_name_t client_name;
		int protocol;
		int options;
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
		client_name_t client_name_res;
		int status;
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
		client_name_t client_name_res;
		int status;
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

#ifdef	__MIG_check__Reply__rpc_jack_client_check_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Reply__rpc_jack_client_check_t__defined */

	__DeclareSendRpc(1001, "rpc_jack_client_check")

	InP->NDR = NDR_record;

	(void) mig_strncpy(InP->client_name, client_name, 128);

	InP->protocol = protocol;

	InP->options = options;

	InP->Head.msgh_bits =
		MACH_MSGH_BITS(19, MACH_MSG_TYPE_MAKE_SEND_ONCE);
	/* msgh_size passed as argument */
	InP->Head.msgh_request_port = server_port;
	InP->Head.msgh_reply_port = mig_get_reply_port();
	InP->Head.msgh_id = 1001;

	__BeforeSendRpc(1001, "rpc_jack_client_check")
	msg_result = mach_msg(&InP->Head, MACH_SEND_MSG|MACH_RCV_MSG|MACH_MSG_OPTION_NONE, (mach_msg_size_t)sizeof(Request), (mach_msg_size_t)sizeof(Reply), InP->Head.msgh_reply_port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	__AfterSendRpc(1001, "rpc_jack_client_check")
	if (msg_result != MACH_MSG_SUCCESS) {
		__MachMsgErrorWithoutTimeout(msg_result);
		{ return msg_result; }
	}


#if	defined(__MIG_check__Reply__rpc_jack_client_check_t__defined)
	check_result = __MIG_check__Reply__rpc_jack_client_check_t((__Reply__rpc_jack_client_check_t *)Out0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ return check_result; }
#endif	/* defined(__MIG_check__Reply__rpc_jack_client_check_t__defined) */

	(void) mig_strncpy(client_name_res, Out0P->client_name_res, 128);

	*status = Out0P->status;

	*result = Out0P->result;

	return KERN_SUCCESS;
    }
}

#if ( __MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Reply__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Reply__rpc_jack_client_close_t__defined)
#define __MIG_check__Reply__rpc_jack_client_close_t__defined
#ifndef __NDR_convert__int_rep__Reply__rpc_jack_client_close_t__RetCode__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_close_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_close_t__RetCode(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__kern_return_t((kern_return_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_close_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_close_t__RetCode(a, f) \
	__NDR_convert__int_rep__kern_return_t((kern_return_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_close_t__RetCode__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_client_close_t__result__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_close_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_close_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_close_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_close_t__result(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_close_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_close_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_close_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_close_t__result(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_close_t__result__defined */



#ifndef __NDR_convert__char_rep__Reply__rpc_jack_client_close_t__result__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_close_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_close_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_close_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_close_t__result(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_close_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_close_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_close_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_close_t__result(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_client_close_t__result__defined */



#ifndef __NDR_convert__float_rep__Reply__rpc_jack_client_close_t__result__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_close_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_close_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_close_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_close_t__result(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_close_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_close_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_close_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_close_t__result(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_client_close_t__result__defined */



mig_internal kern_return_t __MIG_check__Reply__rpc_jack_client_close_t(__Reply__rpc_jack_client_close_t *Out0P)
{

	typedef __Reply__rpc_jack_client_close_t __Reply;
#if	__MigTypeCheck
	unsigned int msgh_size;
#endif	/* __MigTypeCheck */
	if (Out0P->Head.msgh_id != 1102) {
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

#if	defined(__NDR_convert__int_rep__Reply__rpc_jack_client_close_t__RetCode__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_client_close_t__result__defined)
	if (Out0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_client_close_t__RetCode__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_client_close_t__RetCode(&Out0P->RetCode, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_close_t__RetCode__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_client_close_t__result__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_client_close_t__result(&Out0P->result, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_close_t__result__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	0 || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_client_close_t__result__defined)
	if (Out0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_client_close_t__result__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_client_close_t__result(&Out0P->result, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_client_close_t__result__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	0 || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_client_close_t__result__defined)
	if (Out0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_client_close_t__result__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_client_close_t__result(&Out0P->result, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_client_close_t__result__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Reply__rpc_jack_client_close_t__defined) */
#endif /* __MIG_check__Reply__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_client_close */
mig_external kern_return_t rpc_jack_client_close
(
	mach_port_t server_port,
	int refnum,
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

#ifdef	__MIG_check__Reply__rpc_jack_client_close_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Reply__rpc_jack_client_close_t__defined */

	__DeclareSendRpc(1002, "rpc_jack_client_close")

	InP->NDR = NDR_record;

	InP->refnum = refnum;

	InP->Head.msgh_bits =
		MACH_MSGH_BITS(19, MACH_MSG_TYPE_MAKE_SEND_ONCE);
	/* msgh_size passed as argument */
	InP->Head.msgh_request_port = server_port;
	InP->Head.msgh_reply_port = mig_get_reply_port();
	InP->Head.msgh_id = 1002;

	__BeforeSendRpc(1002, "rpc_jack_client_close")
	msg_result = mach_msg(&InP->Head, MACH_SEND_MSG|MACH_RCV_MSG|MACH_MSG_OPTION_NONE, (mach_msg_size_t)sizeof(Request), (mach_msg_size_t)sizeof(Reply), InP->Head.msgh_reply_port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	__AfterSendRpc(1002, "rpc_jack_client_close")
	if (msg_result != MACH_MSG_SUCCESS) {
		__MachMsgErrorWithoutTimeout(msg_result);
		{ return msg_result; }
	}


#if	defined(__MIG_check__Reply__rpc_jack_client_close_t__defined)
	check_result = __MIG_check__Reply__rpc_jack_client_close_t((__Reply__rpc_jack_client_close_t *)Out0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ return check_result; }
#endif	/* defined(__MIG_check__Reply__rpc_jack_client_close_t__defined) */

	*result = Out0P->result;

	return KERN_SUCCESS;
    }
}

#if ( __MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Reply__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Reply__rpc_jack_client_activate_t__defined)
#define __MIG_check__Reply__rpc_jack_client_activate_t__defined
#ifndef __NDR_convert__int_rep__Reply__rpc_jack_client_activate_t__RetCode__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_activate_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_activate_t__RetCode(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__kern_return_t((kern_return_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_activate_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_activate_t__RetCode(a, f) \
	__NDR_convert__int_rep__kern_return_t((kern_return_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_activate_t__RetCode__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_client_activate_t__result__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_activate_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_activate_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_activate_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_activate_t__result(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_activate_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_activate_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_activate_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_activate_t__result(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_activate_t__result__defined */



#ifndef __NDR_convert__char_rep__Reply__rpc_jack_client_activate_t__result__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_activate_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_activate_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_activate_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_activate_t__result(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_activate_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_activate_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_activate_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_activate_t__result(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_client_activate_t__result__defined */



#ifndef __NDR_convert__float_rep__Reply__rpc_jack_client_activate_t__result__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_activate_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_activate_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_activate_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_activate_t__result(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_activate_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_activate_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_activate_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_activate_t__result(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_client_activate_t__result__defined */



mig_internal kern_return_t __MIG_check__Reply__rpc_jack_client_activate_t(__Reply__rpc_jack_client_activate_t *Out0P)
{

	typedef __Reply__rpc_jack_client_activate_t __Reply;
#if	__MigTypeCheck
	unsigned int msgh_size;
#endif	/* __MigTypeCheck */
	if (Out0P->Head.msgh_id != 1103) {
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

#if	defined(__NDR_convert__int_rep__Reply__rpc_jack_client_activate_t__RetCode__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_client_activate_t__result__defined)
	if (Out0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_client_activate_t__RetCode__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_client_activate_t__RetCode(&Out0P->RetCode, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_activate_t__RetCode__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_client_activate_t__result__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_client_activate_t__result(&Out0P->result, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_activate_t__result__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	0 || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_client_activate_t__result__defined)
	if (Out0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_client_activate_t__result__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_client_activate_t__result(&Out0P->result, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_client_activate_t__result__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	0 || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_client_activate_t__result__defined)
	if (Out0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_client_activate_t__result__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_client_activate_t__result(&Out0P->result, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_client_activate_t__result__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Reply__rpc_jack_client_activate_t__defined) */
#endif /* __MIG_check__Reply__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_client_activate */
mig_external kern_return_t rpc_jack_client_activate
(
	mach_port_t server_port,
	int refnum,
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

#ifdef	__MIG_check__Reply__rpc_jack_client_activate_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Reply__rpc_jack_client_activate_t__defined */

	__DeclareSendRpc(1003, "rpc_jack_client_activate")

	InP->NDR = NDR_record;

	InP->refnum = refnum;

	InP->Head.msgh_bits =
		MACH_MSGH_BITS(19, MACH_MSG_TYPE_MAKE_SEND_ONCE);
	/* msgh_size passed as argument */
	InP->Head.msgh_request_port = server_port;
	InP->Head.msgh_reply_port = mig_get_reply_port();
	InP->Head.msgh_id = 1003;

	__BeforeSendRpc(1003, "rpc_jack_client_activate")
	msg_result = mach_msg(&InP->Head, MACH_SEND_MSG|MACH_RCV_MSG|MACH_MSG_OPTION_NONE, (mach_msg_size_t)sizeof(Request), (mach_msg_size_t)sizeof(Reply), InP->Head.msgh_reply_port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	__AfterSendRpc(1003, "rpc_jack_client_activate")
	if (msg_result != MACH_MSG_SUCCESS) {
		__MachMsgErrorWithoutTimeout(msg_result);
		{ return msg_result; }
	}


#if	defined(__MIG_check__Reply__rpc_jack_client_activate_t__defined)
	check_result = __MIG_check__Reply__rpc_jack_client_activate_t((__Reply__rpc_jack_client_activate_t *)Out0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ return check_result; }
#endif	/* defined(__MIG_check__Reply__rpc_jack_client_activate_t__defined) */

	*result = Out0P->result;

	return KERN_SUCCESS;
    }
}

#if ( __MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Reply__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Reply__rpc_jack_client_deactivate_t__defined)
#define __MIG_check__Reply__rpc_jack_client_deactivate_t__defined
#ifndef __NDR_convert__int_rep__Reply__rpc_jack_client_deactivate_t__RetCode__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_deactivate_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_deactivate_t__RetCode(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__kern_return_t((kern_return_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_deactivate_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_deactivate_t__RetCode(a, f) \
	__NDR_convert__int_rep__kern_return_t((kern_return_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_deactivate_t__RetCode__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_client_deactivate_t__result__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_deactivate_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_deactivate_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_deactivate_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_deactivate_t__result(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_deactivate_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_deactivate_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_deactivate_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_client_deactivate_t__result(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_deactivate_t__result__defined */



#ifndef __NDR_convert__char_rep__Reply__rpc_jack_client_deactivate_t__result__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_deactivate_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_deactivate_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_deactivate_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_deactivate_t__result(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_deactivate_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_deactivate_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_deactivate_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_client_deactivate_t__result(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_client_deactivate_t__result__defined */



#ifndef __NDR_convert__float_rep__Reply__rpc_jack_client_deactivate_t__result__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_deactivate_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_deactivate_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_deactivate_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_deactivate_t__result(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_deactivate_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_deactivate_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_deactivate_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_client_deactivate_t__result(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_client_deactivate_t__result__defined */



mig_internal kern_return_t __MIG_check__Reply__rpc_jack_client_deactivate_t(__Reply__rpc_jack_client_deactivate_t *Out0P)
{

	typedef __Reply__rpc_jack_client_deactivate_t __Reply;
#if	__MigTypeCheck
	unsigned int msgh_size;
#endif	/* __MigTypeCheck */
	if (Out0P->Head.msgh_id != 1104) {
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

#if	defined(__NDR_convert__int_rep__Reply__rpc_jack_client_deactivate_t__RetCode__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_client_deactivate_t__result__defined)
	if (Out0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_client_deactivate_t__RetCode__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_client_deactivate_t__RetCode(&Out0P->RetCode, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_deactivate_t__RetCode__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_client_deactivate_t__result__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_client_deactivate_t__result(&Out0P->result, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_client_deactivate_t__result__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	0 || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_client_deactivate_t__result__defined)
	if (Out0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_client_deactivate_t__result__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_client_deactivate_t__result(&Out0P->result, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_client_deactivate_t__result__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	0 || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_client_deactivate_t__result__defined)
	if (Out0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_client_deactivate_t__result__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_client_deactivate_t__result(&Out0P->result, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_client_deactivate_t__result__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Reply__rpc_jack_client_deactivate_t__defined) */
#endif /* __MIG_check__Reply__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_client_deactivate */
mig_external kern_return_t rpc_jack_client_deactivate
(
	mach_port_t server_port,
	int refnum,
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

#ifdef	__MIG_check__Reply__rpc_jack_client_deactivate_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Reply__rpc_jack_client_deactivate_t__defined */

	__DeclareSendRpc(1004, "rpc_jack_client_deactivate")

	InP->NDR = NDR_record;

	InP->refnum = refnum;

	InP->Head.msgh_bits =
		MACH_MSGH_BITS(19, MACH_MSG_TYPE_MAKE_SEND_ONCE);
	/* msgh_size passed as argument */
	InP->Head.msgh_request_port = server_port;
	InP->Head.msgh_reply_port = mig_get_reply_port();
	InP->Head.msgh_id = 1004;

	__BeforeSendRpc(1004, "rpc_jack_client_deactivate")
	msg_result = mach_msg(&InP->Head, MACH_SEND_MSG|MACH_RCV_MSG|MACH_MSG_OPTION_NONE, (mach_msg_size_t)sizeof(Request), (mach_msg_size_t)sizeof(Reply), InP->Head.msgh_reply_port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	__AfterSendRpc(1004, "rpc_jack_client_deactivate")
	if (msg_result != MACH_MSG_SUCCESS) {
		__MachMsgErrorWithoutTimeout(msg_result);
		{ return msg_result; }
	}


#if	defined(__MIG_check__Reply__rpc_jack_client_deactivate_t__defined)
	check_result = __MIG_check__Reply__rpc_jack_client_deactivate_t((__Reply__rpc_jack_client_deactivate_t *)Out0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ return check_result; }
#endif	/* defined(__MIG_check__Reply__rpc_jack_client_deactivate_t__defined) */

	*result = Out0P->result;

	return KERN_SUCCESS;
    }
}

#if ( __MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Reply__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Reply__rpc_jack_port_register_t__defined)
#define __MIG_check__Reply__rpc_jack_port_register_t__defined
#ifndef __NDR_convert__int_rep__Reply__rpc_jack_port_register_t__RetCode__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__RetCode(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__kern_return_t((kern_return_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__RetCode(a, f) \
	__NDR_convert__int_rep__kern_return_t((kern_return_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_port_register_t__RetCode__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_port_register_t__port_index__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__unsigned__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__port_index__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__port_index(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__unsigned((unsigned *)(a), f)
#elif	defined(__NDR_convert__int_rep__unsigned__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__port_index__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__port_index(a, f) \
	__NDR_convert__int_rep__unsigned((unsigned *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__uint32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__port_index__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__port_index(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__uint32_t((uint32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__uint32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__port_index__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__port_index(a, f) \
	__NDR_convert__int_rep__uint32_t((uint32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_port_register_t__port_index__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_port_register_t__result__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__result(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__result(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_port_register_t__result__defined */



#ifndef __NDR_convert__char_rep__Reply__rpc_jack_port_register_t__port_index__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__unsigned__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_register_t__port_index__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_register_t__port_index(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__unsigned((unsigned *)(a), f)
#elif	defined(__NDR_convert__char_rep__unsigned__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_register_t__port_index__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_register_t__port_index(a, f) \
	__NDR_convert__char_rep__unsigned((unsigned *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__uint32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_register_t__port_index__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_register_t__port_index(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__uint32_t((uint32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__uint32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_register_t__port_index__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_register_t__port_index(a, f) \
	__NDR_convert__char_rep__uint32_t((uint32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_port_register_t__port_index__defined */


#ifndef __NDR_convert__char_rep__Reply__rpc_jack_port_register_t__result__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_register_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_register_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_register_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_register_t__result(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_register_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_register_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_register_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_register_t__result(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_port_register_t__result__defined */



#ifndef __NDR_convert__float_rep__Reply__rpc_jack_port_register_t__port_index__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__unsigned__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_register_t__port_index__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_register_t__port_index(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__unsigned((unsigned *)(a), f)
#elif	defined(__NDR_convert__float_rep__unsigned__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_register_t__port_index__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_register_t__port_index(a, f) \
	__NDR_convert__float_rep__unsigned((unsigned *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__uint32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_register_t__port_index__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_register_t__port_index(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__uint32_t((uint32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__uint32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_register_t__port_index__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_register_t__port_index(a, f) \
	__NDR_convert__float_rep__uint32_t((uint32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_port_register_t__port_index__defined */


#ifndef __NDR_convert__float_rep__Reply__rpc_jack_port_register_t__result__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_register_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_register_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_register_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_register_t__result(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_register_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_register_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_register_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_register_t__result(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_port_register_t__result__defined */



mig_internal kern_return_t __MIG_check__Reply__rpc_jack_port_register_t(__Reply__rpc_jack_port_register_t *Out0P)
{

	typedef __Reply__rpc_jack_port_register_t __Reply;
#if	__MigTypeCheck
	unsigned int msgh_size;
#endif	/* __MigTypeCheck */
	if (Out0P->Head.msgh_id != 1105) {
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

#if	defined(__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__RetCode__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__port_index__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__result__defined)
	if (Out0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__RetCode__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__RetCode(&Out0P->RetCode, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_port_register_t__RetCode__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__port_index__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__port_index(&Out0P->port_index, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_port_register_t__port_index__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__result__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_port_register_t__result(&Out0P->result, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_port_register_t__result__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	0 || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_port_register_t__port_index__defined) || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_port_register_t__result__defined)
	if (Out0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_port_register_t__port_index__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_port_register_t__port_index(&Out0P->port_index, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_port_register_t__port_index__defined */
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_port_register_t__result__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_port_register_t__result(&Out0P->result, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_port_register_t__result__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	0 || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_port_register_t__port_index__defined) || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_port_register_t__result__defined)
	if (Out0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_port_register_t__port_index__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_port_register_t__port_index(&Out0P->port_index, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_port_register_t__port_index__defined */
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_port_register_t__result__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_port_register_t__result(&Out0P->result, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_port_register_t__result__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Reply__rpc_jack_port_register_t__defined) */
#endif /* __MIG_check__Reply__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_port_register */
mig_external kern_return_t rpc_jack_port_register
(
	mach_port_t server_port,
	int refnum,
	client_port_name_t name,
	unsigned flags,
	unsigned buffer_size,
	unsigned *port_index,
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
		client_port_name_t name;
		unsigned flags;
		unsigned buffer_size;
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
		unsigned port_index;
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
		unsigned port_index;
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

#ifdef	__MIG_check__Reply__rpc_jack_port_register_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Reply__rpc_jack_port_register_t__defined */

	__DeclareSendRpc(1005, "rpc_jack_port_register")

	InP->NDR = NDR_record;

	InP->refnum = refnum;

	(void) mig_strncpy(InP->name, name, 128);

	InP->flags = flags;

	InP->buffer_size = buffer_size;

	InP->Head.msgh_bits =
		MACH_MSGH_BITS(19, MACH_MSG_TYPE_MAKE_SEND_ONCE);
	/* msgh_size passed as argument */
	InP->Head.msgh_request_port = server_port;
	InP->Head.msgh_reply_port = mig_get_reply_port();
	InP->Head.msgh_id = 1005;

	__BeforeSendRpc(1005, "rpc_jack_port_register")
	msg_result = mach_msg(&InP->Head, MACH_SEND_MSG|MACH_RCV_MSG|MACH_MSG_OPTION_NONE, (mach_msg_size_t)sizeof(Request), (mach_msg_size_t)sizeof(Reply), InP->Head.msgh_reply_port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	__AfterSendRpc(1005, "rpc_jack_port_register")
	if (msg_result != MACH_MSG_SUCCESS) {
		__MachMsgErrorWithoutTimeout(msg_result);
		{ return msg_result; }
	}


#if	defined(__MIG_check__Reply__rpc_jack_port_register_t__defined)
	check_result = __MIG_check__Reply__rpc_jack_port_register_t((__Reply__rpc_jack_port_register_t *)Out0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ return check_result; }
#endif	/* defined(__MIG_check__Reply__rpc_jack_port_register_t__defined) */

	*port_index = Out0P->port_index;

	*result = Out0P->result;

	return KERN_SUCCESS;
    }
}

#if ( __MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Reply__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Reply__rpc_jack_port_unregister_t__defined)
#define __MIG_check__Reply__rpc_jack_port_unregister_t__defined
#ifndef __NDR_convert__int_rep__Reply__rpc_jack_port_unregister_t__RetCode__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_unregister_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_unregister_t__RetCode(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__kern_return_t((kern_return_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_unregister_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_unregister_t__RetCode(a, f) \
	__NDR_convert__int_rep__kern_return_t((kern_return_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_port_unregister_t__RetCode__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_port_unregister_t__result__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_unregister_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_unregister_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_unregister_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_unregister_t__result(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_unregister_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_unregister_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_unregister_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_unregister_t__result(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_port_unregister_t__result__defined */



#ifndef __NDR_convert__char_rep__Reply__rpc_jack_port_unregister_t__result__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_unregister_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_unregister_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_unregister_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_unregister_t__result(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_unregister_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_unregister_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_unregister_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_unregister_t__result(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_port_unregister_t__result__defined */



#ifndef __NDR_convert__float_rep__Reply__rpc_jack_port_unregister_t__result__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_unregister_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_unregister_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_unregister_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_unregister_t__result(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_unregister_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_unregister_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_unregister_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_unregister_t__result(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_port_unregister_t__result__defined */



mig_internal kern_return_t __MIG_check__Reply__rpc_jack_port_unregister_t(__Reply__rpc_jack_port_unregister_t *Out0P)
{

	typedef __Reply__rpc_jack_port_unregister_t __Reply;
#if	__MigTypeCheck
	unsigned int msgh_size;
#endif	/* __MigTypeCheck */
	if (Out0P->Head.msgh_id != 1106) {
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

#if	defined(__NDR_convert__int_rep__Reply__rpc_jack_port_unregister_t__RetCode__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_port_unregister_t__result__defined)
	if (Out0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_port_unregister_t__RetCode__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_port_unregister_t__RetCode(&Out0P->RetCode, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_port_unregister_t__RetCode__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_port_unregister_t__result__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_port_unregister_t__result(&Out0P->result, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_port_unregister_t__result__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	0 || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_port_unregister_t__result__defined)
	if (Out0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_port_unregister_t__result__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_port_unregister_t__result(&Out0P->result, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_port_unregister_t__result__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	0 || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_port_unregister_t__result__defined)
	if (Out0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_port_unregister_t__result__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_port_unregister_t__result(&Out0P->result, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_port_unregister_t__result__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Reply__rpc_jack_port_unregister_t__defined) */
#endif /* __MIG_check__Reply__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_port_unregister */
mig_external kern_return_t rpc_jack_port_unregister
(
	mach_port_t server_port,
	int refnum,
	int port,
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
		int port;
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

#ifdef	__MIG_check__Reply__rpc_jack_port_unregister_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Reply__rpc_jack_port_unregister_t__defined */

	__DeclareSendRpc(1006, "rpc_jack_port_unregister")

	InP->NDR = NDR_record;

	InP->refnum = refnum;

	InP->port = port;

	InP->Head.msgh_bits =
		MACH_MSGH_BITS(19, MACH_MSG_TYPE_MAKE_SEND_ONCE);
	/* msgh_size passed as argument */
	InP->Head.msgh_request_port = server_port;
	InP->Head.msgh_reply_port = mig_get_reply_port();
	InP->Head.msgh_id = 1006;

	__BeforeSendRpc(1006, "rpc_jack_port_unregister")
	msg_result = mach_msg(&InP->Head, MACH_SEND_MSG|MACH_RCV_MSG|MACH_MSG_OPTION_NONE, (mach_msg_size_t)sizeof(Request), (mach_msg_size_t)sizeof(Reply), InP->Head.msgh_reply_port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	__AfterSendRpc(1006, "rpc_jack_port_unregister")
	if (msg_result != MACH_MSG_SUCCESS) {
		__MachMsgErrorWithoutTimeout(msg_result);
		{ return msg_result; }
	}


#if	defined(__MIG_check__Reply__rpc_jack_port_unregister_t__defined)
	check_result = __MIG_check__Reply__rpc_jack_port_unregister_t((__Reply__rpc_jack_port_unregister_t *)Out0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ return check_result; }
#endif	/* defined(__MIG_check__Reply__rpc_jack_port_unregister_t__defined) */

	*result = Out0P->result;

	return KERN_SUCCESS;
    }
}

#if ( __MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Reply__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Reply__rpc_jack_port_connect_t__defined)
#define __MIG_check__Reply__rpc_jack_port_connect_t__defined
#ifndef __NDR_convert__int_rep__Reply__rpc_jack_port_connect_t__RetCode__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_connect_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_connect_t__RetCode(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__kern_return_t((kern_return_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_connect_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_connect_t__RetCode(a, f) \
	__NDR_convert__int_rep__kern_return_t((kern_return_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_port_connect_t__RetCode__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_port_connect_t__result__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_connect_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_connect_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_connect_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_connect_t__result(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_connect_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_connect_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_connect_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_connect_t__result(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_port_connect_t__result__defined */



#ifndef __NDR_convert__char_rep__Reply__rpc_jack_port_connect_t__result__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_connect_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_connect_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_connect_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_connect_t__result(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_connect_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_connect_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_connect_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_connect_t__result(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_port_connect_t__result__defined */



#ifndef __NDR_convert__float_rep__Reply__rpc_jack_port_connect_t__result__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_connect_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_connect_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_connect_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_connect_t__result(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_connect_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_connect_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_connect_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_connect_t__result(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_port_connect_t__result__defined */



mig_internal kern_return_t __MIG_check__Reply__rpc_jack_port_connect_t(__Reply__rpc_jack_port_connect_t *Out0P)
{

	typedef __Reply__rpc_jack_port_connect_t __Reply;
#if	__MigTypeCheck
	unsigned int msgh_size;
#endif	/* __MigTypeCheck */
	if (Out0P->Head.msgh_id != 1107) {
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

#if	defined(__NDR_convert__int_rep__Reply__rpc_jack_port_connect_t__RetCode__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_port_connect_t__result__defined)
	if (Out0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_port_connect_t__RetCode__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_port_connect_t__RetCode(&Out0P->RetCode, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_port_connect_t__RetCode__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_port_connect_t__result__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_port_connect_t__result(&Out0P->result, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_port_connect_t__result__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	0 || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_port_connect_t__result__defined)
	if (Out0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_port_connect_t__result__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_port_connect_t__result(&Out0P->result, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_port_connect_t__result__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	0 || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_port_connect_t__result__defined)
	if (Out0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_port_connect_t__result__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_port_connect_t__result(&Out0P->result, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_port_connect_t__result__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Reply__rpc_jack_port_connect_t__defined) */
#endif /* __MIG_check__Reply__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_port_connect */
mig_external kern_return_t rpc_jack_port_connect
(
	mach_port_t server_port,
	int refnum,
	int src,
	int dst,
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
		int src;
		int dst;
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

#ifdef	__MIG_check__Reply__rpc_jack_port_connect_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Reply__rpc_jack_port_connect_t__defined */

	__DeclareSendRpc(1007, "rpc_jack_port_connect")

	InP->NDR = NDR_record;

	InP->refnum = refnum;

	InP->src = src;

	InP->dst = dst;

	InP->Head.msgh_bits =
		MACH_MSGH_BITS(19, MACH_MSG_TYPE_MAKE_SEND_ONCE);
	/* msgh_size passed as argument */
	InP->Head.msgh_request_port = server_port;
	InP->Head.msgh_reply_port = mig_get_reply_port();
	InP->Head.msgh_id = 1007;

	__BeforeSendRpc(1007, "rpc_jack_port_connect")
	msg_result = mach_msg(&InP->Head, MACH_SEND_MSG|MACH_RCV_MSG|MACH_MSG_OPTION_NONE, (mach_msg_size_t)sizeof(Request), (mach_msg_size_t)sizeof(Reply), InP->Head.msgh_reply_port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	__AfterSendRpc(1007, "rpc_jack_port_connect")
	if (msg_result != MACH_MSG_SUCCESS) {
		__MachMsgErrorWithoutTimeout(msg_result);
		{ return msg_result; }
	}


#if	defined(__MIG_check__Reply__rpc_jack_port_connect_t__defined)
	check_result = __MIG_check__Reply__rpc_jack_port_connect_t((__Reply__rpc_jack_port_connect_t *)Out0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ return check_result; }
#endif	/* defined(__MIG_check__Reply__rpc_jack_port_connect_t__defined) */

	*result = Out0P->result;

	return KERN_SUCCESS;
    }
}

#if ( __MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Reply__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Reply__rpc_jack_port_disconnect_t__defined)
#define __MIG_check__Reply__rpc_jack_port_disconnect_t__defined
#ifndef __NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_t__RetCode__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_t__RetCode(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__kern_return_t((kern_return_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_t__RetCode(a, f) \
	__NDR_convert__int_rep__kern_return_t((kern_return_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_t__RetCode__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_t__result__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_t__result(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_t__result(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_t__result__defined */



#ifndef __NDR_convert__char_rep__Reply__rpc_jack_port_disconnect_t__result__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_disconnect_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_disconnect_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_disconnect_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_disconnect_t__result(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_disconnect_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_disconnect_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_disconnect_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_disconnect_t__result(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_port_disconnect_t__result__defined */



#ifndef __NDR_convert__float_rep__Reply__rpc_jack_port_disconnect_t__result__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_disconnect_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_disconnect_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_disconnect_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_disconnect_t__result(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_disconnect_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_disconnect_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_disconnect_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_disconnect_t__result(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_port_disconnect_t__result__defined */



mig_internal kern_return_t __MIG_check__Reply__rpc_jack_port_disconnect_t(__Reply__rpc_jack_port_disconnect_t *Out0P)
{

	typedef __Reply__rpc_jack_port_disconnect_t __Reply;
#if	__MigTypeCheck
	unsigned int msgh_size;
#endif	/* __MigTypeCheck */
	if (Out0P->Head.msgh_id != 1108) {
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

#if	defined(__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_t__RetCode__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_t__result__defined)
	if (Out0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_t__RetCode__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_t__RetCode(&Out0P->RetCode, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_t__RetCode__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_t__result__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_t__result(&Out0P->result, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_t__result__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	0 || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_port_disconnect_t__result__defined)
	if (Out0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_port_disconnect_t__result__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_port_disconnect_t__result(&Out0P->result, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_port_disconnect_t__result__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	0 || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_port_disconnect_t__result__defined)
	if (Out0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_port_disconnect_t__result__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_port_disconnect_t__result(&Out0P->result, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_port_disconnect_t__result__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Reply__rpc_jack_port_disconnect_t__defined) */
#endif /* __MIG_check__Reply__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_port_disconnect */
mig_external kern_return_t rpc_jack_port_disconnect
(
	mach_port_t server_port,
	int refnum,
	int src,
	int dst,
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
		int src;
		int dst;
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

#ifdef	__MIG_check__Reply__rpc_jack_port_disconnect_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Reply__rpc_jack_port_disconnect_t__defined */

	__DeclareSendRpc(1008, "rpc_jack_port_disconnect")

	InP->NDR = NDR_record;

	InP->refnum = refnum;

	InP->src = src;

	InP->dst = dst;

	InP->Head.msgh_bits =
		MACH_MSGH_BITS(19, MACH_MSG_TYPE_MAKE_SEND_ONCE);
	/* msgh_size passed as argument */
	InP->Head.msgh_request_port = server_port;
	InP->Head.msgh_reply_port = mig_get_reply_port();
	InP->Head.msgh_id = 1008;

	__BeforeSendRpc(1008, "rpc_jack_port_disconnect")
	msg_result = mach_msg(&InP->Head, MACH_SEND_MSG|MACH_RCV_MSG|MACH_MSG_OPTION_NONE, (mach_msg_size_t)sizeof(Request), (mach_msg_size_t)sizeof(Reply), InP->Head.msgh_reply_port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	__AfterSendRpc(1008, "rpc_jack_port_disconnect")
	if (msg_result != MACH_MSG_SUCCESS) {
		__MachMsgErrorWithoutTimeout(msg_result);
		{ return msg_result; }
	}


#if	defined(__MIG_check__Reply__rpc_jack_port_disconnect_t__defined)
	check_result = __MIG_check__Reply__rpc_jack_port_disconnect_t((__Reply__rpc_jack_port_disconnect_t *)Out0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ return check_result; }
#endif	/* defined(__MIG_check__Reply__rpc_jack_port_disconnect_t__defined) */

	*result = Out0P->result;

	return KERN_SUCCESS;
    }
}

#if ( __MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Reply__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Reply__rpc_jack_port_connect_name_t__defined)
#define __MIG_check__Reply__rpc_jack_port_connect_name_t__defined
#ifndef __NDR_convert__int_rep__Reply__rpc_jack_port_connect_name_t__RetCode__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_connect_name_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_connect_name_t__RetCode(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__kern_return_t((kern_return_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_connect_name_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_connect_name_t__RetCode(a, f) \
	__NDR_convert__int_rep__kern_return_t((kern_return_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_port_connect_name_t__RetCode__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_port_connect_name_t__result__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_connect_name_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_connect_name_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_connect_name_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_connect_name_t__result(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_connect_name_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_connect_name_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_connect_name_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_connect_name_t__result(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_port_connect_name_t__result__defined */



#ifndef __NDR_convert__char_rep__Reply__rpc_jack_port_connect_name_t__result__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_connect_name_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_connect_name_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_connect_name_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_connect_name_t__result(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_connect_name_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_connect_name_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_connect_name_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_connect_name_t__result(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_port_connect_name_t__result__defined */



#ifndef __NDR_convert__float_rep__Reply__rpc_jack_port_connect_name_t__result__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_connect_name_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_connect_name_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_connect_name_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_connect_name_t__result(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_connect_name_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_connect_name_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_connect_name_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_connect_name_t__result(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_port_connect_name_t__result__defined */



mig_internal kern_return_t __MIG_check__Reply__rpc_jack_port_connect_name_t(__Reply__rpc_jack_port_connect_name_t *Out0P)
{

	typedef __Reply__rpc_jack_port_connect_name_t __Reply;
#if	__MigTypeCheck
	unsigned int msgh_size;
#endif	/* __MigTypeCheck */
	if (Out0P->Head.msgh_id != 1109) {
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

#if	defined(__NDR_convert__int_rep__Reply__rpc_jack_port_connect_name_t__RetCode__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_port_connect_name_t__result__defined)
	if (Out0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_port_connect_name_t__RetCode__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_port_connect_name_t__RetCode(&Out0P->RetCode, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_port_connect_name_t__RetCode__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_port_connect_name_t__result__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_port_connect_name_t__result(&Out0P->result, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_port_connect_name_t__result__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	0 || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_port_connect_name_t__result__defined)
	if (Out0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_port_connect_name_t__result__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_port_connect_name_t__result(&Out0P->result, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_port_connect_name_t__result__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	0 || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_port_connect_name_t__result__defined)
	if (Out0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_port_connect_name_t__result__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_port_connect_name_t__result(&Out0P->result, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_port_connect_name_t__result__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Reply__rpc_jack_port_connect_name_t__defined) */
#endif /* __MIG_check__Reply__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_port_connect_name */
mig_external kern_return_t rpc_jack_port_connect_name
(
	mach_port_t server_port,
	int refnum,
	client_port_name_t src,
	client_port_name_t dst,
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
		client_port_name_t src;
		client_port_name_t dst;
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

#ifdef	__MIG_check__Reply__rpc_jack_port_connect_name_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Reply__rpc_jack_port_connect_name_t__defined */

	__DeclareSendRpc(1009, "rpc_jack_port_connect_name")

	InP->NDR = NDR_record;

	InP->refnum = refnum;

	(void) mig_strncpy(InP->src, src, 128);

	(void) mig_strncpy(InP->dst, dst, 128);

	InP->Head.msgh_bits =
		MACH_MSGH_BITS(19, MACH_MSG_TYPE_MAKE_SEND_ONCE);
	/* msgh_size passed as argument */
	InP->Head.msgh_request_port = server_port;
	InP->Head.msgh_reply_port = mig_get_reply_port();
	InP->Head.msgh_id = 1009;

	__BeforeSendRpc(1009, "rpc_jack_port_connect_name")
	msg_result = mach_msg(&InP->Head, MACH_SEND_MSG|MACH_RCV_MSG|MACH_MSG_OPTION_NONE, (mach_msg_size_t)sizeof(Request), (mach_msg_size_t)sizeof(Reply), InP->Head.msgh_reply_port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	__AfterSendRpc(1009, "rpc_jack_port_connect_name")
	if (msg_result != MACH_MSG_SUCCESS) {
		__MachMsgErrorWithoutTimeout(msg_result);
		{ return msg_result; }
	}


#if	defined(__MIG_check__Reply__rpc_jack_port_connect_name_t__defined)
	check_result = __MIG_check__Reply__rpc_jack_port_connect_name_t((__Reply__rpc_jack_port_connect_name_t *)Out0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ return check_result; }
#endif	/* defined(__MIG_check__Reply__rpc_jack_port_connect_name_t__defined) */

	*result = Out0P->result;

	return KERN_SUCCESS;
    }
}

#if ( __MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Reply__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Reply__rpc_jack_port_disconnect_name_t__defined)
#define __MIG_check__Reply__rpc_jack_port_disconnect_name_t__defined
#ifndef __NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_name_t__RetCode__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_name_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_name_t__RetCode(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__kern_return_t((kern_return_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_name_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_name_t__RetCode(a, f) \
	__NDR_convert__int_rep__kern_return_t((kern_return_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_name_t__RetCode__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_name_t__result__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_name_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_name_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_name_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_name_t__result(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_name_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_name_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_name_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_name_t__result(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_name_t__result__defined */



#ifndef __NDR_convert__char_rep__Reply__rpc_jack_port_disconnect_name_t__result__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_disconnect_name_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_disconnect_name_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_disconnect_name_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_disconnect_name_t__result(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_disconnect_name_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_disconnect_name_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_disconnect_name_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_port_disconnect_name_t__result(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_port_disconnect_name_t__result__defined */



#ifndef __NDR_convert__float_rep__Reply__rpc_jack_port_disconnect_name_t__result__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_disconnect_name_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_disconnect_name_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_disconnect_name_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_disconnect_name_t__result(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_disconnect_name_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_disconnect_name_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_disconnect_name_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_port_disconnect_name_t__result(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_port_disconnect_name_t__result__defined */



mig_internal kern_return_t __MIG_check__Reply__rpc_jack_port_disconnect_name_t(__Reply__rpc_jack_port_disconnect_name_t *Out0P)
{

	typedef __Reply__rpc_jack_port_disconnect_name_t __Reply;
#if	__MigTypeCheck
	unsigned int msgh_size;
#endif	/* __MigTypeCheck */
	if (Out0P->Head.msgh_id != 1110) {
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

#if	defined(__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_name_t__RetCode__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_name_t__result__defined)
	if (Out0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_name_t__RetCode__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_name_t__RetCode(&Out0P->RetCode, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_name_t__RetCode__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_name_t__result__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_name_t__result(&Out0P->result, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_port_disconnect_name_t__result__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	0 || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_port_disconnect_name_t__result__defined)
	if (Out0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_port_disconnect_name_t__result__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_port_disconnect_name_t__result(&Out0P->result, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_port_disconnect_name_t__result__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	0 || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_port_disconnect_name_t__result__defined)
	if (Out0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_port_disconnect_name_t__result__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_port_disconnect_name_t__result(&Out0P->result, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_port_disconnect_name_t__result__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Reply__rpc_jack_port_disconnect_name_t__defined) */
#endif /* __MIG_check__Reply__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_port_disconnect_name */
mig_external kern_return_t rpc_jack_port_disconnect_name
(
	mach_port_t server_port,
	int refnum,
	client_port_name_t src,
	client_port_name_t dst,
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
		client_port_name_t src;
		client_port_name_t dst;
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

#ifdef	__MIG_check__Reply__rpc_jack_port_disconnect_name_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Reply__rpc_jack_port_disconnect_name_t__defined */

	__DeclareSendRpc(1010, "rpc_jack_port_disconnect_name")

	InP->NDR = NDR_record;

	InP->refnum = refnum;

	(void) mig_strncpy(InP->src, src, 128);

	(void) mig_strncpy(InP->dst, dst, 128);

	InP->Head.msgh_bits =
		MACH_MSGH_BITS(19, MACH_MSG_TYPE_MAKE_SEND_ONCE);
	/* msgh_size passed as argument */
	InP->Head.msgh_request_port = server_port;
	InP->Head.msgh_reply_port = mig_get_reply_port();
	InP->Head.msgh_id = 1010;

	__BeforeSendRpc(1010, "rpc_jack_port_disconnect_name")
	msg_result = mach_msg(&InP->Head, MACH_SEND_MSG|MACH_RCV_MSG|MACH_MSG_OPTION_NONE, (mach_msg_size_t)sizeof(Request), (mach_msg_size_t)sizeof(Reply), InP->Head.msgh_reply_port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	__AfterSendRpc(1010, "rpc_jack_port_disconnect_name")
	if (msg_result != MACH_MSG_SUCCESS) {
		__MachMsgErrorWithoutTimeout(msg_result);
		{ return msg_result; }
	}


#if	defined(__MIG_check__Reply__rpc_jack_port_disconnect_name_t__defined)
	check_result = __MIG_check__Reply__rpc_jack_port_disconnect_name_t((__Reply__rpc_jack_port_disconnect_name_t *)Out0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ return check_result; }
#endif	/* defined(__MIG_check__Reply__rpc_jack_port_disconnect_name_t__defined) */

	*result = Out0P->result;

	return KERN_SUCCESS;
    }
}

#if ( __MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Reply__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Reply__rpc_jack_set_buffer_size_t__defined)
#define __MIG_check__Reply__rpc_jack_set_buffer_size_t__defined
#ifndef __NDR_convert__int_rep__Reply__rpc_jack_set_buffer_size_t__RetCode__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_buffer_size_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_buffer_size_t__RetCode(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__kern_return_t((kern_return_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_buffer_size_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_buffer_size_t__RetCode(a, f) \
	__NDR_convert__int_rep__kern_return_t((kern_return_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_set_buffer_size_t__RetCode__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_set_buffer_size_t__result__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_buffer_size_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_buffer_size_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_buffer_size_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_buffer_size_t__result(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_buffer_size_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_buffer_size_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_buffer_size_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_buffer_size_t__result(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_set_buffer_size_t__result__defined */



#ifndef __NDR_convert__char_rep__Reply__rpc_jack_set_buffer_size_t__result__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_set_buffer_size_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_set_buffer_size_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_set_buffer_size_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_set_buffer_size_t__result(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_set_buffer_size_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_set_buffer_size_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_set_buffer_size_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_set_buffer_size_t__result(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_set_buffer_size_t__result__defined */



#ifndef __NDR_convert__float_rep__Reply__rpc_jack_set_buffer_size_t__result__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_set_buffer_size_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_set_buffer_size_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_set_buffer_size_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_set_buffer_size_t__result(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_set_buffer_size_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_set_buffer_size_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_set_buffer_size_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_set_buffer_size_t__result(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_set_buffer_size_t__result__defined */



mig_internal kern_return_t __MIG_check__Reply__rpc_jack_set_buffer_size_t(__Reply__rpc_jack_set_buffer_size_t *Out0P)
{

	typedef __Reply__rpc_jack_set_buffer_size_t __Reply;
#if	__MigTypeCheck
	unsigned int msgh_size;
#endif	/* __MigTypeCheck */
	if (Out0P->Head.msgh_id != 1111) {
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

#if	defined(__NDR_convert__int_rep__Reply__rpc_jack_set_buffer_size_t__RetCode__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_set_buffer_size_t__result__defined)
	if (Out0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_set_buffer_size_t__RetCode__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_set_buffer_size_t__RetCode(&Out0P->RetCode, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_set_buffer_size_t__RetCode__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_set_buffer_size_t__result__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_set_buffer_size_t__result(&Out0P->result, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_set_buffer_size_t__result__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	0 || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_set_buffer_size_t__result__defined)
	if (Out0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_set_buffer_size_t__result__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_set_buffer_size_t__result(&Out0P->result, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_set_buffer_size_t__result__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	0 || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_set_buffer_size_t__result__defined)
	if (Out0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_set_buffer_size_t__result__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_set_buffer_size_t__result(&Out0P->result, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_set_buffer_size_t__result__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Reply__rpc_jack_set_buffer_size_t__defined) */
#endif /* __MIG_check__Reply__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_set_buffer_size */
mig_external kern_return_t rpc_jack_set_buffer_size
(
	mach_port_t server_port,
	int buffer_size,
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
		int buffer_size;
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

#ifdef	__MIG_check__Reply__rpc_jack_set_buffer_size_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Reply__rpc_jack_set_buffer_size_t__defined */

	__DeclareSendRpc(1011, "rpc_jack_set_buffer_size")

	InP->NDR = NDR_record;

	InP->buffer_size = buffer_size;

	InP->Head.msgh_bits =
		MACH_MSGH_BITS(19, MACH_MSG_TYPE_MAKE_SEND_ONCE);
	/* msgh_size passed as argument */
	InP->Head.msgh_request_port = server_port;
	InP->Head.msgh_reply_port = mig_get_reply_port();
	InP->Head.msgh_id = 1011;

	__BeforeSendRpc(1011, "rpc_jack_set_buffer_size")
	msg_result = mach_msg(&InP->Head, MACH_SEND_MSG|MACH_RCV_MSG|MACH_MSG_OPTION_NONE, (mach_msg_size_t)sizeof(Request), (mach_msg_size_t)sizeof(Reply), InP->Head.msgh_reply_port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	__AfterSendRpc(1011, "rpc_jack_set_buffer_size")
	if (msg_result != MACH_MSG_SUCCESS) {
		__MachMsgErrorWithoutTimeout(msg_result);
		{ return msg_result; }
	}


#if	defined(__MIG_check__Reply__rpc_jack_set_buffer_size_t__defined)
	check_result = __MIG_check__Reply__rpc_jack_set_buffer_size_t((__Reply__rpc_jack_set_buffer_size_t *)Out0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ return check_result; }
#endif	/* defined(__MIG_check__Reply__rpc_jack_set_buffer_size_t__defined) */

	*result = Out0P->result;

	return KERN_SUCCESS;
    }
}

#if ( __MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Reply__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Reply__rpc_jack_set_freewheel_t__defined)
#define __MIG_check__Reply__rpc_jack_set_freewheel_t__defined
#ifndef __NDR_convert__int_rep__Reply__rpc_jack_set_freewheel_t__RetCode__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_freewheel_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_freewheel_t__RetCode(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__kern_return_t((kern_return_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_freewheel_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_freewheel_t__RetCode(a, f) \
	__NDR_convert__int_rep__kern_return_t((kern_return_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_set_freewheel_t__RetCode__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_set_freewheel_t__result__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_freewheel_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_freewheel_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_freewheel_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_freewheel_t__result(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_freewheel_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_freewheel_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_freewheel_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_freewheel_t__result(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_set_freewheel_t__result__defined */



#ifndef __NDR_convert__char_rep__Reply__rpc_jack_set_freewheel_t__result__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_set_freewheel_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_set_freewheel_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_set_freewheel_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_set_freewheel_t__result(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_set_freewheel_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_set_freewheel_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_set_freewheel_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_set_freewheel_t__result(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_set_freewheel_t__result__defined */



#ifndef __NDR_convert__float_rep__Reply__rpc_jack_set_freewheel_t__result__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_set_freewheel_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_set_freewheel_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_set_freewheel_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_set_freewheel_t__result(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_set_freewheel_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_set_freewheel_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_set_freewheel_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_set_freewheel_t__result(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_set_freewheel_t__result__defined */



mig_internal kern_return_t __MIG_check__Reply__rpc_jack_set_freewheel_t(__Reply__rpc_jack_set_freewheel_t *Out0P)
{

	typedef __Reply__rpc_jack_set_freewheel_t __Reply;
#if	__MigTypeCheck
	unsigned int msgh_size;
#endif	/* __MigTypeCheck */
	if (Out0P->Head.msgh_id != 1112) {
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

#if	defined(__NDR_convert__int_rep__Reply__rpc_jack_set_freewheel_t__RetCode__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_set_freewheel_t__result__defined)
	if (Out0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_set_freewheel_t__RetCode__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_set_freewheel_t__RetCode(&Out0P->RetCode, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_set_freewheel_t__RetCode__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_set_freewheel_t__result__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_set_freewheel_t__result(&Out0P->result, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_set_freewheel_t__result__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	0 || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_set_freewheel_t__result__defined)
	if (Out0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_set_freewheel_t__result__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_set_freewheel_t__result(&Out0P->result, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_set_freewheel_t__result__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	0 || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_set_freewheel_t__result__defined)
	if (Out0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_set_freewheel_t__result__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_set_freewheel_t__result(&Out0P->result, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_set_freewheel_t__result__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Reply__rpc_jack_set_freewheel_t__defined) */
#endif /* __MIG_check__Reply__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_set_freewheel */
mig_external kern_return_t rpc_jack_set_freewheel
(
	mach_port_t server_port,
	int onoff,
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
		int onoff;
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

#ifdef	__MIG_check__Reply__rpc_jack_set_freewheel_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Reply__rpc_jack_set_freewheel_t__defined */

	__DeclareSendRpc(1012, "rpc_jack_set_freewheel")

	InP->NDR = NDR_record;

	InP->onoff = onoff;

	InP->Head.msgh_bits =
		MACH_MSGH_BITS(19, MACH_MSG_TYPE_MAKE_SEND_ONCE);
	/* msgh_size passed as argument */
	InP->Head.msgh_request_port = server_port;
	InP->Head.msgh_reply_port = mig_get_reply_port();
	InP->Head.msgh_id = 1012;

	__BeforeSendRpc(1012, "rpc_jack_set_freewheel")
	msg_result = mach_msg(&InP->Head, MACH_SEND_MSG|MACH_RCV_MSG|MACH_MSG_OPTION_NONE, (mach_msg_size_t)sizeof(Request), (mach_msg_size_t)sizeof(Reply), InP->Head.msgh_reply_port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	__AfterSendRpc(1012, "rpc_jack_set_freewheel")
	if (msg_result != MACH_MSG_SUCCESS) {
		__MachMsgErrorWithoutTimeout(msg_result);
		{ return msg_result; }
	}


#if	defined(__MIG_check__Reply__rpc_jack_set_freewheel_t__defined)
	check_result = __MIG_check__Reply__rpc_jack_set_freewheel_t((__Reply__rpc_jack_set_freewheel_t *)Out0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ return check_result; }
#endif	/* defined(__MIG_check__Reply__rpc_jack_set_freewheel_t__defined) */

	*result = Out0P->result;

	return KERN_SUCCESS;
    }
}

#if ( __MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Reply__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Reply__rpc_jack_release_timebase_t__defined)
#define __MIG_check__Reply__rpc_jack_release_timebase_t__defined
#ifndef __NDR_convert__int_rep__Reply__rpc_jack_release_timebase_t__RetCode__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_release_timebase_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_release_timebase_t__RetCode(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__kern_return_t((kern_return_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_release_timebase_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_release_timebase_t__RetCode(a, f) \
	__NDR_convert__int_rep__kern_return_t((kern_return_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_release_timebase_t__RetCode__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_release_timebase_t__result__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_release_timebase_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_release_timebase_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_release_timebase_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_release_timebase_t__result(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_release_timebase_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_release_timebase_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_release_timebase_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_release_timebase_t__result(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_release_timebase_t__result__defined */



#ifndef __NDR_convert__char_rep__Reply__rpc_jack_release_timebase_t__result__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_release_timebase_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_release_timebase_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_release_timebase_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_release_timebase_t__result(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_release_timebase_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_release_timebase_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_release_timebase_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_release_timebase_t__result(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_release_timebase_t__result__defined */



#ifndef __NDR_convert__float_rep__Reply__rpc_jack_release_timebase_t__result__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_release_timebase_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_release_timebase_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_release_timebase_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_release_timebase_t__result(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_release_timebase_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_release_timebase_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_release_timebase_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_release_timebase_t__result(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_release_timebase_t__result__defined */



mig_internal kern_return_t __MIG_check__Reply__rpc_jack_release_timebase_t(__Reply__rpc_jack_release_timebase_t *Out0P)
{

	typedef __Reply__rpc_jack_release_timebase_t __Reply;
#if	__MigTypeCheck
	unsigned int msgh_size;
#endif	/* __MigTypeCheck */
	if (Out0P->Head.msgh_id != 1113) {
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

#if	defined(__NDR_convert__int_rep__Reply__rpc_jack_release_timebase_t__RetCode__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_release_timebase_t__result__defined)
	if (Out0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_release_timebase_t__RetCode__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_release_timebase_t__RetCode(&Out0P->RetCode, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_release_timebase_t__RetCode__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_release_timebase_t__result__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_release_timebase_t__result(&Out0P->result, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_release_timebase_t__result__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	0 || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_release_timebase_t__result__defined)
	if (Out0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_release_timebase_t__result__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_release_timebase_t__result(&Out0P->result, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_release_timebase_t__result__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	0 || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_release_timebase_t__result__defined)
	if (Out0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_release_timebase_t__result__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_release_timebase_t__result(&Out0P->result, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_release_timebase_t__result__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Reply__rpc_jack_release_timebase_t__defined) */
#endif /* __MIG_check__Reply__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_release_timebase */
mig_external kern_return_t rpc_jack_release_timebase
(
	mach_port_t server_port,
	int refnum,
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

#ifdef	__MIG_check__Reply__rpc_jack_release_timebase_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Reply__rpc_jack_release_timebase_t__defined */

	__DeclareSendRpc(1013, "rpc_jack_release_timebase")

	InP->NDR = NDR_record;

	InP->refnum = refnum;

	InP->Head.msgh_bits =
		MACH_MSGH_BITS(19, MACH_MSG_TYPE_MAKE_SEND_ONCE);
	/* msgh_size passed as argument */
	InP->Head.msgh_request_port = server_port;
	InP->Head.msgh_reply_port = mig_get_reply_port();
	InP->Head.msgh_id = 1013;

	__BeforeSendRpc(1013, "rpc_jack_release_timebase")
	msg_result = mach_msg(&InP->Head, MACH_SEND_MSG|MACH_RCV_MSG|MACH_MSG_OPTION_NONE, (mach_msg_size_t)sizeof(Request), (mach_msg_size_t)sizeof(Reply), InP->Head.msgh_reply_port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	__AfterSendRpc(1013, "rpc_jack_release_timebase")
	if (msg_result != MACH_MSG_SUCCESS) {
		__MachMsgErrorWithoutTimeout(msg_result);
		{ return msg_result; }
	}


#if	defined(__MIG_check__Reply__rpc_jack_release_timebase_t__defined)
	check_result = __MIG_check__Reply__rpc_jack_release_timebase_t((__Reply__rpc_jack_release_timebase_t *)Out0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ return check_result; }
#endif	/* defined(__MIG_check__Reply__rpc_jack_release_timebase_t__defined) */

	*result = Out0P->result;

	return KERN_SUCCESS;
    }
}

#if ( __MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Reply__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Reply__rpc_jack_set_timebase_callback_t__defined)
#define __MIG_check__Reply__rpc_jack_set_timebase_callback_t__defined
#ifndef __NDR_convert__int_rep__Reply__rpc_jack_set_timebase_callback_t__RetCode__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_timebase_callback_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_timebase_callback_t__RetCode(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__kern_return_t((kern_return_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_timebase_callback_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_timebase_callback_t__RetCode(a, f) \
	__NDR_convert__int_rep__kern_return_t((kern_return_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_set_timebase_callback_t__RetCode__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_set_timebase_callback_t__result__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_timebase_callback_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_timebase_callback_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_timebase_callback_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_timebase_callback_t__result(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_timebase_callback_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_timebase_callback_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_timebase_callback_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_set_timebase_callback_t__result(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_set_timebase_callback_t__result__defined */



#ifndef __NDR_convert__char_rep__Reply__rpc_jack_set_timebase_callback_t__result__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_set_timebase_callback_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_set_timebase_callback_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_set_timebase_callback_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_set_timebase_callback_t__result(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_set_timebase_callback_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_set_timebase_callback_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_set_timebase_callback_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_set_timebase_callback_t__result(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_set_timebase_callback_t__result__defined */



#ifndef __NDR_convert__float_rep__Reply__rpc_jack_set_timebase_callback_t__result__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_set_timebase_callback_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_set_timebase_callback_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_set_timebase_callback_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_set_timebase_callback_t__result(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_set_timebase_callback_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_set_timebase_callback_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_set_timebase_callback_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_set_timebase_callback_t__result(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_set_timebase_callback_t__result__defined */



mig_internal kern_return_t __MIG_check__Reply__rpc_jack_set_timebase_callback_t(__Reply__rpc_jack_set_timebase_callback_t *Out0P)
{

	typedef __Reply__rpc_jack_set_timebase_callback_t __Reply;
#if	__MigTypeCheck
	unsigned int msgh_size;
#endif	/* __MigTypeCheck */
	if (Out0P->Head.msgh_id != 1114) {
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

#if	defined(__NDR_convert__int_rep__Reply__rpc_jack_set_timebase_callback_t__RetCode__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_set_timebase_callback_t__result__defined)
	if (Out0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_set_timebase_callback_t__RetCode__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_set_timebase_callback_t__RetCode(&Out0P->RetCode, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_set_timebase_callback_t__RetCode__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_set_timebase_callback_t__result__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_set_timebase_callback_t__result(&Out0P->result, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_set_timebase_callback_t__result__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	0 || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_set_timebase_callback_t__result__defined)
	if (Out0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_set_timebase_callback_t__result__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_set_timebase_callback_t__result(&Out0P->result, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_set_timebase_callback_t__result__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	0 || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_set_timebase_callback_t__result__defined)
	if (Out0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_set_timebase_callback_t__result__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_set_timebase_callback_t__result(&Out0P->result, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_set_timebase_callback_t__result__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Reply__rpc_jack_set_timebase_callback_t__defined) */
#endif /* __MIG_check__Reply__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_set_timebase_callback */
mig_external kern_return_t rpc_jack_set_timebase_callback
(
	mach_port_t server_port,
	int refnum,
	int conditional,
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
		int conditional;
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

#ifdef	__MIG_check__Reply__rpc_jack_set_timebase_callback_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Reply__rpc_jack_set_timebase_callback_t__defined */

	__DeclareSendRpc(1014, "rpc_jack_set_timebase_callback")

	InP->NDR = NDR_record;

	InP->refnum = refnum;

	InP->conditional = conditional;

	InP->Head.msgh_bits =
		MACH_MSGH_BITS(19, MACH_MSG_TYPE_MAKE_SEND_ONCE);
	/* msgh_size passed as argument */
	InP->Head.msgh_request_port = server_port;
	InP->Head.msgh_reply_port = mig_get_reply_port();
	InP->Head.msgh_id = 1014;

	__BeforeSendRpc(1014, "rpc_jack_set_timebase_callback")
	msg_result = mach_msg(&InP->Head, MACH_SEND_MSG|MACH_RCV_MSG|MACH_MSG_OPTION_NONE, (mach_msg_size_t)sizeof(Request), (mach_msg_size_t)sizeof(Reply), InP->Head.msgh_reply_port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	__AfterSendRpc(1014, "rpc_jack_set_timebase_callback")
	if (msg_result != MACH_MSG_SUCCESS) {
		__MachMsgErrorWithoutTimeout(msg_result);
		{ return msg_result; }
	}


#if	defined(__MIG_check__Reply__rpc_jack_set_timebase_callback_t__defined)
	check_result = __MIG_check__Reply__rpc_jack_set_timebase_callback_t((__Reply__rpc_jack_set_timebase_callback_t *)Out0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ return check_result; }
#endif	/* defined(__MIG_check__Reply__rpc_jack_set_timebase_callback_t__defined) */

	*result = Out0P->result;

	return KERN_SUCCESS;
    }
}

#if ( __MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Reply__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Reply__rpc_jack_get_internal_clientname_t__defined)
#define __MIG_check__Reply__rpc_jack_get_internal_clientname_t__defined
#ifndef __NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__RetCode__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__RetCode(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__kern_return_t((kern_return_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__RetCode(a, f) \
	__NDR_convert__int_rep__kern_return_t((kern_return_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__RetCode__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__client_name_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__client_name_t((client_name_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__client_name_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res(a, f) \
	__NDR_convert__int_rep__client_name_t((client_name_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__string__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__string(a, f, 128)
#elif	defined(__NDR_convert__int_rep__string__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res(a, f) \
	__NDR_convert__int_rep__string(a, f, 128)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__result__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__result(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__result(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__result__defined */



#ifndef __NDR_convert__char_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__client_name_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__client_name_t((client_name_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__client_name_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res(a, f) \
	__NDR_convert__char_rep__client_name_t((client_name_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__string__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__string(a, f, 128)
#elif	defined(__NDR_convert__char_rep__string__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res(a, f) \
	__NDR_convert__char_rep__string(a, f, 128)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res__defined */


#ifndef __NDR_convert__char_rep__Reply__rpc_jack_get_internal_clientname_t__result__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_get_internal_clientname_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_get_internal_clientname_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_get_internal_clientname_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_get_internal_clientname_t__result(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_get_internal_clientname_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_get_internal_clientname_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_get_internal_clientname_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_get_internal_clientname_t__result(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_get_internal_clientname_t__result__defined */



#ifndef __NDR_convert__float_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__client_name_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__client_name_t((client_name_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__client_name_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res(a, f) \
	__NDR_convert__float_rep__client_name_t((client_name_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__string__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__string(a, f, 128)
#elif	defined(__NDR_convert__float_rep__string__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res(a, f) \
	__NDR_convert__float_rep__string(a, f, 128)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res__defined */


#ifndef __NDR_convert__float_rep__Reply__rpc_jack_get_internal_clientname_t__result__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_get_internal_clientname_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_get_internal_clientname_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_get_internal_clientname_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_get_internal_clientname_t__result(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_get_internal_clientname_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_get_internal_clientname_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_get_internal_clientname_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_get_internal_clientname_t__result(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_get_internal_clientname_t__result__defined */



mig_internal kern_return_t __MIG_check__Reply__rpc_jack_get_internal_clientname_t(__Reply__rpc_jack_get_internal_clientname_t *Out0P)
{

	typedef __Reply__rpc_jack_get_internal_clientname_t __Reply;
#if	__MigTypeCheck
	unsigned int msgh_size;
#endif	/* __MigTypeCheck */
	if (Out0P->Head.msgh_id != 1115) {
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

#if	defined(__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__RetCode__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__result__defined)
	if (Out0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__RetCode__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__RetCode(&Out0P->RetCode, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__RetCode__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res(&Out0P->client_name_res, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__result__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__result(&Out0P->result, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_get_internal_clientname_t__result__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	0 || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res__defined) || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_get_internal_clientname_t__result__defined)
	if (Out0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res(&Out0P->client_name_res, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res__defined */
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_get_internal_clientname_t__result__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_get_internal_clientname_t__result(&Out0P->result, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_get_internal_clientname_t__result__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	0 || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res__defined) || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_get_internal_clientname_t__result__defined)
	if (Out0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res(&Out0P->client_name_res, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_get_internal_clientname_t__client_name_res__defined */
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_get_internal_clientname_t__result__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_get_internal_clientname_t__result(&Out0P->result, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_get_internal_clientname_t__result__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Reply__rpc_jack_get_internal_clientname_t__defined) */
#endif /* __MIG_check__Reply__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_get_internal_clientname */
mig_external kern_return_t rpc_jack_get_internal_clientname
(
	mach_port_t server_port,
	int refnum,
	int int_ref,
	client_name_t client_name_res,
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
		int int_ref;
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
		client_name_t client_name_res;
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
		client_name_t client_name_res;
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

#ifdef	__MIG_check__Reply__rpc_jack_get_internal_clientname_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Reply__rpc_jack_get_internal_clientname_t__defined */

	__DeclareSendRpc(1015, "rpc_jack_get_internal_clientname")

	InP->NDR = NDR_record;

	InP->refnum = refnum;

	InP->int_ref = int_ref;

	InP->Head.msgh_bits =
		MACH_MSGH_BITS(19, MACH_MSG_TYPE_MAKE_SEND_ONCE);
	/* msgh_size passed as argument */
	InP->Head.msgh_request_port = server_port;
	InP->Head.msgh_reply_port = mig_get_reply_port();
	InP->Head.msgh_id = 1015;

	__BeforeSendRpc(1015, "rpc_jack_get_internal_clientname")
	msg_result = mach_msg(&InP->Head, MACH_SEND_MSG|MACH_RCV_MSG|MACH_MSG_OPTION_NONE, (mach_msg_size_t)sizeof(Request), (mach_msg_size_t)sizeof(Reply), InP->Head.msgh_reply_port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	__AfterSendRpc(1015, "rpc_jack_get_internal_clientname")
	if (msg_result != MACH_MSG_SUCCESS) {
		__MachMsgErrorWithoutTimeout(msg_result);
		{ return msg_result; }
	}


#if	defined(__MIG_check__Reply__rpc_jack_get_internal_clientname_t__defined)
	check_result = __MIG_check__Reply__rpc_jack_get_internal_clientname_t((__Reply__rpc_jack_get_internal_clientname_t *)Out0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ return check_result; }
#endif	/* defined(__MIG_check__Reply__rpc_jack_get_internal_clientname_t__defined) */

	(void) mig_strncpy(client_name_res, Out0P->client_name_res, 128);

	*result = Out0P->result;

	return KERN_SUCCESS;
    }
}

#if ( __MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Reply__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Reply__rpc_jack_internal_clienthandle_t__defined)
#define __MIG_check__Reply__rpc_jack_internal_clienthandle_t__defined
#ifndef __NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__RetCode__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__RetCode(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__kern_return_t((kern_return_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__RetCode(a, f) \
	__NDR_convert__int_rep__kern_return_t((kern_return_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__RetCode__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__status__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__status__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__status(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__status__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__status(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__status__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__status(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__status__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__status(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__status__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__result__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__result(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__result(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__result__defined */



#ifndef __NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref__defined */


#ifndef __NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__status__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__status__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__status(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__status__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__status(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__status__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__status(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__status__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__status(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__status__defined */


#ifndef __NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__result__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__result(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__result(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__result__defined */



#ifndef __NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref__defined */


#ifndef __NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__status__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__status__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__status(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__status__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__status(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__status__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__status(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__status__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__status(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__status__defined */


#ifndef __NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__result__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__result(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__result(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__result__defined */



mig_internal kern_return_t __MIG_check__Reply__rpc_jack_internal_clienthandle_t(__Reply__rpc_jack_internal_clienthandle_t *Out0P)
{

	typedef __Reply__rpc_jack_internal_clienthandle_t __Reply;
#if	__MigTypeCheck
	unsigned int msgh_size;
#endif	/* __MigTypeCheck */
	if (Out0P->Head.msgh_id != 1116) {
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

#if	defined(__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__RetCode__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__status__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__result__defined)
	if (Out0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__RetCode__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__RetCode(&Out0P->RetCode, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__RetCode__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref(&Out0P->int_ref, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__status__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__status(&Out0P->status, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__status__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__result__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__result(&Out0P->result, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_internal_clienthandle_t__result__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	0 || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref__defined) || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__status__defined) || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__result__defined)
	if (Out0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref(&Out0P->int_ref, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref__defined */
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__status__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__status(&Out0P->status, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__status__defined */
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__result__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__result(&Out0P->result, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_internal_clienthandle_t__result__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	0 || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref__defined) || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__status__defined) || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__result__defined)
	if (Out0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref(&Out0P->int_ref, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__int_ref__defined */
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__status__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__status(&Out0P->status, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__status__defined */
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__result__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__result(&Out0P->result, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_internal_clienthandle_t__result__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Reply__rpc_jack_internal_clienthandle_t__defined) */
#endif /* __MIG_check__Reply__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_internal_clienthandle */
mig_external kern_return_t rpc_jack_internal_clienthandle
(
	mach_port_t server_port,
	int refnum,
	client_name_t client_name,
	int *int_ref,
	int *status,
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
		int int_ref;
		int status;
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
		int int_ref;
		int status;
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

#ifdef	__MIG_check__Reply__rpc_jack_internal_clienthandle_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Reply__rpc_jack_internal_clienthandle_t__defined */

	__DeclareSendRpc(1016, "rpc_jack_internal_clienthandle")

	InP->NDR = NDR_record;

	InP->refnum = refnum;

	(void) mig_strncpy(InP->client_name, client_name, 128);

	InP->Head.msgh_bits =
		MACH_MSGH_BITS(19, MACH_MSG_TYPE_MAKE_SEND_ONCE);
	/* msgh_size passed as argument */
	InP->Head.msgh_request_port = server_port;
	InP->Head.msgh_reply_port = mig_get_reply_port();
	InP->Head.msgh_id = 1016;

	__BeforeSendRpc(1016, "rpc_jack_internal_clienthandle")
	msg_result = mach_msg(&InP->Head, MACH_SEND_MSG|MACH_RCV_MSG|MACH_MSG_OPTION_NONE, (mach_msg_size_t)sizeof(Request), (mach_msg_size_t)sizeof(Reply), InP->Head.msgh_reply_port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	__AfterSendRpc(1016, "rpc_jack_internal_clienthandle")
	if (msg_result != MACH_MSG_SUCCESS) {
		__MachMsgErrorWithoutTimeout(msg_result);
		{ return msg_result; }
	}


#if	defined(__MIG_check__Reply__rpc_jack_internal_clienthandle_t__defined)
	check_result = __MIG_check__Reply__rpc_jack_internal_clienthandle_t((__Reply__rpc_jack_internal_clienthandle_t *)Out0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ return check_result; }
#endif	/* defined(__MIG_check__Reply__rpc_jack_internal_clienthandle_t__defined) */

	*int_ref = Out0P->int_ref;

	*status = Out0P->status;

	*result = Out0P->result;

	return KERN_SUCCESS;
    }
}

#if ( __MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Reply__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Reply__rpc_jack_internal_clientload_t__defined)
#define __MIG_check__Reply__rpc_jack_internal_clientload_t__defined
#ifndef __NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__RetCode__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__RetCode(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__kern_return_t((kern_return_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__RetCode(a, f) \
	__NDR_convert__int_rep__kern_return_t((kern_return_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__RetCode__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__status__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__status__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__status(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__status__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__status(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__status__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__status(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__status__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__status(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__status__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__int_ref__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__int_ref__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__int_ref(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__int_ref__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__int_ref(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__int_ref__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__int_ref(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__int_ref__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__int_ref(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__int_ref__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__result__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__result(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__result(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__result__defined */



#ifndef __NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__status__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__status__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__status(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__status__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__status(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__status__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__status(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__status__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__status(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__status__defined */


#ifndef __NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__int_ref__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__int_ref__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__int_ref(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__int_ref__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__int_ref(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__int_ref__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__int_ref(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__int_ref__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__int_ref(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__int_ref__defined */


#ifndef __NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__result__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__result(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__result(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__result__defined */



#ifndef __NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__status__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__status__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__status(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__status__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__status(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__status__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__status(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__status__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__status(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__status__defined */


#ifndef __NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__int_ref__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__int_ref__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__int_ref(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__int_ref__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__int_ref(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__int_ref__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__int_ref(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__int_ref__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__int_ref(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__int_ref__defined */


#ifndef __NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__result__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__result(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__result(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__result__defined */



mig_internal kern_return_t __MIG_check__Reply__rpc_jack_internal_clientload_t(__Reply__rpc_jack_internal_clientload_t *Out0P)
{

	typedef __Reply__rpc_jack_internal_clientload_t __Reply;
#if	__MigTypeCheck
	unsigned int msgh_size;
#endif	/* __MigTypeCheck */
	if (Out0P->Head.msgh_id != 1117) {
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

#if	defined(__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__RetCode__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__status__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__int_ref__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__result__defined)
	if (Out0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__RetCode__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__RetCode(&Out0P->RetCode, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__RetCode__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__status__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__status(&Out0P->status, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__status__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__int_ref__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__int_ref(&Out0P->int_ref, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__int_ref__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__result__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__result(&Out0P->result, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_internal_clientload_t__result__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	0 || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__status__defined) || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__int_ref__defined) || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__result__defined)
	if (Out0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__status__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__status(&Out0P->status, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__status__defined */
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__int_ref__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__int_ref(&Out0P->int_ref, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__int_ref__defined */
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__result__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__result(&Out0P->result, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_internal_clientload_t__result__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	0 || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__status__defined) || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__int_ref__defined) || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__result__defined)
	if (Out0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__status__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__status(&Out0P->status, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__status__defined */
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__int_ref__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__int_ref(&Out0P->int_ref, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__int_ref__defined */
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__result__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__result(&Out0P->result, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_internal_clientload_t__result__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Reply__rpc_jack_internal_clientload_t__defined) */
#endif /* __MIG_check__Reply__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_internal_clientload */
mig_external kern_return_t rpc_jack_internal_clientload
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
		so_name_t so_name;
		objet_data_t objet_data;
		int options;
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
		int status;
		int int_ref;
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
		int status;
		int int_ref;
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

#ifdef	__MIG_check__Reply__rpc_jack_internal_clientload_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Reply__rpc_jack_internal_clientload_t__defined */

	__DeclareSendRpc(1017, "rpc_jack_internal_clientload")

	InP->NDR = NDR_record;

	InP->refnum = refnum;

	(void) mig_strncpy(InP->client_name, client_name, 128);

	(void) mig_strncpy(InP->so_name, so_name, 1024);

	(void) mig_strncpy(InP->objet_data, objet_data, 1024);

	InP->options = options;

	InP->Head.msgh_bits =
		MACH_MSGH_BITS(19, MACH_MSG_TYPE_MAKE_SEND_ONCE);
	/* msgh_size passed as argument */
	InP->Head.msgh_request_port = server_port;
	InP->Head.msgh_reply_port = mig_get_reply_port();
	InP->Head.msgh_id = 1017;

	__BeforeSendRpc(1017, "rpc_jack_internal_clientload")
	msg_result = mach_msg(&InP->Head, MACH_SEND_MSG|MACH_RCV_MSG|MACH_MSG_OPTION_NONE, (mach_msg_size_t)sizeof(Request), (mach_msg_size_t)sizeof(Reply), InP->Head.msgh_reply_port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	__AfterSendRpc(1017, "rpc_jack_internal_clientload")
	if (msg_result != MACH_MSG_SUCCESS) {
		__MachMsgErrorWithoutTimeout(msg_result);
		{ return msg_result; }
	}


#if	defined(__MIG_check__Reply__rpc_jack_internal_clientload_t__defined)
	check_result = __MIG_check__Reply__rpc_jack_internal_clientload_t((__Reply__rpc_jack_internal_clientload_t *)Out0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ return check_result; }
#endif	/* defined(__MIG_check__Reply__rpc_jack_internal_clientload_t__defined) */

	*status = Out0P->status;

	*int_ref = Out0P->int_ref;

	*result = Out0P->result;

	return KERN_SUCCESS;
    }
}

#if ( __MigTypeCheck || __NDR_convert__ )
#if __MIG_check__Reply__JackRPCEngine_subsystem__
#if !defined(__MIG_check__Reply__rpc_jack_internal_clientunload_t__defined)
#define __MIG_check__Reply__rpc_jack_internal_clientunload_t__defined
#ifndef __NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__RetCode__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__RetCode(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__kern_return_t((kern_return_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__kern_return_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__RetCode__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__RetCode(a, f) \
	__NDR_convert__int_rep__kern_return_t((kern_return_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__RetCode__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__status__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__status__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__status(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__status__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__status(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__status__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__status(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__status__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__status(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__status__defined */


#ifndef __NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__result__defined
#if	defined(__NDR_convert__int_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__int__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__result(a, f) \
	__NDR_convert__int_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__int_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__result(a, f) \
	__NDR_convert__int_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__int_rep__int32_t__defined)
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__result__defined
#define	__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__result(a, f) \
	__NDR_convert__int_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__result__defined */



#ifndef __NDR_convert__char_rep__Reply__rpc_jack_internal_clientunload_t__status__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientunload_t__status__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientunload_t__status(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientunload_t__status__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientunload_t__status(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientunload_t__status__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientunload_t__status(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientunload_t__status__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientunload_t__status(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_internal_clientunload_t__status__defined */


#ifndef __NDR_convert__char_rep__Reply__rpc_jack_internal_clientunload_t__result__defined
#if	defined(__NDR_convert__char_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientunload_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientunload_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__int__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientunload_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientunload_t__result(a, f) \
	__NDR_convert__char_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__char_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientunload_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientunload_t__result(a, f) \
	__NDR_convert__char_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__char_rep__int32_t__defined)
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientunload_t__result__defined
#define	__NDR_convert__char_rep__Reply__rpc_jack_internal_clientunload_t__result(a, f) \
	__NDR_convert__char_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_internal_clientunload_t__result__defined */



#ifndef __NDR_convert__float_rep__Reply__rpc_jack_internal_clientunload_t__status__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientunload_t__status__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientunload_t__status(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientunload_t__status__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientunload_t__status(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientunload_t__status__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientunload_t__status(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientunload_t__status__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientunload_t__status(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_internal_clientunload_t__status__defined */


#ifndef __NDR_convert__float_rep__Reply__rpc_jack_internal_clientunload_t__result__defined
#if	defined(__NDR_convert__float_rep__JackRPCEngine__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientunload_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientunload_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__int__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientunload_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientunload_t__result(a, f) \
	__NDR_convert__float_rep__int((int *)(a), f)
#elif	defined(__NDR_convert__float_rep__JackRPCEngine__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientunload_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientunload_t__result(a, f) \
	__NDR_convert__float_rep__JackRPCEngine__int32_t((int32_t *)(a), f)
#elif	defined(__NDR_convert__float_rep__int32_t__defined)
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientunload_t__result__defined
#define	__NDR_convert__float_rep__Reply__rpc_jack_internal_clientunload_t__result(a, f) \
	__NDR_convert__float_rep__int32_t((int32_t *)(a), f)
#endif /* defined(__NDR_convert__*__defined) */
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_internal_clientunload_t__result__defined */



mig_internal kern_return_t __MIG_check__Reply__rpc_jack_internal_clientunload_t(__Reply__rpc_jack_internal_clientunload_t *Out0P)
{

	typedef __Reply__rpc_jack_internal_clientunload_t __Reply;
#if	__MigTypeCheck
	unsigned int msgh_size;
#endif	/* __MigTypeCheck */
	if (Out0P->Head.msgh_id != 1118) {
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

#if	defined(__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__RetCode__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__status__defined) || \
	defined(__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__result__defined)
	if (Out0P->NDR.int_rep != NDR_record.int_rep) {
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__RetCode__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__RetCode(&Out0P->RetCode, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__RetCode__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__status__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__status(&Out0P->status, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__status__defined */
#if defined(__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__result__defined)
		__NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__result(&Out0P->result, Out0P->NDR.int_rep);
#endif /* __NDR_convert__int_rep__Reply__rpc_jack_internal_clientunload_t__result__defined */
	}
#endif	/* defined(__NDR_convert__int_rep...) */

#if	0 || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_internal_clientunload_t__status__defined) || \
	defined(__NDR_convert__char_rep__Reply__rpc_jack_internal_clientunload_t__result__defined)
	if (Out0P->NDR.char_rep != NDR_record.char_rep) {
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_internal_clientunload_t__status__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_internal_clientunload_t__status(&Out0P->status, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_internal_clientunload_t__status__defined */
#if defined(__NDR_convert__char_rep__Reply__rpc_jack_internal_clientunload_t__result__defined)
		__NDR_convert__char_rep__Reply__rpc_jack_internal_clientunload_t__result(&Out0P->result, Out0P->NDR.char_rep);
#endif /* __NDR_convert__char_rep__Reply__rpc_jack_internal_clientunload_t__result__defined */
	}
#endif	/* defined(__NDR_convert__char_rep...) */

#if	0 || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_internal_clientunload_t__status__defined) || \
	defined(__NDR_convert__float_rep__Reply__rpc_jack_internal_clientunload_t__result__defined)
	if (Out0P->NDR.float_rep != NDR_record.float_rep) {
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_internal_clientunload_t__status__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_internal_clientunload_t__status(&Out0P->status, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_internal_clientunload_t__status__defined */
#if defined(__NDR_convert__float_rep__Reply__rpc_jack_internal_clientunload_t__result__defined)
		__NDR_convert__float_rep__Reply__rpc_jack_internal_clientunload_t__result(&Out0P->result, Out0P->NDR.float_rep);
#endif /* __NDR_convert__float_rep__Reply__rpc_jack_internal_clientunload_t__result__defined */
	}
#endif	/* defined(__NDR_convert__float_rep...) */

	return MACH_MSG_SUCCESS;
}
#endif /* !defined(__MIG_check__Reply__rpc_jack_internal_clientunload_t__defined) */
#endif /* __MIG_check__Reply__JackRPCEngine_subsystem__ */
#endif /* ( __MigTypeCheck || __NDR_convert__ ) */


/* Routine rpc_jack_internal_clientunload */
mig_external kern_return_t rpc_jack_internal_clientunload
(
	mach_port_t server_port,
	int refnum,
	int int_ref,
	int *status,
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
		int int_ref;
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
		int status;
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
		int status;
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

#ifdef	__MIG_check__Reply__rpc_jack_internal_clientunload_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Reply__rpc_jack_internal_clientunload_t__defined */

	__DeclareSendRpc(1018, "rpc_jack_internal_clientunload")

	InP->NDR = NDR_record;

	InP->refnum = refnum;

	InP->int_ref = int_ref;

	InP->Head.msgh_bits =
		MACH_MSGH_BITS(19, MACH_MSG_TYPE_MAKE_SEND_ONCE);
	/* msgh_size passed as argument */
	InP->Head.msgh_request_port = server_port;
	InP->Head.msgh_reply_port = mig_get_reply_port();
	InP->Head.msgh_id = 1018;

	__BeforeSendRpc(1018, "rpc_jack_internal_clientunload")
	msg_result = mach_msg(&InP->Head, MACH_SEND_MSG|MACH_RCV_MSG|MACH_MSG_OPTION_NONE, (mach_msg_size_t)sizeof(Request), (mach_msg_size_t)sizeof(Reply), InP->Head.msgh_reply_port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	__AfterSendRpc(1018, "rpc_jack_internal_clientunload")
	if (msg_result != MACH_MSG_SUCCESS) {
		__MachMsgErrorWithoutTimeout(msg_result);
		{ return msg_result; }
	}


#if	defined(__MIG_check__Reply__rpc_jack_internal_clientunload_t__defined)
	check_result = __MIG_check__Reply__rpc_jack_internal_clientunload_t((__Reply__rpc_jack_internal_clientunload_t *)Out0P);
	if (check_result != MACH_MSG_SUCCESS)
		{ return check_result; }
#endif	/* defined(__MIG_check__Reply__rpc_jack_internal_clientunload_t__defined) */

	*status = Out0P->status;

	*result = Out0P->result;

	return KERN_SUCCESS;
    }
}

/* SimpleRoutine rpc_jack_client_rt_notify */
mig_external kern_return_t rpc_jack_client_rt_notify
(
	mach_port_t client_port,
	int refnum,
	int notify,
	int value,
	int timeout
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

#ifdef	__MIG_check__Reply__rpc_jack_client_rt_notify_t__defined
	kern_return_t check_result;
#endif	/* __MIG_check__Reply__rpc_jack_client_rt_notify_t__defined */

	__DeclareSendSimple(1019, "rpc_jack_client_rt_notify")

	InP->NDR = NDR_record;

	InP->refnum = refnum;

	InP->notify = notify;

	InP->value = value;

	InP->Head.msgh_bits =
		MACH_MSGH_BITS(19, 0);
	/* msgh_size passed as argument */
	InP->Head.msgh_request_port = client_port;
	InP->Head.msgh_reply_port = MACH_PORT_NULL;
	InP->Head.msgh_id = 1019;

	__BeforeSendSimple(1019, "rpc_jack_client_rt_notify")
	msg_result = mach_msg(&InP->Head, MACH_SEND_MSG|MACH_SEND_TIMEOUT|MACH_MSG_OPTION_NONE, (mach_msg_size_t)sizeof(Request), 0, MACH_PORT_NULL, timeout, MACH_PORT_NULL);
	__AfterSendSimple(1019, "rpc_jack_client_rt_notify")
		return msg_result;
	return KERN_SUCCESS;
    }
}
