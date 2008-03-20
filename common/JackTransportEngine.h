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

#ifndef __JackTransportEngine__
#define __JackTransportEngine__

#include "transport_types.h"
#include "JackClientInterface.h"
#include "JackConstants.h"
#include "JackAtomicArrayState.h"

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
	the resquest takes precedence on the pending one. The server atomically switches to the new position.
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

class JackTransportEngine : public JackAtomicArrayState<jack_position_t>
{

    private:

        jack_transport_state_t fTransportState;
        volatile transport_command_t fTransportCmd;
        transport_command_t fPreviousCmd;		/* previous transport_cmd */
        jack_time_t fSyncTimeout;
        int fSyncTimeLeft;
        int fTimeBaseMaster;
        bool fPendingPos;
        SInt32 fWriteCounter;

        bool CheckAllRolling(JackClientInterface** table);
        
        void MakeAllStartingLocating(JackClientInterface** table);
        void MakeAllStopping(JackClientInterface** table);
        
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

        int GetTimebaseMaster() const
        {
            return fTimeBaseMaster;
        }

        /*
        	\brief 
        */
        int ResetTimebase(int refnum);

        /*
        	\brief 
        */
        int SetTimebase(int refnum, bool conditionnal);

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

        static void TransportCopyPosition(jack_position_t* from, jack_position_t* to);

};


} // end of namespace

#endif
