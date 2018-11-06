/*
Copyright (C) 2001 Paul Davis
Copyright (C) 2004-2008 Grame

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#ifndef __JackRequest__
#define __JackRequest__

#include "JackConstants.h"
#include "JackError.h"
#include "JackPlatformPlug.h"
#include "JackChannel.h"
#include "JackTime.h"
#include "types.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <list>

namespace Jack
{

#define CheckRes(exp) { if ((exp) < 0) { jack_error("CheckRes error"); return -1; } }
#define CheckSize() { CheckRes(trans->Read(&fSize, sizeof(int))); if (fSize != Size()) { jack_error("CheckSize error size = %d Size() = %d", fSize, Size()); return -1; } }

/*!
\brief Session API constants.
*/

enum JackSessionReply {

    kImmediateSessionReply = 1,
    kPendingSessionReply = 2

};

/*!
\brief Request from client to server.
*/

struct JackRequest
{

    enum RequestType {
        kRegisterPort = 1,
        kUnRegisterPort = 2,
        kConnectPorts = 3,
        kDisconnectPorts = 4,
        kSetTimeBaseClient = 5,
        kActivateClient = 6,
        kDeactivateClient = 7,
        kDisconnectPort = 8,
        kSetClientCapabilities = 9,
        kGetPortConnections = 10,
        kGetPortNConnections = 11,
        kReleaseTimebase = 12,
        kSetTimebaseCallback = 13,
        kSetBufferSize = 20,
        kSetFreeWheel = 21,
        kClientCheck = 22,
        kClientOpen = 23,
        kClientClose = 24,
        kConnectNamePorts = 25,
        kDisconnectNamePorts = 26,
        kGetInternalClientName = 27,
        kInternalClientHandle = 28,
        kInternalClientLoad = 29,
        kInternalClientUnload = 30,
        kPortRename = 31,
        kNotification = 32,
        kSessionNotify = 33,
        kSessionReply  = 34,
        kGetClientByUUID = 35,
        kReserveClientName = 36,
        kGetUUIDByClient = 37,
        kClientHasSessionCallback = 38,
        kComputeTotalLatencies = 39,
        kPropertyChangeNotify = 40
    };

    static int ReadType(detail::JackChannelTransactionInterface* trans, RequestType& type)
    {
        type = (RequestType)0;
        CheckRes(trans->Read(&type, sizeof(RequestType)));
        return 0;
    }

    virtual int Read(detail::JackChannelTransactionInterface* trans) = 0;
    virtual int Write(detail::JackChannelTransactionInterface* trans) = 0;
    virtual RequestType getType() = 0;

};


PRE_PACKED_STRUCTURE_ALWAYS
template<class DATA>
struct JackRequestMessage
{
protected:
    const JackRequest::RequestType fType;
    const int fSize;
public:
    DATA d;
protected:
    template<typename... Args>
        JackRequestMessage(Args&... args) : fType(DATA::Type()),
            fSize(sizeof(JackRequestMessage<DATA>) - sizeof(fType) - sizeof(fSize)),
            d(args...)
        {}

        int ReadMessage(detail::JackChannelTransactionInterface* trans)
        {
            int size = 0;
            CheckRes(trans->Read(&size, sizeof(fSize)));
            if (size != fSize) {
                jack_error("CheckSize error expected %d actual %d", fSize, size);
                return -1;
            }

            CheckRes(trans->Read(&d, fSize));
            return 0;
        }

        int WriteMessage(detail::JackChannelTransactionInterface* trans)
        {
            CheckRes(trans->Write(this, sizeof(JackRequestMessage<DATA>)));
            return 0;
        }
} POST_PACKED_STRUCTURE_ALWAYS;


template<class DATA>
struct JackRequestTemplate : public JackRequest, public JackRequestMessage<DATA>
{
    template<typename... Args>
        JackRequestTemplate(Args&... args) : JackRequestMessage<DATA>(args...)
        {}

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        return JackRequestMessage<DATA>::ReadMessage(trans);
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        return JackRequestMessage<DATA>::WriteMessage(trans);
    }

    JackRequest::RequestType getType()
    {
        return JackRequestMessage<DATA>::fType;
    }
};

/*!
\brief Result from the server.
*/

struct JackResult
{

    int fResult;

    JackResult(): fResult( -1)
    {}
    JackResult(int result): fResult(result)
    {}
    virtual ~JackResult()
    {}

