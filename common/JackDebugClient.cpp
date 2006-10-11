/*
Copyright (C) 2004-2006 Grame

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

#include "JackDebugClient.h"
#include "JackError.h"
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
    fTotalPortNumber = 1;	// The total number of port opened and maybe closed. Historical view.
    fOpenPortNumber = 0;	// The current number of opened port.
    fIsActivated = 0;
    fIsDeactivated = 0;
    fIsClosed = 0;
    fClient = client;
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
        *fStream << "!!! WARNING !!! Some ports have not been unregistrated ! Incorrect exiting !" << endl;
    if (fIsDeactivated != fIsActivated)
        *fStream << "!!! ERROR !!! Client seem do not perform symetric activation-deactivation ! (not the same number of activate and deactivate)" << endl;
    if (fIsClosed == 0)
        *fStream << "!!! ERROR !!! Client have not been closed with jack_client_close() !" << endl;

    *fStream << endl << endl << "---------------------------- JackDebugClient detailed port summary ------------------------ " << endl << endl;
    //for (int i = 0; i < fTotalPortNumber ; i++) {
    for (int i = 1; i <= fTotalPortNumber ; i++) {
        *fStream << endl << "Port index (internal debug test value) : " << i << endl;
        *fStream << setw(5) << "- Name : " << fPortList[i].name << endl;
        *fStream << setw(5) << "- idport : " << fPortList[i].idport << endl;
        *fStream << setw(5) << "- IsConnected : " << fPortList[i].IsConnected << endl;
        *fStream << setw(5) << "- IsUnregistrated : " << fPortList[i].IsUnregistrated << endl;
        if (fPortList[i].IsUnregistrated == 0)
            *fStream << "!!! WARNING !!! Port have not been unregistrated ! Incorrect exiting !" << endl;
    }
    *fStream << "delete object JackDebugClient : end of tracing" << endl;
    delete fStream;
    delete fClient;
}

int JackDebugClient::Open(const char* name)
{
    int Tidport;
    Tidport = fClient->Open(name);
    char provstr[256];
    char buffer[256];
    time_t curtime;
    struct tm *loctime;
    /* Get the current time. */
    curtime = time (NULL);
    /* Convert it to local time representation. */
    loctime = localtime (&curtime);
    strftime (buffer, 256, "%I-%M", loctime);
    sprintf(provstr, "JackClientDebug-%s-%s.log", name, buffer);
    fStream = new ofstream(provstr, ios_base::ate);
    if (fStream->is_open()) {
        if (Tidport == -1) {
            *fStream << "Trying to Open Client with name '" << name << "' with bad result (client not opened)." << Tidport << endl;
        } else {
            *fStream << "Open Client with name '" << name << "'." << endl;
        }
    } else {
        JackLog("JackDebugClient::Open : cannot open log file\n");
    }
    strcpy(fClientName, name);
    return Tidport;
}

int JackDebugClient::Close()
{
    fIsClosed++;
    *fStream << "Client '" << fClientName << "' was Closed" << endl;
    return fClient->Close();
}

pthread_t JackDebugClient::GetThreadID()
{
    return fClient->GetThreadID();
}

JackGraphManager* JackDebugClient::GetGraphManager() const
{
    return fClient->GetGraphManager();
}
JackEngineControl* JackDebugClient::GetEngineControl() const
{
    return fClient->GetEngineControl();
}
/*!
\brief Notification received from the server.
*/

int JackDebugClient::ClientNotify(int refnum, const char* name, int notify, int sync, int value)
{
    return fClient->ClientNotify( refnum, name, notify, sync, value);
}

int JackDebugClient::Activate()
{
    int Tidport;
    Tidport = fClient->Activate();
    fIsActivated++;
    if (fIsDeactivated)
        *fStream << "Client '" << fClientName << "' call activate a new time (it already call 'activate' previously)." << endl;
    *fStream << "Client '" << fClientName << "' Activated" << endl;
    if (Tidport != 0)
        *fStream << "Client '" << fClientName << "' try to activate but server return " << Tidport << " ." << endl;
    return Tidport;
}

