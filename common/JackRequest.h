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
        kComputeTotalLatencies = 39
    };

    RequestType fType;
    int fSize;

    JackRequest(): fType((RequestType)0), fSize(0)
    {}

    JackRequest(RequestType type): fType(type), fSize(0)
    {}

    virtual ~JackRequest()
    {}

    virtual int Read(detail::JackChannelTransactionInterface* trans)
    {
        return trans->Read(&fType, sizeof(RequestType));
    }

    virtual int Write(detail::JackChannelTransactionInterface* trans) { return -1; }
    
    virtual int Write(detail::JackChannelTransactionInterface* trans, int size)
    {
        fSize = size;
        CheckRes(trans->Write(&fType, sizeof(RequestType)));
        return trans->Write(&fSize, sizeof(int));
    }
    
    virtual int Size() { return 0; }

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

struct JackClientCheckRequest : public JackRequest
{

    char fName[JACK_CLIENT_NAME_SIZE + 1];
    int fProtocol;
    int fOptions;
    int fUUID;
    int fOpen;

    JackClientCheckRequest()
    {}
    JackClientCheckRequest(const char* name, int protocol, int options, int uuid, int open = false)
        : JackRequest(JackRequest::kClientCheck), fProtocol(protocol), fOptions(options), fUUID(uuid), fOpen(open)
    {
        snprintf(fName, sizeof(fName), "%s", name);
    }

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckSize();
        CheckRes(trans->Read(&fName, sizeof(fName)));
        CheckRes(trans->Read(&fProtocol, sizeof(int)));
        CheckRes(trans->Read(&fOptions, sizeof(int)));
        CheckRes(trans->Read(&fUUID, sizeof(int)));
        return trans->Read(&fOpen, sizeof(int));
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackRequest::Write(trans, Size()));
        CheckRes(trans->Write(&fName, sizeof(fName)));
        CheckRes(trans->Write(&fProtocol, sizeof(int)));
        CheckRes(trans->Write(&fOptions, sizeof(int)));
        CheckRes(trans->Write(&fUUID, sizeof(int)));
        return trans->Write(&fOpen, sizeof(int));
    }
    
    int Size() { return sizeof(fName) + 4 * sizeof(int); }

};

/*!
\brief CheckClient result.
*/

struct JackClientCheckResult : public JackResult
{

    char fName[JACK_CLIENT_NAME_SIZE + 1];
    int fStatus;

    JackClientCheckResult(): JackResult(), fStatus(0)
    {}
    JackClientCheckResult(int32_t result, const char* name, int status)
            : JackResult(result), fStatus(status)
    {
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

struct JackClientOpenRequest : public JackRequest
{

    int fPID;
    int fUUID;
    char fName[JACK_CLIENT_NAME_SIZE + 1];

    JackClientOpenRequest()
    {}
    JackClientOpenRequest(const char* name, int pid, int uuid): JackRequest(JackRequest::kClientOpen)
    {
        snprintf(fName, sizeof(fName), "%s", name);
        fPID = pid;
        fUUID = uuid;
    }

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckSize();
        CheckRes(trans->Read(&fPID, sizeof(int)));
        CheckRes(trans->Read(&fUUID, sizeof(int)));
        return trans->Read(&fName, sizeof(fName));
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackRequest::Write(trans, Size()));
        CheckRes(trans->Write(&fPID, sizeof(int)));
        CheckRes(trans->Write(&fUUID, sizeof(int)));
        return trans->Write(&fName, sizeof(fName));
    }

    int Size() { return 2 * sizeof(int) + sizeof(fName); }
    
};

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

struct JackClientCloseRequest : public JackRequest
{

    int fRefNum;

    JackClientCloseRequest()
    {}
    JackClientCloseRequest(int refnum): JackRequest(JackRequest::kClientClose), fRefNum(refnum)
    {}

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckSize();
        return trans->Read(&fRefNum, sizeof(int));
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackRequest::Write(trans, Size()));
        return trans->Write(&fRefNum, sizeof(int));
    }

    int Size() { return sizeof(int); }
};

