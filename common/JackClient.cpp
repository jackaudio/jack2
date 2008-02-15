/*
Copyright (C) 2001 Paul Davis 
Copyright (C) 2004-2008 Grame

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


#include "JackClient.h"
#include "JackGraphManager.h"
#include "JackClientControl.h"
#include "JackEngineControl.h"
#include "JackGlobals.h"
#include "JackChannel.h"
#include "JackTransportEngine.h"
#include "driver_interface.h"
#include <math.h>
#include <string>
#include <algorithm>

using namespace std;

namespace Jack
{

JackClient::JackClient()
{}

JackClient::JackClient(JackSynchro** table)
{
    fThread = JackGlobals::MakeThread(this);
    fSynchroTable = table;
    fProcess = NULL;
    fGraphOrder = NULL;
    fXrun = NULL;
    fShutdown = NULL;
    fInit = NULL;
    fBufferSize = NULL;
	fClientRegistration = NULL;
    fFreewheel = NULL;
    fPortRegistration = NULL;
	fPortConnect = NULL;
    fSync = NULL;
    fProcessArg = NULL;
    fGraphOrderArg = NULL;
    fXrunArg = NULL;
    fShutdownArg = NULL;
    fInitArg = NULL;
    fBufferSizeArg = NULL;
    fFreewheelArg = NULL;
    fClientRegistrationArg = NULL;
    fPortRegistrationArg = NULL;
	fPortConnectArg = NULL;
    fSyncArg = NULL;
    fConditionnal = 0; // Temporary??
	fWait = false;
}

JackClient::~JackClient()
{
    delete fThread;
}

int JackClient::Close()
{
    JackLog("JackClient::Close ref = %ld\n", GetClientControl()->fRefNum);
    Deactivate();
    int result = -1;
    fChannel->ClientClose(GetClientControl()->fRefNum, &result);
    fChannel->Stop();
    fChannel->Close();
    fSynchroTable[GetClientControl()->fRefNum]->Disconnect();
    return result;
}

bool JackClient::IsActive()
{
    return (GetClientControl()) ? GetClientControl()->fActive : false;
}

pthread_t JackClient::GetThreadID()
{
    return fThread->GetThreadID();
}

/*!
	In "async" mode, the server does not synchronize itself on the output drivers, thus it would never "consume" the activations.
	The synchronization primitives for drivers are setup in "flush" mode that to not keep unneeded activations.
	Drivers synchro are setup in "flush" mode if server is "async" and NOT freewheel.
*/
void JackClient::SetupDriverSync(bool freewheel)
{
    if (!freewheel && !GetEngineControl()->fSyncMode) {
        JackLog("JackClient::SetupDriverSync driver sem in flush mode\n");
		fSynchroTable[AUDIO_DRIVER_REFNUM]->SetFlush(true);
        fSynchroTable[FREEWHEEL_DRIVER_REFNUM]->SetFlush(true);
        fSynchroTable[LOOPBACK_DRIVER_REFNUM]->SetFlush(true);
    } else {
        JackLog("JackClient::SetupDriverSync driver sem in normal mode\n");
		fSynchroTable[AUDIO_DRIVER_REFNUM]->SetFlush(false);
        fSynchroTable[FREEWHEEL_DRIVER_REFNUM]->SetFlush(false);
        fSynchroTable[LOOPBACK_DRIVER_REFNUM]->SetFlush(false);
    }
}

/*!
\brief Notification received from the server.
*/

int JackClient::ClientNotifyImp(int refnum, const char* name, int notify, int sync, int value1, int value2)
{
 	return 0;
}

