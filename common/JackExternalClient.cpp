/*
  Copyright (C) 2001-2003 Paul Davis
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

#include "JackExternalClient.h"
#include "JackClientControl.h"
#include "JackGlobals.h"
#include "JackChannel.h"
#include "JackError.h"

namespace Jack
{

JackExternalClient::JackExternalClient(): fClientControl(NULL)
{}

JackExternalClient::~JackExternalClient()
{}

int JackExternalClient::ClientNotify(int refnum, const char* name, int notify, int sync, const char* message, int value1, int value2)
{
    int result = -1;
    jack_log("JackExternalClient::ClientNotify ref = %ld client = %s name = %s notify = %ld", refnum, fClientControl->fName, name, notify);
    fChannel.ClientNotify(refnum, name, notify, sync, message, value1, value2, &result);
    return result;
}

int JackExternalClient::Open(const char* name, int pid, int refnum, int uuid, int* shared_client)
{
    try {

        if (fChannel.Open(name) < 0) {
            jack_error("Cannot connect to client name = %s\n", name);
            return -1;
        }

        // Use "placement new" to allocate object in shared memory
        JackShmMemAble* shared_mem = static_cast<JackShmMemAble*>(JackShmMem::operator new(sizeof(JackClientControl)));
        shared_mem->Init();
        fClientControl = new(shared_mem) JackClientControl(name, pid, refnum, uuid);

        if (!fClientControl) {
            jack_error("Cannot allocate client shared memory segment");
            return -1;
        }

        *shared_client = shared_mem->GetShmIndex();
        jack_log("JackExternalClient::Open name = %s index = %ld base = %x", name, shared_mem->GetShmIndex(), shared_mem->GetShmAddress());
        return 0;

    } catch (std::exception e) {
        return -1;
    }
}

int JackExternalClient::Close()
{
    jack_log("JackExternalClient::Close");
    fChannel.Close();
    if (fClientControl) {
        fClientControl->~JackClientControl();
        JackShmMem::operator delete(fClientControl);
    }
    return 0;
}

JackClientControl* JackExternalClient::GetClientControl() const
{
    return fClientControl;
}

} // end of namespace