    virtual int Read(detail::JackChannelTransactionInterface* trans)
    {
        return trans->Read(&fResult, sizeof(int));
    }

    virtual int Write(detail::JackChannelTransactionInterface* trans)
    {
        return trans->Write(&fResult, sizeof(int));
    }

};

/*!
\brief CheckClient request.
*/

PRE_PACKED_STRUCTURE_ALWAYS
struct JackClientCheckRequestData
{

    char fName[JACK_CLIENT_NAME_SIZE+1];
    int fProtocol;
    int fOptions;
    int fOpen;
    jack_uuid_t fUUID;

    JackClientCheckRequestData(const char* name="", int protocol=0, int options=0, jack_uuid_t uuid=JACK_UUID_EMPTY_INITIALIZER, int open = false)
        : fProtocol(protocol), fOptions(options), fOpen(open), fUUID(uuid)
    {
        memset(fName, 0, sizeof(fName));
        snprintf(fName, sizeof(fName), "%s", name);
    }

    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kClientCheck;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackClientCheckRequestData> JackClientCheckRequest;

/*!
\brief CheckClient result.
*/

struct JackClientCheckResult : public JackResult
{

    char fName[JACK_CLIENT_NAME_SIZE+1];
    int fStatus;

    JackClientCheckResult(): JackResult(), fStatus(0)
    {
        memset(fName, 0, sizeof(fName));
    }
    JackClientCheckResult(int32_t result, const char* name, int status)
            : JackResult(result), fStatus(status)
    {
        memset(fName, 0, sizeof(fName));
        snprintf(fName, sizeof(fName), "%s", name);
    }

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackResult::Read(trans));
        CheckRes(trans->Read(&fName, sizeof(fName)));
        CheckRes(trans->Read(&fStatus, sizeof(int)));
        return 0;
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackResult::Write(trans));
        CheckRes(trans->Write(&fName, sizeof(fName)));
        CheckRes(trans->Write(&fStatus, sizeof(int)));
        return 0;
    }

};

/*!
\brief NewClient request.
*/

PRE_PACKED_STRUCTURE_ALWAYS
struct JackClientOpenRequestData
{

    int fPID;
    jack_uuid_t fUUID;
    char fName[JACK_CLIENT_NAME_SIZE+1];

    JackClientOpenRequestData(const char* name="", int pid=0, jack_uuid_t uuid=JACK_UUID_EMPTY_INITIALIZER)
    {
        memset(fName, 0, sizeof(fName));
        snprintf(fName, sizeof(fName), "%s", name);
        fPID = pid;
        fUUID = uuid;
    }

    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kClientOpen;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackClientOpenRequestData> JackClientOpenRequest;

/*!
\brief NewClient result.
*/

struct JackClientOpenResult : public JackResult
{

    int fSharedEngine;
    int fSharedClient;
    int fSharedGraph;

    JackClientOpenResult()
            : JackResult(), fSharedEngine(-1), fSharedClient(-1), fSharedGraph(-1)
    {}
    JackClientOpenResult(int32_t result, int index1, int index2, int index3)
            : JackResult(result), fSharedEngine(index1), fSharedClient(index2), fSharedGraph(index3)
    {}

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackResult::Read(trans));
        CheckRes(trans->Read(&fSharedEngine, sizeof(int)));
        CheckRes(trans->Read(&fSharedClient, sizeof(int)));
        CheckRes(trans->Read(&fSharedGraph, sizeof(int)));
        return 0;
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackResult::Write(trans));
        CheckRes(trans->Write(&fSharedEngine, sizeof(int)));
        CheckRes(trans->Write(&fSharedClient, sizeof(int)));
        CheckRes(trans->Write(&fSharedGraph, sizeof(int)));
        return 0;
    }

};

/*!
\brief CloseClient request.
*/

PRE_PACKED_STRUCTURE_ALWAYS
struct JackClientCloseRequestData
{

    int fRefNum;

    JackClientCloseRequestData(int refnum=0): fRefNum(refnum)
    {}

    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kClientClose;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackClientCloseRequestData> JackClientCloseRequest;

/*!
\brief Activate request.
*/

PRE_PACKED_STRUCTURE_ALWAYS
struct JackActivateRequestData
{

    int fRefNum;
    int fIsRealTime;

