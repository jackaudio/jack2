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

#include <iostream>
#include <fstream>
#include <assert.h>

#include "JackEngine.h"
#include "JackExternalClient.h"
#include "JackEngineControl.h"
#include "JackClientControl.h"
#include "JackEngineTiming.h"
#include "JackGlobals.h"
#include "JackChannel.h"
#include "JackSyncInterface.h"

namespace Jack
{

JackEngine::JackEngine(JackGraphManager* manager, 
						JackSynchro** table, 
						JackEngineControl* control)
{
    fGraphManager = manager;
    fSynchroTable = table;
    fEngineControl = control;
	fChannel = JackGlobals::MakeServerNotifyChannel();
	fSignal = JackGlobals::MakeInterProcessSync();
    fEngineTiming = new JackEngineTiming(fClientTable, fGraphManager, fEngineControl);
    for (int i = 0; i < CLIENT_NUM; i++)
        fClientTable[i] = NULL;
    fEngineTiming->ClearTimeMeasures();
    fEngineTiming->ResetRollingUsecs();
}

JackEngine::~JackEngine()
{
    delete fChannel;
    delete fEngineTiming;
	delete fSignal;
}

//-------------------
// Client management
//-------------------

int JackEngine::Open()
{
    JackLog("JackEngine::Open\n");

    // Open audio thread => request thread communication channel
    if (fChannel->Open() < 0) {
        jack_error("Cannot connect to server");
        return -1;
    } else {
        return 0;
    }
}

int JackEngine::Close()
{
    JackLog("JackEngine::Close\n");
    fChannel->Close();

    // Close (possibly) remaining clients (RT is stopped)
    for (int i = 0; i < CLIENT_NUM; i++) {
        JackClientInterface* client = fClientTable[i];
        if (client) {
            JackLog("JackEngine::Close remaining client %ld\n", i);
            ClientCloseAux(i, client, false);
            client->Close();
            delete client;
        }
    }
	
	fSignal->Destroy();
    return 0;
}

int JackEngine::Allocate()
{
    for (int i = 0; i < CLIENT_NUM; i++) {
        if (!fClientTable[i]) {
            JackLog("JackEngine::AllocateRefNum ref = %ld\n", i);
            return i;
        }
    }

    return -1;
}

//------------------
// Graph management
//------------------

void JackEngine::ProcessNext(jack_time_t callback_usecs)
{
	fLastSwitchUsecs = callback_usecs;
	if (fGraphManager->RunNextGraph())	// True if the graph actually switched to a new state
		fChannel->ClientNotify(ALL_CLIENTS, JackNotifyChannelInterface::kGraphOrderCallback, 0);
	fSignal->SignalAll();				// Signal for threads waiting for next cycle
}

void JackEngine::ProcessCurrent(jack_time_t callback_usecs)
{
	if (callback_usecs < fLastSwitchUsecs + 2 * fEngineControl->fPeriodUsecs) // Signal XRun only for the first failling cycle
		CheckXRun(callback_usecs);
	fGraphManager->RunCurrentGraph();
}

bool JackEngine::Process(jack_time_t callback_usecs)
{
	bool res = true;
	
    // Transport begin
 	fEngineControl->CycleBegin(callback_usecs);

    // Timing
 	fEngineControl->IncFrameTime(callback_usecs);
    fEngineTiming->UpdateTiming(callback_usecs);

    // Graph
    if (fGraphManager->IsFinishedGraph()) {
        ProcessNext(callback_usecs);
		res = true;
    } else {
        JackLog("Process: graph not finished!\n");
		if (callback_usecs > fLastSwitchUsecs + fEngineControl->fTimeOutUsecs) {
            JackLog("Process: switch to next state delta = %ld\n", long(callback_usecs - fLastSwitchUsecs));
			//RemoveZombifiedClients(callback_usecs); TODO
            ProcessNext(callback_usecs);
			res = true;
        } else {
            JackLog("Process: waiting to switch delta = %ld\n", long(callback_usecs - fLastSwitchUsecs));
            ProcessCurrent(callback_usecs);
			res = false;
		}
    }

    // Transport end
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
                //fChannel->ClientNotify(i, kXRunCallback, 0); // Notify the failing client
                fChannel->ClientNotify(ALL_CLIENTS, JackNotifyChannelInterface::kXRunCallback, 0);  // Notify all clients
            }
            if (status == Finished && (long)(finished_date - callback_usecs) > 0) {
                jack_error("JackEngine::XRun: client %s finished after current callback", client->GetClientControl()->fName);
                //fChannel->ClientNotify(i, kXRunCallback, 0); // Notify the failing client
                fChannel->ClientNotify(ALL_CLIENTS, JackNotifyChannelInterface::kXRunCallback, 0);  // Notify all clients
            }
        }
    }
}

