

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

  	virtual ~JackRequest()
    {}

 	virtual int Read(JackChannelTransaction* trans)
    {
        //return trans->Read(this, sizeof(JackRequest));
		return trans->Read(&fType, sizeof(RequestType));
    }

 	virtual int Write(JackChannelTransaction* trans)
    {
        //return -1;
		return trans->Write(&fType, sizeof(RequestType));
    }

};

#define CheckRes(res) {if (res < 0) return res;}

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
        //return trans->Read(this, sizeof(JackResult));
		return trans->Read(&fResult, sizeof(int));
    }

    virtual int Write(JackChannelTransaction* trans)
    {
        //return trans->Write(this, sizeof(JackResult));
		return trans->Write(&fResult, sizeof(int));
    }
};

/*!
\brief NewClient request.
*/

struct JackClientNewRequest : public JackRequest
{

    char fName[JACK_CLIENT_NAME_SIZE + 1];

    JackClientNewRequest()
    {}
    JackClientNewRequest(const char* name): JackRequest(JackRequest::kClientNew)
    {
        snprintf(fName, sizeof(fName), "%s", name);
    }

    int Read(JackChannelTransaction* trans)
    {
        //return trans->Read(&fName, sizeof(JackClientNewRequest) - sizeof(JackRequest));
		return trans->Read(&fName, JACK_CLIENT_NAME_SIZE + 1);
    }

    int Write(JackChannelTransaction* trans)
    {
        //return trans->Write(this, sizeof(JackClientNewRequest));
		CheckRes(JackRequest::Write(trans));
		return trans->Write(&fName, JACK_CLIENT_NAME_SIZE + 1);
    }
};

/*!
\brief NewClient result.
*/

struct JackClientNewResult : public JackResult
{

	//JackResult fHeader;
    int fSharedEngine;
    int fSharedClient;
    int fSharedGraph;
    uint32_t fProtocolVersion;

    JackClientNewResult()
		:fSharedEngine(-1), fSharedClient(-1), fSharedGraph(-1), fProtocolVersion(0)
    {}
    JackClientNewResult(int32_t status, int index1, int index2, int index3)
         : JackResult(status), fSharedEngine(index1), fSharedClient(index2), fSharedGraph(index3), fProtocolVersion(0)
    {}

   int Read(JackChannelTransaction* trans)
    {
        //return trans->Read(this, sizeof(JackClientNewResult));
		CheckRes(trans->Read(&fSharedEngine, sizeof(int)));
		CheckRes(trans->Read(&fSharedClient, sizeof(int)));
		CheckRes(trans->Read(&fSharedGraph, sizeof(int)));
		CheckRes(trans->Read(&fProtocolVersion, sizeof(uint32_t)));
		return 0;
    }

    int Write(JackChannelTransaction* trans)
    {
        //return trans->Write(this, sizeof(JackClientNewResult));
		CheckRes(JackResult::Write(trans));
		CheckRes(trans->Write(&fSharedEngine, sizeof(int)));
		CheckRes(trans->Write(&fSharedClient, sizeof(int)));
		CheckRes(trans->Write(&fSharedGraph, sizeof(int)));
		CheckRes(trans->Write(&fProtocolVersion, sizeof(uint32_t)));
		return 0;
    }
};

/*!
\brief CloseClient request.
*/

struct JackClientCloseRequest : public JackRequest
{

	//JackRequest fHeader;
    int fRefNum;

    JackClientCloseRequest()
    {}
    JackClientCloseRequest(int refnum): JackRequest(JackRequest::kClientClose), fRefNum(refnum)
    {}

    int Read(JackChannelTransaction* trans)
    {
        //return trans->Read(&fRefNum, sizeof(JackClientCloseRequest) - sizeof(JackRequest));
		return trans->Read(&fRefNum, sizeof(int));
    }

    int Write(JackChannelTransaction* trans)
    {
        //return trans->Write(this, sizeof(JackClientCloseRequest));
		CheckRes(JackRequest::Write(trans));
		return trans->Write(&fRefNum, sizeof(int));
    }
};

/*!
\brief Activate request.
*/

struct JackActivateRequest : public JackRequest
{

	//JackRequest fHeader;
    int fRefNum;