    JackActivateRequestData(int refnum=0, int is_real_time=0)
        : fRefNum(refnum), fIsRealTime(is_real_time)
    {}

    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kActivateClient;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackActivateRequestData> JackActivateRequest;

/*!
\brief Deactivate request.
*/

PRE_PACKED_STRUCTURE_ALWAYS
struct JackDeactivateRequestData
{

    int fRefNum;

    JackDeactivateRequestData(int refnum=0) : fRefNum(refnum)
    {}

    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kDeactivateClient;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackDeactivateRequestData> JackDeactivateRequest;

/*!
\brief PortRegister request.
*/

PRE_PACKED_STRUCTURE_ALWAYS
struct JackPortRegisterRequestData
{

    int fRefNum;
    char fName[JACK_PORT_NAME_SIZE + 1];   // port short name
    char fPortType[JACK_PORT_TYPE_SIZE + 1];
    unsigned int fFlags;
    unsigned int fBufferSize;

    JackPortRegisterRequestData(int refnum=0, const char* name="", const char* port_type="", unsigned int flags=0, unsigned int buffer_size=0)
            : fRefNum(refnum), fFlags(flags), fBufferSize(buffer_size)
    {
        memset(fName, 0, sizeof(fName));
        memset(fPortType, 0, sizeof(fPortType));
        strncpy(fName, name, sizeof(fName)-1);
        strncpy(fPortType, port_type, sizeof(fPortType)-1);
    }


    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kRegisterPort;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackPortRegisterRequestData> JackPortRegisterRequest;

/*!
\brief PortRegister result.
*/

struct JackPortRegisterResult : public JackResult
{

    jack_port_id_t fPortIndex;

    JackPortRegisterResult(): JackResult(), fPortIndex(NO_PORT)
    {}

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackResult::Read(trans));
        return trans->Read(&fPortIndex, sizeof(jack_port_id_t));
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackResult::Write(trans));
        return trans->Write(&fPortIndex, sizeof(jack_port_id_t));
    }

};

/*!
\brief PortUnregister request.
*/

PRE_PACKED_STRUCTURE_ALWAYS
struct JackPortUnRegisterRequestData
{

    int fRefNum;
    jack_port_id_t fPortIndex;

    JackPortUnRegisterRequestData(int refnum=0, jack_port_id_t index=0)
        : fRefNum(refnum), fPortIndex(index)
    {}

    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kUnRegisterPort;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackPortUnRegisterRequestData> JackPortUnRegisterRequest;

/*!
\brief PortConnectName request.
*/

PRE_PACKED_STRUCTURE_ALWAYS
struct JackPortConnectNameRequestData
{

    int fRefNum;
    char fSrc[REAL_JACK_PORT_NAME_SIZE+1];    // port full name
    char fDst[REAL_JACK_PORT_NAME_SIZE+1];    // port full name

    JackPortConnectNameRequestData(int refnum=0, const char* src_name="", const char* dst_name="")
        : fRefNum(refnum)
    {
        memset(fSrc, 0, sizeof(fSrc));
        memset(fDst, 0, sizeof(fDst));
        strncpy(fSrc, src_name, sizeof(fSrc)-1);
        strncpy(fDst, dst_name, sizeof(fDst)-1);
    }

    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kConnectNamePorts;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackPortConnectNameRequestData> JackPortConnectNameRequest;

/*!
\brief PortDisconnectName request.
*/

PRE_PACKED_STRUCTURE_ALWAYS
struct JackPortDisconnectNameRequestData
{

    int fRefNum;
    char fSrc[REAL_JACK_PORT_NAME_SIZE+1];    // port full name
    char fDst[REAL_JACK_PORT_NAME_SIZE+1];    // port full name

    JackPortDisconnectNameRequestData(int refnum=0, const char* src_name="", const char* dst_name="")
        : fRefNum(refnum)
    {
        memset(fSrc, 0, sizeof(fSrc));
        memset(fDst, 0, sizeof(fDst));
        strncpy(fSrc, src_name, sizeof(fSrc)-1);
        strncpy(fDst, dst_name, sizeof(fDst)-1);
    }

    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kDisconnectNamePorts;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackPortDisconnectNameRequestData> JackPortDisconnectNameRequest;

/*!
\brief PortConnect request.
*/

PRE_PACKED_STRUCTURE_ALWAYS
struct JackPortConnectRequestData
{

    int fRefNum;
    jack_port_id_t fSrc;
    jack_port_id_t fDst;

