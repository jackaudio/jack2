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

#include "JackMachSemaphore.h"
#include "JackMachUtils.h"
#include "JackConstants.h"
#include "JackTools.h"
#include "JackError.h"

#include <mach/message.h>

#define jack_mach_error(kern_result, message) \
        jack_mach_error_uncurried("JackMachSemaphore", kern_result, message)

#define jack_mach_bootstrap_err(kern_result, message, name) \
        jack_mach_bootstrap_err_uncurried("JackMachSemaphore", kern_result, message, name)

namespace Jack
{

void JackMachSemaphore::BuildName(const char* client_name, const char* server_name, char* res, int size)
{
    char ext_client_name[SYNC_MAX_NAME_SIZE + 1];
    JackTools::RewriteName(client_name, ext_client_name);

    // make the name as small as possible, as macos has issues with long semaphore names
    if (strcmp(server_name, "default") == 0)
        server_name = "";

    snprintf(res, std::min(size, 32), "js%d.%s%s", JackTools::GetUID(), server_name, ext_client_name);
}

bool JackMachSemaphore::Signal()
{
    if (fSemaphore == MACH_PORT_NULL) {
        jack_error("JackMachSemaphore::Signal name = %s already deallocated!!", fName);
        return false;
    }

    if (fFlush) {
        return true;
    }

    kern_return_t res;
    if ((res = semaphore_signal(fSemaphore)) != KERN_SUCCESS) {
        jack_error("JackMachSemaphore::Signal name = %s err = %s", fName, mach_error_string(res));
    }
    return (res == KERN_SUCCESS);
}

bool JackMachSemaphore::SignalAll()
{
    if (fSemaphore == MACH_PORT_NULL) {
        jack_error("JackMachSemaphore::SignalAll name = %s already deallocated!!", fName);
        return false;
    }

    if (fFlush) {
        return true;
    }

    kern_return_t res;
    // When signaled several times, do not accumulate signals...
    if ((res = semaphore_signal_all(fSemaphore)) != KERN_SUCCESS) {
        jack_error("JackMachSemaphore::SignalAll name = %s err = %s", fName, mach_error_string(res));
    }
    return (res == KERN_SUCCESS);
}

bool JackMachSemaphore::Wait()
{
    if (fSemaphore == MACH_PORT_NULL) {
        jack_error("JackMachSemaphore::Wait name = %s already deallocated!!", fName);
        return false;
    }

    kern_return_t res;
    if ((res = semaphore_wait(fSemaphore)) != KERN_SUCCESS) {
        jack_error("JackMachSemaphore::Wait name = %s err = %s", fName, mach_error_string(res));
    }
    return (res == KERN_SUCCESS);
}

bool JackMachSemaphore::TimedWait(long usec)
{
    if (fSemaphore == MACH_PORT_NULL) {
        jack_error("JackMachSemaphore::TimedWait name = %s already deallocated!!", fName);
        return false;
    }

    kern_return_t res;
    mach_timespec time;
    time.tv_sec = usec / 1000000;
    time.tv_nsec = (usec % 1000000) * 1000;

    if ((res = semaphore_timedwait(fSemaphore, time)) != KERN_SUCCESS) {
        jack_error("JackMachSemaphore::TimedWait name = %s usec = %ld err = %s", fName, usec, mach_error_string(res));
    }
    return (res == KERN_SUCCESS);
}

/*! \brief Server side: create semaphore and publish IPC primitives to make it accessible.
 *
 * This method;
 * - Allocates a mach semaphore
 * - Allocates a new mach IPC port and obtains a send right for it
 * - Publishes IPC port send right to the bootstrap server
 * - Starts a new JackMachSemaphoreServer thread, which listens for messages on the IPC port and
 * replies with a send right to the mach semaphore.
 *
 * \returns false if any of the above steps fails, or true otherwise.
 */
bool JackMachSemaphore::Allocate(const char* client_name, const char* server_name, int value)
{
    if (fSemaphore != MACH_PORT_NULL) {
        jack_error("JackMachSemaphore::Allocate: Semaphore already allocated; called twice? [%s]", fName);
        return false;
    }

    BuildName(client_name, server_name, fName, sizeof(fName));

    mach_port_t task = mach_task_self();
    kern_return_t res;

    if (fBootPort == MACH_PORT_NULL) {
        if ((res = task_get_bootstrap_port(task, &fBootPort)) != KERN_SUCCESS) {
            jack_mach_error(res, "can't find bootstrap mach port");
            return false;
        }
    }

    if ((res = semaphore_create(task, &fSemaphore, SYNC_POLICY_FIFO, value)) != KERN_SUCCESS) {
        jack_mach_error(res, "failed to create semaphore");
        return false;
    }

    if ((res = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &fServicePort)) != KERN_SUCCESS) {
        jack_mach_error(res, "failed to allocate IPC port");

        // Cleanup created semaphore
        this->Destroy();

        return false;
    }