    JackActivateRequest()
    {}
    JackActivateRequest(int refnum): JackRequest(JackRequest::kActivateClient), fRefNum(refnum)
    {}

    int Read(JackChannelTransaction* trans)
    {
		// return trans->Read(&fRefNum, sizeof(JackActivateRequest) - sizeof(JackRequest));
		return trans->Read(&fRefNum, sizeof(int));
    }

    int Write(JackChannelTransaction* trans)
    {
        //return trans->Write(this, sizeof(JackActivateRequest));
		CheckRes(JackRequest::Write(trans));
		return trans->Write(&fRefNum, sizeof(int));
    }

};

/*!
\brief Deactivate request.
*/

struct JackDeactivateRequest : public JackRequest
{

	//JackRequest fHeader;
    int fRefNum;

    JackDeactivateRequest()
    {}
    JackDeactivateRequest(int refnum): JackRequest(JackRequest::kDeactivateClient), fRefNum(refnum)
    {}

    int Read(JackChannelTransaction* trans)
    {
        //return trans->Read(&fRefNum, sizeof(JackDeactivateRequest) - sizeof(JackRequest));
		return trans->Read(&fRefNum, sizeof(int));
    }

    int Write(JackChannelTransaction* trans)
    {
        //return trans->Write(this, sizeof(JackDeactivateRequest));
		CheckRes(JackRequest::Write(trans));
		return trans->Write(&fRefNum, sizeof(int));
    }

};

/*!
\brief PortRegister request.
*/

struct JackPortRegisterRequest : public JackRequest
{

	//JackRequest fHeader;
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

    int Read(JackChannelTransaction* trans)
    {
        //return trans->Read(&fRefNum, sizeof(JackPortRegisterRequest) - sizeof(JackRequest));
		CheckRes(trans->Read(&fRefNum, sizeof(int)));
		CheckRes(trans->Read(&fName, sizeof(JACK_PORT_NAME_SIZE + 1)));
		CheckRes(trans->Read(&fPortType, sizeof(JACK_PORT_TYPE_SIZE + 1)));
		CheckRes(trans->Read(&fFlags, sizeof(unsigned int)));
		CheckRes(trans->Read(&fBufferSize, sizeof(unsigned int)));
		return 0;
    }

    int Write(JackChannelTransaction* trans)
    {
        //return trans->Write(this, sizeof(JackPortRegisterRequest));
		CheckRes(JackRequest::Write(trans));
		CheckRes(trans->Write(&fRefNum, sizeof(int)));
		CheckRes(trans->Write(&fName, sizeof(JACK_PORT_NAME_SIZE + 1)));
		CheckRes(trans->Write(&fPortType, sizeof(JACK_PORT_TYPE_SIZE + 1)));
		CheckRes(trans->Write(&fFlags, sizeof(unsigned int)));
		CheckRes(trans->Write(&fBufferSize, sizeof(unsigned int)));
		return 0;
    }
};

/*!
\brief PortRegister result.
*/

struct JackPortRegisterResult : public JackResult
{

	//JackResult fHeader;
    jack_port_id_t fPortIndex;

    JackPortRegisterResult(): fPortIndex(NO_PORT)
    {}

    int Read(JackChannelTransaction* trans)
    {
        //return trans->Read(this, sizeof(JackPortRegisterResult));
		return trans->Read(&fPortIndex, sizeof(jack_port_id_t));
    }

    int Write(JackChannelTransaction* trans)
    {
        //return trans->Write(this, sizeof(JackPortRegisterResult));
		CheckRes(JackResult::Write(trans));
		return trans->Read(&fPortIndex, sizeof(jack_port_id_t));
    }
};

/*!
\brief PortUnregister request.
*/

struct JackPortUnRegisterRequest : public JackRequest
{

	//JackRequest fHeader;
    int fRefNum;
    int fPortIndex;

    JackPortUnRegisterRequest()
    {}
    JackPortUnRegisterRequest(int refnum, int index): JackRequest(JackRequest::kUnRegisterPort), fRefNum(refnum), fPortIndex(index)
    {}

    int Read(JackChannelTransaction* trans)
    {
        //return trans->Read(&fRefNum, sizeof(JackPortUnRegisterRequest) - sizeof(JackRequest));
		CheckRes(trans->Read(&fRefNum, sizeof(int)));
		CheckRes(trans->Read(&fPortIndex, sizeof(int)));
		return 0;
    }

