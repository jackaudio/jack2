/*
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

#include <iostream>
#include <fstream>
#include <assert.h>

#include "JackSystemDeps.h"
#include "JackLockedEngine.h"
#include "JackExternalClient.h"
#include "JackInternalClient.h"
#include "JackEngineControl.h"
#include "JackClientControl.h"
#include "JackGlobals.h"
#include "JackChannel.h"
#include "JackError.h"

namespace Jack
{

#define AssertRefnum(ref) assert(ref >= 0 && ref < CLIENT_NUM);

JackEngine::JackEngine(JackGraphManager* manager,
                       JackSynchro* table,
                       JackEngineControl* control)
{
    fGraphManager = manager;
    fSynchroTable = table;
    fEngineControl = control;
    for (int i = 0; i < CLIENT_NUM; i++)
        fClientTable[i] = NULL;
}

JackEngine::~JackEngine()
{
    jack_log("JackEngine::~JackEngine");
}

int JackEngine::Open()
{
    jack_log("JackEngine::Open");

    // Open audio thread => request thread communication channel
    if (fChannel.Open(fEngineControl->fServerName) < 0) {
        jack_error("Cannot connect to server");
        return -1;
    } else {
        return 0;
    }
}

int JackEngine::Close()
{
    jack_log("JackEngine::Close");
    fChannel.Close();
    
    // Close remaining clients (RT is stopped)
    for (int i = 0; i < CLIENT_NUM; i++) {
        if (JackLoadableInternalClient* loadable_client = dynamic_cast<JackLoadableInternalClient*>(fClientTable[i])) {
            jack_log("JackEngine::Close loadable client = %s", loadable_client->GetClientControl()->fName);
            loadable_client->Close();
            // Close does not delete the pointer for internal clients
            fClientTable[i] = NULL;
            delete loadable_client;
        } else if (JackExternalClient* external_client = dynamic_cast<JackExternalClient*>(fClientTable[i])) {
            jack_log("JackEngine::Close external client = %s", external_client->GetClientControl()->fName);
            external_client->Close();
            // Close deletes the pointer for external clients
            fClientTable[i] = NULL;
        }
    }
    
    fSignal.Destroy();
    return 0;
}
    
//-----------------------------
// Client ressource management
//-----------------------------

int JackEngine::AllocateRefnum()
{
    for (int i = 0; i < CLIENT_NUM; i++) {
        if (!fClientTable[i]) {
            jack_log("JackEngine::AllocateRefNum ref = %ld", i);
            return i;
        }
    }
    return -1;
}

void JackEngine::ReleaseRefnum(int ref)
{
    fClientTable[ref] = NULL;

    if (fEngineControl->fTemporary) {
        int i;
        for (i = REAL_REFNUM; i < CLIENT_NUM; i++) {
            if (fClientTable[i])
                break;
        }
        if (i == CLIENT_NUM) {
            // last client and temporay case: quit the server
            jack_log("JackEngine::ReleaseRefnum server quit");
            fEngineControl->fTemporary = false;
#ifndef WIN32
 	    exit(0);
#endif
        }
    }
}

//------------------
// Graph management
//------------------

void JackEngine::ProcessNext(jack_time_t cur_cycle_begin)
{
    fLastSwitchUsecs = cur_cycle_begin;
    if (fGraphManager->RunNextGraph())	// True if the graph actually switched to a new state
        fChannel.Notify(ALL_CLIENTS, kGraphOrderCallback, 0);
    fSignal.SignalAll();                // Signal for threads waiting for next cycle
}

void JackEngine::ProcessCurrent(jack_time_t cur_cycle_begin)
{
    if (cur_cycle_begin < fLastSwitchUsecs + 2 * fEngineControl->fPeriodUsecs) // Signal XRun only for the first failing cycle
        CheckXRun(cur_cycle_begin);
    fGraphManager->RunCurrentGraph();
}

bool JackEngine::Process(jack_time_t cur_cycle_begin, jack_time_t prev_cycle_end)
{
    bool res = true;

    // Cycle  begin
    fEngineControl->CycleBegin(fClientTable, fGraphManager, cur_cycle_begin, prev_cycle_end);

    // Graph
    if (fGraphManager->IsFinishedGraph()) {
        ProcessNext(cur_cycle_begin);
        res = true;
    } else {
        jack_log("Process: graph not finished!");
        if (cur_cycle_begin > fLastSwitchUsecs + fEngineControl->fTimeOutUsecs) {
            jack_log("Process: switch to next state delta = %ld", long(cur_cycle_begin - fLastSwitchUsecs));
            ProcessNext(cur_cycle_begin);
            res = true;
        } else {
            jack_log("Process: waiting to switch delta = %ld", long(cur_cycle_begin - fLastSwitchUsecs));
            ProcessCurrent(cur_cycle_begin);
            res = false;
        }
    }

    // Cycle end
    fEngineControl->CycleEnd(fClientTable);
    return res;
}


/*
Client that finish *after* the callback date are considered late even if their output buffers may have been
correctly mixed in the time window: callbackUsecs <==> Read <==> Write.
*/