int JackDebugClient::Deactivate()
{
    int Tidport;
    Tidport = fClient->Deactivate();
    fIsDeactivated++;
    if (fIsActivated == 0)
        *fStream << "Client '" << fClientName << "' deactivate while it hasn't been previoulsy activated !" << endl;
    *fStream << "Client '" << fClientName << "' Deactivated" << endl;
    if (Tidport != 0)
        *fStream << "Client '" << fClientName << "' try to deactivate but server return " << Tidport << " ." << endl;
    return Tidport;
}

//-----------------
// Port management
//-----------------

int JackDebugClient::PortRegister(const char* port_name, const char* port_type, unsigned long flags, unsigned long buffer_size)
{
    int Tidport;
    Tidport = fClient->PortRegister(port_name, port_type, flags, buffer_size);
    if (Tidport <= 0) {
        *fStream << "Client '" << fClientName << "' try port Register ('" << port_name << "') and server return error  " << Tidport << " ." << endl;
    } else {
        if (fTotalPortNumber < MAX_PORT_HISTORY) {
            fPortList[fTotalPortNumber].idport = Tidport;
            strcpy(fPortList[fTotalPortNumber].name, port_name);
            fPortList[fTotalPortNumber].IsConnected = 0;
            fPortList[fTotalPortNumber].IsUnregistrated = 0;
        } else {
            *fStream << "!!! WARNING !!! History is full : no more port history will be recorded." << endl;
        }
        fTotalPortNumber++;
        fOpenPortNumber++;
        *fStream << "Client '" << fClientName << "' port Register with portname '" << port_name << " port " << Tidport << "' ." << endl;
    }
    return Tidport;
}

int JackDebugClient::PortUnRegister(jack_port_id_t port_index)
{
    int Tidport;
    Tidport = fClient->PortUnRegister(port_index);
    fOpenPortNumber--;
    int i;
    for (i = (fTotalPortNumber - 1); i >= 0; i--) {	// We search the record into the history
        if (fPortList[i].idport == port_index) {		// We found the last record
            if (fPortList[i].IsUnregistrated != 0)
                *fStream << "!!! ERROR !!! : '" << fClientName << "' id deregistering port '" << fPortList[i].name << "' that have already been unregistered !" << endl;
            fPortList[i].IsUnregistrated++;
            break;
        }
    }
    if (i == 0) // Port is not found
        *fStream << "JackClientDebug : PortUnregister : port " << port_index << " was not previously registered !" << endl;
    if (Tidport != 0)
        *fStream << "Client '" << fClientName << "' try to do PortUnregister and server return " << Tidport << " )." << endl;
    *fStream << "Client '" << fClientName << "' unregister port '" << port_index << "'." << endl;
    return Tidport;
}

int JackDebugClient::PortConnect(const char* src, const char* dst)
{
    if (!(fIsActivated))
        *fStream << "!!! ERROR !!! Trying to connect a port ( " << src << " to " << dst << ") while the client has not been activated !" << endl;
    int Tidport;
    int i;
    Tidport = fClient->PortConnect( src, dst);
    for (i = (fTotalPortNumber - 1); i >= 0; i--) {	// We search the record into the history
        if (strcmp(fPortList[i].name, src) == 0) {	// We found the last record in sources
            if (fPortList[i].IsUnregistrated != 0)
                *fStream << "!!! ERROR !!! Connecting port " << src << " previoulsy unregistered !" << endl;
            fPortList[i].IsConnected++;
            *fStream << "Connecting port " << src << " to " << dst << ". ";
            break;
        } else if (strcmp(fPortList[i].name, dst) == 0 ) { // We found the record in dest
            if (fPortList[i].IsUnregistrated != 0)
                *fStream << "!!! ERROR !!! Connecting port  " << dst << " previoulsy unregistered !" << endl;
            fPortList[i].IsConnected++;
            *fStream << "Connecting port " << src << " to " << dst << ". ";
            break;
        }
    }
    if (i == 0) // Port is not found
        *fStream << "JackClientDebug : PortConnect : port was not found in debug database !" << endl;
    if (Tidport != 0)
        *fStream << "Client '" << fClientName << "' try to do PortConnect but server return " << Tidport << " ." << endl;
    //*fStream << "Client Port Connect done with names" << endl;
    return Tidport;
}

