/*
Copyright (C) 2021 Peter Bridgman

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

#ifndef __JackMachSemaphoreServer__
#define __JackMachSemaphoreServer__

#include "JackCompilerDeps.h"
#include "JackMachThread.h"

#include <mach/mach.h>
#include <mach/semaphore.h>

namespace Jack
{

/*! \brief A runnable thread which listens for IPC messages and replies with a semaphore send right. */
class SERVER_EXPORT JackMachSemaphoreServer : public JackRunnableInterface
{
    private:
        /*! \brief The semaphore send right that will be dispatched to clients. */
        semaphore_t fSemaphore;

        /*! \brief The port on which we will listen for IPC messages. */
        mach_port_t fServerReceive;

        /*! \brief A pointer to a null-terminated string buffer that will be read to obtain the
         * server name for reporting purposes. Not managed at all by this type. */
        char* fName;

    public:
        JackMachSemaphoreServer(semaphore_t semaphore, mach_port_t server_recv, char* name):
            fSemaphore(semaphore), fServerReceive(server_recv), fName(name)
        {}

        bool Execute() override;
};

} // end of namespace

#endif