int JackClient::ClientNotify(int refnum, const char* name, int notify, int sync, int value1, int value2)
{
    int res = 0;

    // Done all time: redirected on subclass implementation JackLibClient and JackInternalClient
    switch (notify) {

        case kAddClient:
            res = ClientNotifyImp(refnum, name, notify, sync, value1, value2);
			break;
			
	   case kRemoveClient:
            res = ClientNotifyImp(refnum, name, notify, sync, value1, value2);
			break;
			
		case kActivateClient:
			JackLog("JackClient::kActivateClient name = %s ref = %ld \n", name, refnum);
			Init();
			break;
	}

    /*
    The current semantic is that notifications can only be received when the client has been activated,
    although is this implementation, one could imagine calling notifications as soon as the client has be opened.
    */
    if (IsActive()) {

        switch (notify) {
		
			case kAddClient:
				JackLog("JackClient::kAddClient fName = %s name = %s\n", GetClientControl()->fName, name);
				if (fClientRegistration && strcmp(GetClientControl()->fName, name) != 0)	// Don't call the callback for the registering client itself
					fClientRegistration(name, 1, fClientRegistrationArg);
				break;
			
			case kRemoveClient:
				JackLog("JackClient::kRemoveClient fName = %s name = %s\n", GetClientControl()->fName, name);
				if (fClientRegistration && strcmp(GetClientControl()->fName, name) != 0)	// Don't call the callback for the registering client itself
					fClientRegistration(name, 0, fClientRegistrationArg);
				break;

            case kBufferSizeCallback:
                JackLog("JackClient::kBufferSizeCallback buffer_size = %ld\n", value1);
                if (fBufferSize)
                    res = fBufferSize(value1, fBufferSizeArg);
                break;

            case kGraphOrderCallback:
                JackLog("JackClient::kGraphOrderCallback\n");
                if (fGraphOrder)
                    res = fGraphOrder(fGraphOrderArg);
                break;

            case kStartFreewheelCallback:
                JackLog("JackClient::kStartFreewheel\n");
                SetupDriverSync(true);
                fThread->DropRealTime();
                if (fFreewheel)
                    fFreewheel(1, fFreewheelArg);
                break;

            case kStopFreewheelCallback:
                JackLog("JackClient::kStopFreewheel\n");
                SetupDriverSync(false);
                if (fFreewheel)
                    fFreewheel(0, fFreewheelArg);
                fThread->AcquireRealTime();
                break;

            case kPortRegistrationOnCallback:
                JackLog("JackClient::kPortRegistrationOn port_index = %ld\n", value1);
                if (fPortRegistration)
                    fPortRegistration(value1, 1, fPortRegistrationArg);
                break;

            case kPortRegistrationOffCallback:
                JackLog("JackClient::kPortRegistrationOff port_index = %ld \n", value1);
                if (fPortRegistration)
                    fPortRegistration(value1, 0, fPortRegistrationArg);
                break;
				
			case kPortConnectCallback:
                JackLog("JackClient::kPortConnectCallback src = %ld dst = %ld\n", value1, value2);
                if (fPortConnect)
                    fPortConnect(value1, value2, 1, fPortConnectArg);
                break;
				
			case kPortDisconnectCallback:
                JackLog("JackClient::kPortDisconnectCallback src = %ld dst = %ld\n", value1, value2);
                if (fPortConnect)
                    fPortConnect(value1, value2, 0, fPortConnectArg);
                break;

            case kXRunCallback:
                JackLog("JackClient::kXRunCallback\n");
                if (fXrun)
                    res = fXrun(fXrunArg);
                break;
		}
    }

    return res;
}

/*!
\brief We need to start thread before activating in the server, otherwise the FW driver 
	   connected to the client may not be activated.
*/
int JackClient::Activate()
{
    JackLog("JackClient::Activate \n");
    if (IsActive())
        return 0;

/* TODO : solve WIN32 thread Kill issue
#ifdef WIN32
	// Done first so that the RT thread then access an allocated synchro
	if (!fSynchroTable[GetClientControl()->fRefNum]->Connect(GetClientControl()->fName)) {
        jack_error("Cannot ConnectSemaphore %s client", GetClientControl()->fName);
        return -1;
    }
#endif
*/

	if (fProcess || !fWait) {  // Start thread only of process cb has been setup
		if (StartThread() < 0)
			return -1;
	}

    int result = -1;
    fChannel->ClientActivate(GetClientControl()->fRefNum, &result);
    if (result < 0)
        return result;

    if (fSync != NULL)		/* If a SyncCallback is pending... */
        SetSyncCallback(fSync, fSyncArg);

    if (fTimebase != NULL)	/* If a TimebaseCallback is pending... */
        SetTimebaseCallback(fConditionnal, fTimebase, fTimebaseArg);

    GetClientControl()->fActive = true;
    return 0;
}

