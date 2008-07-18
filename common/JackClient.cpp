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

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

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
#include "JackPlatformThread.h"

using namespace std;

namespace Jack
{

#define IsRealTime() ((fProcess != NULL) | (fThreadFun != NULL) | (fSync != NULL) | (fTimebase != NULL))
        
JackClient::JackClient():fThread(this)
{}

JackClient::JackClient(JackSynchro* table):fThread(this)
{
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
    fTimebase = NULL;
    fSync = NULL;
    fThreadFun = NULL;
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
    fTimebaseArg = NULL;
    fThreadFunArg = NULL;
}

JackClient::~JackClient()
{
}

int JackClient::Close()
{
    jack_log("JackClient::Close ref = %ld", GetClientControl()->fRefNum);
    Deactivate();
    int result = -1;
    fChannel->Stop();  // Channels is stopped first to avoid receiving notifications while closing
    fChannel->ClientClose(GetClientControl()->fRefNum, &result);
    fChannel->Close();
    fSynchroTable[GetClientControl()->fRefNum].Disconnect();
    return result;
}

bool JackClient::IsActive()
{
    return (GetClientControl()) ? GetClientControl()->fActive : false;
}

pthread_t JackClient::GetThreadID()
{
    return fThread.GetThreadID();
}

/*!
	In "async" mode, the server does not synchronize itself on the output drivers, thus it would never "consume" the activations.
	The synchronization primitives for drivers are setup in "flush" mode that to not keep unneeded activations.
	Drivers synchro are setup in "flush" mode if server is "async" and NOT freewheel.
*/
void JackClient::SetupDriverSync(bool freewheel)
{
    if (!freewheel && !GetEngineControl()->fSyncMode) {
        jack_log("JackClient::SetupDriverSync driver sem in flush mode");
        fSynchroTable[AUDIO_DRIVER_REFNUM].SetFlush(true);
        fSynchroTable[FREEWHEEL_DRIVER_REFNUM].SetFlush(true);
        fSynchroTable[LOOPBACK_DRIVER_REFNUM].SetFlush(true);
    } else {
        jack_log("JackClient::SetupDriverSync driver sem in normal mode");
        fSynchroTable[AUDIO_DRIVER_REFNUM].SetFlush(false);
        fSynchroTable[FREEWHEEL_DRIVER_REFNUM].SetFlush(false);
        fSynchroTable[LOOPBACK_DRIVER_REFNUM].SetFlush(false);
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
            jack_log("JackClient::kActivateClient name = %s ref = %ld ", name, refnum);
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
                jack_log("JackClient::kAddClient fName = %s name = %s", GetClientControl()->fName, name);
                if (fClientRegistration && strcmp(GetClientControl()->fName, name) != 0)	// Don't call the callback for the registering client itself
                    fClientRegistration(name, 1, fClientRegistrationArg);
                break;

            case kRemoveClient:
                jack_log("JackClient::kRemoveClient fName = %s name = %s", GetClientControl()->fName, name);
                if (fClientRegistration && strcmp(GetClientControl()->fName, name) != 0)	// Don't call the callback for the registering client itself
                    fClientRegistration(name, 0, fClientRegistrationArg);
                break;

            case kBufferSizeCallback:
                jack_log("JackClient::kBufferSizeCallback buffer_size = %ld", value1);
                if (fBufferSize)
                    res = fBufferSize(value1, fBufferSizeArg);
                break;
                
            case kSampleRateCallback:
                jack_log("JackClient::kSampleRateCallback sample_rate = %ld", value1);
                if (fSampleRate)
                    res = fSampleRate(value1, fSampleRateArg);
                break;

            case kGraphOrderCallback:
                jack_log("JackClient::kGraphOrderCallback");
                if (fGraphOrder)
                    res = fGraphOrder(fGraphOrderArg);
                break;

            case kStartFreewheelCallback:
                jack_log("JackClient::kStartFreewheel");
                SetupDriverSync(true);
                fThread.DropRealTime();
                if (fFreewheel)
                    fFreewheel(1, fFreewheelArg);
                break;

            case kStopFreewheelCallback:
                jack_log("JackClient::kStopFreewheel");
                SetupDriverSync(false);
                if (fFreewheel)
                    fFreewheel(0, fFreewheelArg);
                fThread.AcquireRealTime();
                break;

            case kPortRegistrationOnCallback:
                jack_log("JackClient::kPortRegistrationOn port_index = %ld", value1);
                if (fPortRegistration)
                    fPortRegistration(value1, 1, fPortRegistrationArg);
                break;

            case kPortRegistrationOffCallback:
                jack_log("JackClient::kPortRegistrationOff port_index = %ld ", value1);
                if (fPortRegistration)
                    fPortRegistration(value1, 0, fPortRegistrationArg);
                break;

            case kPortConnectCallback:
                jack_log("JackClient::kPortConnectCallback src = %ld dst = %ld", value1, value2);
                if (fPortConnect)
                    fPortConnect(value1, value2, 1, fPortConnectArg);
                break;

            case kPortDisconnectCallback:
                jack_log("JackClient::kPortDisconnectCallback src = %ld dst = %ld", value1, value2);
                if (fPortConnect)
                    fPortConnect(value1, value2, 0, fPortConnectArg);
                break;

            case kXRunCallback:
                jack_log("JackClient::kXRunCallback");
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
    jack_log("JackClient::Activate");
    if (IsActive())
        return 0;

    // RT thread is started only when needed...
    if (IsRealTime()) {
        if (StartThread() < 0)
            return -1;
    }
    
    /*
    Insertion of client in the graph will cause a kGraphOrderCallback notification 
    to be delivered by the server, the client wants to receive it.
    */
    GetClientControl()->fActive = true;
    
    // Transport related callback become "active"
    GetClientControl()->fTransportSync = true;
    GetClientControl()->fTransportTimebase = true;

    int result = -1;
    GetClientControl()->fCallback[kRealTimeCallback] = IsRealTime();
    fChannel->ClientActivate(GetClientControl()->fRefNum, IsRealTime(), &result);
    return result;
}

/*!
\brief Need to stop thread after deactivating in the server.
*/
int JackClient::Deactivate()
{
    jack_log("JackClient::Deactivate");
    if (!IsActive())
        return 0;

    GetClientControl()->fActive = false;
    
    // Transport related callback become "unactive"
    GetClientControl()->fTransportSync = false;
    GetClientControl()->fTransportTimebase = false;
    
    // We need to wait for the new engine cycle before stopping the RT thread, but this is done by ClientDeactivate
    int result = -1;
    fChannel->ClientDeactivate(GetClientControl()->fRefNum, &result);
    jack_log("JackClient::Deactivate res = %ld", result);
  
    // RT thread is stopped only when needed...
    if (IsRealTime()) 
        fThread.Kill();
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
        jack_log("JackClient::Init calling client thread init callback");
        fInit(fInitArg);
    }
    return true;
}

int JackClient::StartThread()
{
    jack_log("JackClient::StartThread : period = %ld computation = %ld constraint = %ld",
             long(int64_t(GetEngineControl()->fPeriod) / 1000.0f),
             long(int64_t(GetEngineControl()->fComputation) / 1000.0f),
             long(int64_t(GetEngineControl()->fConstraint) / 1000.0f));

    // Will do "something" on OSX only...
    fThread.SetParams(GetEngineControl()->fPeriod, GetEngineControl()->fComputation, GetEngineControl()->fConstraint);

    if (fThread.Start() < 0) {
        jack_error("Start thread error");
        return -1;
    }

    if (GetEngineControl()->fRealTime) {
        if (fThread.AcquireRealTime(GetEngineControl()->fPriority - 1) < 0) {
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
    if (!jack_tls_set(gRealTime, this)) 
        jack_error("failed to set thread realtime key");
        
    if (GetEngineControl()->fRealTime) 
        set_threaded_log_function(); 
    
    if (fThreadFun) {
        // Execute a dummy cycle to be sure thread has the correct properties (ensure thread creation is finished)
        WaitSync();
        SignalSync();
        fThreadFun(fThreadFunArg);
    } else {
        if (WaitFirstSync())
            ExecuteThread();
    }
    return false; 
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
            jack_log("Process called for an inactive client");
        }
        SignalSync();
    }
    return false; 
}

inline void JackClient::ExecuteThread()
{
    while (Wait(CallProcessCallback()) == GetEngineControl()->fBufferSize);
}

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

/*
jack_nframes_t JackClient::Wait(int status)
{
	CycleSignal(status);
	return CycleWait();
}
*/

jack_nframes_t JackClient::CycleWait()
{
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
    jack_log("JackClient::Execute end name = %s", GetClientControl()->fName);
    // Hum... not sure about this, the following "close" code is called in the RT thread...
    int result;
    fThread.DropRealTime();
    GetClientControl()->fActive = false;
    fChannel->ClientDeactivate(GetClientControl()->fRefNum, &result);
    return 0; 
}

inline int JackClient::Error()
{
    jack_error("JackClient::Execute error name = %s", GetClientControl()->fName);
    // Hum... not sure about this, the following "close" code is called in the RT thread...
    int result;
    fThread.DropRealTime();
    GetClientControl()->fActive = false;
    fChannel->ClientDeactivate(GetClientControl()->fRefNum, &result);
    ShutDown();
    return 0; 
}

//-----------------
// Port management
//-----------------

int JackClient::PortRegister(const char* port_name, const char* port_type, unsigned long flags, unsigned long buffer_size)
{
    // Check if port name is empty
    string port_name_str = string(port_name);
    if (port_name_str.size() == 0) {
        jack_error("port_name is empty");
        return 0; // Means failure here...
    }

    // Check port name length
    string name = string(GetClientControl()->fName) + string(":") + port_name_str;
    if (name.size() >= JACK_PORT_NAME_SIZE) {
        jack_error("\"%s:%s\" is too long to be used as a JACK port name.\n"
                   "Please use %lu characters or less",
                   GetClientControl()->fName,
                   port_name,
                   JACK_PORT_NAME_SIZE - 1);
        return 0; // Means failure here...
    }

    int result = -1;
    unsigned int port_index = NO_PORT;
    fChannel->PortRegister(GetClientControl()->fRefNum, name.c_str(), port_type, flags, buffer_size, &port_index, &result);
  
    if (result == 0) {
        jack_log("JackClient::PortRegister ref = %ld name = %s type = %s port_index = %ld", GetClientControl()->fRefNum, name.c_str(), port_type, port_index);
        fPortList.push_back(port_index);
        return port_index;
    } else {
        return 0;
    }
}

int JackClient::PortUnRegister(jack_port_id_t port_index)
{
    jack_log("JackClient::PortUnRegister port_index = %ld", port_index);
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
    jack_log("JackClient::Connect src = %s dst = %s", src, dst);
    int result = -1;
    fChannel->PortConnect(GetClientControl()->fRefNum, src, dst, &result);
    return result;
}

int JackClient::PortDisconnect(const char* src, const char* dst)
{
    jack_log("JackClient::Disconnect src = %s dst = %s", src, dst);
    int result = -1;
    fChannel->PortDisconnect(GetClientControl()->fRefNum, src, dst, &result);
    return result;
}

int JackClient::PortDisconnect(jack_port_id_t src)
{
    jack_log("JackClient::PortDisconnect src = %ld", src);
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
    jack_log("ShutDown");
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
        GetClientControl()->fTransportTimebase = false;
        fTimebase = NULL;
        fTimebaseArg = NULL;
    }
    return result;
}

