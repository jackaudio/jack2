/*
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

#include "JackDebugClient.h"
#include "JackEngineControl.h"
#include "JackException.h"
#include "JackError.h"
#include "JackTime.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <string>
#include <time.h>

using namespace std;

namespace Jack
{

JackDebugClient::JackDebugClient(JackClient * client)
{
    fTotalPortNumber = 1;       // The total number of port opened and maybe closed. Historical view.
    fOpenPortNumber = 0;        // The current number of opened port.
    fIsActivated = 0;
    fIsDeactivated = 0;
    fIsClosed = 0;
    fClient = client;
    fFreewheel = false;
}

JackDebugClient::~JackDebugClient()
{
    fTotalPortNumber--; // fTotalPortNumber start at 1
    *fStream << endl << endl << "----------------------------------- JackDebugClient summary ------------------------------- " << endl << endl;
    *fStream << "Client flags ( 1:yes / 0:no ) :" << endl;
    *fStream << setw(5) << "- Client call activated : " << fIsActivated << endl;
    *fStream << setw(5) << "- Client call deactivated : " << fIsDeactivated << endl;
    *fStream << setw(5) << "- Client call closed : " << fIsClosed << endl;
    *fStream << setw(5) << "- Total number of instantiated port : " << fTotalPortNumber << endl;
    *fStream << setw(5) << "- Number of port remaining open when exiting client : " << fOpenPortNumber << endl;
    if (fOpenPortNumber != 0)
        *fStream << "!!! WARNING !!! Some ports have not been unregistered ! Incorrect exiting !" << endl;
    if (fIsDeactivated != fIsActivated)
        *fStream << "!!! ERROR !!! Client seem to not perform symetric activation-deactivation ! (not the same number of activate and deactivate)" << endl;
    if (fIsClosed == 0)
        *fStream << "!!! ERROR !!! Client have not been closed with jack_client_close() !" << endl;

    *fStream << endl << endl << "---------------------------- JackDebugClient detailed port summary ------------------------ " << endl << endl;
    //for (int i = 0; i < fTotalPortNumber ; i++) {
    for (int i = 1; i <= fTotalPortNumber ; i++) {
        *fStream << endl << "Port index (internal debug test value) : " << i << endl;
        *fStream << setw(5) << "- Name : " << fPortList[i].name << endl;
        *fStream << setw(5) << "- idport : " << fPortList[i].idport << endl;
        *fStream << setw(5) << "- IsConnected : " << fPortList[i].IsConnected << endl;
        *fStream << setw(5) << "- IsUnregistered : " << fPortList[i].IsUnregistered << endl;
        if (fPortList[i].IsUnregistered == 0)
            *fStream << "!!! WARNING !!! Port have not been unregistered ! Incorrect exiting !" << endl;
    }
    *fStream << "delete object JackDebugClient : end of tracing" << endl;
    delete fStream;
    delete fClient;
}

int JackDebugClient::Open(const char* server_name, const char* name, int uuid, jack_options_t options, jack_status_t* status)
{
    int res = fClient->Open(server_name, name, uuid, options, status);
    char provstr[256];
    char buffer[256];
    time_t curtime;
    struct tm *loctime;
    /* Get the current time. */
    curtime = time (NULL);
    /* Convert it to local time representation. */
    loctime = localtime (&curtime);
    strftime (buffer, 256, "%I-%M", loctime);
    snprintf(provstr, sizeof(provstr), "JackClientDebug-%s-%s.log", name, buffer);
    fStream = new ofstream(provstr, ios_base::ate);
    if (fStream->is_open()) {
        if (res == -1) {
            *fStream << "Trying to open client with name '" << name << "' with bad result (client not opened)." << res << endl;
        } else {
            *fStream << "Open client with name '" << name << "'." << endl;
        }
    } else {
        jack_log("JackDebugClient::Open : cannot open log file");
    }
    strcpy(fClientName, name);
    return res;
}