/*!
\brief Activate request.
*/

struct JackActivateRequest : public JackRequest
{

    int fRefNum;
    int fIsRealTime;

    JackActivateRequest()
    {}
    JackActivateRequest(int refnum, int is_real_time)
        : JackRequest(JackRequest::kActivateClient), fRefNum(refnum), fIsRealTime(is_real_time)
    {}

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckSize();
        CheckRes(trans->Read(&fRefNum, sizeof(int)));
        return trans->Read(&fIsRealTime, sizeof(int));
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackRequest::Write(trans, Size()));
        CheckRes(trans->Write(&fRefNum, sizeof(int)));
        return trans->Write(&fIsRealTime, sizeof(int));
    }

    int Size() { return 2 * sizeof(int); }
};

/*!
\brief Deactivate request.
*/

struct JackDeactivateRequest : public JackRequest
{

    int fRefNum;

    JackDeactivateRequest()
    {}
    JackDeactivateRequest(int refnum): JackRequest(JackRequest::kDeactivateClient), fRefNum(refnum)
    {}

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckSize();
        return trans->Read(&fRefNum, sizeof(int));
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackRequest::Write(trans, Size()));
        return trans->Write(&fRefNum, sizeof(int));
    }

    int Size() { return sizeof(int); }
};

/*!
\brief PortRegister request.
*/

struct JackPortRegisterRequest : public JackRequest
{

    int fRefNum;
    char fName[JACK_PORT_NAME_SIZE + 1];
    char fPortType[JACK_PORT_TYPE_SIZE + 1];
    unsigned int fFlags;
    unsigned int fBufferSize;

    JackPortRegisterRequest()
    {}
    JackPortRegisterRequest(int refnum, const char* name, const char* port_type, unsigned int flags, unsigned int buffer_size)
            : JackRequest(JackRequest::kRegisterPort), fRefNum(refnum), fFlags(flags), fBufferSize(buffer_size)
    {
        strcpy(fName, name);
        strcpy(fPortType, port_type);
    }

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckSize();
        CheckRes(trans->Read(&fRefNum, sizeof(int)));
        CheckRes(trans->Read(&fName, sizeof(fName)));
        CheckRes(trans->Read(&fPortType, sizeof(fPortType)));
        CheckRes(trans->Read(&fFlags, sizeof(unsigned int)));
        CheckRes(trans->Read(&fBufferSize, sizeof(unsigned int)));
        return 0;
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackRequest::Write(trans, Size()));
        CheckRes(trans->Write(&fRefNum, sizeof(int)));
        CheckRes(trans->Write(&fName, sizeof(fName)));
        CheckRes(trans->Write(&fPortType, sizeof(fPortType)));
        CheckRes(trans->Write(&fFlags, sizeof(unsigned int)));
        CheckRes(trans->Write(&fBufferSize, sizeof(unsigned int)));
        return 0;
    }
    
    int Size() { return sizeof(int) + sizeof(fName) + sizeof(fPortType) + 2 * sizeof(unsigned int); }

};

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

struct JackPortUnRegisterRequest : public JackRequest
{

    int fRefNum;
    jack_port_id_t fPortIndex;

    JackPortUnRegisterRequest()
    {}
    JackPortUnRegisterRequest(int refnum, jack_port_id_t index)
        : JackRequest(JackRequest::kUnRegisterPort), fRefNum(refnum), fPortIndex(index)
    {}

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckSize();
        CheckRes(trans->Read(&fRefNum, sizeof(int)));
        CheckRes(trans->Read(&fPortIndex, sizeof(jack_port_id_t)));
        return 0;
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackRequest::Write(trans, Size()));
        CheckRes(trans->Write(&fRefNum, sizeof(int)));
        CheckRes(trans->Write(&fPortIndex, sizeof(jack_port_id_t)));
        return 0;
    }

    int Size() { return sizeof(int) + sizeof(jack_port_id_t); }
};