/* Call the server if the client is active, otherwise keeps the arguments */
int JackClient::SetSyncCallback(JackSyncCallback sync_callback, void* arg)
{
    GetClientControl()->fTransportSync = (fSync != NULL);
    fSyncArg = arg;
    fSync = sync_callback;
    return 0;
}

int JackClient::SetSyncTimeout(jack_time_t timeout)
{
    GetEngineControl()->fTransport.SetSyncTimeout(timeout);
    return 0;
}

int JackClient::SetTimebaseCallback(int conditional, JackTimebaseCallback timebase_callback, void* arg)
{
    int result = -1;
    fChannel->SetTimebaseCallback(GetClientControl()->fRefNum, conditional, &result);
    jack_log("SetTimebaseCallback result = %ld", result);
    if (result == 0) {
        GetClientControl()->fTransportTimebase = true;
        fTimebase = timebase_callback;
        fTimebaseArg = arg;
    } else {
        fTimebase = NULL;
        fTimebaseArg = NULL;
    }
    return result;
}

// Must be RT safe

void JackClient::TransportLocate(jack_nframes_t frame)
{
    jack_position_t pos;
    pos.frame = frame;
    pos.valid = (jack_position_bits_t)0;
    jack_log("TransportLocate pos = %ld", pos.frame);
    GetEngineControl()->fTransport.RequestNewPos(&pos);
}