/*!
\brief Need to stop thread after deactivating in the server.
*/
int JackClient::Deactivate()
{
    JackLog("JackClient::Deactivate \n");
    if (!IsActive())
        return 0;

    GetClientControl()->fActive = false;
    int result = -1;
    fChannel->ClientDeactivate(GetClientControl()->fRefNum, &result);

    JackLog("JackClient::Deactivate res = %ld \n", result);
    // We need to wait for the new engine cycle before stopping the RT thread, but this is done by ClientDeactivate

/* TODO : solve WIN32 thread Kill issue    
#ifdef WIN32
	fSynchroTable[GetClientControl()->fRefNum]->Disconnect();
	fThread->Stop();
#else
	fThread->Kill();
#endif
*/
	//if (fProcess || fWait) {  // Kill thread only of process cb has been setup
		fThread->Kill();
	//}
    return result;
}

//----------------------
// RT thread management
//----------------------

/*!
\brief Called once when the thread starts.
*/
bool JackClient::Init()
{
    if (fInit) {
        JackLog("JackClient::Init calling client thread init callback\n");
		fInit(fInitArg);
    }
    return true;
}

int JackClient::StartThread()
{
    JackLog("JackClient::StartThread : period = %ld computation = %ld constraint = %ld\n",
            long(int64_t(GetEngineControl()->fPeriod) / 1000.0f),
            long(int64_t(GetEngineControl()->fComputation) / 1000.0f),
            long(int64_t(GetEngineControl()->fConstraint) / 1000.0f));

    // Will do "something" on OSX only...
    fThread->SetParams(GetEngineControl()->fPeriod, GetEngineControl()->fComputation, GetEngineControl()->fConstraint);

    if (fThread->Start() < 0) {
        jack_error("Start thread error");
        return -1;
    }

    if (GetEngineControl()->fRealTime) {
        if (fThread->AcquireRealTime(GetEngineControl()->fPriority - 1) < 0) {
            jack_error("AcquireRealTime error");
        }
    }

    return 0;
}

/*!
\brief RT thread.
*/
bool JackClient::Execute()
{
	if (WaitFirstSync())
		ExecuteThread();
	return false; // Never reached
}

inline bool JackClient::WaitFirstSync()
{
	while (true) {
		// Start first cycle
		WaitSync();
		if (IsActive()) {
			CallSyncCallback();
			// Finish first cycle
			if (Wait(CallProcessCallback()) != GetEngineControl()->fBufferSize)
				return false;
			return true;
		} else {
			JackLog("Process called for an inactive client\n");
		}
		SignalSync();
	}
	return false; // Never reached
}

inline void JackClient::ExecuteThread()
{
	while (true) { 
		if (Wait(CallProcessCallback()) != GetEngineControl()->fBufferSize)
			return;
	}
}

/*
jack_nframes_t JackClient::Wait(int status)
{
	if (status == 0)
		CallTimebaseCallback();
	SignalSync();
	if (status != 0) 
		return End();
	if (!WaitSync()) 
		return Error();
	CallSyncCallback();
	return GetEngineControl()->fBufferSize;
}
*/

jack_nframes_t JackClient::CycleWait()
{
	fWait = true;
	if (!WaitSync()) 
		return Error();
	CallSyncCallback();
	return GetEngineControl()->fBufferSize;
}

void JackClient::CycleSignal(int status)
{
	if (status == 0)
		CallTimebaseCallback();
	SignalSync();
	if (status != 0) 
		End();
}

jack_nframes_t JackClient::Wait(int status)
{
	CycleSignal(status);
	return CycleWait();
}


inline int JackClient::CallProcessCallback()
{
	return (fProcess != NULL) ? fProcess(GetEngineControl()->fBufferSize, fProcessArg) : 0;
}

inline bool JackClient::WaitSync()
{
	// Suspend itself: wait on the input synchro
    if (GetGraphManager()->SuspendRefNum(GetClientControl(), fSynchroTable, 0x7FFFFFFF) < 0) {
       jack_error("SuspendRefNum error");
		return false;
	} else {
		return true;
	}
}

inline void JackClient::SignalSync()
{
	// Resume: signal output clients connected to the running client
    if (GetGraphManager()->ResumeRefNum(GetClientControl(), fSynchroTable) < 0) {
        jack_error("ResumeRefNum error");
    }
}

