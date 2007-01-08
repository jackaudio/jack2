
/*
    Copyright (C) 2001 Paul Davis 
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
 
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 
*/

#ifndef __JackRequest__
#define __JackRequest__

#include "JackPort.h"
#include "JackChannelTransaction.h"
#include "JackError.h"
#include <stdio.h>

namespace Jack
{

/*!
\brief Request from client to server.
*/

struct JackRequest
{

public:

    typedef enum {
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
        kClientNew = 22,
        kClientClose = 23,
        kConnectNamePorts = 24,
        kDisconnectNamePorts = 25,

        kNotification = 26
    } RequestType;

    RequestType fType;

    JackRequest()
    {}

    JackRequest(RequestType type): fType(type)
    {}

  	~JackRequest()
    {}

 	int Read(JackChannelTransaction* trans)
    {
        return trans->Read(this, sizeof(JackRequest));
    }

 	int Write(JackChannelTransaction* trans)
    {
        return -1;
    }

};

/*!
\brief Result from the server.
*/

struct JackResult
{

    int	fResult;

    JackResult(): fResult( -1)
    {}
    JackResult(int status): fResult(status)
    {}
    virtual ~JackResult()
    {}

    virtual int Read(JackChannelTransaction* trans)
    {
        return trans->Read(this, sizeof(JackResult));
    }

    virtual int Write(JackChannelTransaction* trans)
    {
        return trans->Write(this, sizeof(JackResult));
    }
};

/*!
\brief NewClient request.
*/

struct JackClientNewRequest 
{

	JackRequest fHeader;	
    char fName[JACK_CLIENT_NAME_SIZE + 1];

    JackClientNewRequest()
    {}
    JackClientNewRequest(const char* name): fHeader(JackRequest::kClientNew)
    {
        snprintf(fName, sizeof(fName), "%s", name);
    }

    int Read(JackChannelTransaction* trans)
    {
        return trans->Read(&fName, sizeof(JackClientNewRequest) - sizeof(JackRequest));
    }

    int Write(JackChannelTransaction* trans)
    {
        return trans->Write(this, sizeof(JackClientNewRequest));
    }
};

/*!
\brief NewClient result.
*/

struct JackClientNewResult 
{

	JackResult fHeader;
    int fSharedEngine;
    int fSharedClient;
    int fSharedGraph;
    uint32_t fProtocolVersion;

    JackClientNewResult()
		:fSharedEngine(-1), fSharedClient(-1), fSharedGraph(-1), fProtocolVersion(0)
    {}
    JackClientNewResult(int32_t status, int index1, int index2, int index3)
         : fHeader(status), fSharedEngine(index1), fSharedClient(index2), fSharedGraph(index3), fProtocolVersion(0)
    {}

    //virtual int Read(JackChannelTransaction* trans)
	int Read(JackChannelTransaction* trans)
    {
        return trans->Read(this, sizeof(JackClientNewResult));
    }

    int Write(JackChannelTransaction* trans)
    {
        return trans->Write(this, sizeof(JackClientNewResult));
    }
};

/*!
\brief CloseClient request.
*/

struct JackClientCloseRequest
{

	JackRequest fHeader;
    int fRefNum;

    JackClientCloseRequest()
    {}
    JackClientCloseRequest(int refnum): fHeader(JackRequest::kClientClose), fRefNum(refnum)
    {}

    int Read(JackChannelTransaction* trans)
    {
        return trans->Read(&fRefNum, sizeof(JackClientCloseRequest) - sizeof(JackRequest));
    }

    int Write(JackChannelTransaction* trans)
    {
        return trans->Write(this, sizeof(JackClientCloseRequest));
    }
};

/*!
\brief Activate request.
*/

struct JackActivateRequest
{

	JackRequest fHeader;
    int fRefNum;

    JackActivateRequest()
    {}
    JackActivateRequest(int refnum): fHeader(JackRequest::kActivateClient), fRefNum(refnum)
    {}

    int Read(JackChannelTransaction* trans)
    {
        return trans->Read(&fRefNum, sizeof(JackActivateRequest) - sizeof(JackRequest));
    }

    int Write(JackChannelTransaction* trans)
    {
        return trans->Write(this, sizeof(JackActivateRequest));
    }

};

/*!
\brief Deactivate request.
*/

struct JackDeactivateRequest
{

	JackRequest fHeader;
    int fRefNum;

    JackDeactivateRequest()
    {}
    JackDeactivateRequest(int refnum): fHeader(JackRequest::kDeactivateClient), fRefNum(refnum)
    {}

    int Read(JackChannelTransaction* trans)
    {
        return trans->Read(&fRefNum, sizeof(JackDeactivateRequest) - sizeof(JackRequest));
    }

    int Write(JackChannelTransaction* trans)
    {
        return trans->Write(this, sizeof(JackDeactivateRequest));
    }

};

/*!
\brief PortRegister request.
*/

struct JackPortRegisterRequest
{

