/*
Copyright (C) 2001 Paul Davis
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

#ifndef __JackTransportEngine__
#define __JackTransportEngine__

#include "JackAtomicArrayState.h"
#include "JackCompilerDeps.h"
#include "types.h"

namespace Jack
{

typedef enum {
    TransportCommandNone = 0,
    TransportCommandStart = 1,
    TransportCommandStop = 2,
} transport_command_t;

/*!
\brief The client transport structure.

We have:

	- a "current" position
	- a "pending" position prepared by the server at each cycle
	- a "request" position wanted by a client

	At the beginning of a cycle the server needs to select a new current position. When a request and a pending position are available,
	the request takes precedence on the pending one. The server atomically switches to the new position.
	The current position can be read by clients.

	We use a JackAtomicArrayState pattern that allows to manage several "next" states independantly.

    In jack1 implementation, transport code (jack_transport_cycle_end) was not called if the graph could not be locked (see jack_run_one_cycle).
    Here transport cycle (CycleBegin, CycleEnd) has to run in the RT thread concurrently with code executed from the "command" thread.

    Each client maintains a state in it's shared memory area defined by:

    - it's current transport state
    - a boolean that is "true" when slow-sync cb has to be called
    - a boolean that is "true" when timebase cb is called with new_pos on

    Several operations set the "slow-sync cb" flag to true:

        - setting a new cb (client)
        - activate (client)
        - transport start (server)
        - new pos (server)

    Slow-sync cb calls stops when:

        - the cb return true (client)
        - desactivate (client)
        - transport stop (server)

    Several operations set the "timebase cb" flag to true:

        - setting a new cb (client)
        - activate (client)
        - transport start (server) ??
        - new pos (server)

    Timebase cb "new_pos" argument calls stops when:

        - after one cb call with "new_pos" argument true (client)
        - desactivate (client)
        - release (client)
        - transport stop (server)

*/

class JackClientInterface;

PRE_PACKED_STRUCTURE
class SERVER_EXPORT JackTransportEngine : public JackAtomicArrayState<jack_position_t>
{

    private:

        jack_transport_state_t fTransportState;
        volatile transport_command_t fTransportCmd;
        transport_command_t fPreviousCmd;		/* previous transport_cmd */
        jack_time_t fSyncTimeout;
        int fSyncTimeLeft;
        int fTimeBaseMaster;
        bool fPendingPos;
        bool fNetworkSync;
        bool fConditionnal;
        SInt32 fWriteCounter;

        bool CheckAllRolling(JackClientInterface** table);
        void MakeAllStartingLocating(JackClientInterface** table);
        void MakeAllStopping(JackClientInterface** table);
        void MakeAllLocating(JackClientInterface** table);

        void SyncTimeout(jack_nframes_t frame_rate, jack_nframes_t buffer_size);

    public:

        JackTransportEngine();

        ~JackTransportEngine()
        {}

        void SetCommand(transport_command_t state)
        {
            fTransportCmd = state;
        }

        jack_transport_state_t GetState() const
        {
            return fTransportState;
        }

        void SetState(jack_transport_state_t state)
        {
            fTransportState = state;
        }

        /*
        	\brief
        */
        int ResetTimebase(int refnum);

        /*
        	\brief
        */
        int SetTimebaseMaster(int refnum, bool conditionnal);

        void GetTimebaseMaster(int& refnum, bool& conditionnal)
        {
            refnum = fTimeBaseMaster;
            conditionnal = fConditionnal;
        }

        /*
        	\brief
        */
        void CycleBegin(jack_nframes_t frame_rate, jack_time_t time);

        /*
        	\brief
        */
        void CycleEnd(JackClientInterface** table, jack_nframes_t frame_rate, jack_nframes_t buffer_size);

        /*
        	\brief
        */
        void SetSyncTimeout(jack_time_t timeout)
        {
            fSyncTimeout = timeout;
        }

        void ReadCurrentPos(jack_position_t* pos);

        jack_unique_t GenerateUniqueID()
        {
            return (jack_unique_t)INC_ATOMIC(&fWriteCounter);
        }

        void RequestNewPos(jack_position_t* pos);

        jack_transport_state_t Query(jack_position_t* pos);

        jack_nframes_t GetCurrentFrame();

        static void CopyPosition(jack_position_t* from, jack_position_t* to);

        bool GetNetworkSync() const
        {
            return fNetworkSync;
        }

        void SetNetworkSync(bool sync)
        {
            fNetworkSync = sync;
        }

} POST_PACKED_STRUCTURE;

} // end of namespace

#endif
