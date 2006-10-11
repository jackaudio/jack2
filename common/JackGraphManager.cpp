/*
Copyright (C) 2001 Paul Davis
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
int JackGraphManager::AllocateRefNum()
{
    JackConnectionManager* manager = WriteNextStateStart();
    int res = manager->AllocateRefNum();
    WriteNextStateStop();
    return res;
}

// Server
void JackGraphManager::ReleaseRefNum(int refnum)
{
    JackConnectionManager* manager = WriteNextStateStart();
    manager->ReleaseRefNum(refnum);
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
        float* buffer = GetBuffer(port_index);
        memset(buffer, 0, buffer_size * sizeof(float)); // Clear buffer
        return buffer;
    } else if (len == 1) {	 // One connection: use zero-copy mode - just pass the buffer of the connected (output) port.
        assert(manager->GetPort(port_index, 0) != port_index); // Check recursion
        return GetBuffer(manager->GetPort(port_index, 0), buffer_size);
    } else {  // Multiple connections
        const jack_int_t* connections = manager->GetConnections(port_index);
        float* mixbuffer = GetBuffer(port_index);
        jack_port_id_t src_index;
        float* buffer;

        // Copy first buffer
        src_index = connections[0];
        AssertPort(src_index);
        buffer = (float*)GetBuffer(src_index, buffer_size);
        memcpy(mixbuffer, buffer, buffer_size * sizeof(float));

        // Mix remaining buffers
        for (int i = 1; (i < CONNECTION_NUM) && ((src_index = connections[i]) != EMPTY); i++) {
            AssertPort(src_index);
            buffer = (float*)GetBuffer(src_index, buffer_size);
            JackPort::MixBuffer(mixbuffer, buffer, buffer_size);
        }
        return mixbuffer;
    }
}

int JackGraphManager::RequestMonitor(jack_port_id_t port_index, bool onoff) // Client
{
    AssertPort(port_index);
    JackPort* port = GetPort(port_index);

    /**
    jackd.h 
        * If @ref JackPortCanMonitor is set for this @a port, turn input
        * monitoring on or off.  Otherwise, do nothing.
     
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

jack_nframes_t JackGraphManager::GetTotalLatencyAux(jack_port_id_t port_index, jack_port_id_t src_port_index, JackConnectionManager* manager, int hop_count)
{
    const jack_int_t* connections = manager->GetConnections(port_index);
    jack_nframes_t latency = GetPort(port_index)->GetLatency();
    jack_nframes_t max_latency = 0;
    jack_port_id_t dst_index;

    if (hop_count > 8)
        return latency;

    for (int i = 0; (i < CONNECTION_NUM) && ((dst_index = connections[i]) != EMPTY); i++) {
        if (src_port_index != dst_index) {
            AssertPort(dst_index);
            JackPort* dst_port = GetPort(dst_index);
            jack_nframes_t this_latency = (dst_port->fFlags & JackPortIsTerminal)
                                          ? dst_port->GetLatency()
                                          : GetTotalLatencyAux(dst_index, port_index, manager, hop_count + 1);
            max_latency = MAX(max_latency, this_latency);
        }
    }

    return max_latency + latency;
}

jack_nframes_t JackGraphManager::GetTotalLatency(jack_port_id_t port_index)
{
    UInt16 cur_index;
    UInt16 next_index;
    jack_nframes_t total_latency;
    AssertPort(port_index);
    JackLog("JackGraphManager::GetTotalLatency port_index = %ld\n", port_index);

    do {
        cur_index = GetCurrentIndex();
        total_latency = GetTotalLatencyAux(port_index, port_index, ReadCurrentState(), 0);
        next_index = GetCurrentIndex();
    } while (cur_index != next_index); // Until a coherent state has been read

    return total_latency;
}

// Server
jack_port_id_t JackGraphManager::AllocatePortAux(int refnum, const char* port_name, JackPortFlags flags)
{
    jack_port_id_t port_index;

    // Available ports start at FIRST_AVAILABLE_PORT (= 1), otherwise a port_index of 0 is "seen" as a NULL port by the external API...
    for (port_index = FIRST_AVAILABLE_PORT; port_index < PORT_NUM; port_index++) {
        JackPort* port = GetPort(port_index);
        if (!port->IsUsed()) {
            JackLog("JackGraphManager::AllocatePortAux port_index = %ld name = %s\n", port_index, port_name);
            port->Allocate(refnum, port_name, flags);
            break;
        }
    }

    return (port_index < PORT_NUM) ? port_index : NO_PORT;
}

// Server
jack_port_id_t JackGraphManager::AllocatePort(int refnum, const char* port_name, JackPortFlags flags)
{
    JackConnectionManager* manager = WriteNextStateStart();
    jack_port_id_t port_index = AllocatePortAux(refnum, port_name, flags);

    if (port_index != NO_PORT) {
        int res;
        if (flags & JackPortIsOutput) {
            res = manager->AddOutputPort(refnum, port_index);
        } else {
            res = manager->AddInputPort(refnum, port_index);
        }
        if (res < 0) {
            JackPort* port = GetPort(port_index);
            assert(port);
            port->Release();
            port_index = NO_PORT;
        }
    }

    WriteNextStateStop();
    return port_index;
}

// Server
void JackGraphManager::ReleasePort(jack_port_id_t port_index)
{
    JackPort* port = GetPort(port_index);
    port->Release();
}

// Server
int JackGraphManager::RemovePort(int refnum, jack_port_id_t port_index)
{
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

    WriteNextStateStop();
    return res;
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
        RemovePort(refnum, port_index);
        ReleasePort(port_index);
    }

    // Warning : RemovePort shift port to left, thus we always remove the first port until the "output" table is empty
    const jack_int_t* output = manager->GetOutputPorts(refnum);
    while ((port_index = output[0]) != EMPTY) {
        RemovePort(refnum, port_index);
        ReleasePort(port_index);
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
    bool in_use_src = GetPort(port_src)->fInUse;
    bool in_use_dst = GetPort(port_src)->fInUse;
    int res = 0;

    if (!in_use_src || !in_use_dst) {
        if (!in_use_src)
            jack_error("JackGraphManager::Connect: port_src not %ld used name = %s", port_src, GetPort(port_src)->fName);
        if (!in_use_dst)
            jack_error("JackGraphManager::Connect: port_dst not %ld used name = %s", port_dst, GetPort(port_dst)->fName);
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
        jack_error("JackGraphManager::Connect failed port_src = %ld port_dst = %ld", port_dst, port_src);
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
    if (res < 0)
        jack_error("JackGraphManager::Connect failed port_src = %ld port_dst = %ld", port_dst, port_src);
    return res;
}

// Server
int JackGraphManager::Disconnect(jack_port_id_t port_src, jack_port_id_t port_dst)
{
    JackConnectionManager* manager = WriteNextStateStart();
    JackLog("JackGraphManager::Disconnect port_src = %ld port_dst = %ld\n", port_src, port_dst);
    bool in_use_src = GetPort(port_src)->fInUse;
    bool in_use_dst = GetPort(port_src)->fInUse;
    int res = 0;

    if (!in_use_src || !in_use_dst) {
        if (!in_use_src)
            jack_error("JackGraphManager::Disconnect: port_src not %ld used name = %s", port_src, GetPort(port_src)->fName);
        if (!in_use_dst)
            jack_error("JackGraphManager::Disconnect: port_src not %ld used name = %s", port_dst, GetPort(port_dst)->fName);
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
        jack_error("JackGraphManager::Disconnect failed port_src = %ld port_dst = %ld", port_dst, port_src);
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
int JackGraphManager::ConnectedTo(jack_port_id_t port_src, const char* port_name)
{
    JackLog("JackGraphManager::ConnectedTo port_src = %ld port_name = %s\n", port_src, port_name);
    JackConnectionManager* manager = ReadCurrentState();
    jack_port_id_t port_dst;

    if ((port_dst = GetPort(port_name)) == NO_PORT) {
        jack_error("Unknown destination port port_name = %s", port_name);
        return 0;
    } else {
        return manager->IsConnected(port_src, port_dst);
    }
}

// Server
int JackGraphManager::CheckPort(jack_port_id_t port_index)
{
    JackPort* port = GetPort(port_index);

    if (port->fLocked) {
        jack_error("Port %s is locked against connection changes", port->fName);
        return -1;
    } else {
        return 0;
    }
}

int JackGraphManager::CheckPorts(jack_port_id_t port_src, jack_port_id_t port_dst)
{
    JackPort* src = GetPort(port_src);
    JackPort* dst = GetPort(port_dst);

    if ((dst->Flags() & JackPortIsInput) == 0) {
        jack_error("Destination port in attempted (dis)connection of %s and %s is not an input port", src->fName, dst->fName);
        return -1;
    }

    if ((src->Flags() & JackPortIsOutput) == 0) {
        jack_error("Source port in attempted (dis)connection of %s and %s is not an output port", src->fName, dst->fName);
        return -1;
    }

    if (src->fLocked) {
        jack_error("Source port %s is locked against connection changes", src->fName);
        return -1;
    }

    if (dst->fLocked) {
        jack_error("Destination port %s is locked against connection changes", dst->fName);
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
        if (port->IsUsed() && strcmp(port->fName, name) == 0)
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

    for (i = 0; (i < CONNECTION_NUM) && ((index = connections[i]) != EMPTY) ; i++) {
        JackPort* port = GetPort(index);
        res[i] = port->fName;
    }

    res[i] = NULL;
}

// Client
/*
	Use the state returned by ReadCurrentState and check that the state was not changed during the read operation.
	The operation is lock-free since there is no intermediate state in the write operation that could cause the
	read to loop forever.
*/
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
    const char** matching_ports;
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

    matching_ports = (const char**)malloc(sizeof(char*) * PORT_NUM);

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
                if (regexec(&port_regex, port->fName, 0, NULL, 0)) {
                    matching = false;
                }
            }

            /*
            if (matching && type_name_pattern && type_name_pattern[0]) {
                jack_port_type_id_t ptid = psp[i].ptype_id;
                if (regexec (&type_regex,engine->port_types[ptid].type_name,0, NULL, 0)) {
                    matching = false;
                }
            } 
            */

            // TO BE IMPROVED
            if (matching && type_name_pattern && type_name_pattern[0]) {
                if (regexec(&type_regex, JACK_DEFAULT_AUDIO_TYPE, 0, NULL, 0)) {
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