/*!
\brief PortConnectName request.
*/

struct JackPortConnectNameRequest : public JackRequest
{

    int fRefNum;
    char fSrc[JACK_PORT_NAME_SIZE + 1];
    char fDst[JACK_PORT_NAME_SIZE + 1];

    JackPortConnectNameRequest()
    {}
    JackPortConnectNameRequest(int refnum, const char* src_name, const char* dst_name)
        : JackRequest(JackRequest::kConnectNamePorts), fRefNum(refnum)
    {
        strcpy(fSrc, src_name);
        strcpy(fDst, dst_name);
    }

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckSize();
        CheckRes(trans->Read(&fRefNum, sizeof(int)));
        CheckRes(trans->Read(&fSrc, sizeof(fSrc)));
        CheckRes(trans->Read(&fDst, sizeof(fDst)));
        return 0;
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackRequest::Write(trans, Size()));
        CheckRes(trans->Write(&fRefNum, sizeof(int)));
        CheckRes(trans->Write(&fSrc, sizeof(fSrc)));
        CheckRes(trans->Write(&fDst, sizeof(fDst)));
        return 0;
    }
    
    int Size() { return sizeof(int) + sizeof(fSrc) + sizeof(fDst); }

};

/*!
\brief PortDisconnectName request.
*/

struct JackPortDisconnectNameRequest : public JackRequest
{

    int fRefNum;
    char fSrc[JACK_PORT_NAME_SIZE + 1];
    char fDst[JACK_PORT_NAME_SIZE + 1];

    JackPortDisconnectNameRequest()
    {}
    JackPortDisconnectNameRequest(int refnum, const char* src_name, const char* dst_name)
        : JackRequest(JackRequest::kDisconnectNamePorts), fRefNum(refnum)
    {
        strcpy(fSrc, src_name);
        strcpy(fDst, dst_name);
    }

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckSize();
        CheckRes(trans->Read(&fRefNum, sizeof(int)));
        CheckRes(trans->Read(&fSrc, sizeof(fSrc)));
        CheckRes(trans->Read(&fDst, sizeof(fDst)));
        return 0;
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackRequest::Write(trans, Size()));
        CheckRes(trans->Write(&fRefNum, sizeof(int)));
        CheckRes(trans->Write(&fSrc, sizeof(fSrc)));
        CheckRes(trans->Write(&fDst, sizeof(fDst)));
        return 0;
    }

    int Size() { return sizeof(int) + sizeof(fSrc) + sizeof(fDst); }
    
};

/*!
\brief PortConnect request.
*/

struct JackPortConnectRequest : public JackRequest
{

    int fRefNum;
    jack_port_id_t fSrc;
    jack_port_id_t fDst;

    JackPortConnectRequest()
    {}
    JackPortConnectRequest(int refnum, jack_port_id_t src, jack_port_id_t dst)
        : JackRequest(JackRequest::kConnectPorts), fRefNum(refnum), fSrc(src), fDst(dst)
    {}

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckSize();
        CheckRes(trans->Read(&fRefNum, sizeof(int)));
        CheckRes(trans->Read(&fSrc, sizeof(jack_port_id_t)));
        CheckRes(trans->Read(&fDst, sizeof(jack_port_id_t)));
        return 0;
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackRequest::Write(trans, Size()));
        CheckRes(trans->Write(&fRefNum, sizeof(int)));
        CheckRes(trans->Write(&fSrc, sizeof(jack_port_id_t)));
        CheckRes(trans->Write(&fDst, sizeof(jack_port_id_t)));
        return 0;
    }

    int Size() { return sizeof(int) + sizeof(jack_port_id_t) + sizeof(jack_port_id_t); }
};

/*!
\brief PortDisconnect request.
*/

