/*
Copyright (C) 2001 Paul Davis
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

#include "JackSystemDeps.h"
#include "JackLoopbackDriver.h"
#include "JackEngineControl.h"
#include "JackGraphManager.h"
#include "JackError.h"
#include <iostream>
#include <assert.h>

namespace Jack
{

int JackLoopbackDriver::Process()
{
    assert(fCaptureChannels == fPlaybackChannels);

    // Loopback copy
    for (int i = 0; i < fCaptureChannels; i++) {
        memcpy(GetInputBuffer(i), GetOutputBuffer(i), sizeof(float) * fEngineControl->fBufferSize);
    }

    fGraphManager->ResumeRefNum(&fClientControl, fSynchroTable); // Signal all clients
    if (fEngineControl->fSyncMode) {
        if (fGraphManager->SuspendRefNum(&fClientControl, fSynchroTable, fEngineControl->fTimeOutUsecs) < 0) {
            jack_error("JackLoopbackDriver::ProcessSync SuspendRefNum error");
            return -1;
        }
    }
    return 0;
}

} // end of namespace