    int Write(JackChannelTransaction* trans)
    {
        //return trans->Write(this, sizeof(JackPortUnRegisterRequest));
		CheckRes(JackRequest::Write(trans));
		CheckRes(trans->Write(&fRefNum, sizeof(int)));
		CheckRes(trans->Write(&fPortIndex, sizeof(int)));
		return 0;
    }
};

/*!
\brief PortConnectName request.
*/

struct JackPortConnectNameRequest : public JackRequest
{
	
	//JackRequest fHeader;
    int fRefNum;
    char fSrc[JACK_PORT_NAME_SIZE + 1];
    char fDst[JACK_PORT_NAME_SIZE + 1];

    JackPortConnectNameRequest()
    {}
    JackPortConnectNameRequest(int refnum, const char* src_name, const char* dst_name): JackRequest(JackRequest::kConnectNamePorts), fRefNum(refnum)
    {
        strcpy(fSrc, src_name);
        strcpy(fDst, dst_name);
    }

    int Read(JackChannelTransaction* trans)
    {
        //return trans->Read(&fRefNum, sizeof(JackPortConnectNameRequest) - sizeof(JackRequest));
		CheckRes(trans->Read(&fRefNum, sizeof(int)));
		CheckRes(trans->Read(&fSrc, sizeof(JACK_PORT_NAME_SIZE + 1)));
		CheckRes(trans->Read(&fDst, sizeof(JACK_PORT_NAME_SIZE + 1)));
		return 0;

    }

    int Write(JackChannelTransaction* trans)
    {
        //return trans->Write(this, sizeof(JackPortConnectNameRequest));
		CheckRes(JackRequest::Write(trans));
		CheckRes(trans->Write(&fRefNum, sizeof(int)));
		CheckRes(trans->Write(&fSrc, sizeof(JACK_PORT_NAME_SIZE + 1)));
		CheckRes(trans->Write(&fDst, sizeof(JACK_PORT_NAME_SIZE + 1)));
		return 0;
    }
};

/*!
\brief PortDisconnectName request.
*/

struct JackPortDisconnectNameRequest : public JackRequest
{

	//JackRequest fHeader;
    int fRefNum;
    char fSrc[JACK_PORT_NAME_SIZE + 1];
    char fDst[JACK_PORT_NAME_SIZE + 1];

    JackPortDisconnectNameRequest()
    {}
    JackPortDisconnectNameRequest(int refnum, const char* src_name, const char* dst_name): JackRequest(JackRequest::kDisconnectNamePorts), fRefNum(refnum)
    {
        strcpy(fSrc, src_name);
        strcpy(fDst, dst_name);
    }

    int Read(JackChannelTransaction* trans)
    {
        //return trans->Read(&fRefNum, sizeof(JackPortDisconnectNameRequest) - sizeof(JackRequest));
		CheckRes(trans->Read(&fRefNum, sizeof(int)));
		CheckRes(trans->Read(&fSrc, sizeof(JACK_PORT_NAME_SIZE + 1)));
		CheckRes(trans->Read(&fDst, sizeof(JACK_PORT_NAME_SIZE + 1)));
		return 0;
    }

    int Write(JackChannelTransaction* trans)
    {
        //return trans->Write(this, sizeof(JackPortDisconnectNameRequest));
		CheckRes(JackRequest::Write(trans));
		CheckRes(trans->Write(&fRefNum, sizeof(int)));
		CheckRes(trans->Write(&fSrc, sizeof(JACK_PORT_NAME_SIZE + 1)));
		CheckRes(trans->Write(&fDst, sizeof(JACK_PORT_NAME_SIZE + 1)));
		return 0;
    }
};

/*!
\brief PortConnect request.
*/

struct JackPortConnectRequest : public JackRequest
{

	//JackRequest fHeader;
    int fRefNum;
    jack_port_id_t fSrc;
    jack_port_id_t fDst;

    JackPortConnectRequest()
    {}
    JackPortConnectRequest(int refnum, jack_port_id_t src, jack_port_id_t dst): JackRequest(JackRequest::kConnectPorts), fRefNum(refnum), fSrc(src), fDst(dst)
    {}