int JackDebugClient::Close()
{
    *fStream << "Client '" << fClientName << "' was closed" << endl;
    int res = fClient->Close();
    fIsClosed++;
    return res;
}

void JackDebugClient::CheckClient(const char* function_name) const
{
#ifdef WIN32
    *fStream << "CheckClient : " << function_name << ", calling thread : " << GetCurrentThread() << endl;
#else
    *fStream << "CheckClient : " << function_name << ", calling thread : " << pthread_self() << endl;
#endif

    if (fIsClosed > 0)  {
        *fStream << "!!! ERROR !!! : Accessing a client '" << fClientName << "' already closed " << "from " << function_name << endl;
        *fStream << "This is likely to cause crash !'" << endl;
    #ifdef __APPLE__
       // Debugger();
    #endif
    }
}

jack_native_thread_t JackDebugClient::GetThreadID()
{
    CheckClient("GetThreadID");
    return fClient->GetThreadID();
}

JackGraphManager* JackDebugClient::GetGraphManager() const
{
    CheckClient("GetGraphManager");
    return fClient->GetGraphManager();
}
JackEngineControl* JackDebugClient::GetEngineControl() const
{
    CheckClient("GetEngineControl");
    return fClient->GetEngineControl();
}
/*!
\brief Notification received from the server.
*/

int JackDebugClient::ClientNotify(int refnum, const char* name, int notify, int sync, const char* message, int value1, int value2)
{
    CheckClient("ClientNotify");
    return fClient->ClientNotify( refnum, name, notify, sync, message, value1, value2);
}

int JackDebugClient::Activate()
{
    CheckClient("Activate");
    int res = fClient->Activate();
    fIsActivated++;
    if (fIsDeactivated)
        *fStream << "Client '" << fClientName << "' call activate a new time (it already call 'activate' previously)." << endl;
    *fStream << "Client '" << fClientName << "' Activated" << endl;
    if (res != 0)
        *fStream << "Client '" << fClientName << "' try to activate but server return " << res << " ." << endl;
    return res;
}

int JackDebugClient::Deactivate()
{
    CheckClient("Deactivate");
    int res = fClient->Deactivate();
    fIsDeactivated++;
    if (fIsActivated == 0)
        *fStream << "Client '" << fClientName << "' deactivate while it hasn't been previoulsy activated !" << endl;
    *fStream << "Client '" << fClientName << "' Deactivated" << endl;
    if (res != 0)
        *fStream << "Client '" << fClientName << "' try to deactivate but server return " << res << " ." << endl;
    return res;
}

//-----------------
// Port management
//-----------------

int JackDebugClient::PortRegister(const char* port_name, const char* port_type, unsigned long flags, unsigned long buffer_size)
{
    CheckClient("PortRegister");
    int res = fClient->PortRegister(port_name, port_type, flags, buffer_size);
    if (res <= 0) {
        *fStream << "Client '" << fClientName << "' try port register ('" << port_name << "') and server return error  " << res << " ." << endl;
    } else {
        if (fTotalPortNumber < MAX_PORT_HISTORY) {
            fPortList[fTotalPortNumber].idport = res;
            strcpy(fPortList[fTotalPortNumber].name, port_name);
            fPortList[fTotalPortNumber].IsConnected = 0;
            fPortList[fTotalPortNumber].IsUnregistered = 0;
        } else {
            *fStream << "!!! WARNING !!! History is full : no more port history will be recorded." << endl;
        }
        fTotalPortNumber++;
        fOpenPortNumber++;
        *fStream << "Client '" << fClientName << "' port register with portname '" << port_name << " port " << res << "' ." << endl;
    }
    return res;
}