int JackClient::TransportReposition(jack_position_t* pos)
{
    jack_position_t tmp = *pos;
    jack_log("TransportReposition pos = %ld", pos->frame);
    if (tmp.valid & ~JACK_POSITION_MASK) {
        return EINVAL;
    } else {
        GetEngineControl()->fTransport.RequestNewPos(pos);
        return 0;
    }
}

jack_transport_state_t JackClient::TransportQuery(jack_position_t* pos)
{
    return GetEngineControl()->fTransport.Query(pos);
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
// TODO check concurrency with SetSyncCallback
void JackClient::CallSyncCallback()
{
    if (GetClientControl()->fTransportSync) {
    
        JackTransportEngine& transport = GetEngineControl()->fTransport;
        jack_position_t* cur_pos = transport.ReadCurrentState();
        jack_transport_state_t transport_state = transport.GetState();
    
        if (fSync != NULL) {
            if (fSync(transport_state, cur_pos, fSyncArg)) {
                GetClientControl()->fTransportState = JackTransportRolling;
                GetClientControl()->fTransportSync = false;
            }
        } else {
            GetClientControl()->fTransportState = JackTransportRolling;
            GetClientControl()->fTransportSync = false;
        }
    }
}

void JackClient::CallTimebaseCallback()
{
    JackTransportEngine& transport = GetEngineControl()->fTransport;
    
    if (GetClientControl()->fRefNum == transport.GetTimebaseMaster() && fTimebase) { // Client *is* timebase...
    
        jack_transport_state_t transport_state = transport.GetState();
        jack_position_t* cur_pos = transport.WriteNextStateStart(1);
        
        if (GetClientControl()->fTransportTimebase) {
            fTimebase(transport_state, GetEngineControl()->fBufferSize, cur_pos, true, fTimebaseArg); 
            GetClientControl()->fTransportTimebase = false; // Callback is called only once with "new_pos" = true 
        } else if (transport_state == JackTransportRolling) {
             fTimebase(transport_state, GetEngineControl()->fBufferSize, cur_pos, false, fTimebaseArg);
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
    } else if (fThreadFun) {
        jack_error ("A thread callback has already been setup, both models cannot be used at the same time!");
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
    jack_log("SetGraphOrderCallback ");

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

int JackClient::SetSampleRateCallback(JackSampleRateCallback callback, void *arg)
{
    if (IsActive()) {
        jack_error("You cannot set callbacks on an active client");
        return -1;
    } else {
        GetClientControl()->fCallback[kSampleRateCallback] = (callback != NULL);
        fSampleRateArg = arg;
        fSampleRate = callback;
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

int JackClient::SetProcessThread(JackThreadCallback fun, void *arg)
{
    if (IsActive()) {
        jack_error("You cannot set callbacks on an active client");
        return -1;
    } else if (fProcess) {
        jack_error ("A process callback has already been setup, both models cannot be used at the same time!");
        return -1;
    } else {
        fThreadFun = fun;
        fThreadFunArg = arg;
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