struct JackPortDisconnectRequest : public JackRequest
{

    int fRefNum;
    jack_port_id_t fSrc;
    jack_port_id_t fDst;

    JackPortDisconnectRequest()
    {}
    JackPortDisconnectRequest(int refnum, jack_port_id_t src, jack_port_id_t dst)
        : JackRequest(JackRequest::kDisconnectPorts), fRefNum(refnum), fSrc(src), fDst(dst)
    {}

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckSize();
        CheckRes(trans->Read(&fRefNum, sizeof(int)));
        CheckRes(trans->Read(&fSrc, sizeof(jack_port_id_t)));
        CheckRes(trans->Read(&fDst, sizeof(jack_port_id_t)));
        return 0;
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackRequest::Write(trans, Size()));
        CheckRes(trans->Write(&fRefNum, sizeof(int)));
        CheckRes(trans->Write(&fSrc, sizeof(jack_port_id_t)));
        CheckRes(trans->Write(&fDst, sizeof(jack_port_id_t)));
        return 0;
    }

    int Size() { return sizeof(int) + sizeof(jack_port_id_t) + sizeof(jack_port_id_t); }
};

/*!
\brief PortRename request.
*/

struct JackPortRenameRequest : public JackRequest
{

    int fRefNum;
    jack_port_id_t fPort;
    char fName[JACK_PORT_NAME_SIZE + 1];

    JackPortRenameRequest()
    {}
    JackPortRenameRequest(int refnum, jack_port_id_t port, const char* name)
        : JackRequest(JackRequest::kPortRename), fRefNum(refnum), fPort(port)
    {
        strcpy(fName, name);
    }

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckSize();
        CheckRes(trans->Read(&fRefNum, sizeof(int)));
        CheckRes(trans->Read(&fPort, sizeof(jack_port_id_t)));
        CheckRes(trans->Read(&fName, sizeof(fName)));
        return 0;
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackRequest::Write(trans, Size()));
        CheckRes(trans->Write(&fRefNum, sizeof(int)));
        CheckRes(trans->Write(&fPort, sizeof(jack_port_id_t)));
        CheckRes(trans->Write(&fName, sizeof(fName)));
        return 0;
    }
    
    int Size() { return sizeof(int) + sizeof(jack_port_id_t) + sizeof(fName); }

};

/*!
\brief SetBufferSize request.
*/

struct JackSetBufferSizeRequest : public JackRequest
{

    jack_nframes_t fBufferSize;

    JackSetBufferSizeRequest()
    {}
    JackSetBufferSizeRequest(jack_nframes_t buffer_size)
        : JackRequest(JackRequest::kSetBufferSize), fBufferSize(buffer_size)
    {}

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckSize();
        return trans->Read(&fBufferSize, sizeof(jack_nframes_t));
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackRequest::Write(trans, Size()));
        return trans->Write(&fBufferSize, sizeof(jack_nframes_t));
    }

    int Size() { return sizeof(jack_nframes_t); }
};

/*!
\brief SetFreeWheel request.
*/

struct JackSetFreeWheelRequest : public JackRequest
{

    int fOnOff;

    JackSetFreeWheelRequest()
    {}
    JackSetFreeWheelRequest(int onoff)
        : JackRequest(JackRequest::kSetFreeWheel), fOnOff(onoff)
    {}

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckSize();
        return trans->Read(&fOnOff, sizeof(int));
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackRequest::Write(trans, Size()));
        return trans->Write(&fOnOff, sizeof(int));
    }
    
    int Size() { return sizeof(int); }

};

/*!
\brief ComputeTotalLatencies request.
*/

struct JackComputeTotalLatenciesRequest : public JackRequest
{

    JackComputeTotalLatenciesRequest()
        : JackRequest(JackRequest::kComputeTotalLatencies)
    {}

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckSize();
        return 0;
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackRequest::Write(trans, Size()));
        return 0;
    }

    int Size() { return 0; }
};

