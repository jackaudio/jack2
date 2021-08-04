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

        semaphore_t fSemaphore;

        int fSharedMem;

        /*! \brief Pointer to shared memory containing semaphore send right port.
         *
         * If the semaphore has been Allocate()d (in the case of the server) or Connect()ed (in the
         * case of the client), and not yet Disconnect()ed or Destroy()ed, a pointer to a shared
         * memory segment at which a send right for a semaphore can be found. Otherwise, NULL.
         */
        mach_port_t* fSharedSemaphoreSend;

    protected:

        void BuildName(const char* name, const char* server_name, char* res, int size);

    public:

        JackMachSemaphore():JackSynchro(), fSemaphore(0), fSharedMem(0), fSharedSemaphoreSend(NULL)
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