    JackPortConnectRequestData(int refnum=0, jack_port_id_t src=0, jack_port_id_t dst=0)
        : fRefNum(refnum), fSrc(src), fDst(dst)
    {}

    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kConnectPorts;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackPortConnectRequestData> JackPortConnectRequest;

/*!
\brief PortDisconnect request.
*/

PRE_PACKED_STRUCTURE_ALWAYS
struct JackPortDisconnectRequestData
{

    int fRefNum;
    jack_port_id_t fSrc;
    jack_port_id_t fDst;

    JackPortDisconnectRequestData(int refnum=0, jack_port_id_t src=0, jack_port_id_t dst=0)
        : fRefNum(refnum), fSrc(src), fDst(dst)
    {}

    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kDisconnectPorts;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackPortDisconnectRequestData> JackPortDisconnectRequest;

/*!
\brief PortRename request.
*/

PRE_PACKED_STRUCTURE_ALWAYS
struct JackPortRenameRequestData
{

    int fRefNum;
    jack_port_id_t fPort;
    char fName[JACK_PORT_NAME_SIZE + 1];   // port short name

    JackPortRenameRequestData(int refnum=0, jack_port_id_t port=0, const char* name="")
        : fRefNum(refnum), fPort(port)
    {
        memset(fName, 0, sizeof(fName));
        strncpy(fName, name, sizeof(fName)-1);
    }

    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kPortRename;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackPortRenameRequestData> JackPortRenameRequest;

/*!
\brief SetBufferSize request.
*/

PRE_PACKED_STRUCTURE_ALWAYS
struct JackSetBufferSizeRequestData
{

    jack_nframes_t fBufferSize;

    JackSetBufferSizeRequestData(jack_nframes_t buffer_size=0)
        : fBufferSize(buffer_size)
    {}

    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kSetBufferSize;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackSetBufferSizeRequestData> JackSetBufferSizeRequest;

/*!
\brief SetFreeWheel request.
*/

PRE_PACKED_STRUCTURE_ALWAYS
struct JackSetFreeWheelRequestData
{

    int fOnOff;

    JackSetFreeWheelRequestData(int onoff=0)
        : fOnOff(onoff)
    {}

    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kSetFreeWheel;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackSetFreeWheelRequestData> JackSetFreeWheelRequest;

/*!
\brief ComputeTotalLatencies request.
*/

PRE_PACKED_STRUCTURE_ALWAYS
struct JackComputeTotalLatenciesRequestData
{
    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kComputeTotalLatencies;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackComputeTotalLatenciesRequestData> JackComputeTotalLatenciesRequest;

/*!
\brief ReleaseTimebase request.
*/

PRE_PACKED_STRUCTURE_ALWAYS
struct JackReleaseTimebaseRequestData
{

    int fRefNum;

    JackReleaseTimebaseRequestData(int refnum=0)
        : fRefNum(refnum)
    {}

    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kReleaseTimebase;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackReleaseTimebaseRequestData> JackReleaseTimebaseRequest;

/*!
\brief SetTimebaseCallback request.
*/

PRE_PACKED_STRUCTURE_ALWAYS
struct JackSetTimebaseCallbackRequestData
{

    int fRefNum;
    int fConditionnal;

    JackSetTimebaseCallbackRequestData(int refnum=0, int conditional=0)
        : fRefNum(refnum), fConditionnal(conditional)
    {}

    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kSetTimebaseCallback;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackSetTimebaseCallbackRequestData> JackSetTimebaseCallbackRequest;

/*!
\brief GetInternalClientName request.
*/

PRE_PACKED_STRUCTURE_ALWAYS
struct JackGetInternalClientNameRequestData
{

    int fRefNum;
    int fIntRefNum;

    JackGetInternalClientNameRequestData(int refnum=0, int int_ref=0)
            : fRefNum(refnum), fIntRefNum(int_ref)
    {}

    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kGetInternalClientName;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackGetInternalClientNameRequestData> JackGetInternalClientNameRequest;

/*!
\brief GetInternalClient result.
*/

struct JackGetInternalClientNameResult : public JackResult
{

    char fName[JACK_CLIENT_NAME_SIZE+1];

