/*
Copyright (C) 2012 Grame

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

#include "JackRequestDecoder.h"
#include "JackServer.h"
#include "JackLockedEngine.h"
#include "JackChannel.h"

#include <assert.h>
#include <signal.h>

using namespace std;

namespace Jack
{

#define CheckRead(req, socket)          { if (req.Read(socket) <  0) { jack_error("CheckRead error"); return -1; } }
#define CheckWriteName(error, socket)   { if (res.Write(socket) < 0) { jack_error("%s write error name = %s", error, req.fName); } }
#define CheckWriteRefNum(error, socket) { if (res.Write(socket) < 0) { jack_error("%s write error ref = %d", error, req.fRefNum); } }
#define CheckWrite(error, socket)       { if (res.Write(socket) < 0) { jack_error("%s write error", error); } }

JackRequestDecoder::JackRequestDecoder(JackServer* server, JackClientHandlerInterface* handler)
    :fServer(server), fHandler(handler)
{}

JackRequestDecoder::~JackRequestDecoder()
{}

int JackRequestDecoder::HandleRequest(detail::JackChannelTransactionInterface* socket, int type_aux)
{
    JackRequest::RequestType type = (JackRequest::RequestType)type_aux;

    // Read data
    switch (type) {

        case JackRequest::kClientCheck: {
            jack_log("JackRequest::ClientCheck");
            JackClientCheckRequest req;
            JackClientCheckResult res;
            CheckRead(req, socket);
            res.fResult = fServer->GetEngine()->ClientCheck(req.fName, req.fUUID, res.fName, req.fProtocol, req.fOptions, &res.fStatus);
            CheckWriteName("JackRequest::ClientCheck", socket);
            // Atomic ClientCheck followed by ClientOpen on same socket
            if (req.fOpen) {
                JackRequest header;
                header.Read(socket);
                return HandleRequest(socket, header.fType);
            }
            break;
        }

        case JackRequest::kClientOpen: {
            jack_log("JackRequest::ClientOpen");
            JackClientOpenRequest req;
            JackClientOpenResult res;
            CheckRead(req, socket);
            fHandler->ClientAdd(socket, &req, &res);
            CheckWriteName("JackRequest::ClientOpen", socket);
            break;
        }

        case JackRequest::kClientClose: {
            jack_log("JackRequest::ClientClose");
            JackClientCloseRequest req;
            JackResult res;
            CheckRead(req, socket);
            res.fResult = fServer->GetEngine()->ClientExternalClose(req.fRefNum);
            CheckWriteRefNum("JackRequest::ClientClose", socket);
            fHandler->ClientRemove(socket, req.fRefNum);
            // Will cause the wrapping thread to stop
            return -1;
        }

        case JackRequest::kActivateClient: {
            JackActivateRequest req;
            JackResult res;
            jack_log("JackRequest::ActivateClient");
            CheckRead(req, socket);
            res.fResult = fServer->GetEngine()->ClientActivate(req.fRefNum, req.fIsRealTime);
            CheckWriteRefNum("JackRequest::ActivateClient", socket);
            break;
        }

        case JackRequest::kDeactivateClient: {
            jack_log("JackRequest::DeactivateClient");
            JackDeactivateRequest req;
            JackResult res;
            CheckRead(req, socket);
            res.fResult = fServer->GetEngine()->ClientDeactivate(req.fRefNum);
            CheckWriteRefNum("JackRequest::DeactivateClient", socket);
            break;
        }

        case JackRequest::kRegisterPort: {
            jack_log("JackRequest::RegisterPort");
            JackPortRegisterRequest req;
            JackPortRegisterResult res;
            CheckRead(req, socket);
            res.fResult = fServer->GetEngine()->PortRegister(req.fRefNum, req.fName, req.fPortType, req.fFlags, req.fBufferSize, &res.fPortIndex);
            CheckWriteRefNum("JackRequest::RegisterPort", socket);
            break;
        }

        case JackRequest::kUnRegisterPort: {
            jack_log("JackRequest::UnRegisterPort");
            JackPortUnRegisterRequest req;
            JackResult res;
            CheckRead(req, socket);
            res.fResult = fServer->GetEngine()->PortUnRegister(req.fRefNum, req.fPortIndex);
            CheckWriteRefNum("JackRequest::UnRegisterPort", socket);
            break;
        }

        case JackRequest::kConnectNamePorts: {
            jack_log("JackRequest::ConnectNamePorts");
            JackPortConnectNameRequest req;
            JackResult res;
            CheckRead(req, socket);
            res.fResult = fServer->GetEngine()->PortConnect(req.fRefNum, req.fSrc, req.fDst);
            CheckWriteRefNum("JackRequest::ConnectNamePorts", socket);
            break;
        }

        case JackRequest::kDisconnectNamePorts: {
            jack_log("JackRequest::DisconnectNamePorts");
            JackPortDisconnectNameRequest req;
            JackResult res;
            CheckRead(req, socket);
            res.fResult = fServer->GetEngine()->PortDisconnect(req.fRefNum, req.fSrc, req.fDst);
            CheckWriteRefNum("JackRequest::DisconnectNamePorts", socket);
            break;
        }

        case JackRequest::kConnectPorts: {
            jack_log("JackRequest::ConnectPorts");
            JackPortConnectRequest req;
            JackResult res;
            CheckRead(req, socket);
            res.fResult = fServer->GetEngine()->PortConnect(req.fRefNum, req.fSrc, req.fDst);
            CheckWriteRefNum("JackRequest::ConnectPorts", socket);
            break;
        }

        case JackRequest::kDisconnectPorts: {
            jack_log("JackRequest::DisconnectPorts");
            JackPortDisconnectRequest req;
            JackResult res;
            CheckRead(req, socket);
            res.fResult = fServer->GetEngine()->PortDisconnect(req.fRefNum, req.fSrc, req.fDst);
            CheckWriteRefNum("JackRequest::DisconnectPorts", socket);
            break;
        }

        case JackRequest::kPortRename: {
            jack_log("JackRequest::PortRename");
            JackPortRenameRequest req;
            JackResult res;
            CheckRead(req, socket);
            res.fResult = fServer->GetEngine()->PortRename(req.fRefNum, req.fPort, req.fName);
            CheckWriteRefNum("JackRequest::PortRename", socket);
            break;
        }

        case JackRequest::kSetBufferSize: {
            jack_log("JackRequest::SetBufferSize");
            JackSetBufferSizeRequest req;
            JackResult res;
            CheckRead(req, socket);
            res.fResult = fServer->SetBufferSize(req.fBufferSize);
            CheckWrite("JackRequest::SetBufferSize", socket);
            break;
        }

        case JackRequest::kSetFreeWheel: {
            jack_log("JackRequest::SetFreeWheel");
            JackSetFreeWheelRequest req;
            JackResult res;
            CheckRead(req, socket);
            res.fResult = fServer->SetFreewheel(req.fOnOff);
            CheckWrite("JackRequest::SetFreeWheel", socket);
            break;
        }

         case JackRequest::kComputeTotalLatencies: {
            jack_log("JackRequest::ComputeTotalLatencies");
            JackComputeTotalLatenciesRequest req;
            JackResult res;
            CheckRead(req, socket);
            res.fResult = fServer->GetEngine()->ComputeTotalLatencies();
            CheckWrite("JackRequest::ComputeTotalLatencies", socket);
            break;
        }

        case JackRequest::kReleaseTimebase: {
            jack_log("JackRequest::ReleaseTimebase");
            JackReleaseTimebaseRequest req;
            JackResult res;
            CheckRead(req, socket);
            res.fResult = fServer->ReleaseTimebase(req.fRefNum);
            CheckWriteRefNum("JackRequest::ReleaseTimebase", socket);
            break;
        }

        case JackRequest::kSetTimebaseCallback: {
            jack_log("JackRequest::SetTimebaseCallback");
            JackSetTimebaseCallbackRequest req;
            JackResult res;
            CheckRead(req, socket);
            res.fResult = fServer->SetTimebaseCallback(req.fRefNum, req.fConditionnal);
            CheckWriteRefNum("JackRequest::SetTimebaseCallback", socket);
            break;
        }

        case JackRequest::kGetInternalClientName: {
            jack_log("JackRequest::GetInternalClientName");
            JackGetInternalClientNameRequest req;
            JackGetInternalClientNameResult res;
            CheckRead(req, socket);
            res.fResult = fServer->GetEngine()->GetInternalClientName(req.fIntRefNum, res.fName);
            CheckWriteRefNum("JackRequest::GetInternalClientName", socket);
            break;
        }

        case JackRequest::kInternalClientHandle: {
            jack_log("JackRequest::InternalClientHandle");
            JackInternalClientHandleRequest req;
            JackInternalClientHandleResult res;
            CheckRead(req, socket);
            res.fResult = fServer->GetEngine()->InternalClientHandle(req.fName, &res.fStatus, &res.fIntRefNum);
            CheckWriteRefNum("JackRequest::InternalClientHandle", socket);
            break;
        }

        case JackRequest::kInternalClientLoad: {
            jack_log("JackRequest::InternalClientLoad");
            JackInternalClientLoadRequest req;
            JackInternalClientLoadResult res;
            CheckRead(req, socket);
            res.fResult = fServer->InternalClientLoad1(req.fName, req.fDllName, req.fLoadInitName, req.fOptions, &res.fIntRefNum, req.fUUID, &res.fStatus);
            CheckWriteName("JackRequest::InternalClientLoad", socket);
            break;
        }

        case JackRequest::kInternalClientUnload: {
            jack_log("JackRequest::InternalClientUnload");
            JackInternalClientUnloadRequest req;
            JackInternalClientUnloadResult res;
            CheckRead(req, socket);
            res.fResult = fServer->GetEngine()->InternalClientUnload(req.fIntRefNum, &res.fStatus);
            CheckWriteRefNum("JackRequest::InternalClientUnload", socket);
            break;
        }

        case JackRequest::kNotification: {
            jack_log("JackRequest::Notification");
            JackClientNotificationRequest req;
            CheckRead(req, socket);
            if (req.fNotify == kQUIT) {
                jack_log("JackRequest::Notification kQUIT");
                throw JackQuitException();
            } else {
                fServer->Notify(req.fRefNum, req.fNotify, req.fValue);
            }
            break;
        }

        case JackRequest::kSessionNotify: {
            jack_log("JackRequest::SessionNotify");
            JackSessionNotifyRequest req;
            CheckRead(req, socket);
            fServer->GetEngine()->SessionNotify(req.fRefNum, req.fDst, req.fEventType, req.fPath, socket, NULL);
            break;
        }

        case JackRequest::kSessionReply: {
            jack_log("JackRequest::SessionReply");
            JackSessionReplyRequest req;
            JackResult res;
            CheckRead(req, socket);
            res.fResult = fServer->GetEngine()->SessionReply(req.fRefNum);
            CheckWrite("JackRequest::SessionReply", socket);
            break;
        }

        case JackRequest::kGetClientByUUID: {
            jack_log("JackRequest::GetClientByUUID");
            JackGetClientNameRequest req;
            JackClientNameResult res;
            CheckRead(req, socket);
            res.fResult = fServer->GetEngine()->GetClientNameForUUID(req.fUUID, res.fName);
            CheckWrite("JackRequest::GetClientByUUID", socket);
            break;
        }

        case JackRequest::kGetUUIDByClient: {
            jack_log("JackRequest::GetUUIDByClient");
            JackGetUUIDRequest req;
            JackUUIDResult res;
            CheckRead(req, socket);
            res.fResult = fServer->GetEngine()->GetUUIDForClientName(req.fName, res.fUUID);
            CheckWrite("JackRequest::GetUUIDByClient", socket);
            break;
        }

        case JackRequest::kReserveClientName: {
            jack_log("JackRequest::ReserveClientName");
            JackReserveNameRequest req;
            JackResult res;
            CheckRead(req, socket);
            res.fResult = fServer->GetEngine()->ReserveClientName(req.fName, req.fUUID);
            CheckWrite("JackRequest::ReserveClientName", socket);
            break;
        }

        case JackRequest::kClientHasSessionCallback: {
            jack_log("JackRequest::ClientHasSessionCallback");
            JackClientHasSessionCallbackRequest req;
            JackResult res;
            CheckRead(req, socket);
            res.fResult = fServer->GetEngine()->ClientHasSessionCallback(req.fName);
            CheckWrite("JackRequest::ClientHasSessionCallback", socket);
            break;
        }

        default:
            jack_error("Unknown request %ld", type);
            return -1;
    }

    return 0;
}

} // end of namespace