int JackDebugClient::PortDisconnect(const char* src, const char* dst)
{
    if (!(fIsActivated))
        *fStream << "!!! ERROR !!! Trying to disconnect a port ( " << src << " to " << dst << ") while the client has not been activated !" << endl;
    int Tidport;
    Tidport = fClient->PortDisconnect( src, dst);
    int i;
    for (i = (fTotalPortNumber - 1); i >= 0; i--) { // We search the record into the history
        if (strcmp(fPortList[i].name, src) == 0) { // We found the record in sources
            if (fPortList[i].IsUnregistrated != 0)
                *fStream << "!!! ERROR !!! : Disconnecting port " << src << " previoulsy unregistered !" << endl;
            fPortList[i].IsConnected--;
            *fStream << "disconnecting port " << src << ". ";
            break;
        } else if (strcmp(fPortList[i].name, dst) == 0 ) { // We found the record in dest
            if (fPortList[i].IsUnregistrated != 0)
                *fStream << "!!! ERROR !!! : Disonnecting port  " << dst << " previoulsy unregistered !" << endl;
            fPortList[i].IsConnected--;
            *fStream << "disconnecting port " << dst << ". ";
            break;
        }
    }
    if (i == 0) // Port is not found
        *fStream << "JackClientDebug : PortDisConnect : port was not found in debug database !" << endl;
    if (Tidport != 0)
        *fStream << "Client '" << fClientName << "' try to do PortDisconnect but server return " << Tidport << " ." << endl;
    //*fStream << "Client Port Disconnect done." << endl;
    return Tidport;
}

int JackDebugClient::PortConnect(jack_port_id_t src, jack_port_id_t dst)
{
    if (!(fIsActivated))
        *fStream << "!!! ERROR !!! : Trying to connect port  " << src << " to  " << dst << " while the client has not been activated !" << endl;
    int Tidport;
    Tidport = fClient->PortConnect(src, dst);
    int i;
    for (i = (fTotalPortNumber - 1); i >= 0; i--) {	// We search the record into the history
        if (fPortList[i].idport == src) {		// We found the record in sources
            if (fPortList[i].IsUnregistrated != 0)
                *fStream << "!!! ERROR !!! : Connecting port  " << src << " previoulsy unregistered !" << endl;
            fPortList[i].IsConnected++;
            *fStream << "Connecting port " << src << ". ";
            break;
        } else if (fPortList[i].idport == dst) { // We found the record in dest
            if (fPortList[i].IsUnregistrated != 0)
                *fStream << "!!! ERROR !!! : Connecting port  " << dst << " previoulsy unregistered !" << endl;
            fPortList[i].IsConnected++;
            *fStream << "Connecting port " << dst << ". ";
            break;
        }
    }
    if (i == 0) // Port is not found
        *fStream << "JackClientDebug : PortConnect : port was not found in debug database !" << endl;
    if (Tidport == -1)
        *fStream << "Client '" << fClientName << "' try to do Portconnect but server return " << Tidport << " ." << endl;
    //*fStream << "Client Port Connect with ID done." << endl;
    return Tidport;
}

int JackDebugClient::PortDisconnect(jack_port_id_t src)
{
    if (!(fIsActivated))
        *fStream << "!!! ERROR !!! : Trying to disconnect port  " << src << " while that client has not been activated !" << endl;
    int Tidport;
    Tidport = fClient->PortDisconnect(src);
    int i;
    for (i = (fTotalPortNumber - 1); i >= 0; i--) {		// We search the record into the history
        if (fPortList[i].idport == src) {				// We found the record in sources
            if (fPortList[i].IsUnregistrated != 0)
                *fStream << "!!! ERROR !!! : Disconnecting port  " << src << " previoulsy unregistered !" << endl;
            fPortList[i].IsConnected--;
            *fStream << "Disconnecting port " << src << ". " << endl;
            break;
        }
    }
    if (i == 0) // Port is not found
        *fStream << "JackClientDebug : PortDisconnect : port was not found in debug database !" << endl;
    if (Tidport != 0)
        *fStream << "Client '" << fClientName << "' try to do PortDisconnect but server return " << Tidport << " ." << endl;
    //*fStream << "Client Port Disconnect with ID done." << endl;
    return Tidport;
}

