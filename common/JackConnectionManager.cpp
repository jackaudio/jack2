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
#include <assert.h>
#include "JackConnectionManager.h"
#include "JackClientControl.h"
#include "JackError.h"

namespace Jack
{

JackConnectionManager::JackConnectionManager()
{
    int i;
    JackLog("JackConnectionManager::InitConnections size = %ld \n", sizeof(JackConnectionManager));

    for (i = 0; i < PORT_NUM; i++) {
        fConnection[i].Init();
    }

    fLoopFeedback.Init();

    JackLog("JackConnectionManager::InitClients\n");
    for (i = 0; i < CLIENT_NUM; i++) {
        InitClient(i);
    }
}

JackConnectionManager::~JackConnectionManager()
{}

//--------------
// Internal API
//--------------

bool JackConnectionManager::IsLoopPathAux(int ref1, int ref2) const
{
    JackLog("JackConnectionManager::IsLoopPathAux ref1 = %ld ref2 = %ld\n", ref1, ref2);

    if (ref1 == AUDIO_DRIVER_REFNUM // Driver is reached
            || ref2 == AUDIO_DRIVER_REFNUM
            || ref1 == FREEWHEEL_DRIVER_REFNUM
            || ref2 == FREEWHEEL_DRIVER_REFNUM
            || ref1 == LOOPBACK_DRIVER_REFNUM
            || ref2 == LOOPBACK_DRIVER_REFNUM) {
        return false;
    } else if (ref1 == ref2) {	// Same refnum
        return true;
    } else {
        jack_int_t output[CLIENT_NUM];
        fConnectionRef.GetOutputTable(ref1, output);

        if (fConnectionRef.IsInsideTable(ref2, output)) { // If ref2 is contained in the outputs of ref1
            return true;
        } else {
            for (int i = 0; i < CLIENT_NUM && output[i] != EMPTY; i++) { // Otherwise recurse for all ref1 outputs
                if (IsLoopPathAux(output[i], ref2))
                    return true; // Stop when a path is found
            }
            return false;
        }
    }
}

void JackConnectionManager::InitClient(int refnum)
{
    fInputPort[refnum].Init();
    fOutputPort[refnum].Init();
    fConnectionRef.Init(refnum);
    fInputCounter[refnum].SetValue(0);
}

//--------------
// External API
//--------------

int JackConnectionManager::GetActivation(int refnum) const
{
    return fInputCounter[refnum].GetValue();
}

/*!
\brief Connect port_src to port_dst.
*/
int JackConnectionManager::Connect(jack_port_id_t port_src, jack_port_id_t port_dst)
{
    JackLog("JackConnectionManager::Connect port_src = %ld port_dst = %ld\n", port_src, port_dst);

    if (fConnection[port_src].AddItem(port_dst)) {
        return 0;
    } else {
        jack_error("Connection table is full !!");
        return -1;
    }
}

/*!
\brief Disconnect port_src from port_dst.
*/
int JackConnectionManager::Disconnect(jack_port_id_t port_src, jack_port_id_t port_dst)
{
    JackLog("JackConnectionManager::Disconnect port_src = %ld port_dst = %ld\n", port_src, port_dst);

    if (fConnection[port_src].RemoveItem(port_dst)) {
        return 0;
    } else {
        jack_error("Connection not found !!");
        return -1;
    }
}

/*!
\brief Check if port_src and port_dst are connected.
*/
bool JackConnectionManager::IsConnected(jack_port_id_t port_src, jack_port_id_t port_dst) const
{
    return fConnection[port_src].CheckItem(port_dst);
}

/*!
\brief Get the connection number of a given port.
*/
jack_int_t JackConnectionManager::Connections(jack_port_id_t port_index) const
{
    return fConnection[port_index].GetItemCount();
}

jack_port_id_t JackConnectionManager::GetPort(jack_port_id_t port_index, int connection) const
{
    assert(connection < CONNECTION_NUM);
    return (jack_port_id_t)fConnection[port_index].GetItem(connection);
}

/*!
\brief Get the connection port array.
*/
const jack_int_t* JackConnectionManager::GetConnections(jack_port_id_t port_index) const
{
    return fConnection[port_index].GetItems();
}

//------------------------
// Client port management
//------------------------

/*!
\brief Add an input port to a client.
*/
int JackConnectionManager::AddInputPort(int refnum, jack_port_id_t port_index)
{
    if (fInputPort[refnum].AddItem(port_index)) {
        JackLog("JackConnectionManager::AddInputPort ref = %ld port = %ld\n", refnum, port_index);
        return 0;
    } else {
        jack_error("Maximum number of input ports is reached for application ref = %ld", refnum);
        return -1;
    }
}

/*!
\brief Add an output port to a client.
*/
int JackConnectionManager::AddOutputPort(int refnum, jack_port_id_t port_index)
{
    if (fOutputPort[refnum].AddItem(port_index)) {
        JackLog("JackConnectionManager::AddOutputPort ref = %ld port = %ld\n", refnum, port_index);
        return 0;
    } else {
        jack_error("Maximum number of output ports is reached for application ref = %ld", refnum);
        return -1;
    }
}

/*!
\brief Remove an input port from a client.
*/
int JackConnectionManager::RemoveInputPort(int refnum, jack_port_id_t port_index)
{
    JackLog("JackConnectionManager::RemoveInputPort ref = %ld port_index = %ld \n", refnum, port_index);

    if (fInputPort[refnum].RemoveItem(port_index)) {
        return 0;
    } else {
        jack_error("Input port index = %ld not found for application ref = %ld", port_index, refnum);
        return -1;
    }
}

/*!
\brief Remove an output port from a client.
*/
int JackConnectionManager::RemoveOutputPort(int refnum, jack_port_id_t port_index)
{
    JackLog("JackConnectionManager::RemoveOutputPort ref = %ld port_index = %ld \n", refnum, port_index);

    if (fOutputPort[refnum].RemoveItem(port_index)) {
        return 0;
    } else {
        jack_error("Output port index = %ld not found for application ref = %ld", port_index, refnum);
        return -1;
    }
}

/*!
\brief Get the input port array of a given refnum.
*/
const jack_int_t* JackConnectionManager::GetInputPorts(int refnum)
{
    return fInputPort[refnum].GetItems();
}

/*!
\brief Get the output port array of a given refnum.
*/
const jack_int_t* JackConnectionManager::GetOutputPorts(int refnum)
{
    return fOutputPort[refnum].GetItems();
}

/*!
\brief Return the first available refnum.
*/
int JackConnectionManager::AllocateRefNum()
{
    for (int i = 0; i < CLIENT_NUM; i++) {
        if (fInputPort[i].IsAvailable()) {
            JackLog("JackConnectionManager::AllocateRefNum ref = %ld\n", i);
            return i;
        }
    }

    return -1;
}

/*!
\brief Release the refnum.
*/
void JackConnectionManager::ReleaseRefNum(int refnum)
{
    JackLog("JackConnectionManager::ReleaseRefNum ref = %ld\n", refnum);
    InitClient(refnum);
}

/*!
\brief Reset all clients activation.
*/
void JackConnectionManager::ResetGraph(JackClientTiming* timing)
{
    // Reset activation counter : must be done *before* starting to resume clients
    for (int i = 0; i < CLIENT_NUM; i++) {
        fInputCounter[i].Reset();
		timing[i].fStatus = NotTriggered;
    }
}

/*!
\brief Wait on the input synchro.
*/
int JackConnectionManager::SuspendRefNum(JackClientControl* control, JackSynchro** table, JackClientTiming* timing, long time_out_usec)
{
    bool res;
    if ((res = table[control->fRefNum]->TimedWait(time_out_usec))) {
		timing[control->fRefNum].fStatus = Running;
		timing[control->fRefNum].fAwakeAt = GetMicroSeconds();
    }
    return (res) ? 0 : -1;
}

/*!
\brief Signal clients connected to the given client.
*/
int JackConnectionManager::ResumeRefNum(JackClientControl* control, JackSynchro** table, JackClientTiming* timing)
{
	jack_time_t current_date = GetMicroSeconds(); 
    const jack_int_t* outputRef = fConnectionRef.GetItems(control->fRefNum);
	int res = 0;

    // Update state and timestamp of current client
	timing[control->fRefNum].fStatus = Finished;
	timing[control->fRefNum].fFinishedAt = current_date;

    for (int i = 0; i < CLIENT_NUM; i++) {
	
        // Signal connected clients or drivers
        if (outputRef[i] > 0) {
		
			// Update state and timestamp of destination clients
			timing[i].fStatus = Triggered;
			timing[i].fSignaledAt = current_date;

            if (!fInputCounter[i].Signal(table[i], control)) {
                JackLog("JackConnectionManager::ResumeRefNum error: ref = %ld output = %ld \n", control->fRefNum, i);
                res = -1;
            }
        }
    }

    return res;
}

/*!
\brief Increment the number of ports between 2 clients, if the 2 clients become connected, then the Activation counter is updated.
*/
void JackConnectionManager::IncDirectConnection(jack_port_id_t port_src, jack_port_id_t port_dst)
{
    int ref1 = GetOutputRefNum(port_src);
    int ref2 = GetInputRefNum(port_dst);

    assert(ref1 >= 0 && ref2 >= 0);

    DirectConnect(ref1, ref2);
    JackLog("JackConnectionManager::IncConnectionRef: ref1 = %ld ref2 = %ld\n", ref1, ref2);
}

/*!
\brief Decrement the number of ports between 2 clients, if the 2 clients become disconnected, then the Activation counter is updated.
*/
void JackConnectionManager::DecDirectConnection(jack_port_id_t port_src, jack_port_id_t port_dst)
{
    int ref1 = GetOutputRefNum(port_src);
    int ref2 = GetInputRefNum(port_dst);

    assert(ref1 >= 0 && ref2 >= 0);

    DirectDisconnect(ref1, ref2);
    JackLog("JackConnectionManager::DecConnectionRef: ref1 = %ld ref2 = %ld\n", ref1, ref2);
}

/*!
\brief Directly connect 2 reference numbers.
*/
void JackConnectionManager::DirectConnect(int ref1, int ref2)
{
    assert(ref1 >= 0 && ref2 >= 0);

    if (fConnectionRef.IncItem(ref1, ref2) == 1) { // First connection between client ref1 and client ref2
        JackLog("JackConnectionManager::DirectConnect first: ref1 = %ld ref2 = %ld\n", ref1, ref2);
        fInputCounter[ref2].IncValue();
    }
}

/*!
\brief Directly disconnect 2 reference numbers.
*/
void JackConnectionManager::DirectDisconnect(int ref1, int ref2)
{
    assert(ref1 >= 0 && ref2 >= 0);

    if (fConnectionRef.DecItem(ref1, ref2) == 0) { // Last connection between client ref1 and client ref2
        JackLog("JackConnectionManager::DirectDisconnect last: ref1 = %ld ref2 = %ld\n", ref1, ref2);
        fInputCounter[ref2].DecValue();
    }
}

/*!
\brief Returns the connections state between 2 refnum.
*/
bool JackConnectionManager::IsDirectConnection(int ref1, int ref2) const
{
    assert(ref1 >= 0 && ref2 >= 0);
    return fConnectionRef.GetItemCount(ref1, ref2);
}

/*!
\brief Get the client refnum of a given input port.
*/
int JackConnectionManager::GetInputRefNum(jack_port_id_t port_index) const
{
    for (int i = 0; i < CLIENT_NUM; i++) {
        if (fInputPort[i].CheckItem(port_index))
            return i;
    }

    return -1;
}

/*!
\brief Get the client refnum of a given ouput port.
*/
int JackConnectionManager::GetOutputRefNum(jack_port_id_t port_index) const
{
    for (int i = 0; i < CLIENT_NUM; i++) {
        if (fOutputPort[i].CheckItem(port_index))
            return i;
    }

    return -1;
}

/*!
\brief Test is a connection path exists between port_src and port_dst.
*/
bool JackConnectionManager::IsLoopPath(jack_port_id_t port_src, jack_port_id_t port_dst) const
{
    return IsLoopPathAux(GetInputRefNum(port_dst), GetOutputRefNum(port_src));
}

bool JackConnectionManager::IsFeedbackConnection(jack_port_id_t port_src, jack_port_id_t port_dst) const
{
    return (fLoopFeedback.GetConnectionIndex(GetOutputRefNum(port_src), GetInputRefNum(port_dst)) >= 0);
}

bool JackConnectionManager::IncFeedbackConnection(jack_port_id_t port_src, jack_port_id_t port_dst)
{
    int ref1 = GetOutputRefNum(port_src);
    int ref2 = GetInputRefNum(port_dst);

    // Add an activation connection in the other direction
    JackLog("JackConnectionManager::IncFeedbackConnection ref1 = %ld ref2 = %ld\n", ref1, ref2);
    assert(ref1 >= 0 && ref2 >= 0);

    if (ref1 != ref2)
        DirectConnect(ref2, ref1);

    return fLoopFeedback.IncConnection(ref1, ref2); // Add the feedback connection
}

bool JackConnectionManager::DecFeedbackConnection(jack_port_id_t port_src, jack_port_id_t port_dst)
{
    int ref1 = GetOutputRefNum(port_src);
    int ref2 = GetInputRefNum(port_dst);

    // Remove an activation connection in the other direction
    JackLog("JackConnectionManager::DecFeedbackConnection ref1 = %ld ref2 = %ld\n", ref1, ref2);
    assert(ref1 >= 0 && ref2 >= 0);

    if (ref1 != ref2)
        DirectDisconnect(ref2, ref1);

    return fLoopFeedback.DecConnection(ref1, ref2); // Remove the feedback connection
}

} // end of namespace


