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

#include "JackConnectionManager.h"
#include "JackClientControl.h"
#include "JackEngineControl.h"
#include "JackGlobals.h"
#include "JackError.h"
#include <set>
#include <iostream>
#include <assert.h>

namespace Jack
{

JackConnectionManager::JackConnectionManager()
{
    int i;
    static_assert(offsetof(JackConnectionManager, fInputCounter) % sizeof(UInt32) == 0,
                  "fInputCounter must be aligned within JackConnectionManager");

    jack_log("JackConnectionManager::InitConnections size = %ld ", sizeof(JackConnectionManager));

    for (i = 0; i < PORT_NUM_MAX; i++) {
        fConnection[i].Init();
    }

    fLoopFeedback.Init();

    jack_log("JackConnectionManager::InitClients");
    for (i = 0; i < CLIENT_NUM; i++) {
        InitRefNum(i);
    }
}

JackConnectionManager::~JackConnectionManager()
{}

//--------------
// Internal API
//--------------

bool JackConnectionManager::IsLoopPathAux(int ref1, int ref2) const
{
    jack_log("JackConnectionManager::IsLoopPathAux ref1 = %ld ref2 = %ld", ref1, ref2);

    if (ref1 < GetEngineControl()->fDriverNum || ref2 < GetEngineControl()->fDriverNum) {
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
                if (IsLoopPathAux(output[i], ref2)) {
                    return true; // Stop when a path is found
                }
            }
            return false;
        }
    }
}

//--------------
// External API
//--------------