	JackRequest fHeader;
    int fRefNum;
    char fName[JACK_PORT_NAME_SIZE + 1];
    char fPortType[JACK_PORT_TYPE_SIZE + 1];
    unsigned int fFlags;
    unsigned int fBufferSize;

    JackPortRegisterRequest()
    {}
    JackPortRegisterRequest(int refnum, const char* name, const char* port_type, unsigned int flags, unsigned int buffer_size)
            : fHeader(JackRequest::kRegisterPort), fRefNum(refnum), fFlags(flags), fBufferSize(buffer_size)
    {
        strcpy(fName, name);
        strcpy(fPortType, port_type);
    }

    int Read(JackChannelTransaction* trans)
    {
        return trans->Read(&fRefNum, sizeof(JackPortRegisterRequest) - sizeof(JackRequest)) ;
    }

    int Write(JackChannelTransaction* trans)
    {
        return trans->Write(this, sizeof(JackPortRegisterRequest));
    }
};

/*!
\brief PortRegister result.
*/

struct JackPortRegisterResult
{

	JackResult fHeader;
    jack_port_id_t fPortIndex;

    JackPortRegisterResult(): fPortIndex(NO_PORT)
    {}

    int Read(JackChannelTransaction* trans)
    {
        return trans->Read(this, sizeof(JackPortRegisterResult));
    }

    int Write(JackChannelTransaction* trans)
    {
        return trans->Write(this, sizeof(JackPortRegisterResult));
    }
};

/*!
\brief PortUnregister request.
*/

struct JackPortUnRegisterRequest
{

	JackRequest fHeader;
    int fRefNum;
    int fPortIndex;

    JackPortUnRegisterRequest()
    {}
    JackPortUnRegisterRequest(int refnum, int index): fHeader(JackRequest::kUnRegisterPort), fRefNum(refnum), fPortIndex(index)
    {}

    int Read(JackChannelTransaction* trans)
    {
        return trans->Read(&fRefNum, sizeof(JackPortUnRegisterRequest) - sizeof(JackRequest));
    }

    int Write(JackChannelTransaction* trans)
    {
        return trans->Write(this, sizeof(JackPortUnRegisterRequest));
    }
};

/*!
\brief PortConnectName request.
*/

struct JackPortConnectNameRequest
{
	
	JackRequest fHeader;
    int fRefNum;
    char fSrc[JACK_PORT_NAME_SIZE + 1];
    char fDst[JACK_PORT_NAME_SIZE + 1];

    JackPortConnectNameRequest()
    {}
    JackPortConnectNameRequest(int refnum, const char* src_name, const char* dst_name): fHeader(JackRequest::kConnectNamePorts), fRefNum(refnum)
    {
        strcpy(fSrc, src_name);
        strcpy(fDst, dst_name);
    }

    int Read(JackChannelTransaction* trans)
    {
        return trans->Read(&fRefNum, sizeof(JackPortConnectNameRequest) - sizeof(JackRequest));
    }

    int Write(JackChannelTransaction* trans)
    {
        return trans->Write(this, sizeof(JackPortConnectNameRequest));
    }
};

/*!
\brief PortDisconnectName request.
*/

struct JackPortDisconnectNameRequest
{

	JackRequest fHeader;
    int fRefNum;
    char fSrc[JACK_PORT_NAME_SIZE + 1];
    char fDst[JACK_PORT_NAME_SIZE + 1];

    JackPortDisconnectNameRequest()
    {}
    JackPortDisconnectNameRequest(int refnum, const char* src_name, const char* dst_name): fHeader(JackRequest::kDisconnectNamePorts), fRefNum(refnum)
    {
        strcpy(fSrc, src_name);
        strcpy(fDst, dst_name);
    }

    int Read(JackChannelTransaction* trans)
    {
        return trans->Read(&fRefNum, sizeof(JackPortDisconnectNameRequest) - sizeof(JackRequest));
    }

    int Write(JackChannelTransaction* trans)
    {
        return trans->Write(this, sizeof(JackPortDisconnectNameRequest));
    }
};

/*!
\brief PortConnect request.
*/

struct JackPortConnectRequest
{

	JackRequest fHeader;
    int fRefNum;
    jack_port_id_t fSrc;
    jack_port_id_t fDst;

    JackPortConnectRequest()
    {}
    JackPortConnectRequest(int refnum, jack_port_id_t src, jack_port_id_t dst): fHeader(JackRequest::kConnectPorts), fRefNum(refnum), fSrc(src), fDst(dst)
    {}

    int Read(JackChannelTransaction* trans)
    {
        return trans->Read(&fRefNum, sizeof(JackPortConnectRequest) - sizeof(JackRequest));
    }

    int Write(JackChannelTransaction* trans)
    {
        return trans->Write(this, sizeof(JackPortConnectRequest));
    }
};


/*!
\brief PortDisconnect request.
*/

struct JackPortDisconnectRequest
{

	JackRequest fHeader;
    int fRefNum;
    jack_port_id_t fSrc;
    jack_port_id_t fDst;