//---------------
// Zombification
//---------------

bool JackEngine::IsZombie(JackClientInterface* client, jack_time_t current_time)
{
	return ((current_time - fGraphManager->GetClientTiming(client->GetClientControl()->fRefNum)->fFinishedAt) > 2 * fEngineControl->fTimeOutUsecs);  // A VERIFIER
}

// TODO : check what happens with looped sub-graph....
void JackEngine::GetZombifiedClients(bool zombi_clients[CLIENT_NUM], jack_time_t current_time)
{
    for (int i = REAL_REFNUM; i < CLIENT_NUM; i++) {
        JackClientInterface* client1 = fClientTable[i];
        if (client1 && IsZombie(client1, current_time)) {
            JackLog("JackEngine::GetZombifiedClients: %s\n", client1->GetClientControl()->fName);
            zombi_clients[i] = true; // Assume client is dead
            // If another dead client is connected to the scanned one, then the scanned one is not the first of the dead subgraph
            for (int j = REAL_REFNUM; j < CLIENT_NUM; j++) {
                JackClientInterface* client2 = fClientTable[j];
                if (client2 && IsZombie(client2, current_time) && fGraphManager->IsDirectConnection(j, i)) {
                    zombi_clients[i] = false;
                    break;
                }
            }
        } else {
            zombi_clients[i] = false;
        }
    }
}

void JackEngine::RemoveZombifiedClients(jack_time_t current_time)
{
    bool zombi_clients[CLIENT_NUM];
    GetZombifiedClients(zombi_clients, current_time);

    for (int i = REAL_REFNUM; i < CLIENT_NUM; i++) {
        if (zombi_clients[i] && !fClientTable[i]->GetClientControl()->fZombie) {
            fClientTable[i]->GetClientControl()->fZombie = true;
            JackLog("RemoveZombifiedCients: name = %s\n", fClientTable[i]->GetClientControl()->fName);
            fGraphManager->DirectDisconnect(FREEWHEEL_DRIVER_REFNUM, i);
            fGraphManager->DirectDisconnect(i, FREEWHEEL_DRIVER_REFNUM);
            fGraphManager->DisconnectAllPorts(i);
            fChannel->ClientNotify(i, JackNotifyChannelInterface::kZombifyClient, 0); // Signal engine
        }
    }
}

void JackEngine::ZombifyClient(int refnum)
{
    NotifyClient(refnum, JackNotifyChannelInterface::kZombifyClient, false, 0);
}

//---------------
// Notifications
//---------------

void JackEngine::NotifyClient(int refnum, int event, int sync, int value)
{
    JackClientInterface* client = fClientTable[refnum];
    // The client may be notified by the RT thread while closing
    if (client) {
		if (client->ClientNotify(refnum, client->GetClientControl()->fName, event, sync, value) < 0)
			jack_error("NotifyClient fails name = %s event = %ld = val = %ld", client->GetClientControl()->fName, event, value);
    } else {
        JackLog("JackEngine::NotifyClient: client not available anymore\n");
    }
}

