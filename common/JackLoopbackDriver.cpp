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

// When used in "slave" mode

int JackLoopbackDriver::ProcessReadSync()
{
    int res = 0;

    // Loopback copy
    for (int i = 0; i < fCaptureChannels; i++) {
        memcpy(GetInputBuffer(i), GetOutputBuffer(i), sizeof(jack_default_audio_sample_t) * fEngineControl->fBufferSize);
    }

    // Resume connected clients in the graph
    if (ResumeRefNum() < 0) {
        jack_error("JackLoopbackDriver::ProcessReadSync - ResumeRefNum error");
        res = -1;
    }

    return res;
}

int JackLoopbackDriver::ProcessWriteSync()
{
    // Suspend on connected clients in the graph
    if (SuspendRefNum() < 0) {
        jack_error("JackLoopbackDriver::ProcessWriteSync - SuspendRefNum error");
        return -1;
    }
    return 0;
}

int JackLoopbackDriver::ProcessReadAsync()
{
    int res = 0;

    // Loopback copy
    for (int i = 0; i < fCaptureChannels; i++) {
        memcpy(GetInputBuffer(i), GetOutputBuffer(i), sizeof(jack_default_audio_sample_t) * fEngineControl->fBufferSize);
    }

    // Resume connected clients in the graph
    if (ResumeRefNum() < 0) {
        jack_error("JackLoopbackDriver::ProcessReadAsync - ResumeRefNum error");
        res = -1;
    }

    return res;
}

int JackLoopbackDriver::ProcessWriteAsync()
{
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
        jack_driver_desc_filler_t filler;
        jack_driver_param_value_t value;

        desc = jack_driver_descriptor_construct("loopback", JackDriverSlave, "Loopback backend", &filler);

        value.i = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "channels", 'c', JackDriverParamInt, &value, NULL, "Maximum number of loopback ports", NULL);

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
        if (driver->Open(0, 0, 1, 1, channels, channels, false, "loopback", "loopback", 0, 0) == 0) {
            return driver;
        } else {
            delete driver;
            return NULL;
        }
    }

#ifdef __cplusplus
}
#endif