    if ((res = mach_port_insert_right(mach_task_self(), fServicePort, fServicePort, MACH_MSG_TYPE_MAKE_SEND)) != KERN_SUCCESS) {
        jack_mach_error(res, "failed to obtain send right for IPC port");

        // Cleanup created semaphore & mach port
        this->Destroy();

        return false;
    }

    if ((res = bootstrap_register(fBootPort, fName, fServicePort)) != KERN_SUCCESS) {
        jack_mach_bootstrap_err(res, "can't register IPC port with bootstrap server", fName);

        // Cleanup created semaphore & mach port
        this->Destroy();

        return false;
    }

    fSemServer = new JackMachSemaphoreServer(fSemaphore, fServicePort, fName);
    fThreadSemServer = new JackMachThread(fSemServer);

    if (fThreadSemServer->Start() < 0) {
        jack_error("JackMachSemaphore::Allocate: failed to start semaphore IPC server thread [%s]", fName);

        // Cleanup created semaphore, mach port (incl. service registration), and server
        this->Destroy();

        return false;
    }

    jack_log("JackMachSemaphore::Allocate: OK, name = %s", fName);
    return true;
}

/*! \brief Client side: Obtain semaphore from server via published IPC port.
 *
 * This method;
 * - Looks up the service port for the jackd semaphore server for this client by name
 * - Sends a message to that server asking for a semaphore port send right
 * - Receives a semaphore send right in return and stores it locally
 *
 * \returns False if any of the above steps fails, or true otherwise.
 */
bool JackMachSemaphore::ConnectInput(const char* client_name, const char* server_name)
{
    BuildName(client_name, server_name, fName, sizeof(fName));

    mach_port_t task = mach_task_self();
    kern_return_t res;

    if (fSemaphore != MACH_PORT_NULL) {
        jack_log("JackMachSemaphore::Connect: Already connected name = %s", fName);
        return true;
    }

    if (fBootPort == MACH_PORT_NULL) {
        if ((res = task_get_bootstrap_port(task, &fBootPort)) != KERN_SUCCESS) {
            jack_mach_error(res, "can't find bootstrap port");
            return false;
        }
    }

    if ((res = bootstrap_look_up(fBootPort, fName, &fServicePort)) != KERN_SUCCESS) {
        jack_mach_bootstrap_err(res, "can't find IPC service port to request semaphore", fName);
        return false;
    }

    mach_port_t semaphore_req_port;

    if ((res = mach_port_allocate(task, MACH_PORT_RIGHT_RECEIVE, &semaphore_req_port)) != KERN_SUCCESS) {
        jack_mach_error(res, "failed to allocate request port");

        if ((res = mach_port_deallocate(task, fServicePort)) != KERN_SUCCESS) {
            jack_mach_error(res, "failed to deallocate IPC service port during cleanup");
        } else {
            fServicePort = MACH_PORT_NULL;
        }

        return false;
    }

    // Prepare a message buffer on the stack. We'll use it for both sending and receiving a message.
    struct {
        mach_msg_header_t hdr;
        mach_msg_trailer_t trailer;
    } msg;

    /*
     * Configure the message to consume the destination port we give it (_MOVE_SEND), and to
     * transmute the local port receive right we give it into a send_once right at the destination.
     * The server will use that send_once right to reply to us.
     */
    msg.hdr.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_MOVE_SEND, MACH_MSG_TYPE_MAKE_SEND_ONCE);
    msg.hdr.msgh_local_port = semaphore_req_port;
    msg.hdr.msgh_remote_port = fServicePort;

    mach_msg_return_t send_err = mach_msg(
        &msg.hdr,
        MACH_SEND_MSG,
        sizeof(msg.hdr), // no trailer on send
        0,
        MACH_PORT_NULL,
        MACH_MSG_TIMEOUT_NONE,
        MACH_PORT_NULL);

    if (send_err != MACH_MSG_SUCCESS) {
        jack_mach_error(send_err, "failed to send semaphore port request IPC");

        if ((res = mach_port_deallocate(task, fServicePort)) != KERN_SUCCESS) {
            jack_mach_error(res, "failed to deallocate IPC service port during cleanup");
        } else {
            fServicePort = MACH_PORT_NULL;
        }

        if ((res = mach_port_destroy(task, semaphore_req_port)) != KERN_SUCCESS) {
            jack_mach_error(res, "failed to destroy IPC request port during cleanup");
        }

        return false;
    } else {
        fServicePort = MACH_PORT_NULL; // We moved it into the message and away to the destination
    }

    mach_msg_return_t recv_err = mach_msg(
        &msg.hdr,
        MACH_RCV_MSG,
        0,
        sizeof(msg),
        semaphore_req_port,
        MACH_MSG_TIMEOUT_NONE,
        MACH_PORT_NULL
    );

    /* Don't leak ports: irrespective of if we succeeded to read or not, destroy the port we created
     * to send/receive the request as we have no further use for it either way. */
    if ((res = mach_port_destroy(task, semaphore_req_port)) != KERN_SUCCESS) {
        jack_mach_error(res, "failed to destroy semaphore_req_port");
        // This isn't good, but doesn't actually stop the semaphore from working... don't bail
    }

    if (recv_err != MACH_MSG_SUCCESS) {
        jack_mach_error(recv_err, "failed to receive semaphore port");
        return false;
    } else {
        fSemaphore = msg.hdr.msgh_remote_port;

        jack_log("JackMachSemaphore::Connect: OK, name = %s ", fName);
        return true;
    }
}