int JackDebugClient::PortUnRegister(jack_port_id_t port_index)
{
    CheckClient("PortUnRegister");
    int res = fClient->PortUnRegister(port_index);
    fOpenPortNumber--;
    int i;
    for (i = (fTotalPortNumber - 1); i >= 0; i--) {     // We search the record into the history
        if (fPortList[i].idport == port_index) {                // We found the last record
            if (fPortList[i].IsUnregistered != 0)
                *fStream << "!!! ERROR !!! : '" << fClientName << "' id deregistering port '" << fPortList[i].name << "' that have already been unregistered !" << endl;
            fPortList[i].IsUnregistered++;
            break;
        }
    }
    if (i == 0) // Port is not found
        *fStream << "JackClientDebug : PortUnregister : port " << port_index << " was not previously registered !" << endl;
    if (res != 0)
        *fStream << "Client '" << fClientName << "' try to do PortUnregister and server return " << res << endl;
    *fStream << "Client '" << fClientName << "' unregister port '" << port_index << "'." << endl;
    return res;
}

int JackDebugClient::PortConnect(const char* src, const char* dst)
{
    CheckClient("PortConnect");
    if (!fIsActivated)
        *fStream << "!!! ERROR !!! Trying to connect a port ( " << src << " to " << dst << ") while the client has not been activated !" << endl;
    int i;
    int res = fClient->PortConnect( src, dst);
    for (i = (fTotalPortNumber - 1); i >= 0; i--) {     // We search the record into the history
        if (strcmp(fPortList[i].name, src) == 0) {      // We found the last record in sources
            if (fPortList[i].IsUnregistered != 0)
                *fStream << "!!! ERROR !!! Connecting port " << src << " previoulsy unregistered !" << endl;
            fPortList[i].IsConnected++;
            *fStream << "Connecting port " << src << " to " << dst << ". ";
            break;
        } else if (strcmp(fPortList[i].name, dst) == 0 ) { // We found the record in dest
            if (fPortList[i].IsUnregistered != 0)
                *fStream << "!!! ERROR !!! Connecting port  " << dst << " previoulsy unregistered !" << endl;
            fPortList[i].IsConnected++;
            *fStream << "Connecting port " << src << " to " << dst << ". ";
            break;
        }
    }
    if (i == 0) // Port is not found
        *fStream << "JackClientDebug : PortConnect : port was not found in debug database !" << endl;
    if (res != 0)
        *fStream << "Client '" << fClientName << "' try to do PortConnect but server return " << res << " ." << endl;
    //*fStream << "Client Port Connect done with names" << endl;
    return res;
}

int JackDebugClient::PortDisconnect(const char* src, const char* dst)
{
    CheckClient("PortDisconnect");
    if (!fIsActivated)
        *fStream << "!!! ERROR !!! Trying to disconnect a port ( " << src << " to " << dst << ") while the client has not been activated !" << endl;
    int res = fClient->PortDisconnect( src, dst);
    int i;
    for (i = (fTotalPortNumber - 1); i >= 0; i--) { // We search the record into the history
        if (strcmp(fPortList[i].name, src) == 0) { // We found the record in sources
            if (fPortList[i].IsUnregistered != 0)
                *fStream << "!!! ERROR !!! : Disconnecting port " << src << " previoulsy unregistered !" << endl;
            fPortList[i].IsConnected--;
            *fStream << "disconnecting port " << src << ". ";
            break;
        } else if (strcmp(fPortList[i].name, dst) == 0 ) { // We found the record in dest
            if (fPortList[i].IsUnregistered != 0)
                *fStream << "!!! ERROR !!! : Disonnecting port  " << dst << " previoulsy unregistered !" << endl;
            fPortList[i].IsConnected--;
            *fStream << "disconnecting port " << dst << ". ";
            break;
        }
    }
    if (i == 0) // Port is not found
        *fStream << "JackClientDebug : PortDisConnect : port was not found in debug database !" << endl;
    if (res != 0)
        *fStream << "Client '" << fClientName << "' try to do PortDisconnect but server return " << res << " ." << endl;
    //*fStream << "Client Port Disconnect done." << endl;
    return res;
}

