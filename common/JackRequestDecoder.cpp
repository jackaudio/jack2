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

JackRequestDecoder::JackRequestDecoder(JackServer* server, JackClientHandlerInterface* handler)
    :fServer(server), fHandler(handler)
{}

JackRequestDecoder::~JackRequestDecoder()
{}

bool JackRequestDecoder::HandleRequest(detail::JackChannelTransactionInterface* socket)
{
    // Read header
    JackRequest header;
    if (header.Read(socket) < 0) {
        jack_log("HandleRequest: cannot read header");
        return false;
    }

   // Read data
    switch (header.fType) {

        case JackRequest::kClientCheck: {
            jack_log("JackRequest::ClientCheck");
            JackClientCheckRequest req;
            JackClientCheckResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->GetEngine()->ClientCheck(req.fName, req.fUUID, res.fName, req.fProtocol, req.fOptions, &res.fStatus);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::ClientCheck write error name = %s", req.fName);
            // Atomic ClientCheck followed by ClientOpen on same socket
            if (req.fOpen)
                HandleRequest(socket);
            break;
        }

        case JackRequest::kClientOpen: {
            jack_log("JackRequest::ClientOpen");
            JackClientOpenRequest req;
            JackClientOpenResult res;
            if (req.Read(socket) == 0)
                fHandler->ClientAdd(socket, &req, &res);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::ClientOpen write error name = %s", req.fName);
            break;
        }

        case JackRequest::kClientClose: {
            jack_log("JackRequest::ClientClose");
            JackClientCloseRequest req;
            JackResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->GetEngine()->ClientExternalClose(req.fRefNum);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::ClientClose write error ref = %d", req.fRefNum);
            fHandler->ClientRemove(socket, req.fRefNum);
            break;
        }

        case JackRequest::kActivateClient: {
            JackActivateRequest req;
            JackResult res;
            jack_log("JackRequest::ActivateClient");
            if (req.Read(socket) == 0)
                res.fResult = fServer->GetEngine()->ClientActivate(req.fRefNum, req.fIsRealTime);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::ActivateClient write error ref = %d", req.fRefNum);
            break;
        }

        case JackRequest::kDeactivateClient: {
            jack_log("JackRequest::DeactivateClient");
            JackDeactivateRequest req;
            JackResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->GetEngine()->ClientDeactivate(req.fRefNum);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::DeactivateClient write error ref = %d", req.fRefNum);
            break;
        }

        case JackRequest::kRegisterPort: {
            jack_log("JackRequest::RegisterPort");
            JackPortRegisterRequest req;
            JackPortRegisterResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->GetEngine()->PortRegister(req.fRefNum, req.fName, req.fPortType, req.fFlags, req.fBufferSize, &res.fPortIndex);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::RegisterPort write error ref = %d", req.fRefNum);
            break;
        }

        case JackRequest::kUnRegisterPort: {
            jack_log("JackRequest::UnRegisterPort");
            JackPortUnRegisterRequest req;
            JackResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->GetEngine()->PortUnRegister(req.fRefNum, req.fPortIndex);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::UnRegisterPort write error ref = %d", req.fRefNum);
            break;
        }

        case JackRequest::kConnectNamePorts: {
            jack_log("JackRequest::ConnectNamePorts");
            JackPortConnectNameRequest req;
            JackResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->GetEngine()->PortConnect(req.fRefNum, req.fSrc, req.fDst);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::ConnectNamePorts write error ref = %d", req.fRefNum);
            break;
        }

        case JackRequest::kDisconnectNamePorts: {
            jack_log("JackRequest::DisconnectNamePorts");
            JackPortDisconnectNameRequest req;
            JackResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->GetEngine()->PortDisconnect(req.fRefNum, req.fSrc, req.fDst);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::DisconnectNamePorts write error ref = %d", req.fRefNum);
            break;
        }

        case JackRequest::kConnectPorts: {
            jack_log("JackRequest::ConnectPorts");
            JackPortConnectRequest req;
            JackResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->GetEngine()->PortConnect(req.fRefNum, req.fSrc, req.fDst);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::ConnectPorts write error ref = %d", req.fRefNum);
            break;
        }

        case JackRequest::kDisconnectPorts: {
            jack_log("JackRequest::DisconnectPorts");
            JackPortDisconnectRequest req;
            JackResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->GetEngine()->PortDisconnect(req.fRefNum, req.fSrc, req.fDst);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::DisconnectPorts write error ref = %d", req.fRefNum);
            break;
        }

        case JackRequest::kPortRename: {
            jack_log("JackRequest::PortRename");
            JackPortRenameRequest req;
            JackResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->GetEngine()->PortRename(req.fRefNum, req.fPort, req.fName);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::PortRename write error ref = %d", req.fRefNum);
            break;
        }

        case JackRequest::kSetBufferSize: {
            jack_log("JackRequest::SetBufferSize");
            JackSetBufferSizeRequest req;
            JackResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->SetBufferSize(req.fBufferSize);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::SetBufferSize write error");
            break;
        }

        case JackRequest::kSetFreeWheel: {
            jack_log("JackRequest::SetFreeWheel");
            JackSetFreeWheelRequest req;
            JackResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->SetFreewheel(req.fOnOff);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::SetFreeWheel write error");
            break;
        }

         case JackRequest::kComputeTotalLatencies: {
            jack_log("JackRequest::ComputeTotalLatencies");
            JackComputeTotalLatenciesRequest req;
            JackResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->GetEngine()->ComputeTotalLatencies();
            if (res.Write(socket) < 0)
                jack_error("JackRequest::ComputeTotalLatencies write error");
            break;
        }

        case JackRequest::kReleaseTimebase: {
            jack_log("JackRequest::ReleaseTimebase");
            JackReleaseTimebaseRequest req;
            JackResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->ReleaseTimebase(req.fRefNum);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::ReleaseTimebase write error ref = %d", req.fRefNum);
            break;
        }

        case JackRequest::kSetTimebaseCallback: {
            jack_log("JackRequest::SetTimebaseCallback");
            JackSetTimebaseCallbackRequest req;
            JackResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->SetTimebaseCallback(req.fRefNum, req.fConditionnal);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::SetTimebaseCallback write error ref = %d", req.fRefNum);
            break;
        }

        case JackRequest::kGetInternalClientName: {
            jack_log("JackRequest::GetInternalClientName");
            JackGetInternalClientNameRequest req;
            JackGetInternalClientNameResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->GetEngine()->GetInternalClientName(req.fIntRefNum, res.fName);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::GetInternalClientName write error ref = %d", req.fRefNum);
            break;
        }

        case JackRequest::kInternalClientHandle: {
            jack_log("JackRequest::InternalClientHandle");
            JackInternalClientHandleRequest req;
            JackInternalClientHandleResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->GetEngine()->InternalClientHandle(req.fName, &res.fStatus, &res.fIntRefNum);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::InternalClientHandle write error ref = %d", req.fRefNum);
            break;
        }

        case JackRequest::kInternalClientLoad: {
            jack_log("JackRequest::InternalClientLoad");
            JackInternalClientLoadRequest req;
            JackInternalClientLoadResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->InternalClientLoad1(req.fName, req.fDllName, req.fLoadInitName, req.fOptions, &res.fIntRefNum, req.fUUID, &res.fStatus);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::InternalClientLoad write error name = %s", req.fName);
            break;
        }

        case JackRequest::kInternalClientUnload: {
            jack_log("JackRequest::InternalClientUnload");
            JackInternalClientUnloadRequest req;
            JackInternalClientUnloadResult res;
            if (req.Read(socket) == 0)
                res.fResult = fServer->GetEngine()->InternalClientUnload(req.fIntRefNum, &res.fStatus);
            if (res.Write(socket) < 0)
                jack_error("JackRequest::InternalClientUnload write error ref = %d", req.fRefNum);
            break;
        }

        case JackRequest::kNotification: {
            jack_log("JackRequest::Notification");
            JackClientNotificationRequest req;
            if (req.Read(socket) == 0) {
                if (req.fNotify == kQUIT) {
                    jack_log("JackRequest::Notification kQUIT");
                    throw JackQuitException();
                } else {
                    fServer->Notify(req.fRefNum, req.fNotify, req.fValue);
                }
            }
            break;
        }

        case JackRequest::kSessionNotify: {
            jack_log("JackRequest::SessionNotify");
            JackSessionNotifyRequest req;
            if (req.Read(socket) == 0) {
                fServer->GetEngine()->SessionNotify(req.fRefNum, req.fDst, req.fEventType, req.fPath, socket, NULL);
            }
            break;
        }

        case JackRequest::kSessionReply: {
            jack_log("JackRequest::SessionReply");
            JackSessionReplyRequest req;
            JackResult res;
            if (req.Read(socket) == 0) {
                fServer->GetEngine()->SessionReply(req.fRefNum);
                res.fResult = 0;
            }
            if (res.Write(socket) < 0)
                jack_error("JackRequest::SessionReply write error");
            break;
        }

        case JackRequest::kGetClientByUUID: {
            jack_log("JackRequest::GetClientByUUID");
            JackGetClientNameRequest req;
            JackClientNameResult res;
            if (req.Read(socket) == 0) {
                fServer->GetEngine()->GetClientNameForUUID(req.fUUID, res.fName, &res.fResult);
            }
            if (res.Write(socket) < 0)
                jack_error("JackRequest::GetClientByUUID write error");
            break;
        }

        case JackRequest::kGetUUIDByClient: {
            jack_log("JackRequest::GetUUIDByClient");
            JackGetUUIDRequest req;
            JackUUIDResult res;
            if (req.Read(socket) == 0) {
                fServer->GetEngine()->GetUUIDForClientName(req.fName, res.fUUID, &res.fResult);
            }
            if (res.Write(socket) < 0)
                jack_error("JackRequest::GetUUIDByClient write error");
            break;
        }

        case JackRequest::kReserveClientName: {
            jack_log("JackRequest::ReserveClientName");
            JackReserveNameRequest req;
            JackResult res;
            if (req.Read(socket) == 0) {
                fServer->GetEngine()->ReserveClientName(req.fName, req.fUUID, &res.fResult);
            }
            if (res.Write(socket) < 0)
                jack_error("JackRequest::ReserveClientName write error");
            break;
        }

        case JackRequest::kClientHasSessionCallback: {
            jack_log("JackRequest::ClientHasSessionCallback");
            JackClientHasSessionCallbackRequest req;
            JackResult res;
            if (req.Read(socket) == 0) {
                fServer->GetEngine()->ClientHasSessionCallback(req.fName, &res.fResult);
            }
            if (res.Write(socket) < 0)
                jack_error("JackRequest::ClientHasSessionCallback write error");
            break;
        }

        default:
            jack_error("Unknown request %ld", header.fType);
            break;
    }

    return true;
}

} // end of namespace


