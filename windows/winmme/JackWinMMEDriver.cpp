/*
Copyright (C) 2009 Grame
Copyright (C) 2011 Devin Anderson

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

#include <cmath>

#include "JackEngineControl.h"
#include "JackWinMMEDriver.h"
#include "driver_interface.h"

using Jack::JackWinMMEDriver;

JackWinMMEDriver::JackWinMMEDriver(const char *name, const char *alias,
                                   JackLockedEngine *engine,
                                   JackSynchro *table):
    JackMidiDriver(name, alias, engine, table)
{
    input_ports = 0;
    output_ports = 0;
    period = 0;
}

JackWinMMEDriver::~JackWinMMEDriver()
{}

int
JackWinMMEDriver::Attach()
{
    jack_nframes_t buffer_size = fEngineControl->fBufferSize;
    jack_port_id_t index;
    jack_nframes_t latency = buffer_size;
    jack_latency_range_t latency_range;
    const char *name;
    JackPort *port;
    latency_range.max = latency +
        ((jack_nframes_t) std::ceil((period / 1000.0) *
                                    fEngineControl->fSampleRate));
    latency_range.min = latency;

    jack_log("JackWinMMEDriver::Attach - fCaptureChannels  %d", fCaptureChannels);
    jack_log("JackWinMMEDriver::Attach - fPlaybackChannels  %d", fPlaybackChannels);

    // Inputs
    for (int i = 0; i < fCaptureChannels; i++) {
        JackWinMMEInputPort *input_port = input_ports[i];
        name = input_port->GetName();
        if (fEngine->PortRegister(fClientControl.fRefNum, name,
                                JACK_DEFAULT_MIDI_TYPE,
                                CaptureDriverFlags, buffer_size, &index) < 0) {
            jack_error("JackWinMMEDriver::Attach - cannot register input port "
                       "with name '%s'.", name);
            // X: Do we need to deallocate ports?
            return -1;
        }
        port = fGraphManager->GetPort(index);
        port->SetAlias(input_port->GetAlias());
        port->SetLatencyRange(JackCaptureLatency, &latency_range);
        fCapturePortList[i] = index;
    }

    if (! fEngineControl->fSyncMode) {
        latency += buffer_size;
        latency_range.max = latency;
        latency_range.min = latency;
    }

    // Outputs
    for (int i = 0; i < fPlaybackChannels; i++) {
        JackWinMMEOutputPort *output_port = output_ports[i];
        name = output_port->GetName();
        if (fEngine->PortRegister(fClientControl.fRefNum, name,
                                JACK_DEFAULT_MIDI_TYPE,
                                PlaybackDriverFlags, buffer_size, &index) < 0) {
            jack_error("JackWinMMEDriver::Attach - cannot register output "
                       "port with name '%s'.", name);
            // X: Do we need to deallocate ports?
            return -1;
        }
        port = fGraphManager->GetPort(index);
        port->SetAlias(output_port->GetAlias());
        port->SetLatencyRange(JackPlaybackLatency, &latency_range);
        fPlaybackPortList[i] = index;
    }

    return 0;
}

int
JackWinMMEDriver::Close()
{
    // Generic MIDI driver close
    int result = JackMidiDriver::Close();

    if (input_ports) {
        for (int i = 0; i < fCaptureChannels; i++) {
            delete input_ports[i];
        }
        delete[] input_ports;
        input_ports = 0;
    }
    if (output_ports) {
        for (int i = 0; i < fPlaybackChannels; i++) {
            delete output_ports[i];
        }
        delete[] output_ports;
        output_ports = 0;
    }
    if (period) {
        if (timeEndPeriod(period) != TIMERR_NOERROR) {
            jack_error("JackWinMMEDriver::Close - failed to unset timer "
                       "resolution.");
            result = -1;
        }
    }
    return result;
}

int
JackWinMMEDriver::Open(bool capturing, bool playing, int in_channels,
                       int out_channels, bool monitor,
                       const char* capture_driver_name,
                       const char* playback_driver_name,
                       jack_nframes_t capture_latency,
                       jack_nframes_t playback_latency)
{
    const char *client_name = fClientControl.fName;
    int input_count = 0;
    int output_count = 0;
    int num_potential_inputs = midiInGetNumDevs();
    int num_potential_outputs = midiOutGetNumDevs();

    jack_log("JackWinMMEDriver::Open - num_potential_inputs %d", num_potential_inputs);
    jack_log("JackWinMMEDriver::Open - num_potential_outputs %d", num_potential_outputs);

    period = 0;
    TIMECAPS caps;
    if (timeGetDevCaps(&caps, sizeof(TIMECAPS)) != TIMERR_NOERROR) {
        jack_error("JackWinMMEDriver::Open - could not get timer device "
                   "capabilities.  Continuing anyway ...");
    } else {
        period = caps.wPeriodMin;
        if (timeBeginPeriod(period) != TIMERR_NOERROR) {
            jack_error("JackWinMMEDriver::Open - could not set minimum timer "
                       "resolution.  Continuing anyway ...");
            period = 0;
        } else {
            jack_log("JackWinMMEDriver::Open - multimedia timer resolution "
                      "set to %d milliseconds.", period);
        }
    }

    if (num_potential_inputs) {
        try {
            input_ports = new JackWinMMEInputPort *[num_potential_inputs];
        } catch (std::exception& e) {
            jack_error("JackWinMMEDriver::Open - while creating input port "
                       "array: %s", e.what());
            goto unset_timer_resolution;
        }
        for (int i = 0; i < num_potential_inputs; i++) {
            try {
                input_ports[input_count] =
                    new JackWinMMEInputPort(fAliasName, client_name,
                                            capture_driver_name, i);
            } catch (std::exception& e) {
                jack_error("JackWinMMEDriver::Open - while creating input "
                           "port: %s", e.what());
                continue;
            }
            input_count++;
        }
    }
    if (num_potential_outputs) {
        try {
            output_ports = new JackWinMMEOutputPort *[num_potential_outputs];
        } catch (std::exception& e) {
            jack_error("JackWinMMEDriver::Open - while creating output port "
                       "array: %s", e.what());
            goto destroy_input_ports;
        }
        for (int i = 0; i < num_potential_outputs; i++) {
            try {
                output_ports[output_count] =
                    new JackWinMMEOutputPort(fAliasName, client_name,
                                             playback_driver_name, i);
            } catch (std::exception& e) {
                jack_error("JackWinMMEDriver::Open - while creating output "
                           "port: %s", e.what());
                continue;
            }
            output_count++;
        }
    }

    jack_log("JackWinMMEDriver::Open - input_count  %d", input_count);
    jack_log("JackWinMMEDriver::Open - output_count  %d", output_count);

    if (! (input_count || output_count)) {
        jack_error("JackWinMMEDriver::Open - no WinMME inputs or outputs "
                   "allocated.");
    } else if (! JackMidiDriver::Open(capturing, playing, input_count,
                                      output_count, monitor,
                                      capture_driver_name,
                                      playback_driver_name, capture_latency,
                                      playback_latency)) {
        return 0;
    }

    if (output_ports) {
        for (int i = 0; i < output_count; i++) {
            delete output_ports[i];
        }
        delete[] output_ports;
        output_ports = 0;
    }
 destroy_input_ports:
    if (input_ports) {
        for (int i = 0; i < input_count; i++) {
            delete input_ports[i];
        }
        delete[] input_ports;
        input_ports = 0;
    }
 unset_timer_resolution:
    if (period) {
        if (timeEndPeriod(period) != TIMERR_NOERROR) {
            jack_error("JackWinMMEDriver::Open - failed to unset timer "
                       "resolution.");
        }
    }
    return -1;
}

int
JackWinMMEDriver::Read()
{
    jack_nframes_t buffer_size = fEngineControl->fBufferSize;
    for (int i = 0; i < fCaptureChannels; i++) {
        input_ports[i]->ProcessJack(GetInputBuffer(i), buffer_size);
    }

    return 0;
}

int
JackWinMMEDriver::Write()
{
    jack_nframes_t buffer_size = fEngineControl->fBufferSize;
    for (int i = 0; i < fPlaybackChannels; i++) {
        output_ports[i]->ProcessJack(GetOutputBuffer(i), buffer_size);
    }

    return 0;
}

int
JackWinMMEDriver::Start()
{
    jack_log("JackWinMMEDriver::Start - Starting driver.");

    JackMidiDriver::Start();

    int input_count = 0;
    int output_count = 0;

    jack_log("JackWinMMEDriver::Start - Enabling input ports.");

    for (; input_count < fCaptureChannels; input_count++) {
        if (input_ports[input_count]->Start() < 0) {
            jack_error("JackWinMMEDriver::Start - Failed to enable input "
                       "port.");
            goto stop_input_ports;
        }
    }

    jack_log("JackWinMMEDriver::Start - Enabling output ports.");

    for (; output_count < fPlaybackChannels; output_count++) {
        if (output_ports[output_count]->Start() < 0) {
            jack_error("JackWinMMEDriver::Start - Failed to enable output "
                       "port.");
            goto stop_output_ports;
        }
    }

    jack_log("JackWinMMEDriver::Start - Driver started.");
    return 0;

 stop_output_ports:
    for (int i = 0; i < output_count; i++) {
        if (output_ports[i]->Stop() < 0) {
            jack_error("JackWinMMEDriver::Start - Failed to disable output "
                       "port.");
        }
    }
 stop_input_ports:
    for (int i = 0; i < input_count; i++) {
        if (input_ports[i]->Stop() < 0) {
            jack_error("JackWinMMEDriver::Start - Failed to disable input "
                       "port.");
        }
    }

    return -1;
}

int
JackWinMMEDriver::Stop()
{
    int result = 0;

    JackMidiDriver::Stop();

    jack_log("JackWinMMEDriver::Stop - disabling input ports.");

    for (int i = 0; i < fCaptureChannels; i++) {
        if (input_ports[i]->Stop() < 0) {
            jack_error("JackWinMMEDriver::Stop - Failed to disable input "
                       "port.");
            result = -1;
        }
    }

    jack_log("JackWinMMEDriver::Stop - disabling output ports.");

    for (int i = 0; i < fPlaybackChannels; i++) {
        if (output_ports[i]->Stop() < 0) {
            jack_error("JackWinMMEDriver::Stop - Failed to disable output "
                       "port.");
            result = -1;
        }
    }

    return result;
}

#ifdef __cplusplus
extern "C"
{
#endif

    // singleton kind of driver
    static Jack::JackWinMMEDriver* driver = NULL;

    SERVER_EXPORT jack_driver_desc_t * driver_get_descriptor()
    {
        return jack_driver_descriptor_construct("winmme", JackDriverSlave, "WinMME API based MIDI backend", NULL);
    }

    SERVER_EXPORT Jack::JackDriverClientInterface* driver_initialize(Jack::JackLockedEngine* engine, Jack::JackSynchro* table, const JSList* params)
    {
        /*
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
        */

        // singleton kind of driver
        if (!driver) {
            driver = new Jack::JackWinMMEDriver("system_midi", "winmme", engine, table);
            if (driver->Open(1, 1, 0, 0, false, "in", "out", 0, 0) == 0) {
                return driver;
            } else {
                delete driver;
                return NULL;
            }
        } else {
            jack_info("JackWinMMEDriver already allocated, cannot be loaded twice");
            return NULL;
        }

    }

#ifdef __cplusplus
}
#endif


/*
jack_connect system:midi_capture_1 system_midi:playback_1
jack_connect system:midi_capture_1 system_midi:playback_2

jack_connect system:midi_capture_1 system_midi:playback_1

jack_connect system:midi_capture_1 system_midi:playback_1

jack_connect system:midi_capture_1 system_midi:playback_1

jack_connect system_midi:capture_1 system:midi_playback_1
jack_connect system_midi:capture_2 system:midi_playback_1

jack_connect system_midi:capture_1  system_midi:playback_1

*/