inline int JackClient::End()
{
	JackLog("JackClient::Execute end name = %s\n", GetClientControl()->fName);
	// Hum... not sure about this, the following "close" code is called in the RT thread...
	int result;
	fThread->DropRealTime();
	GetClientControl()->fActive = false;
	fChannel->ClientDeactivate(GetClientControl()->fRefNum, &result);
    fThread->Terminate();
	return 0; // Never reached
}

inline int JackClient::Error()
{
	jack_error("JackClient::Execute error name = %s", GetClientControl()->fName);
	// Hum... not sure about this, the following "close" code is called in the RT thread...
	int result;
    fThread->DropRealTime();
	GetClientControl()->fActive = false;
	fChannel->ClientDeactivate(GetClientControl()->fRefNum, &result);
    ShutDown();
	fThread->Terminate();
	return 0; // Never reached
}

//-----------------
// Port management
//-----------------

int JackClient::PortRegister(const char* port_name, const char* port_type, unsigned long flags, unsigned long buffer_size)
{
    // Check port name length
    string port_name_str = string(port_name);
    if (port_name_str.size() == 0) {
        jack_error("port_name is empty.");
        return 0; // Means failure here...
    }

    string name = string(GetClientControl()->fName) + string(":") + port_name_str;
    if (name.size() >= JACK_PORT_NAME_SIZE) {
        jack_error("\"%s:%s\" is too long to be used as a JACK port name.\n"
                   "Please use %lu characters or less.",
                   GetClientControl()->fName,
                   port_name,
                   JACK_PORT_NAME_SIZE - 1);
        return 0; // Means failure here...
    }

    // Check if port name already exists
    if (GetGraphManager()->GetPort(name.c_str()) != NO_PORT) {
        jack_error("port_name \"%s\" already exists.", port_name);
        return 0; // Means failure here...
    }

    JackLog("JackClient::PortRegister ref = %ld  name = %s type = %s\n", GetClientControl()->fRefNum, name.c_str(), port_type);

    int result = -1;
    unsigned int port_index = NO_PORT;
    fChannel->PortRegister(GetClientControl()->fRefNum, name.c_str(), port_type, flags, buffer_size, &port_index, &result);
    JackLog("JackClient::PortRegister port_index = %ld \n", port_index);

    if (result == 0) {
        fPortList.push_back(port_index);
        return port_index;
    } else {
        return 0;
    }
}

int JackClient::PortUnRegister(jack_port_id_t port_index)
{
    JackLog("JackClient::PortUnRegister port_index = %ld\n", port_index);
  	list<jack_port_id_t>::iterator it = find(fPortList.begin(), fPortList.end(), port_index);

    if (it != fPortList.end()) {
        fPortList.erase(it);
        int result = -1;
        fChannel->PortUnRegister(GetClientControl()->fRefNum, port_index, &result);
        return result;
    } else {
        jack_error("unregistering a port %ld that is not own by the client", port_index);
        return -1;
    }
}

int JackClient::PortConnect(const char* src, const char* dst)
{
    JackLog("JackClient::Connect src = %s dst = %s\n", src, dst);
    int result = -1;
    fChannel->PortConnect(GetClientControl()->fRefNum, src, dst, &result);
    return result;
}

int JackClient::PortDisconnect(const char* src, const char* dst)
{
    JackLog("JackClient::Disconnect src = %s dst = %s\n", src, dst);
    int result = -1;
    fChannel->PortDisconnect(GetClientControl()->fRefNum, src, dst, &result);
    return result;
}

int JackClient::PortConnect(jack_port_id_t src, jack_port_id_t dst)
{
    JackLog("JackClient::PortConnect src = %ld dst = %ld\n", src, dst);
    int result = -1;
    fChannel->PortConnect(GetClientControl()->fRefNum, src, dst, &result);
    return result;
}

int JackClient::PortDisconnect(jack_port_id_t src)
{
    JackLog("JackClient::PortDisconnect src = %ld\n", src);
    int result = -1;
    fChannel->PortDisconnect(GetClientControl()->fRefNum, src, ALL_PORTS, &result);
    return result;
}

int JackClient::PortIsMine(jack_port_id_t port_index)
{
    JackPort* port = GetGraphManager()->GetPort(port_index);
    return GetClientControl()->fRefNum == port->GetRefNum();
}

//--------------------
// Context management
//--------------------

int JackClient::SetBufferSize(jack_nframes_t buffer_size)
{
    int result = -1;
    fChannel->SetBufferSize(buffer_size, &result);
    return result;
}