    JackGetInternalClientNameResult(): JackResult()
    {
        memset(fName, 0, sizeof(fName));
    }
    JackGetInternalClientNameResult(int32_t result, const char* name)
            : JackResult(result)
    {
        memset(fName, 0, sizeof(fName));
        snprintf(fName, sizeof(fName), "%s", name);
    }

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackResult::Read(trans));
        CheckRes(trans->Read(&fName, sizeof(fName)));
        return 0;
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackResult::Write(trans));
        CheckRes(trans->Write(&fName, sizeof(fName)));
        return 0;
    }

    int Size() { return sizeof(fName); }
};

/*!
\brief InternalClientHandle request.
*/

PRE_PACKED_STRUCTURE_ALWAYS
struct JackInternalClientHandleRequestData
{

    int fRefNum;
    char fName[JACK_CLIENT_NAME_SIZE+1];

    JackInternalClientHandleRequestData(int refnum=0, const char* client_name="")
            : fRefNum(refnum)
    {
        memset(fName, 0, sizeof(fName));
        snprintf(fName, sizeof(fName), "%s", client_name);
    }

    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kInternalClientHandle;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackInternalClientHandleRequestData> JackInternalClientHandleRequest;

/*!
\brief InternalClientHandle result.
*/

struct JackInternalClientHandleResult : public JackResult
{

    int fStatus;
    int fIntRefNum;

    JackInternalClientHandleResult(): JackResult(), fStatus(0), fIntRefNum(0)
    {}
    JackInternalClientHandleResult(int32_t result, int status, int int_ref)
            : JackResult(result), fStatus(status), fIntRefNum(int_ref)
    {}

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackResult::Read(trans));
        CheckRes(trans->Read(&fStatus, sizeof(int)));
        CheckRes(trans->Read(&fIntRefNum, sizeof(int)));
        return 0;
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackResult::Write(trans));
        CheckRes(trans->Write(&fStatus, sizeof(int)));
        CheckRes(trans->Write(&fIntRefNum, sizeof(int)));
        return 0;
    }

    int Size() { return sizeof(int) + sizeof(int); }
};

/*!
\brief InternalClientLoad request.
*/

PRE_PACKED_STRUCTURE_ALWAYS
struct JackInternalClientLoadRequestData
{

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

    int fRefNum;
    char fName[JACK_CLIENT_NAME_SIZE+1];
    char fDllName[MAX_PATH+1];
    char fLoadInitName[JACK_LOAD_INIT_LIMIT+1];
    int fOptions;
    jack_uuid_t fUUID;

    JackInternalClientLoadRequestData(int refnum=0, const char* client_name="", const char* so_name="", const char* objet_data="", int options=0, jack_uuid_t uuid=JACK_UUID_EMPTY_INITIALIZER)
            : fRefNum(refnum), fOptions(options), fUUID(uuid)
    {
        memset(fName, 0, sizeof(fName));
        memset(fDllName, 0, sizeof(fDllName));
        memset(fLoadInitName, 0, sizeof(fLoadInitName));
        strncpy(fName, client_name, sizeof(fName)-1);
        if (so_name) {
            strncpy(fDllName, so_name, sizeof(fDllName)-1);
        }
        if (objet_data) {
            strncpy(fLoadInitName, objet_data, sizeof(fLoadInitName)-1);
        }
    }

    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kInternalClientLoad;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackInternalClientLoadRequestData> JackInternalClientLoadRequest;

/*!
\brief InternalClientLoad result.
*/

struct JackInternalClientLoadResult : public JackResult
{

    int fStatus;
    int fIntRefNum;

    JackInternalClientLoadResult(): JackResult(), fStatus(0), fIntRefNum(0)
    {}
    JackInternalClientLoadResult(int32_t result, int status, int int_ref)
            : JackResult(result), fStatus(status), fIntRefNum(int_ref)
    {}

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackResult::Read(trans));
        CheckRes(trans->Read(&fStatus, sizeof(int)));
        CheckRes(trans->Read(&fIntRefNum, sizeof(int)));
        return 0;
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackResult::Write(trans));
        CheckRes(trans->Write(&fStatus, sizeof(int)));
        CheckRes(trans->Write(&fIntRefNum, sizeof(int)));
        return 0;
    }

    int Size() { return sizeof(int) + sizeof(int); }
};

/*!
\brief InternalClientUnload request.
*/

PRE_PACKED_STRUCTURE_ALWAYS
struct JackInternalClientUnloadRequestData
{

    int fRefNum;
    int fIntRefNum;