/*!
\brief ReleaseTimebase request.
*/

struct JackReleaseTimebaseRequest : public JackRequest
{

    int fRefNum;

    JackReleaseTimebaseRequest()
    {}
    JackReleaseTimebaseRequest(int refnum)
        : JackRequest(JackRequest::kReleaseTimebase), fRefNum(refnum)
    {}

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckSize();
        return trans->Read(&fRefNum, sizeof(int));
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackRequest::Write(trans, Size()));
        return trans->Write(&fRefNum, sizeof(int));
    }
    
    int Size() { return sizeof(int); }

};

/*!
\brief SetTimebaseCallback request.
*/

struct JackSetTimebaseCallbackRequest : public JackRequest
{

    int fRefNum;
    int fConditionnal;

    JackSetTimebaseCallbackRequest()
    {}
    JackSetTimebaseCallbackRequest(int refnum, int conditional)
        : JackRequest(JackRequest::kSetTimebaseCallback), fRefNum(refnum), fConditionnal(conditional)
    {}

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckSize();
        CheckRes(trans->Read(&fRefNum, sizeof(int)));
        return trans->Read(&fConditionnal, sizeof(int));
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackRequest::Write(trans, Size()));
        CheckRes(trans->Write(&fRefNum, sizeof(int)));
        return trans->Write(&fConditionnal, sizeof(int));
    }

    int Size() { return sizeof(int) + sizeof(int); }
};

/*!
\brief GetInternalClientName request.
*/

struct JackGetInternalClientNameRequest : public JackRequest
{

    int fRefNum;
    int fIntRefNum;

    JackGetInternalClientNameRequest()
    {}
    JackGetInternalClientNameRequest(int refnum, int int_ref)
            : JackRequest(JackRequest::kGetInternalClientName), fRefNum(refnum), fIntRefNum(int_ref)
    {}

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckSize();
        CheckRes(trans->Read(&fRefNum, sizeof(int)));
        return trans->Read(&fIntRefNum, sizeof(int));
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackRequest::Write(trans, Size()));
        CheckRes(trans->Write(&fRefNum, sizeof(int)));
        return trans->Write(&fIntRefNum, sizeof(int));
    }

    int Size() { return sizeof(int) + sizeof(int); }
};

/*!
\brief GetInternalClient result.
*/

struct JackGetInternalClientNameResult : public JackResult
{

    char fName[JACK_CLIENT_NAME_SIZE + 1];

    JackGetInternalClientNameResult(): JackResult()
    {}
    JackGetInternalClientNameResult(int32_t result, const char* name)
            : JackResult(result)
    {
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

struct JackInternalClientHandleRequest : public JackRequest
{

    int fRefNum;
    char fName[JACK_CLIENT_NAME_SIZE + 1];

    JackInternalClientHandleRequest()
    {}
    JackInternalClientHandleRequest(int refnum, const char* client_name)
            : JackRequest(JackRequest::kInternalClientHandle), fRefNum(refnum)
    {
        snprintf(fName, sizeof(fName), "%s", client_name);
    }

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckSize();
        CheckRes(trans->Read(&fRefNum, sizeof(int)));
        return trans->Read(&fName, sizeof(fName));
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackRequest::Write(trans, Size()));
        CheckRes(trans->Write(&fRefNum, sizeof(int)));
        return trans->Write(&fName, sizeof(fName));
    }

    int Size() { return sizeof(int) + sizeof(fName); }
};

/*!
\brief InternalClientHandle result.
*/

struct JackInternalClientHandleResult : public JackResult
{

    int fStatus;
    int fIntRefNum;

    JackInternalClientHandleResult(): JackResult()
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

struct JackInternalClientLoadRequest : public JackRequest
{

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