int JackClient::SetFreeWheel(int onoff)
{
    int result = -1;
    fChannel->SetFreewheel(onoff, &result);
    return result;
}

/*
ShutDown is called:
- from the RT thread when Execute method fails
- possibly from a "closed" notification channel
(Not needed since the synch object used (Sema of Fifo will fails when server quits... see ShutDown))
*/

void JackClient::ShutDown()
{
    JackLog("ShutDown\n");
    if (fShutdown) {
        GetClientControl()->fActive = false;
        fShutdown(fShutdownArg);
        fShutdown = NULL;
    }
}

//----------------------
// Transport management
//----------------------

int JackClient::ReleaseTimebase()
{
    int result = -1;
    fChannel->ReleaseTimebase(GetClientControl()->fRefNum, &result);
    if (result == 0) {
        fTimebase = NULL;
        fTimebaseArg = NULL;
    }
    return result;
}

/* Call the server if the client is active, otherwise keeps the arguments */
int JackClient::SetSyncCallback(JackSyncCallback sync_callback, void* arg)
{
    if (IsActive())
        GetClientControl()->fTransportState = (sync_callback == NULL) ? JackTransportStopped : JackTransportSynching;
    fSync = sync_callback;
    fSyncArg = arg;
    return 0;
}

int JackClient::SetSyncTimeout(jack_time_t timeout)
{
    GetEngineControl()->fTransport.SetSyncTimeout(timeout);
    return 0;
}

/* Call the server if the client is active, otherwise keeps the arguments */
int JackClient::SetTimebaseCallback(int conditional, JackTimebaseCallback timebase_callback, void* arg)
{
    if (IsActive()) {
        int result = -1;
        fChannel->SetTimebaseCallback(GetClientControl()->fRefNum, conditional, &result);
        JackLog("SetTimebaseCallback result = %ld\n", result);
        if (result == 0) {
            fTimebase = timebase_callback;
            fTimebaseArg = arg;
        } else {
            fTimebase = NULL;
            fTimebaseArg = NULL;
        }
        JackLog("SetTimebaseCallback OK result = %ld\n", result);
        return result;
    } else {
        fTimebase = timebase_callback;
        fTimebaseArg = arg;
        fConditionnal = conditional;
        return 0;
    }
}

// Must be RT safe
int JackClient::RequestNewPos(jack_position_t* pos)
{
    JackTransportEngine& transport = GetEngineControl()->fTransport;
    jack_position_t* request = transport.WriteNextStateStart(2);
    pos->unique_1 = pos->unique_2 = transport.GenerateUniqueID();
    JackTransportEngine::TransportCopyPosition(pos, request);
    JackLog("RequestNewPos pos = %ld\n", pos->frame);
    transport.WriteNextStateStop(2);
    return 0;
}

int JackClient::TransportLocate(jack_nframes_t frame)
{
    jack_position_t pos;
    pos.frame = frame;
    pos.valid = (jack_position_bits_t)0;
    JackLog("TransportLocate pos = %ld\n", pos.frame);
    return RequestNewPos(&pos);
}

int JackClient::TransportReposition(jack_position_t* pos)
{
    jack_position_t tmp = *pos;
    JackLog("TransportReposition pos = %ld\n", pos->frame);
    return (tmp.valid & ~JACK_POSITION_MASK) ? EINVAL : RequestNewPos(&tmp);
}

jack_transport_state_t JackClient::TransportQuery(jack_position_t* pos)
{
    if (pos)
        GetEngineControl()->fTransport.ReadCurrentPos(pos);
    return GetEngineControl()->fTransport.GetState();
}

jack_nframes_t JackClient::GetCurrentTransportFrame()
{
    jack_position_t pos;
    jack_transport_state_t state = TransportQuery(&pos);

    if (state == JackTransportRolling) {
        float usecs = GetMicroSeconds() - pos.usecs;
        jack_nframes_t elapsed = (jack_nframes_t)floor((((float) pos.frame_rate) / 1000000.0f) * usecs);
        return pos.frame + elapsed;
    } else {
        return pos.frame;
    }
}

// Must be RT safe: directly write in the transport shared mem
void JackClient::TransportStart()
{
    GetEngineControl()->fTransport.SetCommand(TransportCommandStart);
}