void JackEngine::CheckXRun(jack_time_t callback_usecs)  // REVOIR les conditions de fin
{
    for (int i = REAL_REFNUM; i < CLIENT_NUM; i++) {
        JackClientInterface* client = fClientTable[i];
        if (client && client->GetClientControl()->fActive) {
            JackClientTiming* timing = fGraphManager->GetClientTiming(i);
            jack_client_state_t status = timing->fStatus;
            jack_time_t finished_date = timing->fFinishedAt;

            if (status != NotTriggered && status != Finished) {
                jack_error("JackEngine::XRun: client = %s was not run: state = %ld", client->GetClientControl()->fName, status);
                fChannel.Notify(ALL_CLIENTS, kXRunCallback, 0);  // Notify all clients
            }

            if (status == Finished && (long)(finished_date - callback_usecs) > 0) {
                jack_error("JackEngine::XRun: client %s finished after current callback", client->GetClientControl()->fName);
                fChannel.Notify(ALL_CLIENTS, kXRunCallback, 0);  // Notify all clients
            }
        }
    }
}

//---------------
// Notifications
//---------------

void JackEngine::NotifyClient(int refnum, int event, int sync, int value1, int value2)
{
    JackClientInterface* client = fClientTable[refnum];

    // The client may be notified by the RT thread while closing
    if (!client) {
        jack_log("JackEngine::NotifyClient: client not available anymore");
    } else if (client->GetClientControl()->fCallback[event]) {
        if (client->ClientNotify(refnum, client->GetClientControl()->fName, event, sync, value1, value2) < 0)
            jack_error("NotifyClient fails name = %s event = %ld = val1 = %ld val2 = %ld", client->GetClientControl()->fName, event, value1, value2);
    } else {
        jack_log("JackEngine::NotifyClient: no callback for event = %ld", event);
    }
}

void JackEngine::NotifyClients(int event, int sync, int value1, int value2)
{
    for (int i = 0; i < CLIENT_NUM; i++) {
        JackClientInterface* client = fClientTable[i];
        if (client) {
            if (client->GetClientControl()->fCallback[event]) {
                if (client->ClientNotify(i, client->GetClientControl()->fName, event, sync, value1, value2) < 0)
                    jack_error("NotifyClient fails name = %s event = %ld = val1 = %ld val2 = %ld", client->GetClientControl()->fName, event, value1, value2);
            } else {
                jack_log("JackEngine::NotifyClients: no callback for event = %ld", event);
            }
        }
    }
}