    int fRefNum;
    char fName[JACK_CLIENT_NAME_SIZE + 1];
    char fDllName[MAX_PATH + 1];
    char fLoadInitName[JACK_LOAD_INIT_LIMIT + 1];
    int fOptions;
    int fUUID;

    JackInternalClientLoadRequest()
    {}
    JackInternalClientLoadRequest(int refnum, const char* client_name, const char* so_name, const char* objet_data, int options, int uuid )
            : JackRequest(JackRequest::kInternalClientLoad), fRefNum(refnum), fOptions(options), fUUID(uuid)
    {
        snprintf(fName, sizeof(fName), "%s", client_name);
        snprintf(fDllName, sizeof(fDllName), "%s", so_name);
        snprintf(fLoadInitName, sizeof(fLoadInitName), "%s", objet_data);
        snprintf(fLoadInitName, sizeof(fLoadInitName), "%s", objet_data);
    }

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckSize();
        CheckRes(trans->Read(&fRefNum, sizeof(int)));
        CheckRes(trans->Read(&fName, sizeof(fName)));
        CheckRes(trans->Read(&fDllName, sizeof(fDllName)));
        CheckRes(trans->Read(&fLoadInitName, sizeof(fLoadInitName)));
        CheckRes(trans->Read(&fUUID, sizeof(int)));
        return trans->Read(&fOptions, sizeof(int));
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackRequest::Write(trans, Size()));
        CheckRes(trans->Write(&fRefNum, sizeof(int)));
        CheckRes(trans->Write(&fName, sizeof(fName)));
        CheckRes(trans->Write(&fDllName, sizeof(fDllName)));
        CheckRes(trans->Write(&fLoadInitName, sizeof(fLoadInitName)));
        CheckRes(trans->Write(&fUUID, sizeof(int)));
        return trans->Write(&fOptions, sizeof(int));
    }

    int Size() { return sizeof(int) + sizeof(fName) + sizeof(fDllName) + sizeof(fLoadInitName) + 2 * sizeof(int); }
};

/*!
\brief InternalClientLoad result.
*/

struct JackInternalClientLoadResult : public JackResult
{

    int fStatus;
    int fIntRefNum;

    JackInternalClientLoadResult(): JackResult()
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

struct JackInternalClientUnloadRequest : public JackRequest
{

    int fRefNum;
    int fIntRefNum;

    JackInternalClientUnloadRequest()
    {}
    JackInternalClientUnloadRequest(int refnum, int int_ref)
            : JackRequest(JackRequest::kInternalClientUnload), fRefNum(refnum), fIntRefNum(int_ref)
    {}

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckSize();
        CheckRes(trans->Read(&fRefNum, sizeof(int)));
        return trans->Read(&fIntRefNum, sizeof(int));
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackRequest::Write(trans, Size()));
        CheckRes(trans->Write(&fRefNum, sizeof(int)));
        return trans->Write(&fIntRefNum, sizeof(int));
    }
    
    int Size() { return sizeof(int) + sizeof(int); }
};

/*!
\brief InternalClientLoad result.
*/

struct JackInternalClientUnloadResult : public JackResult
{

    int fStatus;

    JackInternalClientUnloadResult(): JackResult()
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

struct JackClientNotificationRequest : public JackRequest
{

    int fRefNum;
    int fNotify;
    int fValue;

    JackClientNotificationRequest()
    {}
    JackClientNotificationRequest(int refnum, int notify, int value)
            : JackRequest(JackRequest::kNotification), fRefNum(refnum), fNotify(notify), fValue(value)
    {}

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckSize();
        CheckRes(trans->Read(&fRefNum, sizeof(int)));
        CheckRes(trans->Read(&fNotify, sizeof(int)));
        CheckRes(trans->Read(&fValue, sizeof(int)));
        return 0;
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackRequest::Write(trans, Size()));
        CheckRes(trans->Write(&fRefNum, sizeof(int)));
        CheckRes(trans->Write(&fNotify, sizeof(int)));
        CheckRes(trans->Write(&fValue, sizeof(int)));
        return 0;
    }
    
    int Size() { return 3 * sizeof(int); }

};