    JackInternalClientUnloadRequestData(int refnum=0, int int_ref=0)
            : fRefNum(refnum), fIntRefNum(int_ref)
    {}

    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kInternalClientUnload;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackInternalClientUnloadRequestData> JackInternalClientUnloadRequest;

/*!
\brief InternalClientLoad result.
*/

struct JackInternalClientUnloadResult : public JackResult
{

    int fStatus;

    JackInternalClientUnloadResult(): JackResult(), fStatus(0)
    {}
    JackInternalClientUnloadResult(int32_t result, int status)
            : JackResult(result), fStatus(status)
    {}

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackResult::Read(trans));
        CheckRes(trans->Read(&fStatus, sizeof(int)));
        return 0;
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackResult::Write(trans));
        CheckRes(trans->Write(&fStatus, sizeof(int)));
        return 0;
    }

    int Size() { return sizeof(int); }
};

/*!
\brief ClientNotification request.
*/

PRE_PACKED_STRUCTURE_ALWAYS
struct JackClientNotificationRequestData
{

    int fRefNum;
    int fNotify;
    int fValue;

    JackClientNotificationRequestData(int refnum=0, int notify=0, int value=0)
            : fRefNum(refnum), fNotify(notify), fValue(value)
    {}


    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kNotification;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackClientNotificationRequestData> JackClientNotificationRequest;

struct JackSessionCommand
{
    char fUUID[JACK_UUID_STRING_SIZE];
    char fClientName[JACK_CLIENT_NAME_SIZE+1];
    char fCommand[JACK_SESSION_COMMAND_SIZE+1];
    jack_session_flags_t fFlags;

    JackSessionCommand() : fFlags(JackSessionSaveError)
    {
        memset(fUUID, 0, sizeof(fUUID));
        memset(fClientName, 0, sizeof(fClientName));
        memset(fCommand, 0, sizeof(fCommand));
    }
    JackSessionCommand(const char *uuid, const char *clientname, const char *command, jack_session_flags_t flags)
    {
        memset(fUUID, 0, sizeof(fUUID));
        memset(fClientName, 0, sizeof(fClientName));
        memset(fCommand, 0, sizeof(fCommand));
        strncpy(fUUID, uuid, sizeof(fUUID)-1);
        strncpy(fClientName, clientname, sizeof(fClientName)-1);
        strncpy(fCommand, command, sizeof(fCommand)-1);
        fFlags = flags;
    }
};

struct JackSessionNotifyResult : public JackResult
{

    std::list<JackSessionCommand> fCommandList;
    bool fDone;

    JackSessionNotifyResult(): JackResult(), fDone(false)
    {}
    JackSessionNotifyResult(int32_t result)
            : JackResult(result), fDone(false)
    {}

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        if (trans == NULL)
        {
            return 0;
        }

        CheckRes(JackResult::Read(trans));
        while (true) {
            JackSessionCommand buffer;

            CheckRes(trans->Read(buffer.fUUID, sizeof(buffer.fUUID)));
            if (buffer.fUUID[0] == '\0')
                break;

            CheckRes(trans->Read(buffer.fClientName, sizeof(buffer.fClientName)));
            CheckRes(trans->Read(buffer.fCommand, sizeof(buffer.fCommand)));
            CheckRes(trans->Read(&(buffer.fFlags), sizeof(buffer.fFlags)));

            fCommandList.push_back(buffer);
        }

        fDone = true;

        return 0;
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        if (trans == NULL)
        {
            fDone = true;
            return 0;
        }

        char terminator[JACK_UUID_STRING_SIZE];
        memset(terminator, 0, sizeof(terminator));

        CheckRes(JackResult::Write(trans));
        for (std::list<JackSessionCommand>::iterator i = fCommandList.begin(); i != fCommandList.end(); i++) {
            CheckRes(trans->Write(i->fUUID, sizeof(i->fUUID)));
            CheckRes(trans->Write(i->fClientName, sizeof(i->fClientName)));
            CheckRes(trans->Write(i->fCommand, sizeof(i->fCommand)));
            CheckRes(trans->Write(&(i->fFlags), sizeof(i->fFlags)));
        }
        CheckRes(trans->Write(terminator, sizeof(terminator)));
        return 0;
    }

