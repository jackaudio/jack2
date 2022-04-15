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

#include "JackMachSemaphoreServer.h"
#include "JackMachUtils.h"
#include "JackConstants.h"
#include "JackTools.h"
#include "JackError.h"

#include <mach/message.h>

#define jack_mach_error(kern_result, message) \
        jack_mach_error_uncurried("JackMachSemaphoreServer", kern_result, message)

namespace Jack
{

bool JackMachSemaphoreServer::Execute() {
    jack_log("JackMachSemaphoreServer::Execute: %s", fName);

    /* Setup a message struct in our local stack frame which we can receive messages into and send
     * messages from. */
    struct {
        mach_msg_header_t hdr;
        mach_msg_trailer_t trailer;
    } msg;

    // Block until we receive a message on the fServerReceive port.
    mach_msg_return_t recv_err = mach_msg(
        &msg.hdr,
        MACH_RCV_MSG,
        0,
        sizeof(msg),
        fServerReceive,
        MACH_MSG_TIMEOUT_NONE,
        MACH_PORT_NULL
    );

    // this error is expected when deleting ports, we get notified that they somehow changed
    if (recv_err == MACH_RCV_PORT_CHANGED) {
        return fRunning;
    }

    if (recv_err != MACH_MSG_SUCCESS) {
        jack_mach_error(recv_err, "receive error");
        return fRunning; // Continue processing more connections
    }

    /* We're going to reuse the message struct that we received the message into to send a reply.
     * Setup the semaphore send port that we want to give to the client as the local port... */
    msg.hdr.msgh_local_port = fSemaphore;

    /*
     * ... to be returned by copy (_COPY_SEND), to a destination that is _SEND_ONCE that we no
     * longer require. That destination will have been set by the client as their local_port, so
     * will now already be the remote_port in the message we received (nifty, eh?).
     */
    msg.hdr.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE, MACH_MSG_TYPE_COPY_SEND);

    mach_msg_return_t send_err = mach_msg(
        &msg.hdr,
        MACH_SEND_MSG,
        sizeof(msg.hdr), // no trailer on send
        0,
        MACH_PORT_NULL,
        MACH_MSG_TIMEOUT_NONE,
        MACH_PORT_NULL);

    if (send_err != MACH_MSG_SUCCESS) {
        jack_mach_error(send_err, "send error");
    }

    return fRunning;
}

bool JackMachSemaphoreServer::Invalidate() {
    fRunning = false;

    const mach_port_t task = mach_task_self();
    kern_return_t res;

    if ((res = mach_port_destroy(task, fServerReceive)) != KERN_SUCCESS) {
        jack_mach_error(res, "failed to destroy IPC port");
    }

    if ((res = semaphore_destroy(task, fSemaphore)) != KERN_SUCCESS) {
        jack_mach_error(res, "failed to destroy semaphore");
    }

    return true;
}

} // end of namespace