int JackEngine::NotifyAddClient(JackClientInterface* new_client, const char* name, int refnum)
{
    // Notify existing clients of the new client and new client of existing clients.
    for (int i = 0; i < CLIENT_NUM; i++) {
        JackClientInterface* old_client = fClientTable[i];
        if (old_client) {
            if (old_client->ClientNotify(refnum, name, kAddClient, true, 0, 0) < 0) {
                jack_error("NotifyAddClient old_client fails name = %s", old_client->GetClientControl()->fName);
                return -1;
            }
            if (new_client->ClientNotify(i, old_client->GetClientControl()->fName, kAddClient, true, 0, 0) < 0) {
                jack_error("NotifyAddClient new_client fails name = %s", name);
                return -1;
            }
        }
    }

    return 0;
}

void JackEngine::NotifyRemoveClient(const char* name, int refnum)
{
    // Notify existing clients (including the one beeing suppressed) of the removed client
    for (int i = 0; i < CLIENT_NUM; i++) {
        JackClientInterface* client = fClientTable[i];
        if (client) {
            client->ClientNotify(refnum, name, kRemoveClient, true, 0, 0);
        }
    }
}

// Coming from the driver
void JackEngine::NotifyXRun(jack_time_t callback_usecs, float delayed_usecs)
{
    // Use the audio thread => request thread communication channel
    fEngineControl->ResetFrameTime(callback_usecs);
    fEngineControl->NotifyXRun(delayed_usecs);
    fChannel.Notify(ALL_CLIENTS, kXRunCallback, 0);
}

void JackEngine::NotifyXRun(int refnum)
{
    if (refnum == ALL_CLIENTS) {
        NotifyClients(kXRunCallback, false, 0, 0);
    } else {
        NotifyClient(refnum, kXRunCallback, false, 0, 0);
    }
}

void JackEngine::NotifyGraphReorder()
{
    NotifyClients(kGraphOrderCallback, false, 0, 0);
}

void JackEngine::NotifyBufferSize(jack_nframes_t buffer_size)
{
    NotifyClients(kBufferSizeCallback, true, buffer_size, 0);
}

void JackEngine::NotifySampleRate(jack_nframes_t sample_rate)
{
    NotifyClients(kSampleRateCallback, true, sample_rate, 0);
}

void JackEngine::NotifyFreewheel(bool onoff)
{
    fEngineControl->fRealTime = !onoff;
    NotifyClients((onoff ? kStartFreewheelCallback : kStopFreewheelCallback), true, 0, 0);
}

void JackEngine::NotifyPortRegistation(jack_port_id_t port_index, bool onoff)
{
    NotifyClients((onoff ? kPortRegistrationOnCallback : kPortRegistrationOffCallback), false, port_index, 0);
}

void JackEngine::NotifyPortRename(jack_port_id_t port)
{
    NotifyClients(kPortRenameCallback, false, port, 0);
}

void JackEngine::NotifyPortConnect(jack_port_id_t src, jack_port_id_t dst, bool onoff)
{
    NotifyClients((onoff ? kPortConnectCallback : kPortDisconnectCallback), false, src, dst);
}

void JackEngine::NotifyActivate(int refnum)
{
    NotifyClient(refnum, kActivateClient, true, 0, 0);
}

//----------------------------
// Loadable client management
//----------------------------

int JackEngine::GetInternalClientName(int refnum, char* name_res)
{
    AssertRefnum(refnum);
    JackClientInterface* client = fClientTable[refnum];
    if (client) {
        strncpy(name_res, client->GetClientControl()->fName, JACK_CLIENT_NAME_SIZE);
        return 0;
    } else {
        return -1;
    }
}

int JackEngine::InternalClientHandle(const char* client_name, int* status, int* int_ref)
{
    // Clear status
    *status = 0;

    for (int i = 0; i < CLIENT_NUM; i++) {
        JackClientInterface* client = fClientTable[i];
        if (client && dynamic_cast<JackLoadableInternalClient*>(client) && (strcmp(client->GetClientControl()->fName, client_name) == 0)) {
            jack_log("InternalClientHandle found client name = %s ref = %ld",  client_name, i);
            *int_ref = i;
            return 0;
        }
    }

    *status |= (JackNoSuchClient | JackFailure);
    return -1;
}

