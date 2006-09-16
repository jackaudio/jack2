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

#include "JackTransportEngine.h"
#include "JackClientControl.h"
#include "JackError.h"
#include "JackTime.h" 
#include <assert.h>
#include <stdlib.h>

using namespace std;

namespace Jack
{

JackTransportEngine::JackTransportEngine(): JackAtomicArrayState<jack_position_t>()
{
    fTransportState = JackTransportStopped;
    fTransportCmd = fPreviousCmd = TransportCommandStop;
    fSyncTimeout = 2000000;	/* 2 second default */
    fSyncTimeLeft = 0;
    fTimeBaseMaster = -1;
    fWriteCounter = 0;
    fPendingPos = false;
}

// compute the number of cycle for timeout
void JackTransportEngine::SyncTimeout(jack_nframes_t frame_rate, jack_nframes_t buffer_size)
{
    long buf_usecs = (long)((buffer_size * (jack_time_t) 1000000) / frame_rate);
    fSyncTimeLeft = fSyncTimeout / buf_usecs;
    JackLog("SyncTimeout fSyncTimeout = %ld fSyncTimeLeft = %ld\n", (long)fSyncTimeout, (long)fSyncTimeLeft);
}

int JackTransportEngine::ResetTimebase(int refnum)
{
    if (fTimeBaseMaster == refnum) {
        jack_position_t* request = WriteNextStateStart(2); // To check
        request->valid = (jack_position_bits_t)0;
        WriteNextStateStop(2);
        fTimeBaseMaster = -1;
        return 0;
    } else {
        return EINVAL;
    }
}

int JackTransportEngine::SetTimebase(int refnum, bool conditionnal)
{
    if (conditionnal && fTimeBaseMaster > 0) {
        if (refnum != fTimeBaseMaster) {
            JackLog("conditional timebase for ref = %ld failed: %ld is already the master\n", refnum, fTimeBaseMaster);
            return EBUSY;
        } else {
            JackLog("ref = %ld was already timebase master\n", refnum);
            return 0;
        }
    } else {
        fTimeBaseMaster = refnum;
        JackLog("new timebase master: ref = %ld\n", refnum);
        return 0;
    }
}

bool JackTransportEngine::CheckOneSynching(JackClientInterface** table)
{
    for (int i = REAL_REFNUM; i < CLIENT_NUM; i++) {
        JackClientInterface* client = table[i];
        if (client && client->GetClientControl()->fTransportState == JackTransportSynching) {
            JackLog("CheckOneSynching\n");
            return true;
        }
    }
    return false;
}

bool JackTransportEngine::CheckAllRolling(JackClientInterface** table)
{
    for (int i = REAL_REFNUM; i < CLIENT_NUM; i++) {
        JackClientInterface* client = table[i];
        if (client && client->GetClientControl()->fTransportState != JackTransportRolling) {
            JackLog("CheckAllRolling refnum = %ld is not rolling\n", i);
            return false;
        }
    }
    JackLog("CheckAllRolling\n");
    return true;
}

void JackTransportEngine::MakeAllStarting(JackClientInterface** table)
{
    for (int i = REAL_REFNUM; i < CLIENT_NUM; i++) {
        JackClientInterface* client = table[i];
        if (client) {
            // Unactive clients don't have their process function called at all, they appear as already "rolling" for the transport....
            client->GetClientControl()->fTransportState = (client->GetClientControl()->fActive) ? JackTransportStarting : JackTransportRolling;
            JackLog("MakeAllStarting refnum = %ld \n", i);
        }
    }
    JackLog("MakeAllStarting\n");
}

void JackTransportEngine::CycleBegin(jack_nframes_t frame_rate, jack_time_t time) // really needed?? (would be done in CycleEnd...)
{
    jack_position_t* pending = WriteNextStateStart(1); // Update "pending" state
    pending->usecs = time;
    pending->frame_rate = frame_rate;
    WriteNextStateStop(1);
}

void JackTransportEngine::CycleEnd(JackClientInterface** table, jack_nframes_t frame_rate, jack_nframes_t buffer_size)
{
    TrySwitchState(1);	// Switch from "pending" to "current", it always works since there is always a pending state

    /* Handle any new transport command from the last cycle. */
    transport_command_t cmd = fTransportCmd;
    if (cmd != fPreviousCmd) {
        fPreviousCmd = cmd;
        JackLog("transport command: %s\n", (cmd == TransportCommandStart ? "START" : "STOP"));
    } else {
        cmd = TransportCommandNone;
    }

    /* state transition switch */
    switch (fTransportState) {

        case JackTransportSynching:
            if (cmd == TransportCommandStart) {
                fTransportState = JackTransportStarting;
                MakeAllStarting(table);
                SyncTimeout(frame_rate, buffer_size);
                JackLog("transport locate ==> starting....\n");
            } else if (fPendingPos) {
                fTransportState = JackTransportSynching;
                JackLog("transport locate ==> locate....\n");
            } else {
                fTransportState = JackTransportStopped;
                JackLog("transport locate ==> stopped....\n");
            }
            break;

        case JackTransportStopped:
            // Set a JackTransportStarting for the current cycle, if all clients are ready (now slow_sync) ==> JackTransportRolling next state
            if (cmd == TransportCommandStart) {
                fTransportState = JackTransportStarting;
                MakeAllStarting(table);
                SyncTimeout(frame_rate, buffer_size);
                JackLog("transport stopped ==> starting....\n");
            } else if (fPendingPos || CheckOneSynching(table)) {
                fTransportState = JackTransportSynching;
                JackLog("transport stopped ==> locate....\n");
            }
            break;

        case JackTransportStarting:
            JackLog("transport starting fSyncTimeLeft %ld\n", fSyncTimeLeft);

            if (cmd == TransportCommandStop) {
                fTransportState = JackTransportStopped;
                JackLog("transport starting ==> stopped\n");
            } else if (fPendingPos) {
                fTransportState = JackTransportStarting;
                MakeAllStarting(table);
                SyncTimeout(frame_rate, buffer_size);
            } else if (--fSyncTimeLeft == 0 || CheckAllRolling(table)) {
                fTransportState = JackTransportRolling;
                JackLog("transport starting ==> rolling.... fSyncTimeLeft %ld\n", fSyncTimeLeft);
            }
            break;

        case JackTransportRolling:
            if (cmd == TransportCommandStop) {
                fTransportState = JackTransportStopped;
                JackLog("transport rolling ==> stopped\n");
            } else if (fPendingPos || CheckOneSynching(table)) {
                fTransportState = JackTransportStarting;
                MakeAllStarting(table);
                SyncTimeout(frame_rate, buffer_size);
                JackLog("transport rolling ==> starting....\n");
            }
            break;

        default:
            jack_error("Invalid JACK transport state: %d", fTransportState);
    }

    /* Update timebase, if needed. */
    if (fTransportState == JackTransportRolling) {
        jack_position_t* pending = WriteNextStateStart(1); // Update "pending" state
        pending->frame += buffer_size;
        WriteNextStateStop(1);
    }

    /* See if an asynchronous position request arrived during the last cycle. */
    jack_position_t* request = WriteNextStateStart(2, &fPendingPos);
    if (fPendingPos) {
        JackLog("New pos = %ld\n", request->frame);
        jack_position_t* pending = WriteNextStateStart(1);
        TransportCopyPosition(request, pending);
        WriteNextStateStop(1);
    }
}

void JackTransportEngine::ReadCurrentPos(jack_position_t* pos)
{
    UInt16 next_index = GetCurrentIndex();
    UInt16 cur_index;
    do {
        cur_index = next_index;
        memcpy(pos, ReadCurrentState(), sizeof(jack_position_t));
        next_index = GetCurrentIndex();
    } while (cur_index != next_index); // Until a coherent state has been read
}

void JackTransportEngine::TransportCopyPosition(jack_position_t* from, jack_position_t* to)
{
    int tries = 0;
    long timeout = 1000;

    do {
        /* throttle the busy wait if we don't get the answer
         * very quickly. See comment above about this
         * design.
         */
        if (tries > 10) {
            JackSleep(20);
            tries = 0;

            /* debug code to avoid system hangs... */
            if (--timeout == 0) {
                jack_error("hung in loop copying position B");
                abort();
            }
        }
        *to = *from;
        tries++;

    } while (to->unique_1 != to->unique_2);
}


} // end of namespace
