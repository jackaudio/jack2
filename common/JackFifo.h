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

#ifndef __JackFifo__
#define __JackFifo__


#include "JackSynchro.h"
#include <assert.h>
#include <stdio.h>
#include <poll.h>

namespace Jack
{

/*!
\brief Inter process synchronization using Fifo.
*/

class JackFifo : public JackSynchro
{

    private:

        int fFifo;
        pollfd fPoll;

        bool ConnectAux(const char* name, const char* server_name, int access);

    protected:

        void BuildName(const char* name, const char* server_name, char* res);

    public:

        JackFifo(): JackSynchro(), fFifo( -1)
        {}
        virtual ~JackFifo()
        {}

        bool Signal();
        bool SignalAll();
        bool Wait();
        bool TimedWait(long usec);

        bool Allocate(const char* name, const char* server_name, int value);
        bool Connect(const char* name, const char* server_name);
        bool ConnectInput(const char* name, const char* server_name);
        bool ConnectOutput(const char* name, const char* server_name);
        bool Disconnect();
        void Destroy();
};

} // end of namespace


#endif

