/*
Copyright (C) 2004-2008 Grame
Copyright (C) 2016-2024 Filipe Coelho

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

#ifndef __JackMachFutex__
#define __JackMachFutex__

#include "JackSynchro.h"
#include "JackCompilerDeps.h"
#include <stddef.h>

namespace Jack
{

/*!
\brief Inter process synchronization using macOS futexes (through an internal macOS API).

 Heavily based on the JackLinuxFutex class..
*/

class SERVER_EXPORT JackMachFutex : public detail::JackSynchro
{

    private:
        struct FutexData {
            int futex;         // futex, needs to be 1st member
            bool internal;     // current internal state
            bool wasInternal;  // initial internal state, only changes in allocate
            bool needsChange;  // change state on next wait call
            int externalCount; // how many external clients have connected
        };

        int fSharedMem;
        FutexData* fFutex;
        bool fPrivate;
        bool fPromiscuous;
        int fPromiscuousGid;

    protected:

        void BuildName(const char* name, const char* server_name, char* res, int size);

    public:

        JackMachFutex();

        bool Signal();
        bool SignalAll();
        bool Wait();
        bool TimedWait(long usec);

        bool Allocate(const char* name, const char* server_name, int value, bool internal = false);
        bool Connect(const char* name, const char* server_name);
        bool ConnectInput(const char* name, const char* server_name);
        bool ConnectOutput(const char* name, const char* server_name);
        bool Disconnect();
        void Destroy();

        void MakePrivate(bool priv);
};

} // end of namespace


#endif

