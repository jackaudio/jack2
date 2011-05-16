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

#include "JackDummyDriver.h"
#include "JackEngineControl.h"
#include "JackGraphManager.h"
#include "JackDriverLoader.h"
#include "JackThreadedDriver.h"
#include "JackCompilerDeps.h"
#include <iostream>
#include <unistd.h>

namespace Jack
{

int JackDummyDriver::Open(jack_nframes_t buffer_size,
                          jack_nframes_t samplerate,
                          bool capturing,
                          bool playing,
                          int inchannels,
                          int outchannels,
                          bool monitor,
                          const char* capture_driver_name,
                          const char* playback_driver_name,
                          jack_nframes_t capture_latency,
                          jack_nframes_t playback_latency)
{
    if (JackAudioDriver::Open(buffer_size,
                            samplerate,
                            capturing,
                            playing,
                            inchannels,
                            outchannels,
                            monitor,
                            capture_driver_name,
                            playback_driver_name,
                            capture_latency,
                            playback_latency) == 0) {
        int buffer_size = int((fWaitTime * fEngineControl->fSampleRate) / 1000000.0f);
        if (buffer_size > BUFFER_SIZE_MAX) {
            buffer_size = BUFFER_SIZE_MAX;
            jack_error("Buffer size set to %d ", BUFFER_SIZE_MAX);
        }
        SetBufferSize(buffer_size);
        return 0;
    } else {
        return -1;
    }
}

int JackDummyDriver::Process()
{
    JackDriver::CycleTakeBeginTime();
    JackAudioDriver::Process();
    JackSleep(std::max(0L, long(fWaitTime - (GetMicroSeconds() - fBeginDateUst))));
    return 0;
}

int JackDummyDriver::SetBufferSize(jack_nframes_t buffer_size)
{
    // Generic change, never fails
    JackAudioDriver::SetBufferSize(buffer_size);
    fWaitTime = (unsigned long)((((float)buffer_size) / ((float)fEngineControl->fSampleRate)) * 1000000.0f);
    return 0;
}

} // end of namespace

#ifdef __cplusplus
extern "C"
{
#endif

    SERVER_EXPORT jack_driver_desc_t * driver_get_descriptor () {
        jack_driver_desc_t * desc;
        jack_driver_desc_filler_t filler;
        jack_driver_param_value_t value;

        desc = jack_driver_descriptor_construct("dummy", "Timer based backend", &filler);

        value.ui = 2U;
        jack_driver_descriptor_add_parameter(desc, &filler, "capture", 'C', JackDriverParamUInt, &value, NULL, "Number of capture ports", NULL);
        jack_driver_descriptor_add_parameter(desc, &filler, "playback", 'P', JackDriverParamUInt, &value, NULL, "Number of playback ports", NULL);

        value.ui = 48000U;
        jack_driver_descriptor_add_parameter(desc, &filler, "rate", 'r', JackDriverParamUInt, &value, NULL, "Sample rate", NULL);

        value.i = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "monitor", 'm', JackDriverParamBool, &value, NULL, "Provide monitor ports for the output", NULL);

        value.ui = 1024U;
        jack_driver_descriptor_add_parameter(desc, &filler, "period", 'p', JackDriverParamUInt, &value, NULL, "Frames per period", NULL);

        value.ui = 21333U;
        jack_driver_descriptor_add_parameter(desc, &filler, "wait", 'w', JackDriverParamUInt, &value, NULL, "Number of usecs to wait between engine processes", NULL);

        return desc;
    }

    SERVER_EXPORT Jack::JackDriverClientInterface* driver_initialize(Jack::JackLockedEngine* engine, Jack::JackSynchro* table, const JSList* params) {
        jack_nframes_t sample_rate = 48000;
        jack_nframes_t period_size = 1024;
        unsigned int capture_ports = 2;
        unsigned int playback_ports = 2;
        unsigned long wait_time = 0;
        const JSList * node;
        const jack_driver_param_t * param;
        bool monitor = false;

        for (node = params; node; node = jack_slist_next (node)) {
            param = (const jack_driver_param_t *) node->data;

            switch (param->character) {

                case 'C':
                    capture_ports = param->value.ui;
                    break;

                case 'P':
                    playback_ports = param->value.ui;
                    break;

                case 'r':
                    sample_rate = param->value.ui;
                    break;

                case 'p':
                    period_size = param->value.ui;
                    break;

                case 'w':
                    wait_time = param->value.ui;
                    break;

                case 'm':
                    monitor = param->value.i;
                    break;
            }
        }

        if (wait_time == 0) // Not set
            wait_time = (unsigned long)((((float)period_size) / ((float)sample_rate)) * 1000000.0f);

        Jack::JackDriverClientInterface* driver = new Jack::JackThreadedDriver(new Jack::JackDummyDriver("system", "dummy_pcm", engine, table, wait_time));
        if (driver->Open(period_size, sample_rate, 1, 1, capture_ports, playback_ports, monitor, "dummy", "dummy", 0, 0) == 0) {
            return driver;
        } else {
            delete driver;
            return NULL;
        }
    }

#ifdef __cplusplus
}
#endif