struct JackSessionCommand
{
    char fUUID[JACK_UUID_SIZE];
    char fClientName[JACK_CLIENT_NAME_SIZE+1];
    char fCommand[JACK_SESSION_COMMAND_SIZE];
    jack_session_flags_t fFlags;

    JackSessionCommand()
    {}

    JackSessionCommand(const char *uuid, const char *clientname, const char *command, jack_session_flags_t flags)
    {
        strncpy(fUUID, uuid, sizeof(fUUID));
        strncpy(fClientName, clientname, sizeof(fClientName));
        strncpy(fCommand, command, sizeof(fCommand));
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

        char terminator[JACK_UUID_SIZE];
        terminator[0] = '\0';

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

struct JackSessionNotifyRequest : public JackRequest
{
    char fPath[JACK_MESSAGE_SIZE + 1];
    char fDst[JACK_CLIENT_NAME_SIZE + 1];
    jack_session_event_type_t fEventType;
    int fRefNum;

    JackSessionNotifyRequest()
    {}
    JackSessionNotifyRequest(int refnum, const char* path, jack_session_event_type_t type, const char* dst)
            : JackRequest(JackRequest::kSessionNotify), fEventType(type), fRefNum(refnum)
    {
        snprintf(fPath, sizeof(fPath), "%s", path);
        if (dst) {
            snprintf(fDst, sizeof(fDst), "%s", dst);
        } else {
            fDst[0] = '\0';
        }
    }

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckSize();
        CheckRes(trans->Read(&fRefNum, sizeof(fRefNum)));
        CheckRes(trans->Read(&fPath, sizeof(fPath)));
        CheckRes(trans->Read(&fDst, sizeof(fDst)));
        CheckRes(trans->Read(&fEventType, sizeof(fEventType)));
        return 0;
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackRequest::Write(trans, Size()));
        CheckRes(trans->Write(&fRefNum, sizeof(fRefNum)));
        CheckRes(trans->Write(&fPath, sizeof(fPath)));
        CheckRes(trans->Write(&fDst, sizeof(fDst)));
        CheckRes(trans->Write(&fEventType, sizeof(fEventType)));
        return 0;
    }

    int Size() { return sizeof(fRefNum) + sizeof(fPath) + sizeof(fDst) + sizeof(fEventType); }
};

struct JackSessionReplyRequest : public JackRequest
{
    int fRefNum;

    JackSessionReplyRequest()
    {}

    JackSessionReplyRequest(int refnum)
            : JackRequest(JackRequest::kSessionReply), fRefNum(refnum)
    {}

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckSize();
        CheckRes(trans->Read(&fRefNum, sizeof(int)));
        return 0;
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackRequest::Write(trans, Size()));
        CheckRes(trans->Write(&fRefNum, sizeof(int)));
        return 0;
    }

    int Size() { return sizeof(int); }
    
};

struct JackClientNameResult : public JackResult
{
    char fName[JACK_CLIENT_NAME_SIZE + 1];

    JackClientNameResult(): JackResult()
    {}
    JackClientNameResult(int32_t result, const char* name)
            : JackResult(result)
    {
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
   
};

struct JackUUIDResult : public JackResult
{
    char fUUID[JACK_UUID_SIZE];

    JackUUIDResult(): JackResult()
    {}
    JackUUIDResult(int32_t result, const char* uuid)
            : JackResult(result)
    {
        snprintf(fUUID, sizeof(fUUID), "%s", uuid);
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

struct JackGetUUIDRequest : public JackRequest
{
    char fName[JACK_CLIENT_NAME_SIZE + 1];

    JackGetUUIDRequest()
    {}

    JackGetUUIDRequest(const char* client_name)
            : JackRequest(JackRequest::kGetUUIDByClient)
    {
        strncpy(fName, client_name, sizeof(fName));
    }

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckSize();
        CheckRes(trans->Read(&fName, sizeof(fName)));
        return 0;
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackRequest::Write(trans, Size()));
        CheckRes(trans->Write(&fName, sizeof(fName)));
        return 0;
    }