    jack_session_command_t* GetCommands()
    {
        /* TODO: some kind of signal should be used instead */
        while (!fDone)
        {
            JackSleep(50000);    /* 50 ms */
        }

        jack_session_command_t* session_command = (jack_session_command_t *)malloc(sizeof(jack_session_command_t) * (fCommandList.size() + 1));
        int i = 0;

        for (std::list<JackSessionCommand>::iterator ci = fCommandList.begin(); ci != fCommandList.end(); ci++) {
            session_command[i].uuid = strdup(ci->fUUID);
            session_command[i].client_name = strdup(ci->fClientName);
            session_command[i].command = strdup(ci->fCommand);
            session_command[i].flags = ci->fFlags;
            i += 1;
        }

        session_command[i].uuid = NULL;
        session_command[i].client_name = NULL;
        session_command[i].command = NULL;
        session_command[i].flags = (jack_session_flags_t)0;

        return session_command;
    }
};

/*!
\brief SessionNotify request.
*/

PRE_PACKED_STRUCTURE_ALWAYS
struct JackSessionNotifyRequestData
{
    char fPath[JACK_MESSAGE_SIZE+1];
    char fDst[JACK_CLIENT_NAME_SIZE+1];
    jack_session_event_type_t fEventType;
    int fRefNum;

    JackSessionNotifyRequestData(int refnum=0, const char* path="", jack_session_event_type_t type=JackSessionSave, const char* dst=NULL)
            : fEventType(type), fRefNum(refnum)
    {
        memset(fPath, 0, sizeof(fPath));
        memset(fDst, 0, sizeof(fDst));
        strncpy(fPath, path, sizeof(fPath)-1);
        if (dst) {
            strncpy(fDst, dst, sizeof(fDst)-1);
        }
    }

    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kSessionNotify;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackSessionNotifyRequestData> JackSessionNotifyRequest;

PRE_PACKED_STRUCTURE_ALWAYS
struct JackSessionReplyRequestData
{
    int fRefNum;

    JackSessionReplyRequestData(int refnum=0)
            : fRefNum(refnum)
    {}

    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kSessionReply;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackSessionReplyRequestData> JackSessionReplyRequest;

struct JackClientNameResult : public JackResult
{
    char fName[JACK_CLIENT_NAME_SIZE+1];

    JackClientNameResult(): JackResult()
    {
        memset(fName, 0, sizeof(fName));
    }
    JackClientNameResult(int32_t result, const char* name)
            : JackResult(result)
    {
        memset(fName, 0, sizeof(fName));
        strncpy(fName, name, sizeof(fName)-1);
    }

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackResult::Read(trans));
        CheckRes(trans->Read(&fName, sizeof(fName)));
        return 0;
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackResult::Write(trans));
        CheckRes(trans->Write(&fName, sizeof(fName)));
        return 0;
    }

};

struct JackUUIDResult : public JackResult
{
    char fUUID[JACK_UUID_STRING_SIZE];

    JackUUIDResult(): JackResult()
    {
        memset(fUUID, 0, sizeof(fUUID));
    }
    JackUUIDResult(int32_t result, const char* uuid)
            : JackResult(result)
    {
        memset(fUUID, 0, sizeof(fUUID));
        strncpy(fUUID, uuid, sizeof(fUUID)-1);
    }

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackResult::Read(trans));
        CheckRes(trans->Read(&fUUID, sizeof(fUUID)));
        return 0;
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackResult::Write(trans));
        CheckRes(trans->Write(&fUUID, sizeof(fUUID)));
        return 0;
    }

};

PRE_PACKED_STRUCTURE_ALWAYS
struct JackGetUUIDRequestData
{
    char fName[JACK_CLIENT_NAME_SIZE+1];

    JackGetUUIDRequestData(const char* client_name="")
    {
        memset(fName, 0, sizeof(fName));
        strncpy(fName, client_name, sizeof(fName)-1);
    }

    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kGetUUIDByClient;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackGetUUIDRequestData> JackGetUUIDRequest;

PRE_PACKED_STRUCTURE_ALWAYS
struct JackGetClientNameRequestData
{
    char fUUID[JACK_UUID_STRING_SIZE];

    JackGetClientNameRequestData(const char* uuid="")
    {
        memset(fUUID, 0, sizeof(fUUID));
        strncpy(fUUID, uuid, sizeof(fUUID)-1);
    }

    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kGetClientByUUID;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackGetClientNameRequestData> JackGetClientNameRequest;

PRE_PACKED_STRUCTURE_ALWAYS
struct JackReserveNameRequestData
{
    int  fRefNum;
    char fName[JACK_CLIENT_NAME_SIZE+1];
    char fUUID[JACK_UUID_STRING_SIZE];

