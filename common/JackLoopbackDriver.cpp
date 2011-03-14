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
#include "JackDriverLoader.h"
#include "JackEngineControl.h"
#include "JackGraphManager.h"
#include "JackError.h"
#include <iostream>
#include <assert.h>

namespace Jack
{

int JackLoopbackDriver::Process()
{
    // Loopback copy
    for (int i = 0; i < fCaptureChannels; i++) {
        memcpy(GetInputBuffer(i), GetOutputBuffer(i), sizeof(jack_default_audio_sample_t) * fEngineControl->fBufferSize);
    }

    fGraphManager->ResumeRefNum(&fClientControl, fSynchroTable); // Signal all clients
    if (fEngineControl->fSyncMode) {
        if (fGraphManager->SuspendRefNum(&fClientControl, fSynchroTable, DRIVER_TIMEOUT_FACTOR * fEngineControl->fTimeOutUsecs) < 0) {
            jack_error("JackLoopbackDriver::ProcessSync SuspendRefNum error");
            return -1;
        }
    }
    return 0;
}

} // end of namespace

#ifdef __cplusplus
extern "C"
{
#endif

    SERVER_EXPORT jack_driver_desc_t * driver_get_descriptor()
    {
        jack_driver_desc_t * desc;
        unsigned int i;

        desc = (jack_driver_desc_t*)calloc (1, sizeof (jack_driver_desc_t));
        strcpy(desc->name, "loopback");              // size MUST be less then JACK_DRIVER_NAME_MAX + 1
        strcpy(desc->desc, "Loopback backend");      // size MUST be less then JACK_DRIVER_PARAM_DESC + 1

        desc->nparams = 1;
        desc->params = (jack_driver_param_desc_t*)calloc (desc->nparams, sizeof (jack_driver_param_desc_t));

        i = 0;
        strcpy(desc->params[i].name, "channels");
        desc->params[i].character = 'c';
        desc->params[i].type = JackDriverParamInt;
        desc->params[i].value.ui = 0;
        strcpy(desc->params[i].short_desc, "Maximum number of loopback ports");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        return desc;
    }

    SERVER_EXPORT Jack::JackDriverClientInterface* driver_initialize(Jack::JackLockedEngine* engine, Jack::JackSynchro* table, const JSList* params)
    {
        const JSList * node;
        const jack_driver_param_t * param;
        int channels = 2;

        for (node = params; node; node = jack_slist_next (node)) {
            param = (const jack_driver_param_t *) node->data;

            switch (param->character) {

                case 'c':
                    channels = param->value.ui;
                    break;
                }
        }

        Jack::JackDriverClientInterface* driver = new Jack::JackLoopbackDriver(engine, table);
        if (driver->Open(1, 1, channels, channels, false, "loopback", "loopback", 0, 0) == 0) {
            return driver;
        } else {
            delete driver;
            return NULL;
        }
    }

#ifdef __cplusplus
}
#endif