int JackDebugClient::PortDisconnect(jack_port_id_t src)
{
    CheckClient("PortDisconnect");
    if (!fIsActivated)
        *fStream << "!!! ERROR !!! : Trying to disconnect port " << src << " while that client has not been activated !" << endl;
    int res = fClient->PortDisconnect(src);
    int i;
    for (i = (fTotalPortNumber - 1); i >= 0; i--) {             // We search the record into the history
        if (fPortList[i].idport == src) {                               // We found the record in sources
            if (fPortList[i].IsUnregistered != 0)
                *fStream << "!!! ERROR !!! : Disconnecting port " << src << " previoulsy unregistered !" << endl;
            fPortList[i].IsConnected--;
            *fStream << "Disconnecting port " << src << ". " << endl;
            break;
        }
    }
    if (i == 0) // Port is not found
        *fStream << "JackClientDebug : PortDisconnect : port was not found in debug database !" << endl;
    if (res != 0)
        *fStream << "Client '" << fClientName << "' try to do PortDisconnect but server return " << res << " ." << endl;
    //*fStream << "Client Port Disconnect with ID done." << endl;
    return res;
}

int JackDebugClient::PortIsMine(jack_port_id_t port_index)
{
    CheckClient("PortIsMine");
    *fStream << "JackClientDebug : PortIsMine port_index " << port_index << endl;
    return fClient->PortIsMine(port_index);
}

int JackDebugClient::PortRename(jack_port_id_t port_index, const char* name)
{
    CheckClient("PortRename");
    *fStream << "JackClientDebug : PortRename port_index " << port_index << "name" << name << endl;
    return fClient->PortRename(port_index, name);
}

//--------------------
// Context management
//--------------------

int JackDebugClient::SetBufferSize(jack_nframes_t buffer_size)
{
    CheckClient("SetBufferSize");
    *fStream << "JackClientDebug : SetBufferSize buffer_size " << buffer_size << endl;
    return fClient->SetBufferSize(buffer_size);
}

int JackDebugClient::SetFreeWheel(int onoff)
{
    CheckClient("SetFreeWheel");
    if (onoff && fFreewheel)
        *fStream << "!!! ERROR !!! : Freewheel setup seems incorrect : set = ON while FW is already ON " << endl;
    if (!onoff && !fFreewheel)
        *fStream << "!!! ERROR !!! : Freewheel setup seems incorrect : set = OFF while FW is already OFF " << endl;
    fFreewheel = onoff ? true : false;
    return fClient->SetFreeWheel(onoff);
}

int JackDebugClient::ComputeTotalLatencies()
{
    CheckClient("ComputeTotalLatencies");
    return fClient->ComputeTotalLatencies();
}

/*
ShutDown is called:
- from the RT thread when Execute method fails
- possibly from a "closed" notification channel
(Not needed since the synch object used (Sema of Fifo will fails when server quits... see ShutDown))
*/

void JackDebugClient::ShutDown()
{
    CheckClient("ShutDown");
    fClient->ShutDown();
}

//---------------------
// Transport management
//---------------------

int JackDebugClient::ReleaseTimebase()
{
    CheckClient("ReleaseTimebase");
    return fClient->ReleaseTimebase();
}

int JackDebugClient::SetSyncCallback(JackSyncCallback sync_callback, void* arg)
{
    CheckClient("SetSyncCallback");
    return fClient->SetSyncCallback(sync_callback, arg);
}

int JackDebugClient::SetSyncTimeout(jack_time_t timeout)
{
    CheckClient("SetSyncTimeout");
    *fStream << "JackClientDebug : SetSyncTimeout timeout " << timeout << endl;
    return fClient->SetSyncTimeout(timeout);
}

int JackDebugClient::SetTimebaseCallback(int conditional, JackTimebaseCallback timebase_callback, void* arg)
{
    CheckClient("SetTimebaseCallback");
    return fClient->SetTimebaseCallback( conditional, timebase_callback, arg);
}

