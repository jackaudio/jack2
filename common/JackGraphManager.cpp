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

#include "JackGraphManager.h"
#include "JackConstants.h"
#include <assert.h>
#include <stdlib.h>
#include <algorithm>
#include <regex.h>

namespace Jack
{

static inline jack_nframes_t MAX(jack_nframes_t a, jack_nframes_t b)
{
    return (a < b) ? b : a;
}

static void AssertPort(jack_port_id_t port_index)
{
    if (port_index >= PORT_NUM) {
        JackLog("JackGraphManager::AssertPort port_index = %ld\n", port_index);
        assert(port_index < PORT_NUM);
    }
}

static void AssertBufferSize(jack_nframes_t buffer_size)
{
    if (buffer_size > BUFFER_SIZE_MAX) {
        JackLog("JackGraphManager::AssertBufferSize frames = %ld\n", buffer_size);
        assert(buffer_size <= BUFFER_SIZE_MAX);
    }
}

JackPort* JackGraphManager::GetPort(jack_port_id_t port_index)
{
    AssertPort(port_index);
    return &fPortArray[port_index];
}

float* JackGraphManager::GetBuffer(jack_port_id_t port_index)
{
    return fPortArray[port_index].GetBuffer();
}

// RT, client
int JackGraphManager::GetConnectionsNum(jack_port_id_t port_index)
{
    JackConnectionManager* manager = ReadCurrentState();
    return manager->Connections(port_index);
}

// Server
void JackGraphManager::InitRefNum(int refnum)
{
    JackConnectionManager* manager = WriteNextStateStart();
    manager->InitRefNum(refnum);
    WriteNextStateStop();
}

// RT
void JackGraphManager::RunCurrentGraph()
{
    JackConnectionManager* manager = ReadCurrentState();
    manager->ResetGraph(fClientTiming);
}

// RT
bool JackGraphManager::RunNextGraph()
{
    bool res;
    JackConnectionManager* manager = TrySwitchState(&res);
    manager->ResetGraph(fClientTiming);
    return res;
}

// RT
bool JackGraphManager::IsFinishedGraph()
{
    JackConnectionManager* manager = ReadCurrentState();
    return (manager->GetActivation(FREEWHEEL_DRIVER_REFNUM) == 0);
}

// RT
int JackGraphManager::ResumeRefNum(JackClientControl* control, JackSynchro** table)
{
    JackConnectionManager* manager = ReadCurrentState();
    return manager->ResumeRefNum(control, table, fClientTiming);
}

// RT
int JackGraphManager::SuspendRefNum(JackClientControl* control, JackSynchro** table, long usec)
{
    JackConnectionManager* manager = ReadCurrentState();
    return manager->SuspendRefNum(control, table, fClientTiming, usec);
}

JackClientTiming* JackGraphManager::GetClientTiming(int ref)
{
    return &fClientTiming[ref];
}

// Server
void JackGraphManager::DirectConnect(int ref1, int ref2)
{
    JackConnectionManager* manager = WriteNextStateStart();
    manager->DirectConnect(ref1, ref2);
    JackLog("JackGraphManager::ConnectRefNum cur_index = %ld ref1 = %ld ref2 = %ld\n", CurIndex(fCounter), ref1, ref2);
    WriteNextStateStop();
}

// Server
void JackGraphManager::DirectDisconnect(int ref1, int ref2)
{
    JackConnectionManager* manager = WriteNextStateStart();
    manager->DirectDisconnect(ref1, ref2);
    JackLog("JackGraphManager::DisconnectRefNum cur_index = %ld ref1 = %ld ref2 = %ld\n", CurIndex(fCounter), ref1, ref2);
    WriteNextStateStop();
}

// Server
bool JackGraphManager::IsDirectConnection(int ref1, int ref2)
{
    JackConnectionManager* manager = ReadCurrentState();
    return manager->IsDirectConnection(ref1, ref2);
}

// RT
void* JackGraphManager::GetBuffer(jack_port_id_t port_index, jack_nframes_t buffer_size)
{
    AssertPort(port_index);
    AssertBufferSize(buffer_size);

    JackConnectionManager* manager = ReadCurrentState();
    JackPort* port = GetPort(port_index);

    if (!port->IsUsed()) {
        // This happens when a port has just been unregistered and is still used by the RT code.
        JackLog("JackGraphManager::GetBuffer : port = %ld is released state\n", port_index);
        return GetBuffer(0); // port_index 0 is not used
    }

    // Output port
    if (port->fFlags & JackPortIsOutput) {
        return (port->fTied != NO_PORT) ? GetBuffer(port->fTied, buffer_size) : GetBuffer(port_index);
    }

    // Input port
    jack_int_t len = manager->Connections(port_index);

    if (len == 0) {  // No connections: return a zero-filled buffer
        port->ClearBuffer(buffer_size);
        return port->GetBuffer();
    } else if (len == 1) {	 // One connection: use zero-copy mode - just pass the buffer of the connected (output) port.
        assert(manager->GetPort(port_index, 0) != port_index); // Check recursion
        return GetBuffer(manager->GetPort(port_index, 0), buffer_size);
    } else {  // Multiple connections

        const jack_int_t* connections = manager->GetConnections(port_index);
        void* buffers[CONNECTION_NUM];
        jack_port_id_t src_index;
        int i;

        for (i = 0; (i < CONNECTION_NUM) && ((src_index = connections[i]) != EMPTY); i++) {
            AssertPort(src_index);
            buffers[i] = GetBuffer(src_index, buffer_size);
        }

        JackPort* port = GetPort(port_index);
        port->MixBuffers(buffers, i, buffer_size);
        return port->GetBuffer();
    }
}

// Server
int JackGraphManager::RequestMonitor(jack_port_id_t port_index, bool onoff) // Client
{
    AssertPort(port_index);
    JackPort* port = GetPort(port_index);

    /**
    jackd.h 
        * If @ref JackPortCanMonitor is set for this @a port, turn input
        * monitoring on or off. Otherwise, do nothing.
     
     if (!(fFlags & JackPortCanMonitor))
    	return -1;
    */

    port->RequestMonitor(onoff);

    const jack_int_t* connections = ReadCurrentState()->GetConnections(port_index);
    if ((port->fFlags & JackPortIsOutput) == 0) { // ?? Taken from jack, why not (port->fFlags  & JackPortIsInput) ?
        jack_port_id_t src_index;
        for (int i = 0; (i < CONNECTION_NUM) && ((src_index = connections[i]) != EMPTY); i++) {
            // XXX much worse things will happen if there is a feedback loop !!!
            RequestMonitor(src_index, onoff);
        }
    }

    return 0;
}

// Client
jack_nframes_t JackGraphManager::ComputeTotalLatencyAux(jack_port_id_t port_index, jack_port_id_t src_port_index, JackConnectionManager* manager, int hop_count)
{
    const jack_int_t* connections = manager->GetConnections(port_index);
    jack_nframes_t max_latency = 0;
    jack_port_id_t dst_index;

    if (hop_count > 8)
        return GetPort(port_index)->GetLatency();

    for (int i = 0; (i < CONNECTION_NUM) && ((dst_index = connections[i]) != EMPTY); i++) {
        if (src_port_index != dst_index) {
            AssertPort(dst_index);
            JackPort* dst_port = GetPort(dst_index);
            jack_nframes_t this_latency = (dst_port->fFlags & JackPortIsTerminal)
                                          ? dst_port->GetLatency()
                                          : ComputeTotalLatencyAux(dst_index, port_index, manager, hop_count + 1);
            max_latency = MAX(max_latency, this_latency);
        }
    }

    return max_latency + GetPort(port_index)->GetLatency();
}

// Client
int JackGraphManager::ComputeTotalLatency(jack_port_id_t port_index)
{
    UInt16 cur_index;
    UInt16 next_index;
    JackPort* port = GetPort(port_index);
    AssertPort(port_index);

    do {
        cur_index = GetCurrentIndex();
        port->fTotalLatency = ComputeTotalLatencyAux(port_index, port_index, ReadCurrentState(), 0);
        next_index = GetCurrentIndex();
    } while (cur_index != next_index); // Until a coherent state has been read

    JackLog("JackGraphManager::GetTotalLatency port_index = %ld total latency = %ld\n", port_index, port->fTotalLatency);
    return 0;
}

// Client
int JackGraphManager::ComputeTotalLatencies()
{
    jack_port_id_t port_index;
    for (port_index = FIRST_AVAILABLE_PORT; port_index < PORT_NUM; port_index++) {
        JackPort* port = GetPort(port_index);
        if (port->IsUsed())
            ComputeTotalLatency(port_index);
    }
    return 0;
}

// Server
void JackGraphManager::SetBufferSize(jack_nframes_t buffer_size)
{
    JackLock lock (this);
    JackLog("JackGraphManager::SetBufferSize size = %ld\n", (long int)buffer_size);

    jack_port_id_t port_index;
    for (port_index = FIRST_AVAILABLE_PORT; port_index < PORT_NUM; port_index++) {
        JackPort* port = GetPort(port_index);
        if (port->IsUsed())
            port->ClearBuffer(buffer_size);
    }
}

// Server
jack_port_id_t JackGraphManager::AllocatePortAux(int refnum, const char* port_name, const char* port_type, JackPortFlags flags)
{
    jack_port_id_t port_index;

    // Available ports start at FIRST_AVAILABLE_PORT (= 1), otherwise a port_index of 0 is "seen" as a NULL port by the external API...
    for (port_index = FIRST_AVAILABLE_PORT; port_index < PORT_NUM; port_index++) {
        JackPort* port = GetPort(port_index);
        if (!port->IsUsed()) {
            JackLog("JackGraphManager::AllocatePortAux port_index = %ld name = %s type = %s\n", port_index, port_name, port_type);
            if (!port->Allocate(refnum, port_name, port_type, flags))
                return NO_PORT;
            break;
        }
    }

    return (port_index < PORT_NUM) ? port_index : NO_PORT;
}

// Server
jack_port_id_t JackGraphManager::AllocatePort(int refnum, const char* port_name, const char* port_type, JackPortFlags flags, jack_nframes_t buffer_size)
{
    JackLock lock (this);
    JackConnectionManager* manager = WriteNextStateStart();
    jack_port_id_t port_index = AllocatePortAux(refnum, port_name, port_type, flags);

    if (port_index != NO_PORT) {
        JackPort* port = GetPort(port_index);
        assert(port);
        port->ClearBuffer(buffer_size);

        int res;
        if (flags & JackPortIsOutput) {
            res = manager->AddOutputPort(refnum, port_index);
        } else {
            res = manager->AddInputPort(refnum, port_index);
        }
        // Insertion failure
        if (res < 0) {
            port->Release();
            port_index = NO_PORT;
        }
    }

    WriteNextStateStop();
    return port_index;
}

// Server
int JackGraphManager::ReleasePort(int refnum, jack_port_id_t port_index)
{
    JackLock lock (this);
    JackConnectionManager* manager = WriteNextStateStart();
    JackPort* port = GetPort(port_index);
    int res;

    if (port->fFlags & JackPortIsOutput) {
        DisconnectAllOutput(port_index);
        res = manager->RemoveOutputPort(refnum, port_index);
    } else {
        DisconnectAllInput(port_index);
        res = manager->RemoveInputPort(refnum, port_index);
    }

    port->Release();
    WriteNextStateStop();
    return res;
}

void JackGraphManager::GetInputPorts(int refnum, jack_int_t* res)
{
    JackConnectionManager* manager = WriteNextStateStart();
    const jack_int_t* input = manager->GetInputPorts(refnum);
    memcpy(res, input, sizeof(jack_int_t) * PORT_NUM_FOR_CLIENT);
    WriteNextStateStop();
}

void JackGraphManager::GetOutputPorts(int refnum, jack_int_t* res)
{
    JackConnectionManager* manager = WriteNextStateStart();
    const jack_int_t* output = manager->GetOutputPorts(refnum);
    memcpy(res, output, sizeof(jack_int_t) * PORT_NUM_FOR_CLIENT);
    WriteNextStateStop();
}

// Server
void JackGraphManager::RemoveAllPorts(int refnum)
{
    JackLog("JackGraphManager::RemoveAllPorts ref = %ld\n", refnum);
    JackConnectionManager* manager = WriteNextStateStart();
    jack_port_id_t port_index;

    // Warning : RemovePort shift port to left, thus we always remove the first port until the "input" table is empty
    const jack_int_t* input = manager->GetInputPorts(refnum);
    while ((port_index = input[0]) != EMPTY) {
        ReleasePort(refnum, port_index);
    }

    // Warning : RemovePort shift port to left, thus we always remove the first port until the "output" table is empty
    const jack_int_t* output = manager->GetOutputPorts(refnum);
    while ((port_index = output[0]) != EMPTY) {
        ReleasePort(refnum, port_index);
    }

    WriteNextStateStop();
}

// Server
void JackGraphManager::DisconnectAllPorts(int refnum)
{
    int i;
    JackLog("JackGraphManager::DisconnectAllPorts ref = %ld\n", refnum);
    JackConnectionManager* manager = WriteNextStateStart();

    const jack_int_t* input = manager->GetInputPorts(refnum);
    for (i = 0; i < PORT_NUM_FOR_CLIENT && input[i] != EMPTY ; i++) {
        DisconnectAllInput(input[i]);
    }

    const jack_int_t* output = manager->GetOutputPorts(refnum);
    for (i = 0; i < PORT_NUM_FOR_CLIENT && output[i] != EMPTY; i++) {
        DisconnectAllOutput(output[i]);
    }

    WriteNextStateStop();
}

// Server
void JackGraphManager::DisconnectAllInput(jack_port_id_t port_index)
{
    JackLog("JackGraphManager::DisconnectAllInput port_index = %ld \n", port_index);
    JackConnectionManager* manager = WriteNextStateStart();

    for (int i = 0; i < PORT_NUM; i++) {
        if (manager->IsConnected(i, port_index)) {
            JackLog("JackGraphManager::Disconnect i = %ld  port_index = %ld\n", i, port_index);
            Disconnect(i, port_index);
        }
    }
    WriteNextStateStop();
}

// Server
void JackGraphManager::DisconnectAllOutput(jack_port_id_t port_index)
{
    JackLog("JackGraphManager::DisconnectAllOutput port_index = %ld \n", port_index);
    JackConnectionManager* manager = WriteNextStateStart();

    const jack_int_t* connections = manager->GetConnections(port_index);
    while (connections[0] != EMPTY) {
        Disconnect(port_index, connections[0]); // Warning : Disconnect shift port to left
    }
    WriteNextStateStop();
}

// Server
int JackGraphManager::DisconnectAll(jack_port_id_t port_index)
{
    AssertPort(port_index);

    JackPort* port = GetPort(port_index);
    if (port->fFlags & JackPortIsOutput) {
        DisconnectAllOutput(port_index);
    } else {
        DisconnectAllInput(port_index);
    }
    return 0;
}

// Server
void JackGraphManager::GetConnections(jack_port_id_t port_index, jack_int_t* res)
{
    JackConnectionManager* manager = WriteNextStateStart();
    const jack_int_t* connections = manager->GetConnections(port_index);
    memcpy(res, connections, sizeof(jack_int_t) * CONNECTION_NUM);
    WriteNextStateStop();
}

// Server
void JackGraphManager::Activate(int refnum)
{
    DirectConnect(FREEWHEEL_DRIVER_REFNUM, refnum);
    DirectConnect(refnum, FREEWHEEL_DRIVER_REFNUM);
}

/*
	Disconnection from the FW must be done in last otherwise an intermediate "unconnected"
	(thus unactivated) state may happen where the client is still checked for its end.
*/

// Server
void JackGraphManager::Deactivate(int refnum)
{
    DisconnectAllPorts(refnum);

    // Disconnect only when needed
    if (IsDirectConnection(refnum, FREEWHEEL_DRIVER_REFNUM)) {
        DirectDisconnect(refnum, FREEWHEEL_DRIVER_REFNUM);
    } else {
        JackLog("JackServer::Deactivate: client = %ld was not activated \n", refnum);
    }

    // Disconnect only when needed
    if (IsDirectConnection(FREEWHEEL_DRIVER_REFNUM, refnum)) {
        DirectDisconnect(FREEWHEEL_DRIVER_REFNUM, refnum);
    } else {
        JackLog("JackServer::Deactivate: client = %ld was not activated \n", refnum);
    }
}

// Server
int JackGraphManager::GetInputRefNum(jack_port_id_t port_index)
{
    AssertPort(port_index);
    JackConnectionManager* manager = WriteNextStateStart();
    int res = manager->GetInputRefNum(port_index);
    WriteNextStateStop();
    return res;
}

// Server
int JackGraphManager::GetOutputRefNum(jack_port_id_t port_index)
{
    AssertPort(port_index);
    JackConnectionManager* manager = WriteNextStateStart();
    int res = manager->GetOutputRefNum(port_index);
    WriteNextStateStop();
    return res;
}

int JackGraphManager::Connect(jack_port_id_t port_src, jack_port_id_t port_dst)
{
    JackConnectionManager* manager = WriteNextStateStart();
    JackLog("JackGraphManager::Connect port_src = %ld port_dst = %ld\n", port_src, port_dst);
    JackPort* src = GetPort(port_src);
    JackPort* dst = GetPort(port_dst);
    int res = 0;

    if (!src->fInUse || !dst->fInUse) {
        if (!src->fInUse)
            jack_error("JackGraphManager::Connect: port_src = %ld not used name = %s", port_src, GetPort(port_src)->fName);
        if (!dst->fInUse)
            jack_error("JackGraphManager::Connect: port_dst = %ld not used name = %s", port_dst, GetPort(port_dst)->fName);
        res = -1;
        goto end;
    }
    if (src->fTypeId != dst->fTypeId) {
        jack_error("JackGraphManager::Connect: different port types: port_src = %ld port_dst = %ld", port_src, port_dst);
        res = -1;
        goto end;
    }
    if (manager->IsConnected(port_src, port_dst)) {
        jack_error("JackGraphManager::Connect already connected port_src = %ld port_dst = %ld", port_src, port_dst);
        res = EEXIST;
        goto end;
    }

    res = manager->Connect(port_src, port_dst);
    if (res < 0) {
        jack_error("JackGraphManager::Connect failed port_src = %ld port_dst = %ld", port_src, port_dst);
        goto end;
    }
    manager->Connect(port_dst, port_src);
    if (res < 0) {
        jack_error("JackGraphManager::Connect failed port_dst = %ld port_src = %ld", port_dst, port_src);
        goto end;
    }

    if (manager->IsLoopPath(port_src, port_dst)) {
        JackLog("JackGraphManager::Connect: LOOP detected\n");
        manager->IncFeedbackConnection(port_src, port_dst);
    } else {
        manager->IncDirectConnection(port_src, port_dst);
    }

end:
    WriteNextStateStop();
    return res;
}

// Server
int JackGraphManager::Disconnect(jack_port_id_t port_src, jack_port_id_t port_dst)
{
    JackConnectionManager* manager = WriteNextStateStart();
    JackLog("JackGraphManager::Disconnect port_src = %ld port_dst = %ld\n", port_src, port_dst);
    bool in_use_src = GetPort(port_src)->fInUse;
    bool in_use_dst = GetPort(port_dst)->fInUse;
    int res = 0;

    if (!in_use_src || !in_use_dst) {
        if (!in_use_src)
            jack_error("JackGraphManager::Disconnect: port_src = %ld not used name = %s", port_src, GetPort(port_src)->fName);
        if (!in_use_dst)
            jack_error("JackGraphManager::Disconnect: port_src = %ld not used name = %s", port_dst, GetPort(port_dst)->fName);
        res = -1;
        goto end;
    }
    if (!manager->IsConnected(port_src, port_dst)) {
        jack_error("JackGraphManager::Disconnect not connected port_src = %ld port_dst = %ld", port_src, port_dst);
        res = -1;
        goto end;
    }

    manager->Disconnect(port_src, port_dst);
    if (res < 0) {
        jack_error("JackGraphManager::Disconnect failed port_src = %ld port_dst = %ld", port_src, port_dst);
        goto end;
    }
    manager->Disconnect(port_dst, port_src);
    if (res < 0) {
        jack_error("JackGraphManager::Disconnect failed port_dst = %ld port_src = %ld", port_dst, port_src);
        goto end;
    }

    if (manager->IsFeedbackConnection(port_src, port_dst)) {
        JackLog("JackGraphManager::Disconnect: FEEDBACK removed\n");
        manager->DecFeedbackConnection(port_src, port_dst);
    } else {
        manager->DecDirectConnection(port_src, port_dst);
    }

end:
    WriteNextStateStop();
    return res;
}

// Client
int JackGraphManager::IsConnected(jack_port_id_t port_src, jack_port_id_t port_dst)
{
    JackConnectionManager* manager = ReadCurrentState();
    return manager->IsConnected(port_src, port_dst);
}

// Server
int JackGraphManager::CheckPorts(jack_port_id_t port_src, jack_port_id_t port_dst)
{
    JackPort* src = GetPort(port_src);
    JackPort* dst = GetPort(port_dst);

    if ((dst->fFlags & JackPortIsInput) == 0) {
        jack_error("Destination port in attempted (dis)connection of %s and %s is not an input port", src->fName, dst->fName);
        return -1;
    }

    if ((src->fFlags & JackPortIsOutput) == 0) {
        jack_error("Source port in attempted (dis)connection of %s and %s is not an output port", src->fName, dst->fName);
        return -1;
    }

    return 0;
}

int JackGraphManager::CheckPorts(const char* src_name, const char* dst_name, jack_port_id_t* port_src, jack_port_id_t* port_dst)
{
    JackLog("JackGraphManager::CheckConnect src_name = %s dst_name = %s\n", src_name, dst_name);

    if ((*port_src = GetPort(src_name)) == NO_PORT) {
        jack_error("Unknown source port in attempted (dis)connection src_name [%s] dst_name [%s]", src_name, dst_name);
        return -1;
    }

    if ((*port_dst = GetPort(dst_name)) == NO_PORT) {
        jack_error("Unknown destination port in attempted (dis)connection src_name [%s] dst_name [%s]", src_name, dst_name);
        return -1;
    }

    return CheckPorts(*port_src, *port_dst);
}

// Client : port array
jack_port_id_t JackGraphManager::GetPort(const char* name)
{
    for (int i = 0; i < PORT_NUM; i++) {
        JackPort* port = GetPort(i);
        if (port->IsUsed() && port->NameEquals(name))
            return i;
    }
    return NO_PORT;
}

/*!
\brief Get the connection port name array.
*/

// Client
void JackGraphManager::GetConnectionsAux(JackConnectionManager* manager, const char** res, jack_port_id_t port_index)
{
    const jack_int_t* connections = manager->GetConnections(port_index);
    jack_int_t index;
    int i;

    for (i = 0; (i < CONNECTION_NUM) && ((index = connections[i]) != EMPTY); i++) {
        JackPort* port = GetPort(index);
        res[i] = port->fName;
    }

    res[i] = NULL;
}

/*
	Use the state returned by ReadCurrentState and check that the state was not changed during the read operation.
	The operation is lock-free since there is no intermediate state in the write operation that could cause the
	read to loop forever.
*/

// Client
const char** JackGraphManager::GetConnections(jack_port_id_t port_index)
{
    const char** res = (const char**)malloc(sizeof(char*) * (CONNECTION_NUM + 1));
    UInt16 cur_index;
    UInt16 next_index;
    AssertPort(port_index);

    do {
        cur_index = GetCurrentIndex();
        GetConnectionsAux(ReadCurrentState(), res, port_index);
        next_index = GetCurrentIndex();
    } while (cur_index != next_index); // Until a coherent state has been read

    return res;
}

// Client
const char** JackGraphManager::GetPortsAux(const char* port_name_pattern, const char* type_name_pattern, unsigned long flags)
{
    unsigned long match_cnt = 0;
    regex_t port_regex;
    regex_t type_regex;
    bool matching;

    if (port_name_pattern && port_name_pattern[0]) {
        regcomp(&port_regex, port_name_pattern, REG_EXTENDED | REG_NOSUB);
    }
    if (type_name_pattern && type_name_pattern[0]) {
        regcomp(&type_regex, type_name_pattern, REG_EXTENDED | REG_NOSUB);
    }

    const char** matching_ports = (const char**)malloc(sizeof(char*) * PORT_NUM);

    for (int i = 0; i < PORT_NUM; i++) {
        matching = true;
        JackPort* port = GetPort(i);

        if (port->IsUsed()) {

            if (flags) {
                if ((port->fFlags & flags) != flags) {
                    matching = false;
                }
            }

            if (matching && port_name_pattern && port_name_pattern[0]) {
                if (regexec(&port_regex, port->GetName(), 0, NULL, 0)) {
                    matching = false;
                }
            }
            if (matching && type_name_pattern && type_name_pattern[0]) {
                if (regexec(&type_regex, port->GetType(), 0, NULL, 0)) {
                    matching = false;
                }
            }

            if (matching) {
                matching_ports[match_cnt++] = port->fName;
            }
        }
    }

    matching_ports[match_cnt] = 0;

    if (match_cnt == 0) {
        free(matching_ports);
        matching_ports = NULL;
    }

    if (port_name_pattern && port_name_pattern[0]) {
        regfree(&port_regex);
    }
    if (type_name_pattern && type_name_pattern[0]) {
        regfree(&type_regex);
    }

    return matching_ports;
}

// Client
/*
	Check that the state was not changed during the read operation.
	The operation is lock-free since there is no intermediate state in the write operation that could cause the
	read to loop forever.
*/
const char** JackGraphManager::GetPorts(const char* port_name_pattern, const char* type_name_pattern, unsigned long flags)
{
    const char** matching_ports = NULL;
    UInt16 cur_index;
    UInt16 next_index;

    do {
        cur_index = GetCurrentIndex();
        if (matching_ports) {
            free(matching_ports);
            JackLog("JackGraphManager::GetPorts retry... \n");
        }
        matching_ports = GetPortsAux(port_name_pattern, type_name_pattern, flags);
        next_index = GetCurrentIndex();
    } while (cur_index != next_index); // Until a coherent state has been read

    return matching_ports;
}

// Server
void JackGraphManager::Save(JackConnectionManager* dst)
{
    JackConnectionManager* manager = WriteNextStateStart();
    memcpy(dst, manager, sizeof(JackConnectionManager));
    WriteNextStateStop();
}

// Server
void JackGraphManager::Restore(JackConnectionManager* src)
{
    JackConnectionManager* manager = WriteNextStateStart();
    memcpy(manager, src, sizeof(JackConnectionManager));
    WriteNextStateStop();
}

} // end of namespace