int JackEngine::InternalClientUnload(int refnum, int* status)
{
    AssertRefnum(refnum);
    JackClientInterface* client = fClientTable[refnum];
    if (client) {
        int res = client->Close();
        delete client;
        *status = 0;
        return res;
    } else {
        *status = (JackNoSuchClient | JackFailure);
        return -1;
    }
}

//-------------------
// Client management
//-------------------

int JackEngine::ClientCheck(const char* name, char* name_res, int protocol, int options, int* status)
{
    // Clear status
    *status = 0;
    strcpy(name_res, name);

    jack_log("Check protocol client %ld server = %ld", protocol, JACK_PROTOCOL_VERSION);

    if (protocol != JACK_PROTOCOL_VERSION) {
        *status |= (JackFailure | JackVersionError);
        jack_error("JACK protocol mismatch (%d vs %d)", protocol, JACK_PROTOCOL_VERSION);
        return -1;
    }

    if (ClientCheckName(name)) {

        *status |= JackNameNotUnique;

        if (options & JackUseExactName) {
            jack_error("cannot create new client; %s already exists", name);
            *status |= JackFailure;
            return -1;
        }

        if (GenerateUniqueName(name_res)) {
            *status |= JackFailure;
            return -1;
        }
    }

    return 0;
}

bool JackEngine::GenerateUniqueName(char* name)
{
    int tens, ones;
    int length = strlen(name);

    if (length > JACK_CLIENT_NAME_SIZE - 4) {
        jack_error("%s exists and is too long to make unique", name);
        return true;		/* failure */
    }

    /*  generate a unique name by appending "-01".."-99" */
    name[length++] = '-';
    tens = length++;
    ones = length++;
    name[tens] = '0';
    name[ones] = '1';
    name[length] = '\0';

    while (ClientCheckName(name)) {
        if (name[ones] == '9') {
            if (name[tens] == '9') {
                jack_error("client %s has 99 extra instances already", name);
                return true; /* give up */
            }
            name[tens]++;
            name[ones] = '0';
        } else {
            name[ones]++;
        }
    }
    return false;
}

bool JackEngine::ClientCheckName(const char* name)
{
    for (int i = 0; i < CLIENT_NUM; i++) {
        JackClientInterface* client = fClientTable[i];
        if (client && (strcmp(client->GetClientControl()->fName, name) == 0))
            return true;
    }

    return false;
}

int JackEngine::GetClientPID(const char* name)
{
    for (int i = 0; i < CLIENT_NUM; i++) {
        JackClientInterface* client = fClientTable[i];
        if (client && (strcmp(client->GetClientControl()->fName, name) == 0))
            return client->GetClientControl()->fPID;
    }
    
    return 0;
}

// Used for external clients
int JackEngine::ClientExternalOpen(const char* name, int pid, int* ref, int* shared_engine, int* shared_client, int* shared_graph_manager)
{
    jack_log("JackEngine::ClientOpen: name = %s ", name);

    int refnum = AllocateRefnum();
    if (refnum < 0) {
        jack_error("No more refnum available");
        return -1;
    }

    JackExternalClient* client = new JackExternalClient();

    if (!fSynchroTable[refnum].Allocate(name, fEngineControl->fServerName, 0)) {
        jack_error("Cannot allocate synchro");
        goto error;
    }

    if (client->Open(name, pid, refnum, shared_client) < 0) {
        jack_error("Cannot open client");
        goto error;
    }

    if (!fSignal.TimedWait(DRIVER_OPEN_TIMEOUT * 1000000)) {
        // Failure if RT thread is not running (problem with the driver...)
        jack_error("Driver is not running");
        goto error;
    }

    fClientTable[refnum] = client;

    if (NotifyAddClient(client, name, refnum) < 0) {
        jack_error("Cannot notify add client");
        goto error;
    }
 
    fGraphManager->InitRefNum(refnum);
    fEngineControl->ResetRollingUsecs();
    *shared_engine = fEngineControl->GetShmIndex();
    *shared_graph_manager = fGraphManager->GetShmIndex();
    *ref = refnum;
    return 0;

error:
    // Cleanup...
    fSynchroTable[refnum].Destroy();
    fClientTable[refnum] = 0;
    client->Close();
    delete client;
    return -1;
}

