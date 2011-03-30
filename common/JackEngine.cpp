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
#include <set>
#include <assert.h>

#include "JackSystemDeps.h"
#include "JackLockedEngine.h"
#include "JackExternalClient.h"
#include "JackInternalClient.h"
#include "JackEngineControl.h"
#include "JackClientControl.h"
#include "JackServerGlobals.h"
#include "JackGlobals.h"
#include "JackChannel.h"
#include "JackError.h"

namespace Jack
{

JackEngine::JackEngine(JackGraphManager* manager,
                       JackSynchro* table,
                       JackEngineControl* control)
{
    fGraphManager = manager;
    fSynchroTable = table;
    fEngineControl = control;
    for (int i = 0; i < CLIENT_NUM; i++)
        fClientTable[i] = NULL;
    fLastSwitchUsecs = 0;
    fMaxUUID = 0;
    fSessionPendingReplies = 0;
    fSessionTransaction = NULL;
    fSessionResult = NULL;
}

JackEngine::~JackEngine()
{}

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
    for (int i = fEngineControl->fDriverNum; i < CLIENT_NUM; i++) {
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

    return 0;
}

void JackEngine::NotifyQuit()
{
    fChannel.NotifyQuit();
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
        for (i = fEngineControl->fDriverNum; i < CLIENT_NUM; i++) {
            if (fClientTable[i])
                break;
        }
        if (i == CLIENT_NUM) {
            // last client and temporay case: quit the server
            jack_log("JackEngine::ReleaseRefnum server quit");
            fEngineControl->fTemporary = false;
            throw JackTemporaryException();
        }
    }
}

//------------------
// Graph management
//------------------

void JackEngine::ProcessNext(jack_time_t cur_cycle_begin)
{
    fLastSwitchUsecs = cur_cycle_begin;
    if (fGraphManager->RunNextGraph())  { // True if the graph actually switched to a new state
        fChannel.Notify(ALL_CLIENTS, kGraphOrderCallback, 0);
        //NotifyGraphReorder();
    }
    fSignal.Signal();                   // Signal for threads waiting for next cycle
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
    for (int i = fEngineControl->fDriverNum; i < CLIENT_NUM; i++) {
        JackClientInterface* client = fClientTable[i];
        if (client && client->GetClientControl()->fActive) {
            JackClientTiming* timing = fGraphManager->GetClientTiming(i);
            jack_client_state_t status = timing->fStatus;
            jack_time_t finished_date = timing->fFinishedAt;

            if (status != NotTriggered && status != Finished) {
                jack_error("JackEngine::XRun: client = %s was not run: state = %ld", client->GetClientControl()->fName, status);
                fChannel.Notify(ALL_CLIENTS, kXRunCallback, 0);  // Notify all clients
                //NotifyXRun(ALL_CLIENTS);
            }

            if (status == Finished && (long)(finished_date - callback_usecs) > 0) {
                jack_error("JackEngine::XRun: client %s finished after current callback", client->GetClientControl()->fName);
                fChannel.Notify(ALL_CLIENTS, kXRunCallback, 0);  // Notify all clients
                //NotifyXRun(ALL_CLIENTS);
            }
        }
    }
}

int JackEngine::ComputeTotalLatencies()
{
    std::vector<jack_int_t> sorted;
    std::vector<jack_int_t>::iterator it;
    std::vector<jack_int_t>::reverse_iterator rit;

    fGraphManager->TopologicalSort(sorted);

    /* iterate over all clients in graph order, and emit
	 * capture latency callback.
	 */

    for (it = sorted.begin(); it != sorted.end(); it++) {
        NotifyClient(*it, kLatencyCallback, true, "", 0, 0);
    }

    /* now issue playback latency callbacks in reverse graph order.
	 */
    for (rit = sorted.rbegin(); rit != sorted.rend(); rit++) {
        NotifyClient(*rit, kLatencyCallback, true, "", 1, 0);
    }

    return 0;
}

//---------------
// Notifications
//---------------