/*!
\brief Connect port_src to port_dst.
*/
int JackConnectionManager::Connect(jack_port_id_t port_src, jack_port_id_t port_dst)
{
    jack_log("JackConnectionManager::Connect port_src = %ld port_dst = %ld", port_src, port_dst);

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
    jack_log("JackConnectionManager::Disconnect port_src = %ld port_dst = %ld", port_src, port_dst);

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
        jack_log("JackConnectionManager::AddInputPort ref = %ld port = %ld", refnum, port_index);
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
        jack_log("JackConnectionManager::AddOutputPort ref = %ld port = %ld", refnum, port_index);
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
    jack_log("JackConnectionManager::RemoveInputPort ref = %ld port_index = %ld ", refnum, port_index);

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
    jack_log("JackConnectionManager::RemoveOutputPort ref = %ld port_index = %ld ", refnum, port_index);

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
\brief Init the refnum.
*/
void JackConnectionManager::InitRefNum(int refnum)
{
    fInputPort[refnum].Init();
    fOutputPort[refnum].Init();
    fConnectionRef.Init(refnum);
    fInputCounter[refnum].SetValue(0);
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
int JackConnectionManager::SuspendRefNum(JackClientControl* control, JackSynchro* table, JackClientTiming* timing, long time_out_usec)
{
    bool res;
    if ((res = table[control->fRefNum].TimedWait(time_out_usec))) {
        timing[control->fRefNum].fStatus = Running;
        timing[control->fRefNum].fAwakeAt = GetMicroSeconds();
    }
    return (res) ? 0 : -1;
}

/*!
\brief Signal clients connected to the given client.
*/
int JackConnectionManager::ResumeRefNum(JackClientControl* control, JackSynchro* table, JackClientTiming* timing)
{
    jack_time_t current_date = GetMicroSeconds();
    const jack_int_t* output_ref = fConnectionRef.GetItems(control->fRefNum);
    int res = 0;

    // Update state and timestamp of current client
    timing[control->fRefNum].fStatus = Finished;
    timing[control->fRefNum].fFinishedAt = current_date;

    for (int i = 0; i < CLIENT_NUM; i++) {

        // Signal connected clients or drivers
        if (output_ref[i] > 0) {

            // Update state and timestamp of destination clients
            timing[i].fStatus = Triggered;
            timing[i].fSignaledAt = current_date;

            if (!fInputCounter[i].Signal(table + i, control)) {
                jack_log("JackConnectionManager::ResumeRefNum error: ref = %ld output = %ld ", control->fRefNum, i);
                res = -1;
            }
        }
    }

    return res;
}

static bool HasNoConnection(jack_int_t* table)
{
    for (int ref = 0; ref < CLIENT_NUM; ref++) {
        if (table[ref] > 0) return false;
    }
    return true;
}

// Using http://en.wikipedia.org/wiki/Topological_sorting

void JackConnectionManager::TopologicalSort(std::vector<jack_int_t>& sorted)
{
    JackFixedMatrix<CLIENT_NUM>* tmp = new JackFixedMatrix<CLIENT_NUM>;
    std::set<jack_int_t> level;

    fConnectionRef.Copy(*tmp);

    // Inputs of the graph
    level.insert(AUDIO_DRIVER_REFNUM);
    level.insert(FREEWHEEL_DRIVER_REFNUM);

    while (level.size() > 0) {
        jack_int_t refnum = *level.begin();
        sorted.push_back(refnum);
        level.erase(level.begin());
        const jack_int_t* output_ref1 = tmp->GetItems(refnum);
        for (int dst = 0; dst < CLIENT_NUM; dst++) {
            if (output_ref1[dst] > 0) {
                tmp->ClearItem(refnum, dst);
                jack_int_t output_ref2[CLIENT_NUM];
                tmp->GetOutputTable1(dst, output_ref2);
                if (HasNoConnection(output_ref2)) {
                    level.insert(dst);
                }
            }
        }
    }

    delete tmp;
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
    jack_log("JackConnectionManager::IncConnectionRef: ref1 = %ld ref2 = %ld", ref1, ref2);
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
    jack_log("JackConnectionManager::DecConnectionRef: ref1 = %ld ref2 = %ld", ref1, ref2);
}

/*!
\brief Directly connect 2 reference numbers.
*/
void JackConnectionManager::DirectConnect(int ref1, int ref2)
{
    assert(ref1 >= 0 && ref2 >= 0);

    if (fConnectionRef.IncItem(ref1, ref2) == 1) { // First connection between client ref1 and client ref2
        jack_log("JackConnectionManager::DirectConnect first: ref1 = %ld ref2 = %ld", ref1, ref2);
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
        jack_log("JackConnectionManager::DirectDisconnect last: ref1 = %ld ref2 = %ld", ref1, ref2);
        fInputCounter[ref2].DecValue();
    }
}

/*!
\brief Returns the connections state between 2 refnum.
*/
bool JackConnectionManager::IsDirectConnection(int ref1, int ref2) const
{
    assert(ref1 >= 0 && ref2 >= 0);
    return (fConnectionRef.GetItemCount(ref1, ref2) > 0);
}

/*!
\brief Get the client refnum of a given input port.
*/
int JackConnectionManager::GetInputRefNum(jack_port_id_t port_index) const
{
    for (int i = 0; i < CLIENT_NUM; i++) {
        if (fInputPort[i].CheckItem(port_index)) {
            return i;
        }
    }

    return -1;
}

/*!
\brief Get the client refnum of a given output port.
*/
int JackConnectionManager::GetOutputRefNum(jack_port_id_t port_index) const
{
    for (int i = 0; i < CLIENT_NUM; i++) {
        if (fOutputPort[i].CheckItem(port_index)) {
            return i;
        }
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
    jack_log("JackConnectionManager::IncFeedbackConnection ref1 = %ld ref2 = %ld", ref1, ref2);
    assert(ref1 >= 0 && ref2 >= 0);

    if (ref1 != ref2) {
        DirectConnect(ref2, ref1);
    }

    return fLoopFeedback.IncConnection(ref1, ref2); // Add the feedback connection
}

bool JackConnectionManager::DecFeedbackConnection(jack_port_id_t port_src, jack_port_id_t port_dst)
{
    int ref1 = GetOutputRefNum(port_src);
    int ref2 = GetInputRefNum(port_dst);

    // Remove an activation connection in the other direction
    jack_log("JackConnectionManager::DecFeedbackConnection ref1 = %ld ref2 = %ld", ref1, ref2);
    assert(ref1 >= 0 && ref2 >= 0);

    if (ref1 != ref2) {
        DirectDisconnect(ref2, ref1);
    }

    return fLoopFeedback.DecConnection(ref1, ref2); // Remove the feedback connection
}

} // end of namespace