// Used for server driver clients
int JackEngine::ClientInternalOpen(const char* name, int* ref, JackEngineControl** shared_engine, JackGraphManager** shared_manager, JackClientInterface* client, bool wait)
{
    jack_log("JackEngine::ClientInternalNew: name = %s", name);

    int refnum = AllocateRefnum();
    if (refnum < 0) {
        jack_error("No more refnum available");
        goto error;
    }

    if (!fSynchroTable[refnum].Allocate(name, fEngineControl->fServerName, 0)) {
        jack_error("Cannot allocate synchro");
        goto error;
    }

    if (wait && !fSignal.TimedWait(DRIVER_OPEN_TIMEOUT * 1000000)) {
        // Failure if RT thread is not running (problem with the driver...)
        jack_error("Driver is not running");
        goto error;
    }

    fClientTable[refnum] = client;

    if (NotifyAddClient(client, name, refnum) < 0) {
        jack_error("Cannot notify add client");
        goto error;
    }

    fGraphManager->InitRefNum(refnum);
    fEngineControl->ResetRollingUsecs();
    *shared_engine = fEngineControl;
    *shared_manager = fGraphManager;
    *ref = refnum;
    return 0;

error:
    // Cleanup...
    fSynchroTable[refnum].Destroy();
    fClientTable[refnum] = 0;
    return -1;
}

// Used for external clients
int JackEngine::ClientExternalClose(int refnum)
{
    AssertRefnum(refnum);
    JackClientInterface* client = fClientTable[refnum];
    
    if (client)	{
        fEngineControl->fTransport.ResetTimebase(refnum);
        int res = ClientCloseAux(refnum, client, true);
        client->Close();
        delete client;
        return res;
    } else {
        return -1;
    }
}

// Used for server internal clients or drivers when the RT thread is stopped
int JackEngine::ClientInternalClose(int refnum, bool wait)
{
    AssertRefnum(refnum);
    JackClientInterface* client = fClientTable[refnum];
    return (client)	? ClientCloseAux(refnum, client, wait) : -1;
}

int JackEngine::ClientCloseAux(int refnum, JackClientInterface* client, bool wait)
{
    jack_log("JackEngine::ClientCloseAux ref = %ld", refnum);

    // Unregister all ports ==> notifications are sent
    jack_int_t ports[PORT_NUM_FOR_CLIENT];
    int i;

    fGraphManager->GetInputPorts(refnum, ports);
    for (i = 0; (i < PORT_NUM_FOR_CLIENT) && (ports[i] != EMPTY) ; i++) {
        PortUnRegister(refnum, ports[i]);
    }

    fGraphManager->GetOutputPorts(refnum, ports);
    for (i = 0; (i < PORT_NUM_FOR_CLIENT) && (ports[i] != EMPTY) ; i++) {
        PortUnRegister(refnum, ports[i]);
    }
    
    // Remove the client from the table
    ReleaseRefnum(refnum);

    // Remove all ports
    fGraphManager->RemoveAllPorts(refnum);

    // Wait until next cycle to be sure client is not used anymore
    if (wait) {
        if (!fSignal.TimedWait(fEngineControl->fTimeOutUsecs * 2)) { // Must wait at least until a switch occurs in Process, even in case of graph end failure
            jack_error("JackEngine::ClientCloseAux wait error ref = %ld", refnum);
        }
    }

    // Notify running clients
    NotifyRemoveClient(client->GetClientControl()->fName, client->GetClientControl()->fRefNum);

    // Cleanup...
    fSynchroTable[refnum].Destroy();
    fEngineControl->ResetRollingUsecs();
    return 0;
}