int JackDebugClient::PortIsMine(jack_port_id_t port_index)
{
    return fClient->PortIsMine(port_index);
}

//--------------------
// Context management
//--------------------

int JackDebugClient::SetBufferSize(jack_nframes_t buffer_size)
{
    return fClient->SetBufferSize(buffer_size);
}

int JackDebugClient::SetFreeWheel(int onoff)
{
    return fClient->SetFreeWheel(onoff);
}

/*
ShutDown is called:
- from the RT thread when Execute method fails
- possibly from a "closed" notification channel
(Not needed since the synch object used (Sema of Fifo will fails when server quits... see ShutDown))
*/

void JackDebugClient::ShutDown()
{
    fClient->ShutDown();
}

//---------------------
// Transport management
//---------------------

int JackDebugClient::ReleaseTimebase()
{
    return fClient->ReleaseTimebase();
}

int JackDebugClient::SetSyncCallback(JackSyncCallback sync_callback, void* arg)
{
    return fClient->SetSyncCallback(sync_callback, arg);
}

int JackDebugClient::SetSyncTimeout(jack_time_t timeout)
{
    return fClient->SetSyncTimeout(timeout);
}

int JackDebugClient::SetTimebaseCallback(int conditional, JackTimebaseCallback timebase_callback, void* arg)
{
    return fClient->SetTimebaseCallback( conditional, timebase_callback, arg);
}

int JackDebugClient::TransportLocate(jack_nframes_t frame)
{
    return fClient->TransportLocate(frame);
}

jack_transport_state_t JackDebugClient::TransportQuery(jack_position_t* pos)
{
    return fClient->TransportQuery(pos);
}

jack_nframes_t JackDebugClient::GetCurrentTransportFrame()
{
    return fClient->GetCurrentTransportFrame();
}

int JackDebugClient::TransportReposition(jack_position_t* pos)
{
    return fClient->TransportReposition(pos);
}

void JackDebugClient::TransportStart()
{
    fClient->TransportStart();
}

void JackDebugClient::TransportStop()
{
    fClient->TransportStop();
}

//---------------------
// Callback management
//---------------------

void JackDebugClient::OnShutdown(JackShutdownCallback callback, void *arg)
{
    fClient->OnShutdown(callback, arg);
}

int JackDebugClient::SetProcessCallback(JackProcessCallback callback, void *arg)
{
    return fClient->SetProcessCallback( callback, arg);
}

int JackDebugClient::SetXRunCallback(JackXRunCallback callback, void *arg)
{
    return fClient->SetXRunCallback(callback, arg);
}

int JackDebugClient::SetInitCallback(JackThreadInitCallback callback, void *arg)
{
    return fClient->SetInitCallback(callback, arg);
}

int JackDebugClient::SetGraphOrderCallback(JackGraphOrderCallback callback, void *arg)
{
    return fClient->SetGraphOrderCallback(callback, arg);
}

int JackDebugClient::SetBufferSizeCallback(JackBufferSizeCallback callback, void *arg)
{
    return fClient->SetBufferSizeCallback(callback, arg);
}

int JackDebugClient::SetFreewheelCallback(JackFreewheelCallback callback, void *arg)
{
    return fClient->SetFreewheelCallback(callback, arg);
}

int JackDebugClient::SetPortRegistrationCallback(JackPortRegistrationCallback callback, void *arg)
{
    return fClient->SetPortRegistrationCallback(callback, arg);
}

JackClientControl* JackDebugClient::GetClientControl() const
{
    return fClient->GetClientControl();
}

} // end of namespace