    int Read(JackChannelTransaction* trans)
    {
        //return trans->Read(&fRefNum, sizeof(JackPortConnectRequest) - sizeof(JackRequest));
		CheckRes(trans->Read(&fRefNum, sizeof(int)));
		CheckRes(trans->Read(&fSrc, sizeof(jack_port_id_t)));
		CheckRes(trans->Read(&fDst, sizeof(jack_port_id_t)));
		return 0;
    }

    int Write(JackChannelTransaction* trans)
    {
        //return trans->Write(this, sizeof(JackPortConnectRequest));
		CheckRes(JackRequest::Write(trans));
		CheckRes(trans->Write(&fRefNum, sizeof(int)));
		CheckRes(trans->Write(&fSrc, sizeof(jack_port_id_t)));
		CheckRes(trans->Write(&fDst, sizeof(jack_port_id_t)));
		return 0;
    }
};


/*!
\brief PortDisconnect request.
*/

struct JackPortDisconnectRequest : public JackRequest
{

	//JackRequest fHeader;
    int fRefNum;
    jack_port_id_t fSrc;
    jack_port_id_t fDst;

    JackPortDisconnectRequest()
    {}
    JackPortDisconnectRequest(int refnum, jack_port_id_t src, jack_port_id_t dst): JackRequest(JackRequest::kDisconnectPorts), fRefNum(refnum), fSrc(src), fDst(dst)
    {}

    int Read(JackChannelTransaction* trans)
    {
		//return trans->Read(&fRefNum, sizeof(JackPortConnectRequest) - sizeof(JackRequest));
		CheckRes(trans->Read(&fRefNum, sizeof(int)));
		CheckRes(trans->Read(&fSrc, sizeof(jack_port_id_t)));
		CheckRes(trans->Read(&fDst, sizeof(jack_port_id_t)));
		return 0;
    }

    int Write(JackChannelTransaction* trans)
    {
        //return trans->Write(this, sizeof(JackPortConnectRequest));
		CheckRes(JackRequest::Write(trans));
		CheckRes(trans->Write(&fRefNum, sizeof(int)));
		CheckRes(trans->Write(&fSrc, sizeof(jack_port_id_t)));
		CheckRes(trans->Write(&fDst, sizeof(jack_port_id_t)));
		return 0;

    }
};

/*!
\brief SetBufferSize request.
*/

struct JackSetBufferSizeRequest : public JackRequest
{

	//JackRequest fHeader;
    jack_nframes_t fBufferSize;

    JackSetBufferSizeRequest()
    {}
    JackSetBufferSizeRequest(jack_nframes_t buffer_size): JackRequest(JackRequest::kSetBufferSize), fBufferSize(buffer_size)
    {}

    int Read(JackChannelTransaction* trans)
    {
        //return trans->Read(&fBufferSize, sizeof(JackSetBufferSizeRequest) - sizeof(JackRequest));
		return trans->Read(&fBufferSize, sizeof(jack_nframes_t));
    }

    int Write(JackChannelTransaction* trans)
    {
        //return trans->Write(this, sizeof(JackSetBufferSizeRequest));
		CheckRes(JackRequest::Write(trans));
		return trans->Write(&fBufferSize, sizeof(jack_nframes_t));
    }
};

/*!
\brief SetFreeWheel request.
*/

struct JackSetFreeWheelRequest : public JackRequest
{

	//JackRequest fHeader;
    int fOnOff;

    JackSetFreeWheelRequest()
    {}
    JackSetFreeWheelRequest(int onoff): JackRequest(JackRequest::kSetFreeWheel), fOnOff(onoff)
    {}

    int Read(JackChannelTransaction* trans)
    {
        //return trans->Read(&fOnOff, sizeof(JackSetFreeWheelRequest) - sizeof(JackRequest));
		return trans->Read(&fOnOff, sizeof(int));
    }

    int Write(JackChannelTransaction* trans)
    {
        //return trans->Write(this, sizeof(JackSetFreeWheelRequest));
		CheckRes(JackRequest::Write(trans));
		return trans->Write(&fOnOff, sizeof(int));
    }
};

/*!
\brief ReleaseTimebase request.
*/