// Must be RT safe: directly write in the transport shared mem
void JackClient::TransportStop()
{
    GetEngineControl()->fTransport.SetCommand(TransportCommandStop);
}

// Never called concurently with the server
// TODO check concurency with SetSyncCallback

void JackClient::CallSyncCallback()
{
    JackTransportEngine& transport = GetEngineControl()->fTransport;
    jack_position_t* cur_pos = transport.ReadCurrentState();
    jack_transport_state_t transport_state = transport.GetState();

    switch (transport_state) {

        case JackTransportStarting:  // Starting...
            if (fSync == NULL) {
                GetClientControl()->fTransportState = JackTransportRolling;
            } else if (GetClientControl()->fTransportState == JackTransportStarting) {
                if (fSync(transport_state, cur_pos, fSyncArg))
                    GetClientControl()->fTransportState = JackTransportRolling;
            }
            break;

        case JackTransportRolling:
            if (fSync != NULL && GetClientControl()->fTransportState == JackTransportStarting) { // Client still not ready
                if (fSync(transport_state, cur_pos, fSyncArg))
                    GetClientControl()->fTransportState = JackTransportRolling;
            }
            break;

        case JackTransportSynching:
            // New pos when transport engine is stopped...
            if (fSync != NULL) {
                fSync(JackTransportStopped, cur_pos, fSyncArg);
                GetClientControl()->fTransportState = JackTransportStopped;
            }
            break;

        default:
            break;
    }
}

void JackClient::CallTimebaseCallback()
{
    JackTransportEngine& transport = GetEngineControl()->fTransport;

    if (fTimebase != NULL && fTimebaseArg != NULL && GetClientControl()->fRefNum == transport.GetTimebaseMaster()) {

        jack_transport_state_t transport_state = transport.GetState();
        jack_position_t* cur_pos = transport.WriteNextStateStart(1);

        switch (transport_state) {

            case JackTransportRolling:
                fTimebase(transport_state, GetEngineControl()->fBufferSize, cur_pos, false, fTimebaseArg);
                break;

            case JackTransportSynching:
                fTimebase(JackTransportStopped, GetEngineControl()->fBufferSize, cur_pos, true, fTimebaseArg);
                break;

            default:
                break;
        }

        transport.WriteNextStateStop(1);
    }
}

//---------------------
// Callback management
//---------------------

void JackClient::OnShutdown(JackShutdownCallback callback, void *arg)
{
    if (IsActive()) {
        jack_error("You cannot set callbacks on an active client");
    } else {
        fShutdownArg = arg;
        fShutdown = callback;
    }
}

int JackClient::SetProcessCallback(JackProcessCallback callback, void *arg)
{
    if (IsActive()) {
        jack_error("You cannot set callbacks on an active client");
        return -1;
    } else {
        fProcessArg = arg;
        fProcess = callback;
        return 0;
    }
}

int JackClient::SetXRunCallback(JackXRunCallback callback, void *arg)
{
    if (IsActive()) {
        jack_error("You cannot set callbacks on an active client");
        return -1;
    } else {
		GetClientControl()->fCallback[kXRunCallback] = (callback != NULL);
        fXrunArg = arg;
        fXrun = callback;
        return 0;
    }
}

int JackClient::SetInitCallback(JackThreadInitCallback callback, void *arg)
{
    if (IsActive()) {
        jack_error("You cannot set callbacks on an active client");
        return -1;
    } else {
        fInitArg = arg;
        fInit = callback;
        return 0;
    }
}

int JackClient::SetGraphOrderCallback(JackGraphOrderCallback callback, void *arg)
{
    JackLog("SetGraphOrderCallback \n");

    if (IsActive()) {
        jack_error("You cannot set callbacks on an active client");
        return -1;
    } else {
		GetClientControl()->fCallback[kGraphOrderCallback] = (callback != NULL);
        fGraphOrder = callback;
        fGraphOrderArg = arg;
        return 0;
    }
}

int JackClient::SetBufferSizeCallback(JackBufferSizeCallback callback, void *arg)
{
    if (IsActive()) {
        jack_error("You cannot set callbacks on an active client");
        return -1;
    } else {
		GetClientControl()->fCallback[kBufferSizeCallback] = (callback != NULL);
        fBufferSizeArg = arg;
        fBufferSize = callback;
        return 0;
    }
}