void JackDebugClient::TransportLocate(jack_nframes_t frame)
{
    CheckClient("TransportLocate");
    *fStream << "JackClientDebug : TransportLocate frame " << frame << endl;
    fClient->TransportLocate(frame);
}

jack_transport_state_t JackDebugClient::TransportQuery(jack_position_t* pos)
{
    CheckClient("TransportQuery");
    return fClient->TransportQuery(pos);
}

jack_nframes_t JackDebugClient::GetCurrentTransportFrame()
{
    CheckClient("GetCurrentTransportFrame");
    return fClient->GetCurrentTransportFrame();
}

int JackDebugClient::TransportReposition(jack_position_t* pos)
{
    CheckClient("TransportReposition");
    return fClient->TransportReposition(pos);
}

void JackDebugClient::TransportStart()
{
    CheckClient("TransportStart");
    fClient->TransportStart();
}

void JackDebugClient::TransportStop()
{
    CheckClient("TransportStop");
    fClient->TransportStop();
}

//---------------------
// Callback management
//---------------------

void JackDebugClient::OnShutdown(JackShutdownCallback callback, void *arg)
{
    CheckClient("OnShutdown");
    fClient->OnShutdown(callback, arg);
}

void JackDebugClient::OnInfoShutdown(JackInfoShutdownCallback callback, void *arg)
{
    CheckClient("OnInfoShutdown");
    fClient->OnInfoShutdown(callback, arg);
}

int JackDebugClient::TimeCallback(jack_nframes_t nframes, void *arg)
{
    JackDebugClient* client = (JackDebugClient*)arg;
    jack_time_t t1 = GetMicroSeconds();
    int res = client->fProcessTimeCallback(nframes, client->fProcessTimeCallbackArg);
    if (res == 0) {
        jack_time_t t2 = GetMicroSeconds();
        long delta = long((t2 - t1) - client->GetEngineControl()->fPeriodUsecs);
        if (delta > 0 && !client->fFreewheel) {
            *client->fStream << "!!! ERROR !!! : Process overload of " << delta << " us" << endl;
        }
    }
    return res;
}

int JackDebugClient::SetProcessCallback(JackProcessCallback callback, void *arg)
{
    CheckClient("SetProcessCallback");
    
    fProcessTimeCallback = callback;
    fProcessTimeCallbackArg = arg;
        
    if (callback == NULL) {
        // Clear the callback...
        return fClient->SetProcessCallback(callback, arg);
    } else {
        // Setup the measuring version...
        return fClient->SetProcessCallback(TimeCallback, this);
    }
}

int JackDebugClient::SetXRunCallback(JackXRunCallback callback, void *arg)
{
    CheckClient("SetXRunCallback");
    return fClient->SetXRunCallback(callback, arg);
}

int JackDebugClient::SetInitCallback(JackThreadInitCallback callback, void *arg)
{
    CheckClient("SetInitCallback");
    return fClient->SetInitCallback(callback, arg);
}

int JackDebugClient::SetGraphOrderCallback(JackGraphOrderCallback callback, void *arg)
{
    CheckClient("SetGraphOrderCallback");
    return fClient->SetGraphOrderCallback(callback, arg);
}

int JackDebugClient::SetBufferSizeCallback(JackBufferSizeCallback callback, void *arg)
{
    CheckClient("SetBufferSizeCallback");
    return fClient->SetBufferSizeCallback(callback, arg);
}

int JackDebugClient::SetClientRegistrationCallback(JackClientRegistrationCallback callback, void* arg)
{
    CheckClient("SetClientRegistrationCallback");
    return fClient->SetClientRegistrationCallback(callback, arg);
}

int JackDebugClient::SetFreewheelCallback(JackFreewheelCallback callback, void *arg)
{
    CheckClient("SetFreewheelCallback");
    return fClient->SetFreewheelCallback(callback, arg);
}

