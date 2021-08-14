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

#ifndef __JackMachSemaphore__
#define __JackMachSemaphore__

#include "JackCompilerDeps.h"
#include "JackSynchro.h"

#include "JackMachThread.h"
#include "JackMachSemaphoreServer.h"

#include <mach/mach.h>
#include <servers/bootstrap.h>
#include <mach/semaphore.h>

namespace Jack
{

/*!
\brief Inter process synchronization using using Mach semaphore.
*/

class SERVER_EXPORT JackMachSemaphore : public detail::JackSynchro
{

    private:

        /*! \brief A mach send right to the mach semaphore, or MACH_PORT_NULL if not yet Allocate()d
         * (server) or Connect()ed (client). */
        semaphore_t fSemaphore;

        /*! \brief The bootstrap port for this task, or MACH_PORT_NULL if not yet obtained. */
        mach_port_t fBootPort;

        /*! \brief The IPC port used to pass the semaphore port from the server to the client, and
         * for the client to request that this occurs. MACH_PORT_NULL if not yet created (server) or
         * looked up (client). */
        mach_port_t fServicePort;

        /*! \brief On the server, if allocated, a runnable semaphore server which listens for IPC
         * messages and replies with a send right for a semaphore port. */
        JackMachSemaphoreServer* fSemServer;

        /*! \brief On the server, if allocated, a thread that runs \ref fSemServer. */
        JackMachThread* fThreadSemServer;

    protected:

        void BuildName(const char* name, const char* server_name, char* res, int size);

    public:

        JackMachSemaphore():
            JackSynchro(),
            fSemaphore(MACH_PORT_NULL),
            fBootPort(MACH_PORT_NULL),
            fServicePort(MACH_PORT_NULL),
            fSemServer(NULL),
            fThreadSemServer(NULL)
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