void JackEngine::NotifyClients(int event, int sync, int value)
{
    for (int i = 0; i < CLIENT_NUM; i++) {
        JackClientInterface* client = fClientTable[i];
        if (client && (client->ClientNotify(i, client->GetClientControl()->fName, event, sync, value) < 0)) {
            jack_error("NotifyClient fails name = %s event = %ld = val = %ld", client->GetClientControl()->fName, event, value);
        }
    }
}

int JackEngine::NotifyAddClient(JackClientInterface* new_client, const char* name, int refnum)
{
    // Notify existing clients of the new client and new client of existing clients.
    for (int i = 0; i < CLIENT_NUM; i++) {
        JackClientInterface* old_client = fClientTable[i];
        if (old_client) {
            if (old_client->ClientNotify(refnum, name, JackNotifyChannelInterface::kAddClient, true, 0) < 0) {
                jack_error("NotifyAddClient old_client fails name = %s", old_client->GetClientControl()->fName);
				return -1;
			}
			if (new_client->ClientNotify(i, old_client->GetClientControl()->fName, JackNotifyChannelInterface::kAddClient, true, 0) < 0) {
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
            client->ClientNotify(refnum, name, JackNotifyChannelInterface::kRemoveClient, true, 0);
        }
    }
}

// Coming from the driver
void JackEngine::NotifyXRun(jack_time_t callback_usecs)
{
    // Use the audio thread => request thread communication channel
	fEngineControl->ResetFrameTime(callback_usecs);
    fChannel->ClientNotify(ALL_CLIENTS, JackNotifyChannelInterface::kXRunCallback, 0);
}

void JackEngine::NotifyXRun(int refnum)
{
    if (refnum == ALL_CLIENTS) {
        NotifyClients(JackNotifyChannelInterface::kXRunCallback, false, 0);
    } else {
        NotifyClient(refnum, JackNotifyChannelInterface::kXRunCallback, false, 0);
    }
}

void JackEngine::NotifyGraphReorder()
{
    NotifyClients(JackNotifyChannelInterface::kGraphOrderCallback, false, 0);
}

void JackEngine::NotifyBufferSize(jack_nframes_t nframes)
{
    NotifyClients(JackNotifyChannelInterface::kBufferSizeCallback, true, nframes);
}

void JackEngine::NotifyFreewheel(bool onoff)
{
    fEngineControl->fRealTime = !onoff;
    NotifyClients((onoff ? JackNotifyChannelInterface::kStartFreewheel : JackNotifyChannelInterface::kStopFreewheel), true, 0);
}

void JackEngine::NotifyPortRegistation(jack_port_id_t port_index, bool onoff)
{
    NotifyClients((onoff ? JackNotifyChannelInterface::kPortRegistrationOn : JackNotifyChannelInterface::kPortRegistrationOff), false, port_index);
}

void JackEngine::NotifyActivate(int refnum)
{
	NotifyClient(refnum, JackNotifyChannelInterface::kActivateClient, true, 0);
}

//-------------------
// Client management
//-------------------

bool JackEngine::ClientCheckName(const char* name)
{
    for (int i = 0; i < CLIENT_NUM; i++) {
        JackClientInterface* client = fClientTable[i];
        if (client && (strcmp(client->GetClientControl()->fName, name) == 0))
            return true;
    }

    return false;
}

// Used for external clients
int JackEngine::ClientExternalOpen(const char* name, int* ref, int* shared_engine, int* shared_client, int* shared_graph_manager)
{
    JackLog("JackEngine::ClientOpen: name = %s \n", name);
	
	if (ClientCheckName(name)) {
        jack_error("client %s already registered", name);
        return -1;
    }
	
	int refnum = Allocate();
    if (refnum < 0) {
        jack_error("No more refnum available");
        return -1;
    }
	
	JackExternalClient* client = new JackExternalClient();

    if (!fSynchroTable[refnum]->Allocate(name, 0)) {
        jack_error("Cannot allocate synchro");
		goto error;
    }

    if (client->Open(name, refnum, shared_client) < 0) {
        jack_error("Cannot open client");
        goto error;
    }

    if (!fSignal->TimedWait(5 * 1000000)) {
        // Failure if RT thread is not running (problem with the driver...)
        jack_error("Driver is not running");
        goto error;
    }

    if (NotifyAddClient(client, name, refnum) < 0) {
        jack_error("Cannot notify add client");
        goto error;
    }

    fClientTable[refnum] = client;
	fGraphManager->InitRefNum(refnum);
    fEngineTiming->ResetRollingUsecs();
    *shared_engine = fEngineControl->GetShmIndex();
    *shared_graph_manager = fGraphManager->GetShmIndex();
    *ref = refnum;
    return 0;

error:
    ClientCloseAux(refnum, client, false);
    client->Close();
	delete client;
    return -1;
}

// Used for server driver clients
int JackEngine::ClientInternalOpen(const char* name, int* ref, JackEngineControl** shared_engine, JackGraphManager** shared_manager, JackClientInterface* client)
{
    JackLog("JackEngine::ClientInternalNew: name = %s\n", name);
	
	if (ClientCheckName(name)) {
        jack_error("client %s already registered", name);
        return -1;
    }
	
	int refnum = Allocate();
	if (refnum < 0) {
        jack_error("No more refnum available");
        return -1;
    }

    if (!fSynchroTable[refnum]->Allocate(name, 0)) {
        jack_error("Cannot allocate synchro");
		return -1;
    }

    if (NotifyAddClient(client, name, refnum) < 0) {
        jack_error("Cannot notify add client");
		return -1;
    }

    fClientTable[refnum] = client;
	fGraphManager->InitRefNum(refnum);
    fEngineTiming->ResetRollingUsecs();
    *shared_engine = fEngineControl;
    *shared_manager = fGraphManager;
    *ref = refnum;
    return 0;
}

// Used for externall clients
int JackEngine::ClientExternalClose(int refnum)
{
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

// Used for server internal clients
int JackEngine::ClientInternalClose(int refnum)
{
    JackClientInterface* client = fClientTable[refnum];
    return (client)	? ClientCloseAux(refnum, client, true) : -1;
}

// Used for drivers that close when the RT thread is stopped
int JackEngine::ClientInternalCloseIm(int refnum)
{
    JackClientInterface* client = fClientTable[refnum];
    return (client)	? ClientCloseAux(refnum, client, false) : -1;
}

int JackEngine::ClientCloseAux(int refnum, JackClientInterface* client, bool wait)
{
    JackLog("JackEngine::ClientCloseAux ref = %ld name = %s\n", refnum, client->GetClientControl()->fName);

    // Remove the client from the table
    fClientTable[refnum] = NULL;

    // Remove ports
    fGraphManager->RemoveAllPorts(refnum);

    // Wait until next cycle to be sure client is not used anymore
    if (wait) {
        if (!fSignal->TimedWait(fEngineControl->fTimeOutUsecs * 2)) { // Must wait at least until a switch occurs in Process, even in case of graph end failure
            jack_error("JackEngine::ClientCloseAux wait error ref = %ld", refnum);
        }
    }

    // Notify running clients
    NotifyRemoveClient(client->GetClientControl()->fName, client->GetClientControl()->fRefNum);

    // Cleanup...
    fSynchroTable[refnum]->Destroy();
    fEngineTiming->ResetRollingUsecs();
    return 0;
}

int JackEngine::ClientActivate(int refnum)
{
    JackClientInterface* client = fClientTable[refnum];
	assert(fClientTable[refnum]);
        
	JackLog("JackEngine::ClientActivate ref = %ld name = %s\n", refnum, client->GetClientControl()->fName);
	fGraphManager->Activate(refnum);
  	
	// Wait for graph state change to be effective
	if (!fSignal->TimedWait(fEngineControl->fTimeOutUsecs * 10)) {
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
    JackClientInterface* client = fClientTable[refnum];
	if (client == NULL) 
	    return -1;

	JackLog("JackEngine::ClientDeactivate ref = %ld name = %s\n", refnum, client->GetClientControl()->fName);	
	fGraphManager->Deactivate(refnum);
	fLastSwitchUsecs = 0; // Force switch to occur next cycle, even when called with "dead" clients
		
	// Wait for graph state change to be effective
	if (!fSignal->TimedWait(fEngineControl->fTimeOutUsecs * 10)) {
		jack_error("JackEngine::ClientDeactivate wait error ref = %ld name = %s", refnum, client->GetClientControl()->fName);
		return -1;
	} else {
		return 0;
	}
}

//-----------------
// Port management
//-----------------

int JackEngine::PortRegister(int refnum, const char* name, unsigned int flags, unsigned int buffer_size, jack_port_id_t* port_index)
{
    JackLog("JackEngine::PortRegister ref = %ld name = %s flags = %d buffer_size = %d\n", refnum, name, flags, buffer_size);
    assert(fClientTable[refnum]);

    *port_index = fGraphManager->AllocatePort(refnum, name, (JackPortFlags)flags);
    if (*port_index != NO_PORT) {
        NotifyPortRegistation(*port_index, true);
        return 0;
    } else {
        return -1;
    }
}

int JackEngine::PortUnRegister(int refnum, jack_port_id_t port_index)
{
    JackLog("JackEngine::PortUnRegister ref = %ld port_index = %ld\n", refnum, port_index);
    assert(fClientTable[refnum]);

    if (fGraphManager->ReleasePort(refnum, port_index) == 0) {
        NotifyPortRegistation(port_index, false);
        return 0;
    } else {
        return -1;
    }
}

int JackEngine::PortConnect(int refnum, const char* src, const char* dst)
{
    JackLog("JackEngine::PortConnect src = %s dst = %s\n", src, dst);
    jack_port_id_t port_src, port_dst;

    return (fGraphManager->CheckPorts(src, dst, &port_src, &port_dst) < 0)
           ? -1
           : PortConnect(refnum, port_src, port_dst);
}

int JackEngine::PortDisconnect(int refnum, const char* src, const char* dst)
{
    JackLog("JackEngine::PortDisconnect src = %s dst = %s\n", src, dst);
    jack_port_id_t port_src, port_dst;

    return (fGraphManager->CheckPorts(src, dst, &port_src, &port_dst) < 0)
           ? -1
           : fGraphManager->Disconnect(port_src, port_dst);
}

int JackEngine::PortConnect(int refnum, jack_port_id_t src, jack_port_id_t dst)
{
    JackLog("JackEngine::PortConnect src = %d dst = %d\n", src, dst);
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

    return fGraphManager->Connect(src, dst);
}

int JackEngine::PortDisconnect(int refnum, jack_port_id_t src, jack_port_id_t dst)
{
    JackLog("JackEngine::PortDisconnect src = %d dst = %d\n", src, dst);

    if (dst == ALL_PORTS) {
        return (fGraphManager->CheckPort(src) < 0)
               ? -1
               : fGraphManager->DisconnectAll(src);
    } else {
        return (fGraphManager->CheckPorts(src, dst) < 0)
               ? -1
               : fGraphManager->Disconnect(src, dst);
    }
}

//----------------------
// Transport management
//----------------------

int JackEngine::ReleaseTimebase(int refnum)
{
    return fEngineControl->fTransport.ResetTimebase(refnum);
}

int JackEngine::SetTimebaseCallback(int refnum, int conditional)
{
    return fEngineControl->fTransport.SetTimebase(refnum, conditional);
}

//-----------
// Debugging
//-----------

void JackEngine::PrintState()
{
    std::cout << "Engine State" << std::endl;

    for (int i = 0; i < CLIENT_NUM; i++) {
        JackClientInterface* client = fClientTable[i];
        if (client)
            std::cout << "Client : " << client->GetClientControl()->fName << " : " << i << std::endl;
    }

    //fGraphManager->PrintState();
    fEngineTiming->PrintState();
}

} // end of namespace