int JackEngine::ClientActivate(int refnum, bool state)
{
    AssertRefnum(refnum);
    JackClientInterface* client = fClientTable[refnum];
    assert(fClientTable[refnum]);
  
    jack_log("JackEngine::ClientActivate ref = %ld name = %s", refnum, client->GetClientControl()->fName);
    if (state) 
        fGraphManager->Activate(refnum);
 
    // Wait for graph state change to be effective
    if (!fSignal.TimedWait(fEngineControl->fTimeOutUsecs * 10)) {
        jack_error("JackEngine::ClientActivate wait error ref = %ld name = %s", refnum, client->GetClientControl()->fName);
        return -1;
    } else {
        NotifyActivate(refnum);
        return 0;
    }
}

// May be called without client
int JackEngine::ClientDeactivate(int refnum)
{
    AssertRefnum(refnum);
    JackClientInterface* client = fClientTable[refnum];
    if (client == NULL)
        return -1;

    jack_log("JackEngine::ClientDeactivate ref = %ld name = %s", refnum, client->GetClientControl()->fName);
    
    // Disconnect all ports ==> notifications are sent
    jack_int_t ports[PORT_NUM_FOR_CLIENT];
    int i;
    
    fGraphManager->GetInputPorts(refnum, ports);
    for (i = 0; (i < PORT_NUM_FOR_CLIENT) && (ports[i] != EMPTY) ; i++) {
        PortDisconnect(refnum, ports[i], ALL_PORTS);
    }

    fGraphManager->GetOutputPorts(refnum, ports);
    for (i = 0; (i < PORT_NUM_FOR_CLIENT) && (ports[i] != EMPTY) ; i++) {
        PortDisconnect(refnum, ports[i], ALL_PORTS);
    }
    
    fGraphManager->Deactivate(refnum);
    fLastSwitchUsecs = 0; // Force switch to occur next cycle, even when called with "dead" clients

    // Wait for graph state change to be effective
    if (!fSignal.TimedWait(fEngineControl->fTimeOutUsecs * 10)) {
        jack_error("JackEngine::ClientDeactivate wait error ref = %ld name = %s", refnum, client->GetClientControl()->fName);
        return -1;
    } else {
        return 0;
    }
}

//-----------------
// Port management
//-----------------

int JackEngine::PortRegister(int refnum, const char* name, const char *type, unsigned int flags, unsigned int buffer_size, jack_port_id_t* port_index)
{
    jack_log("JackEngine::PortRegister ref = %ld name = %s type = %s flags = %d buffer_size = %d", refnum, name, type, flags, buffer_size);
    AssertRefnum(refnum);
    assert(fClientTable[refnum]);
    
    // Check if port name already exists
    if (fGraphManager->GetPort(name) != NO_PORT) {
        jack_error("port_name \"%s\" already exists", name);
        return -1; 
    }

    *port_index = fGraphManager->AllocatePort(refnum, name, type, (JackPortFlags)flags, fEngineControl->fBufferSize);
    if (*port_index != NO_PORT) {
        NotifyPortRegistation(*port_index, true);
        return 0;
    } else {
        return -1;
    }
}

int JackEngine::PortUnRegister(int refnum, jack_port_id_t port_index)
{
    jack_log("JackEngine::PortUnRegister ref = %ld port_index = %ld", refnum, port_index);
    AssertRefnum(refnum);
    assert(fClientTable[refnum]);
    
    // Disconnect port ==> notification is sent
    PortDisconnect(refnum, port_index, ALL_PORTS);

    if (fGraphManager->ReleasePort(refnum, port_index) == 0) {
        NotifyPortRegistation(port_index, false);
        return 0;
    } else {
        return -1;
    }
}