struct JackReleaseTimebaseRequest : public JackRequest
{

	//JackRequest fHeader;
    int fRefNum;

    JackReleaseTimebaseRequest()
    {}
    JackReleaseTimebaseRequest(int refnum): JackRequest(JackRequest::kReleaseTimebase), fRefNum(refnum)
    {}

    int Read(JackChannelTransaction* trans)
    {
        //return trans->Read(&fRefNum, sizeof(JackReleaseTimebaseRequest) - sizeof(JackRequest));
		return trans->Read(&fRefNum, sizeof(int));
    }

    int Write(JackChannelTransaction* trans)
    {
        //return trans->Write(this, sizeof(JackReleaseTimebaseRequest));
		CheckRes(JackRequest::Write(trans));
		return trans->Write(&fRefNum, sizeof(int));
    }
};

/*!
\brief SetTimebaseCallback request.
*/

struct JackSetTimebaseCallbackRequest : public JackRequest
{

	//JackRequest fHeader;
    int fRefNum;
    int fConditionnal;

    JackSetTimebaseCallbackRequest()
    {}
    JackSetTimebaseCallbackRequest(int refnum, int conditional): JackRequest(JackRequest::kSetTimebaseCallback), fRefNum(refnum), fConditionnal(conditional)
    {}

    int Read(JackChannelTransaction* trans)
    {
        //return trans->Read(&fRefNum, sizeof(JackSetTimebaseCallbackRequest) - sizeof(JackRequest));
		CheckRes(trans->Read(&fRefNum, sizeof(int)));
		return trans->Read(&fConditionnal, sizeof(int));
    }

    int Write(JackChannelTransaction* trans)
    {
        //return trans->Write(this, sizeof(JackSetTimebaseCallbackRequest));
		CheckRes(JackRequest::Write(trans));
		CheckRes(trans->Write(&fRefNum, sizeof(int)));
		return trans->Write(&fConditionnal, sizeof(int));
    }
};

/*!
\brief ClientNotification request.
*/

struct JackClientNotificationRequest : public JackRequest
{
	
	//JackRequest fHeader;
    int fRefNum;
    int fNotify;
    int fValue;

    JackClientNotificationRequest()
    {}
    JackClientNotificationRequest(int refnum, int notify, int value)
            : JackRequest(JackRequest::kNotification), fRefNum(refnum), fNotify(notify), fValue(value)
    {}

    int Read(JackChannelTransaction* trans)
    {
        //return trans->Read(&fRefNum, sizeof(JackClientNotificationRequest) - sizeof(JackRequest));
		CheckRes(trans->Read(&fRefNum, sizeof(int)));
		CheckRes(trans->Read(&fNotify, sizeof(int)));
		CheckRes(trans->Read(&fValue, sizeof(int)));
		return 0;
    }

    int Write(JackChannelTransaction* trans)
    {
        //return trans->Write(this, sizeof(JackClientNotificationRequest));
		CheckRes(JackRequest::Write(trans));
		CheckRes(trans->Write(&fRefNum, sizeof(int)));
		CheckRes(trans->Write(&fNotify, sizeof(int)));
		CheckRes(trans->Write(&fValue, sizeof(int)));
		return 0;
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
        //return trans->Read(this, sizeof(JackClientNotification));
		CheckRes(trans->Read(&fName, sizeof(JACK_CLIENT_NAME_SIZE + 1)));
		CheckRes(trans->Read(&fRefNum, sizeof(int)));
		CheckRes(trans->Read(&fNotify, sizeof(int)));
		CheckRes(trans->Read(&fValue, sizeof(int)));
		CheckRes(trans->Read(&fSync, sizeof(int)));
		return 0;
    }

    int Write(JackChannelTransaction* trans)
    {
        //return trans->Write(this, sizeof(JackClientNotification));
		CheckRes(trans->Write(&fName, sizeof(JACK_CLIENT_NAME_SIZE + 1)));
		CheckRes(trans->Write(&fRefNum, sizeof(int)));
		CheckRes(trans->Write(&fNotify, sizeof(int)));
		CheckRes(trans->Write(&fValue, sizeof(int)));
		CheckRes(trans->Write(&fSync, sizeof(int)));
		return 0;
    }

};

} // end of namespace

#endif