int JackDebugClient::SetPortRegistrationCallback(JackPortRegistrationCallback callback, void *arg)
{
    CheckClient("SetPortRegistrationCallback");
    return fClient->SetPortRegistrationCallback(callback, arg);
}

int JackDebugClient::SetPortConnectCallback(JackPortConnectCallback callback, void *arg)
{
    CheckClient("SetPortConnectCallback");
    return fClient->SetPortConnectCallback(callback, arg);
}

int JackDebugClient::SetPortRenameCallback(JackPortRenameCallback callback, void *arg)
{
    CheckClient("SetPortRenameCallback");
    return fClient->SetPortRenameCallback(callback, arg);
}

int JackDebugClient::SetSessionCallback(JackSessionCallback callback, void *arg)
{
    CheckClient("SetSessionCallback");
    return fClient->SetSessionCallback(callback, arg);
}

int JackDebugClient::SetLatencyCallback(JackLatencyCallback callback, void *arg)
{
    CheckClient("SetLatencyCallback");
    return fClient->SetLatencyCallback(callback, arg);
}

int JackDebugClient::SetProcessThread(JackThreadCallback fun, void *arg)
{
    CheckClient("SetProcessThread");
    return fClient->SetProcessThread(fun, arg);
}

jack_session_command_t* JackDebugClient::SessionNotify(const char* target, jack_session_event_type_t type, const char* path)
{
    CheckClient("SessionNotify");
    *fStream << "JackClientDebug : SessionNotify target " << target << "type " << type << "path " << path << endl;
    return fClient->SessionNotify(target, type, path);
}

int JackDebugClient::SessionReply(jack_session_event_t* ev)
{
    CheckClient("SessionReply");
    return fClient->SessionReply(ev);
}

char* JackDebugClient::GetUUIDForClientName(const char* client_name)
{
    CheckClient("GetUUIDForClientName");
    *fStream << "JackClientDebug : GetUUIDForClientName client_name " << client_name << endl;
    return fClient->GetUUIDForClientName(client_name);
}

char* JackDebugClient::GetClientNameByUUID(const char* uuid)
{
    CheckClient("GetClientNameByUUID");
    *fStream << "JackClientDebug : GetClientNameByUUID uuid " << uuid << endl;
    return fClient->GetClientNameByUUID(uuid);
}

int JackDebugClient::ReserveClientName(const char* client_name, const char* uuid)
{
    CheckClient("ReserveClientName");
    *fStream << "JackClientDebug : ReserveClientName client_name " << client_name << "uuid " << uuid << endl;
    return fClient->ReserveClientName(client_name, uuid);
}

int JackDebugClient::ClientHasSessionCallback(const char* client_name)
{
    CheckClient("ClientHasSessionCallback");
    *fStream << "JackClientDebug : ClientHasSessionCallback client_name " << client_name << endl;
    return fClient->ClientHasSessionCallback(client_name);
}

JackClientControl* JackDebugClient::GetClientControl() const
{
    CheckClient("GetClientControl");
    return fClient->GetClientControl();
}

// Internal clients
char* JackDebugClient::GetInternalClientName(int ref)
{
    CheckClient("GetInternalClientName");
    return fClient->GetInternalClientName(ref);
}

int JackDebugClient::InternalClientHandle(const char* client_name, jack_status_t* status)
{
    CheckClient("InternalClientHandle");
    return fClient->InternalClientHandle(client_name, status);
}

int JackDebugClient::InternalClientLoad(const char* client_name, jack_options_t options, jack_status_t* status, jack_varargs_t* va)
{
    CheckClient("InternalClientLoad");
    return fClient->InternalClientLoad(client_name, options, status, va);
}

void JackDebugClient::InternalClientUnload(int ref, jack_status_t* status)
{
    CheckClient("InternalClientUnload");
    fClient->InternalClientUnload(ref, status);
}

} // end of namespace