    JackPortDisconnectRequest()
    {}
    JackPortDisconnectRequest(int refnum, jack_port_id_t src, jack_port_id_t dst): fHeader(JackRequest::kDisconnectPorts), fRefNum(refnum), fSrc(src), fDst(dst)
    {}

    int Read(JackChannelTransaction* trans)
    {
        return trans->Read(&fRefNum, sizeof(JackPortDisconnectRequest) - sizeof(JackRequest));
    }

    int Write(JackChannelTransaction* trans)
    {
        return trans->Write(this, sizeof(JackPortDisconnectRequest));
    }
};

/*!
\brief SetBufferSize request.
*/

struct JackSetBufferSizeRequest
{

	JackRequest fHeader;
    jack_nframes_t fBufferSize;

    JackSetBufferSizeRequest()
    {}
    JackSetBufferSizeRequest(jack_nframes_t buffer_size): fHeader(JackRequest::kSetBufferSize), fBufferSize(buffer_size)
    {}

    int Read(JackChannelTransaction* trans)
    {
        return trans->Read(&fBufferSize, sizeof(JackSetBufferSizeRequest) - sizeof(JackRequest));
    }

    int Write(JackChannelTransaction* trans)
    {
        return trans->Write(this, sizeof(JackSetBufferSizeRequest));
    }
};

/*!
\brief SetFreeWheel request.
*/

struct JackSetFreeWheelRequest
{

	JackRequest fHeader;
    int fOnOff;

    JackSetFreeWheelRequest()
    {}
    JackSetFreeWheelRequest(int onoff): fHeader(JackRequest::kSetFreeWheel), fOnOff(onoff)
    {}

    int Read(JackChannelTransaction* trans)
    {
        return trans->Read(&fOnOff, sizeof(JackSetFreeWheelRequest) - sizeof(JackRequest));
    }

    int Write(JackChannelTransaction* trans)
    {
        return trans->Write(this, sizeof(JackSetFreeWheelRequest));
    }
};

/*!
\brief ReleaseTimebase request.
*/

struct JackReleaseTimebaseRequest
{

	JackRequest fHeader;
    int fRefNum;

    JackReleaseTimebaseRequest()
    {}
    JackReleaseTimebaseRequest(int refnum): fHeader(JackRequest::kReleaseTimebase), fRefNum(refnum)
    {}

    int Read(JackChannelTransaction* trans)
    {
        return trans->Read(&fRefNum, sizeof(JackReleaseTimebaseRequest) - sizeof(JackRequest));
    }

    int Write(JackChannelTransaction* trans)
    {
        return trans->Write(this, sizeof(JackReleaseTimebaseRequest));
    }

};

/*!
\brief SetTimebaseCallback request.
*/

struct JackSetTimebaseCallbackRequest
{

	JackRequest fHeader;
    int fRefNum;
    int fConditionnal;

    JackSetTimebaseCallbackRequest()
    {}
    JackSetTimebaseCallbackRequest(int refnum, int conditional): fHeader(JackRequest::kSetTimebaseCallback), fRefNum(refnum), fConditionnal(conditional)
    {}

    int Read(JackChannelTransaction* trans)
    {
        return trans->Read(&fRefNum, sizeof(JackSetTimebaseCallbackRequest) - sizeof(JackRequest));
    }

    int Write(JackChannelTransaction* trans)
    {
        return trans->Write(this, sizeof(JackSetTimebaseCallbackRequest));
    }
};

/*!
\brief ClientNotification request.
*/

struct JackClientNotificationRequest
{
	
	JackRequest fHeader;
    int fRefNum;
    int fNotify;
    int fValue;

    JackClientNotificationRequest()
    {}
    JackClientNotificationRequest(int refnum, int notify, int value)
            : fHeader(JackRequest::kNotification), fRefNum(refnum), fNotify(notify), fValue(value)
    {}

    int Read(JackChannelTransaction* trans)
    {
        return trans->Read(&fRefNum, sizeof(JackClientNotificationRequest) - sizeof(JackRequest));
    }

    int Write(JackChannelTransaction* trans)
    {
        return trans->Write(this, sizeof(JackClientNotificationRequest));
    }

};

/*!
\brief ClientNotification.
*/

struct JackClientNotification
{
    char fName[JACK_CLIENT_NAME_SIZE + 1];
    int fRefNum;
    int fNotify;
    int fValue;
    int fSync;

    JackClientNotification(): fNotify( -1), fValue( -1)
    {}
    JackClientNotification(const char* name, int refnum, int notify, int sync, int value)
            : fRefNum(refnum), fNotify(notify), fValue(value), fSync(sync)
    {
        snprintf(fName, sizeof(fName), "%s", name);
    }

    int Read(JackChannelTransaction* trans)
    {
        return trans->Read(this, sizeof(JackClientNotification));
    }

    int Write(JackChannelTransaction* trans)
    {
        return trans->Write(this, sizeof(JackClientNotification));
    }

};

} // end of namespace

#endif