    int Size() { return sizeof(fName); }
    
};

struct JackGetClientNameRequest : public JackRequest
{
    char fUUID[JACK_UUID_SIZE];

    JackGetClientNameRequest()
    {}

    JackGetClientNameRequest(const char* uuid)
            : JackRequest(JackRequest::kGetClientByUUID)
    {
        strncpy(fUUID, uuid, sizeof(fUUID));
    }

    int Read(detail::JackChannelTransactionInterface* trans)
    {   
        CheckSize();
        CheckRes(trans->Read(&fUUID, sizeof(fUUID)));
        return 0;
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackRequest::Write(trans, Size()));
        CheckRes(trans->Write(&fUUID, sizeof(fUUID)));
        return 0;
    }

    int Size() { return sizeof(fUUID); }
    
};

struct JackReserveNameRequest : public JackRequest
{
    int  fRefNum;
    char fName[JACK_CLIENT_NAME_SIZE + 1];
    char fUUID[JACK_UUID_SIZE];

    JackReserveNameRequest()
    {}

    JackReserveNameRequest(int refnum, const char *name, const char* uuid)
            : JackRequest(JackRequest::kReserveClientName), fRefNum(refnum)
    {
        strncpy(fName, name, sizeof(fName));
        strncpy(fUUID, uuid, sizeof(fUUID));
    }

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckSize();
        CheckRes(trans->Read(&fUUID, sizeof(fUUID)));
        CheckRes(trans->Read(&fName, sizeof(fName)));
        CheckRes(trans->Read(&fRefNum, sizeof(fRefNum)));
        return 0;
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackRequest::Write(trans, Size()));
        CheckRes(trans->Write(&fUUID, sizeof(fUUID)));
        CheckRes(trans->Write(&fName, sizeof(fName)));
        CheckRes(trans->Write(&fRefNum, sizeof(fRefNum)));
        return 0;
    }

    int Size() { return sizeof(fUUID) + sizeof(fName) + sizeof(fRefNum); }
    
};

struct JackClientHasSessionCallbackRequest : public JackRequest
{
    char fName[JACK_CLIENT_NAME_SIZE + 1];

    JackClientHasSessionCallbackRequest()
    {}

    JackClientHasSessionCallbackRequest(const char *name)
            : JackRequest(JackRequest::kClientHasSessionCallback)
    {
        strncpy(fName, name, sizeof(fName));
    }

    int Read(detail::JackChannelTransactionInterface* trans)
    {
        CheckSize();
        CheckRes(trans->Read(&fName, sizeof(fName)));
        return 0;
    }

    int Write(detail::JackChannelTransactionInterface* trans)
    {
        CheckRes(JackRequest::Write(trans, Size()));
        CheckRes(trans->Write(&fName, sizeof(fName)));
        return 0;
    }

    int Size() { return sizeof(fName); }
    
};

/*!
\brief ClientNotification.
*/

struct JackClientNotification
{
    int fSize; 
    char fName[JACK_CLIENT_NAME_SIZE + 1];
    int fRefNum;
    int fNotify;
    int fValue1;
    int fValue2;
    int fSync;
    char fMessage[JACK_MESSAGE_SIZE + 1];

    JackClientNotification(): fNotify(-1), fValue1(-1), fValue2(-1)
    {}
    JackClientNotification(const char* name, int refnum, int notify, int sync, const char* message, int value1, int value2)
            : fRefNum(refnum), fNotify(notify), fValue1(value1), fValue2(value2), fSync(sync)
    {
        snprintf(fName, sizeof(fName), "%s", name);
        snprintf(fMessage, sizeof(fMessage), "%s", message);
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