    JackReserveNameRequestData(int refnum=0, const char *name="", const char* uuid="")
            : fRefNum(refnum)
    {
        memset(fName, 0, sizeof(fName));
        memset(fUUID, 0, sizeof(fUUID));
        strncpy(fName, name, sizeof(fName)-1);
        strncpy(fUUID, uuid, sizeof(fUUID)-1);
    }
    

    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kReserveClientName;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackReserveNameRequestData> JackReserveNameRequest;

PRE_PACKED_STRUCTURE_ALWAYS
struct JackClientHasSessionCallbackRequestData
{
    char fName[JACK_CLIENT_NAME_SIZE+1];

    JackClientHasSessionCallbackRequestData(const char *name="")
    {
        memset(fName, 0, sizeof(fName));
        strncpy(fName, name, sizeof(fName)-1);
    }

    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kClientHasSessionCallback;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackClientHasSessionCallbackRequestData> JackClientHasSessionCallbackRequest;


PRE_PACKED_STRUCTURE_ALWAYS
struct JackPropertyChangeNotifyRequestData
{
    jack_uuid_t fSubject;
    char fKey[MAX_PATH+1];
    jack_property_change_t fChange;

    JackPropertyChangeNotifyRequestData(jack_uuid_t subject=0, const char* key="", jack_property_change_t change=(jack_property_change_t)0)
        : fChange(change)
    {
        jack_uuid_copy(&fSubject, subject);
        memset(fKey, 0, sizeof(fKey));
        if (key)
            strncpy(fKey, key, sizeof(fKey)-1);
    }

    static JackRequest::RequestType Type()
    {
        return JackRequest::RequestType::kPropertyChangeNotify;
    }
} POST_PACKED_STRUCTURE_ALWAYS;

typedef JackRequestTemplate<JackPropertyChangeNotifyRequestData> JackPropertyChangeNotifyRequest;

/*!
\brief ClientNotification.
*/

struct JackClientNotification
{
    int fSize;
    char fName[JACK_CLIENT_NAME_SIZE+1];
    int fRefNum;
    int fNotify;
    int fValue1;
    int fValue2;
    int fSync;
    char fMessage[JACK_MESSAGE_SIZE+1];

    JackClientNotification(): fSize(0), fRefNum(0), fNotify(-1), fValue1(-1), fValue2(-1), fSync(0)
    {
        memset(fName, 0, sizeof(fName));
        memset(fMessage, 0, sizeof(fMessage));
    }
    JackClientNotification(const char* name, int refnum, int notify, int sync, const char* message, int value1, int value2)
            : fRefNum(refnum), fNotify(notify), fValue1(value1), fValue2(value2), fSync(sync)
    {
        memset(fName, 0, sizeof(fName));
        memset(fMessage, 0, sizeof(fMessage));
        strncpy(fName, name, sizeof(fName)-1);
        if (message) {
            strncpy(fMessage, message, sizeof(fMessage)-1);
        }
        fSize = Size();
    }

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckSize();
        CheckRes(trans->Read(&fName, sizeof(fName)));
        CheckRes(trans->Read(&fRefNum, sizeof(int)));
        CheckRes(trans->Read(&fNotify, sizeof(int)));
        CheckRes(trans->Read(&fValue1, sizeof(int)));
        CheckRes(trans->Read(&fValue2, sizeof(int)));
        CheckRes(trans->Read(&fSync, sizeof(int)));
        CheckRes(trans->Read(&fMessage, sizeof(fMessage)));
        return 0;
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(trans->Write(&fSize, sizeof(int)));
        CheckRes(trans->Write(&fName, sizeof(fName)));
        CheckRes(trans->Write(&fRefNum, sizeof(int)));
        CheckRes(trans->Write(&fNotify, sizeof(int)));
        CheckRes(trans->Write(&fValue1, sizeof(int)));
        CheckRes(trans->Write(&fValue2, sizeof(int)));
        CheckRes(trans->Write(&fSync, sizeof(int)));
        CheckRes(trans->Write(&fMessage, sizeof(fMessage)));
        return 0;
    }

    int Size() { return sizeof(int) + sizeof(fName) + 5 * sizeof(int) + sizeof(fMessage); }

};

} // end of namespace

#endif