void JackEngine::NotifyClient(int refnum, int event, int sync, const char* message, int value1, int value2)
{
    JackClientInterface* client = fClientTable[refnum];

    // The client may be notified by the RT thread while closing
    if (client) {

        if (client->GetClientControl()->fCallback[event]) {
            /*
                Important for internal clients : unlock before calling the notification callbacks.
            */
            bool res = fMutex.Unlock();
            if (client->ClientNotify(refnum, client->GetClientControl()->fName, event, sync, message, value1, value2) < 0)
                jack_error("NotifyClient fails name = %s event = %ld val1 = %ld val2 = %ld", client->GetClientControl()->fName, event, value1, value2);
            if (res)
                fMutex.Lock();

        } else {
            jack_log("JackEngine::NotifyClient: no callback for event = %ld", event);
        }
    }
}

void JackEngine::NotifyClients(int event, int sync, const char* message, int value1, int value2)
{
    for (int i = 0; i < CLIENT_NUM; i++) {
        NotifyClient(i, event, sync, message, value1, value2);
    }
}

int JackEngine::NotifyAddClient(JackClientInterface* new_client, const char* name, int refnum)
{
    jack_log("JackEngine::NotifyAddClient: name = %s", name);
    // Notify existing clients of the new client and new client of existing clients.
    for (int i = 0; i < CLIENT_NUM; i++) {
        JackClientInterface* old_client = fClientTable[i];
        if (old_client) {
            if (old_client->ClientNotify(refnum, name, kAddClient, true, "", 0, 0) < 0) {
                jack_error("NotifyAddClient old_client fails name = %s", old_client->GetClientControl()->fName);
                return -1;
            }
            if (new_client->ClientNotify(i, old_client->GetClientControl()->fName, kAddClient, true, "", 0, 0) < 0) {
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
            client->ClientNotify(refnum, name, kRemoveClient, true, "",0, 0);
        }
    }
}

// Coming from the driver
void JackEngine::NotifyXRun(jack_time_t callback_usecs, float delayed_usecs)
{
    // Use the audio thread => request thread communication channel
    fEngineControl->NotifyXRun(callback_usecs, delayed_usecs);
    fChannel.Notify(ALL_CLIENTS, kXRunCallback, 0);
    //NotifyXRun(ALL_CLIENTS);
}

void JackEngine::NotifyXRun(int refnum)
{
    if (refnum == ALL_CLIENTS) {
        NotifyClients(kXRunCallback, false, "", 0, 0);
    } else {
        NotifyClient(refnum, kXRunCallback, false, "", 0, 0);
    }
}

void JackEngine::NotifyGraphReorder()
{
    NotifyClients(kGraphOrderCallback, false, "", 0, 0);
    ComputeTotalLatencies();
}

void JackEngine::NotifyBufferSize(jack_nframes_t buffer_size)
{
    NotifyClients(kBufferSizeCallback, true, "", buffer_size, 0);
}

void JackEngine::NotifySampleRate(jack_nframes_t sample_rate)
{
    NotifyClients(kSampleRateCallback, true, "", sample_rate, 0);
}

void JackEngine::NotifyFailure(int code, const char* reason)
{
    NotifyClients(kShutDownCallback, false, reason, code, 0);
}

void JackEngine::NotifyFreewheel(bool onoff)
{
    if (onoff) {
        // Save RT state
        fEngineControl->fSavedRealTime = fEngineControl->fRealTime;
        fEngineControl->fRealTime = false;
    } else {
        // Restore RT state
        fEngineControl->fRealTime = fEngineControl->fSavedRealTime;
        fEngineControl->fSavedRealTime = false;
    }
    NotifyClients((onoff ? kStartFreewheelCallback : kStopFreewheelCallback), true, "", 0, 0);
}

void JackEngine::NotifyPortRegistation(jack_port_id_t port_index, bool onoff)
{
    NotifyClients((onoff ? kPortRegistrationOnCallback : kPortRegistrationOffCallback), false, "", port_index, 0);
}

void JackEngine::NotifyPortRename(jack_port_id_t port, const char* old_name)
{
    NotifyClients(kPortRenameCallback, false, old_name, port, 0);
}

void JackEngine::NotifyPortConnect(jack_port_id_t src, jack_port_id_t dst, bool onoff)
{
    NotifyClients((onoff ? kPortConnectCallback : kPortDisconnectCallback), false, "", src, dst);
}

void JackEngine::NotifyActivate(int refnum)
{
    NotifyClient(refnum, kActivateClient, true, "", 0, 0);
}

//----------------------------
// Loadable client management
//----------------------------

int JackEngine::GetInternalClientName(int refnum, char* name_res)
{
    JackClientInterface* client = fClientTable[refnum];
    strncpy(name_res, client->GetClientControl()->fName, JACK_CLIENT_NAME_SIZE);
    return 0;
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

int JackEngine::ClientCheck(const char* name, int uuid, char* name_res, int protocol, int options, int* status)
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

    std::map<int,std::string>::iterator res = fReservationMap.find(uuid);

    if (res != fReservationMap.end()) {
        strncpy(name_res, res->second.c_str(), JACK_CLIENT_NAME_SIZE);
    } else if (ClientCheckName(name)) {

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
        return true;            /* failure */
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

    for (std::map<int,std::string>::iterator i = fReservationMap.begin(); i != fReservationMap.end(); i++) {
        if (i->second == name)
            return true;
    }

    return false;
}

int JackEngine::GetNewUUID()
{
    return fMaxUUID++;
}

void JackEngine::EnsureUUID(int uuid)
{
    if (uuid > fMaxUUID)
        fMaxUUID = uuid+1;

    for (int i = 0; i < CLIENT_NUM; i++) {
        JackClientInterface* client = fClientTable[i];
        if (client && (client->GetClientControl()->fSessionID == uuid)) {
            client->GetClientControl()->fSessionID = GetNewUUID();
        }
    }
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

int JackEngine::GetClientRefNum(const char* name)
{
    for (int i = 0; i < CLIENT_NUM; i++) {
        JackClientInterface* client = fClientTable[i];
        if (client && (strcmp(client->GetClientControl()->fName, name) == 0))
            return client->GetClientControl()->fRefNum;
    }

    return -1;
}

// Used for external clients
int JackEngine::ClientExternalOpen(const char* name, int pid, int uuid, int* ref, int* shared_engine, int* shared_client, int* shared_graph_manager)
{
    char real_name[JACK_CLIENT_NAME_SIZE + 1];

    if (uuid < 0) {
        uuid = GetNewUUID();
        strncpy(real_name, name, JACK_CLIENT_NAME_SIZE);
    } else {
        std::map<int, std::string>::iterator res = fReservationMap.find(uuid);
        if (res != fReservationMap.end()) {
            strncpy(real_name, res->second.c_str(), JACK_CLIENT_NAME_SIZE);
            fReservationMap.erase(uuid);
        } else {
            strncpy(real_name, name, JACK_CLIENT_NAME_SIZE);
        }

        EnsureUUID(uuid);
    }

    jack_log("JackEngine::ClientExternalOpen: uuid = %d, name = %s ", uuid, real_name);

    int refnum = AllocateRefnum();
    if (refnum < 0) {
        jack_error("No more refnum available");
        return -1;
    }

    JackExternalClient* client = new JackExternalClient();

    if (!fSynchroTable[refnum].Allocate(real_name, fEngineControl->fServerName, 0)) {
        jack_error("Cannot allocate synchro");
        goto error;
    }

    if (client->Open(real_name, pid, refnum, uuid, shared_client) < 0) {
        jack_error("Cannot open client");
        goto error;
    }

    if (!fSignal.LockedTimedWait(DRIVER_OPEN_TIMEOUT * 1000000)) {
        // Failure if RT thread is not running (problem with the driver...)
        jack_error("Driver is not running");
        goto error;
    }

    fClientTable[refnum] = client;

    if (NotifyAddClient(client, real_name, refnum) < 0) {
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
    jack_log("JackEngine::ClientInternalOpen: name = %s", name);

    int refnum = AllocateRefnum();
    if (refnum < 0) {
        jack_error("No more refnum available");
        goto error;
    }

    if (!fSynchroTable[refnum].Allocate(name, fEngineControl->fServerName, 0)) {
        jack_error("Cannot allocate synchro");
        goto error;
    }

    if (wait && !fSignal.LockedTimedWait(DRIVER_OPEN_TIMEOUT * 1000000)) {
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
    JackClientInterface* client = fClientTable[refnum];
    fEngineControl->fTransport.ResetTimebase(refnum);
    int res = ClientCloseAux(refnum, client, true);
    client->Close();
    delete client;
    return res;
}

// Used for server internal clients or drivers when the RT thread is stopped
int JackEngine::ClientInternalClose(int refnum, bool wait)
{
    JackClientInterface* client = fClientTable[refnum];
    return ClientCloseAux(refnum, client, wait);
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
        if (!fSignal.LockedTimedWait(fEngineControl->fTimeOutUsecs * 2)) { // Must wait at least until a switch occurs in Process, even in case of graph end failure
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

int JackEngine::ClientActivate(int refnum, bool is_real_time)
{
    JackClientInterface* client = fClientTable[refnum];
    jack_log("JackEngine::ClientActivate ref = %ld name = %s", refnum, client->GetClientControl()->fName);

    if (is_real_time)
        fGraphManager->Activate(refnum);

    // Wait for graph state change to be effective
    if (!fSignal.LockedTimedWait(fEngineControl->fTimeOutUsecs * 10)) {
        jack_error("JackEngine::ClientActivate wait error ref = %ld name = %s", refnum, client->GetClientControl()->fName);
        return -1;
    } else {
        jack_int_t input_ports[PORT_NUM_FOR_CLIENT];
        jack_int_t output_ports[PORT_NUM_FOR_CLIENT];
        fGraphManager->GetInputPorts(refnum, input_ports);
        fGraphManager->GetOutputPorts(refnum, output_ports);

        // Notify client
        NotifyActivate(refnum);

        // Then issue port registration notification
        for (int i = 0; (i < PORT_NUM_FOR_CLIENT) && (input_ports[i] != EMPTY); i++) {
            NotifyPortRegistation(input_ports[i], true);
        }
        for (int i = 0; (i < PORT_NUM_FOR_CLIENT) && (output_ports[i] != EMPTY); i++) {
            NotifyPortRegistation(output_ports[i], true);
        }

        return 0;
    }
}

// May be called without client
int JackEngine::ClientDeactivate(int refnum)
{
    JackClientInterface* client = fClientTable[refnum];
    jack_log("JackEngine::ClientDeactivate ref = %ld name = %s", refnum, client->GetClientControl()->fName);

    jack_int_t input_ports[PORT_NUM_FOR_CLIENT];
    jack_int_t output_ports[PORT_NUM_FOR_CLIENT];
    fGraphManager->GetInputPorts(refnum, input_ports);
    fGraphManager->GetOutputPorts(refnum, output_ports);

    // First disconnect all ports
    for (int i = 0; (i < PORT_NUM_FOR_CLIENT) && (input_ports[i] != EMPTY); i++) {
        PortDisconnect(refnum, input_ports[i], ALL_PORTS);
    }
    for (int i = 0; (i < PORT_NUM_FOR_CLIENT) && (output_ports[i] != EMPTY); i++) {
        PortDisconnect(refnum, output_ports[i], ALL_PORTS);
    }

    // Then issue port registration notification
    for (int i = 0; (i < PORT_NUM_FOR_CLIENT) && (input_ports[i] != EMPTY); i++) {
        NotifyPortRegistation(input_ports[i], false);
    }
    for (int i = 0; (i < PORT_NUM_FOR_CLIENT) && (output_ports[i] != EMPTY); i++) {
        NotifyPortRegistation(output_ports[i], false);
    }

    fGraphManager->Deactivate(refnum);
    fLastSwitchUsecs = 0; // Force switch to occur next cycle, even when called with "dead" clients

    // Wait for graph state change to be effective
    if (!fSignal.LockedTimedWait(fEngineControl->fTimeOutUsecs * 10)) {
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
    JackClientInterface* client = fClientTable[refnum];

    // Check if port name already exists
    if (fGraphManager->GetPort(name) != NO_PORT) {
        jack_error("port_name \"%s\" already exists", name);
        return -1;
    }

    *port_index = fGraphManager->AllocatePort(refnum, name, type, (JackPortFlags)flags, fEngineControl->fBufferSize);
    if (*port_index != NO_PORT) {
        if (client->GetClientControl()->fActive)
            NotifyPortRegistation(*port_index, true);
        return 0;
    } else {
        return -1;
    }
}

int JackEngine::PortUnRegister(int refnum, jack_port_id_t port_index)
{
    jack_log("JackEngine::PortUnRegister ref = %ld port_index = %ld", refnum, port_index);
    JackClientInterface* client = fClientTable[refnum];

    // Disconnect port ==> notification is sent
    PortDisconnect(refnum, port_index, ALL_PORTS);

    if (fGraphManager->ReleasePort(refnum, port_index) == 0) {
        if (client->GetClientControl()->fActive)
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

    return (fGraphManager->GetTwoPorts(src, dst, &port_src, &port_dst) < 0)
           ? -1
           : PortConnect(refnum, port_src, port_dst);
}

int JackEngine::PortConnect(int refnum, jack_port_id_t src, jack_port_id_t dst)
{
    jack_log("JackEngine::PortConnect src = %d dst = %d", src, dst);
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
    jack_port_id_t port_src, port_dst;

    return (fGraphManager->GetTwoPorts(src, dst, &port_src, &port_dst) < 0)
           ? -1
           : PortDisconnect(refnum, port_src, port_dst);
}

int JackEngine::PortDisconnect(int refnum, jack_port_id_t src, jack_port_id_t dst)
{
    jack_log("JackEngine::PortDisconnect src = %d dst = %d", src, dst);

    if (dst == ALL_PORTS) {

        jack_int_t connections[CONNECTION_NUM_FOR_PORT];
        fGraphManager->GetConnections(src, connections);

        JackPort* port = fGraphManager->GetPort(src);
        int ret = 0;
        if (port->GetFlags() & JackPortIsOutput) {
            for (int i = 0; (i < CONNECTION_NUM_FOR_PORT) && (connections[i] != EMPTY); i++) {
                if (PortDisconnect(refnum, src, connections[i]) != 0) {
                    ret = -1;
                }
            }
        } else {
            for (int i = 0; (i < CONNECTION_NUM_FOR_PORT) && (connections[i] != EMPTY); i++) {
                if (PortDisconnect(refnum, connections[i], src) != 0) {
                    ret = -1;
                }
            }
        }

        return ret;
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
    char old_name[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
    strcpy(old_name, fGraphManager->GetPort(port)->GetName());
    fGraphManager->GetPort(port)->SetName(name);
    NotifyPortRename(port, old_name);
    return 0;
}

//--------------------
// Session management
//--------------------

void JackEngine::SessionNotify(int refnum, const char *target, jack_session_event_type_t type, const char *path, JackChannelTransaction *socket)
{
    if (fSessionPendingReplies != 0) {
        JackSessionNotifyResult res(-1);
        res.Write(socket);
        jack_log("JackEngine::SessionNotify ... busy");
        return;
    }

    for (int i = 0; i < CLIENT_NUM; i++) {
        JackClientInterface* client = fClientTable[i];
        if (client && (client->GetClientControl()->fSessionID < 0)) {
            client->GetClientControl()->fSessionID = GetNewUUID();
        }
    }
    fSessionResult = new JackSessionNotifyResult();

    for (int i = 0; i < CLIENT_NUM; i++) {
        JackClientInterface* client = fClientTable[i];
        if (client && client->GetClientControl()->fCallback[kSessionCallback]) {

            // check if this is a notification to a specific client.
            if (target != NULL && strlen(target) != 0) {
                if (strcmp(target, client->GetClientControl()->fName)) {
                    continue;
                }
            }

            char path_buf[JACK_PORT_NAME_SIZE];
            snprintf( path_buf, sizeof(path_buf), "%s%s%c", path, client->GetClientControl()->fName, DIR_SEPARATOR );

            int res = JackTools::MkDir(path_buf);
            if (res)
                jack_error( "JackEngine::SessionNotify: can not create session directory '%s'", path_buf );

            int result = client->ClientNotify(i, client->GetClientControl()->fName, kSessionCallback, true, path_buf, (int) type, 0);

            if (result == 2) {
                fSessionPendingReplies += 1;
            } else if (result == 1) {
                char uuid_buf[JACK_UUID_SIZE];
                snprintf( uuid_buf, sizeof(uuid_buf), "%d", client->GetClientControl()->fSessionID );
                fSessionResult->fCommandList.push_back( JackSessionCommand( uuid_buf,
                                                                            client->GetClientControl()->fName,
                                                                            client->GetClientControl()->fSessionCommand,
                                                                            client->GetClientControl()->fSessionFlags ));
            }
        }
    }

    if (fSessionPendingReplies == 0) {
        fSessionResult->Write(socket);
        delete fSessionResult;
        fSessionResult = NULL;
    } else {
        fSessionTransaction = socket;
    }
}

void JackEngine::SessionReply(int refnum)
{
    JackClientInterface* client = fClientTable[refnum];
    char uuid_buf[JACK_UUID_SIZE];
    snprintf( uuid_buf, sizeof(uuid_buf), "%d", client->GetClientControl()->fSessionID);
    fSessionResult->fCommandList.push_back(JackSessionCommand(uuid_buf,
                                                            client->GetClientControl()->fName,
                                                            client->GetClientControl()->fSessionCommand,
                                                            client->GetClientControl()->fSessionFlags));
    fSessionPendingReplies -= 1;

    if (fSessionPendingReplies == 0) {
        fSessionResult->Write(fSessionTransaction);
        delete fSessionResult;
        fSessionResult = NULL;
    }
}

void JackEngine::GetUUIDForClientName(const char *client_name, char *uuid_res, int *result)
{
    for (int i = 0; i < CLIENT_NUM; i++) {
        JackClientInterface* client = fClientTable[i];

        if (client && (strcmp(client_name, client->GetClientControl()->fName) == 0)) {
            snprintf(uuid_res, JACK_UUID_SIZE, "%d", client->GetClientControl()->fSessionID);
            *result = 0;
            return;
        }
    }
    // Did not find name.
    *result = -1;
}

void JackEngine::GetClientNameForUUID(const char *uuid, char *name_res, int *result)
{
    for (int i = 0; i < CLIENT_NUM; i++) {
        JackClientInterface* client = fClientTable[i];

        if (!client)
            continue;

        char uuid_buf[JACK_UUID_SIZE];
        snprintf(uuid_buf, JACK_UUID_SIZE, "%d", client->GetClientControl()->fSessionID);

        if (strcmp(uuid,uuid_buf) == 0) {
            strncpy(name_res, client->GetClientControl()->fName, JACK_CLIENT_NAME_SIZE);
            *result = 0;
            return;
        }
    }
    // Did not find uuid.
    *result = -1;
}

void JackEngine::ReserveClientName(const char *name, const char *uuid, int *result)
{
    jack_log("JackEngine::ReserveClientName ( name = %s, uuid = %s )", name, uuid);

    if (ClientCheckName(name)) {
        *result = -1;
        jack_log("name already taken");
        return;
    }

    EnsureUUID(atoi(uuid));
    fReservationMap[atoi(uuid)] = name;
    *result = 0;
}

void JackEngine::ClientHasSessionCallbackRequest(const char *name, int *result)
{
    JackClientInterface* client = NULL;
    for (int i = 0; i < CLIENT_NUM; i++) {
        JackClientInterface* client = fClientTable[i];
        if (client && (strcmp(client->GetClientControl()->fName, name) == 0))
            break;
    }

    if (client) {
        *result = client->GetClientControl()->fCallback[kSessionCallback];
     } else {
        *result = -1;
    }
}

} // end of namespace

