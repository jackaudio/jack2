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


#include "JackWinNamedPipeClientChannel.h"
#include "JackRequest.h"
#include "JackClient.h"
#include "JackGlobals.h"
#include "JackError.h"

namespace Jack
{

JackWinNamedPipeClientChannel::JackWinNamedPipeClientChannel()
    :JackGenericClientChannel(),fThread(this)
{
     fRequest = new JackWinNamedPipeClient();
}

JackWinNamedPipeClientChannel::~JackWinNamedPipeClientChannel()
{
    delete fRequest;
}

int JackWinNamedPipeClientChannel::Open(const char* server_name, const char* name, int uuid, char* name_res, JackClient* obj, jack_options_t options, jack_status_t* status)
{
    int result = 0;
    jack_log("JackWinNamedPipeClientChannel::Open name = %s", name);

    /*
    16/08/07: was called before doing "fRequest->Connect" .... still necessary?
    if (fNotificationListenPipe.Bind(jack_client_dir, name, 0) < 0) {
        jack_error("Cannot bind pipe");
        goto error;
    }
    */

    if (fRequest->Connect(jack_server_dir, server_name, 0) < 0) {
        jack_error("Cannot connect to server pipe");
        goto error;
    }

    // Check name in server
    ClientCheck(name, uuid, name_res, JACK_PROTOCOL_VERSION, (int)options, (int*)status, &result, true);
    if (result < 0) {
        int status1 = *status;
        if (status1 & JackVersionError) {
            jack_error("JACK protocol mismatch %d", JACK_PROTOCOL_VERSION);
        } else {
            jack_error("Client name = %s conflits with another running client", name);
        }
    }

    if (fNotificationListenPipe.Bind(jack_client_dir, name_res, 0) < 0) {
        jack_error("Cannot bind pipe");
        goto error;
    }

    fClient = obj;
    return 0;

error:
    fRequest->Close();
    fNotificationListenPipe.Close();
    return -1;
}

void JackWinNamedPipeClientChannel::Close()
{
    fRequest->Close();
    fNotificationListenPipe.Close();
    // Here the thread will correctly stop when the pipe are closed
    fThread.Stop();
}

int JackWinNamedPipeClientChannel::Start()
{
    jack_log("JackWinNamedPipeClientChannel::Start");
    /*
     To be sure notification thread is started before ClientOpen is called.
    */
    if (fThread.StartSync() != 0) {
        jack_error("Cannot start Jack client listener");
        return -1;
    } else {
        return 0;
    }
}

void JackWinNamedPipeClientChannel::Stop()
{
    jack_log("JackWinNamedPipeClientChannel::Stop");
    fThread.Kill();  // Unsafe on WIN32... TODO : solve WIN32 thread Kill issue
}

bool JackWinNamedPipeClientChannel::Init()
{
    jack_log("JackWinNamedPipeClientChannel::Init");
    
    // Setup context
    if (!jack_tls_set(JackGlobals::fNotificationThread, this)) {
        jack_error("Failed to set thread notification key");
    }

    if (!fNotificationListenPipe.Accept()) {
        jack_error("JackWinNamedPipeClientChannel: cannot establish notification pipe");
        return false;
    } else {
        return true;
    }
}

bool JackWinNamedPipeClientChannel::Execute()
{
    JackClientNotification event;
    JackResult res;

    if (event.Read(&fNotificationListenPipe) < 0) {
        jack_error("JackWinNamedPipeClientChannel read fail");
        goto error;
    }

    res.fResult = fClient->ClientNotify(event.fRefNum, event.fName, event.fNotify, event.fSync, event.fMessage, event.fValue1, event.fValue2);

    if (event.fSync) {
        if (res.Write(&fNotificationListenPipe) < 0) {
            jack_error("JackWinNamedPipeClientChannel write fail");
            goto error;
        }
    }
    return true;

error:
    // Close the pipes, server wont be able to create them otherwise.
    fNotificationListenPipe.Close();
    fRequest->Close();
    fClient->ShutDown();
    return false;
}

} // end of namespace