bool JackMachSemaphore::Connect(const char* name, const char* server_name)
{
    return ConnectInput(name, server_name);
}

bool JackMachSemaphore::ConnectOutput(const char* name, const char* server_name)
{
    return ConnectInput(name, server_name);
}

bool JackMachSemaphore::Disconnect()
{
    if (fSemaphore == MACH_PORT_NULL) {
        return true;
    }

    mach_port_t task = mach_task_self();
    kern_return_t res;

    jack_log("JackMachSemaphore::Disconnect name = %s", fName);

    if (fServicePort != MACH_PORT_NULL) {
        // If we're still holding onto a service port send right for some reason, deallocate it
        if ((res = mach_port_deallocate(task, fServicePort)) != KERN_SUCCESS) {
            jack_mach_error(res, "failed to deallocate stray service port");
            // Continue cleanup even if this fails; don't bail
        } else {
            fServicePort = MACH_PORT_NULL;
        }
    }

    if ((res = mach_port_deallocate(task, fSemaphore)) != KERN_SUCCESS) {
        jack_mach_error(res, "failed to deallocate semaphore port");
        return false;
    } else {
        fSemaphore = MACH_PORT_NULL;
        return true;
    }
}

// Server side : destroy the JackGlobals
void JackMachSemaphore::Destroy()
{
    kern_return_t res;
    mach_port_t task = mach_task_self();

    if (fSemaphore == MACH_PORT_NULL) {
        jack_error("JackMachSemaphore::Destroy semaphore is MACH_PORT_NULL; already destroyed?");
        return;
    }

    if (fThreadSemServer) {
        if (fThreadSemServer->Kill() < 0) {
            jack_error("JackMachSemaphore::Destroy failed to kill semaphore server thread...");
            // Oh dear. How sad. Never mind.
        }

        JackMachThread* thread = fThreadSemServer;
        fThreadSemServer = NULL;
        delete thread;
    }

    if (fSemServer) {
        JackMachSemaphoreServer* server = fSemServer;
        fSemServer = NULL;
        delete server;
    }

    if ((res = mach_port_destroy(task, fServicePort)) != KERN_SUCCESS) {
        jack_mach_error(res, "failed to destroy IPC port");
    } else {
        fServicePort = MACH_PORT_NULL;
    }

    if ((res = semaphore_destroy(mach_task_self(), fSemaphore)) != KERN_SUCCESS) {
        jack_mach_error(res, "failed to destroy semaphore");
    } else {
        fSemaphore = MACH_PORT_NULL;
    }

    jack_log("JackMachSemaphore::Destroy: OK, name = %s", fName);
}

} // end of namespace

