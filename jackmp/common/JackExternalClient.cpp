/*
  Copyright (C) 2001-2003 Paul Davis
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

#include "JackExternalClient.h"
#include "JackClientControl.h"
#include "JackGlobals.h"
#include "JackChannel.h"

namespace Jack
{

JackExternalClient::JackExternalClient(): fClientControl(NULL)
{
    fChannel = JackGlobals::MakeNotifyChannel();
}

JackExternalClient::~JackExternalClient()
{
    delete fChannel;
}

int JackExternalClient::ClientNotify(int refnum, const char* name, int notify, int sync, int value)
{
    int result = -1;
    JackLog("JackExternalClient::ClientNotify ref = %ld name = %s notify = %ld\n", refnum, name, notify);
    fChannel->ClientNotify(refnum, name, notify, sync, value, &result);
    return result;
}

int JackExternalClient::Open(const char* name, int refnum, int* shared_client)
{
    try {

        if (fChannel->Open(name) < 0) {
            jack_error("Cannot connect to client name = %s port\n", name);
            return -1;
        }

        fClientControl = new JackClientControl(name, refnum);
        if (!fClientControl) {
            jack_error("Cannot allocate client shared memory segment");
            return -1;
        }

        *shared_client = fClientControl->GetShmIndex();
        JackLog("JackExternalClient::Open name = %s index = %ld base = %x\n", name, fClientControl->GetShmIndex(), fClientControl->GetShmAddress());
        return 0;

    } catch (std::exception e) {
        return -1;
    }
}

int JackExternalClient::Close()
{
    fChannel->Close();
    delete fClientControl;
    return 0;
}

JackClientControl* JackExternalClient::GetClientControl() const
{
    return fClientControl;
}

} // end of namespace