int JackClient::SetClientRegistrationCallback(JackClientRegistrationCallback callback, void* arg)
{
    if (IsActive()) {
        jack_error("You cannot set callbacks on an active client");
        return -1;
    } else {
		// kAddClient and kRemoveClient notifications must be delivered by the server in any case
		fClientRegistrationArg = arg;
        fClientRegistration = callback;
        return 0;
    }
}

int JackClient::SetFreewheelCallback(JackFreewheelCallback callback, void *arg)
{
    if (IsActive()) {
        jack_error("You cannot set callbacks on an active client");
        return -1;
    } else {
		GetClientControl()->fCallback[kStartFreewheelCallback] = (callback != NULL);
		GetClientControl()->fCallback[kStopFreewheelCallback] = (callback != NULL);
        fFreewheelArg = arg;
        fFreewheel = callback;
        return 0;
    }
}

int JackClient::SetPortRegistrationCallback(JackPortRegistrationCallback callback, void *arg)
{
    if (IsActive()) {
        jack_error("You cannot set callbacks on an active client");
        return -1;
    } else {
		GetClientControl()->fCallback[kPortRegistrationOnCallback] = (callback != NULL);
		GetClientControl()->fCallback[kPortRegistrationOffCallback] = (callback != NULL);
        fPortRegistrationArg = arg;
        fPortRegistration = callback;
        return 0;
    }
}

int JackClient::SetPortConnectCallback(JackPortConnectCallback callback, void *arg)
{
    if (IsActive()) {
        jack_error("You cannot set callbacks on an active client");
        return -1;
    } else {
		GetClientControl()->fCallback[kPortConnectCallback] = (callback != NULL);
		GetClientControl()->fCallback[kPortDisconnectCallback] = (callback != NULL);
        fPortConnectArg = arg;
        fPortConnect = callback;
        return 0;
    }
}

//------------------
// Internal clients
//------------------

char* JackClient::GetInternalClientName(int ref)
{
	char name_res[JACK_CLIENT_NAME_SIZE]; 
	int result = -1;
	fChannel->GetInternalClientName(GetClientControl()->fRefNum, ref, name_res, &result);
	
	if (result < 0) {
		return NULL;
	} else {
		char* name = (char*)malloc(strlen(name_res));
		strcpy(name, name_res);
		return name;
	}
}

int JackClient::InternalClientHandle(const char* client_name, jack_status_t* status)
{
	int int_ref, result = -1;
	fChannel->InternalClientHandle(GetClientControl()->fRefNum, client_name, (int*)status, &int_ref, &result);
	return int_ref;
}

int JackClient::InternalClientLoad(const char* client_name, jack_options_t options, jack_status_t* status, jack_varargs_t* va)
{
	if (strlen(client_name) >= JACK_CLIENT_NAME_SIZE) {
		jack_error ("\"%s\" is too long for a JACK client name.\n"
			    "Please use %lu characters or less.",
			    client_name, JACK_CLIENT_NAME_SIZE);
		return 0;
	}

	if (va->load_name && (strlen(va->load_name) >= PATH_MAX)) {
		jack_error("\"%s\" is too long for a shared object name.\n"
			     "Please use %lu characters or less.",
			    va->load_name, PATH_MAX);
		int my_status1 = *status | (JackFailure | JackInvalidOption);
		*status = (jack_status_t)my_status1;
		return 0;
	}

	if (va->load_init && (strlen(va->load_init) >= JACK_LOAD_INIT_LIMIT)) {
		jack_error ("\"%s\" is too long for internal client init "
			    "string.\nPlease use %lu characters or less.",
			    va->load_init, JACK_LOAD_INIT_LIMIT);
		int my_status1 = *status | (JackFailure | JackInvalidOption);
		*status = (jack_status_t)my_status1;
		return 0;
	}
	
	int int_ref, result = -1;
	fChannel->InternalClientLoad(GetClientControl()->fRefNum, client_name, va->load_name, va->load_init, options, (int*)status, &int_ref, &result);
	return int_ref;
}

void JackClient::InternalClientUnload(int ref, jack_status_t* status)
{
	int result = -1;
	fChannel->InternalClientUnload(GetClientControl()->fRefNum, ref, (int*)status, &result);
}


} // end of namespace