int JackEngine::PortConnect(int refnum, const char* src, const char* dst)
{
    jack_log("JackEngine::PortConnect src = %s dst = %s", src, dst);
    jack_port_id_t port_src, port_dst;

    return (fGraphManager->CheckPorts(src, dst, &port_src, &port_dst) < 0)
           ? -1
           : PortConnect(refnum, port_src, port_dst);
}

int JackEngine::PortConnect(int refnum, jack_port_id_t src, jack_port_id_t dst)
{
    jack_log("JackEngine::PortConnect src = %d dst = %d", src, dst);
    AssertRefnum(refnum);
    JackClientInterface* client;
    int ref;

    if (fGraphManager->CheckPorts(src, dst) < 0)
        return -1;

    ref = fGraphManager->GetOutputRefNum(src);
    assert(ref >= 0);
    client = fClientTable[ref];
    assert(client);
    if (!client->GetClientControl()->fActive) {
        jack_error("Cannot connect ports owned by inactive clients:"
                   " \"%s\" is not active", client->GetClientControl()->fName);
        return -1;
    }

    ref = fGraphManager->GetInputRefNum(dst);
    assert(ref >= 0);
    client = fClientTable[ref];
    assert(client);
    if (!client->GetClientControl()->fActive) {
        jack_error("Cannot connect ports owned by inactive clients:"
                   " \"%s\" is not active", client->GetClientControl()->fName);
        return -1;
    }

    int res = fGraphManager->Connect(src, dst);
    if (res == 0) 
        NotifyPortConnect(src, dst, true);
    return res;
}

int JackEngine::PortDisconnect(int refnum, const char* src, const char* dst)
{
    jack_log("JackEngine::PortDisconnect src = %s dst = %s", src, dst);
    AssertRefnum(refnum);
    jack_port_id_t port_src, port_dst;

    if (fGraphManager->CheckPorts(src, dst, &port_src, &port_dst) < 0) {
        return -1;
    } else if (fGraphManager->Disconnect(port_src, port_dst) == 0) {
        NotifyPortConnect(port_src, port_dst, false);
        return 0;
    } else {
        return -1;
    }
}

int JackEngine::PortDisconnect(int refnum, jack_port_id_t src, jack_port_id_t dst)
{
    jack_log("JackEngine::PortDisconnect src = %d dst = %d", src, dst);
    AssertRefnum(refnum);

    if (dst == ALL_PORTS) {

        jack_int_t connections[CONNECTION_NUM_FOR_PORT];
        fGraphManager->GetConnections(src, connections);

        // Notifications
        JackPort* port = fGraphManager->GetPort(src);
        if (port->GetFlags() & JackPortIsOutput) {
            for (int i = 0; (i < CONNECTION_NUM_FOR_PORT) && (connections[i] != EMPTY); i++) {
                jack_log("NotifyPortConnect src = %ld dst = %ld false", src, connections[i]);
                NotifyPortConnect(src, connections[i], false);
            }
        } else {
            for (int i = 0; (i < CONNECTION_NUM_FOR_PORT) && (connections[i] != EMPTY); i++) {
                jack_log("NotifyPortConnect src = %ld dst = %ld false", connections[i], src);
                NotifyPortConnect(connections[i], src, false);
            }
        }

        return fGraphManager->DisconnectAll(src);
    } else if (fGraphManager->CheckPorts(src, dst) < 0) {
        return -1;
    } else if (fGraphManager->Disconnect(src, dst) == 0) {
        // Notifications
        NotifyPortConnect(src, dst, false);
        return 0;
    } else {
        return -1;
    }
}

int JackEngine::PortRename(int refnum, jack_port_id_t port, const char* name)
{
    fGraphManager->GetPort(port)->SetName(name);
    NotifyPortRename(port);
    return 0;
}

} // end of namespace

