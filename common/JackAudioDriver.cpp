/*
Copyright (C) 2001 Paul Davis
Copyright (C) 2004-2008 Grame.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
(at your option) any later version.

GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "JackSystemDeps.h"
#include "JackAudioDriver.h"
#include "JackTime.h"
#include "JackError.h"
#include "JackEngineControl.h"
#include "JackPort.h"
#include "JackGraphManager.h"
#include "JackLockedEngine.h"
#include "JackException.h"
#include <assert.h>

using namespace std;

namespace Jack
{

JackAudioDriver::JackAudioDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table)
        : JackDriver(name, alias, engine, table)
{}

JackAudioDriver::~JackAudioDriver()
{}

int JackAudioDriver::SetBufferSize(jack_nframes_t buffer_size)
{
    // Update engine and graph manager state
    fEngineControl->fBufferSize = buffer_size;
    fGraphManager->SetBufferSize(buffer_size);
    
    fEngineControl->UpdateTimeOut();
    UpdateLatencies();

    // Redirected on slaves drivers...
    return JackDriver::SetBufferSize(buffer_size);
}

int JackAudioDriver::SetSampleRate(jack_nframes_t sample_rate)
{
    fEngineControl->fSampleRate = sample_rate;
    fEngineControl->UpdateTimeOut();

    // Redirected on slaves drivers...
    return JackDriver::SetSampleRate(sample_rate);
}

int JackAudioDriver::Open(jack_nframes_t buffer_size,
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
    fCaptureChannels = inchannels;
    fPlaybackChannels = outchannels;
    fWithMonitorPorts = monitor;
    memset(fCapturePortList, 0, sizeof(jack_port_id_t) * DRIVER_PORT_NUM);
    memset(fPlaybackPortList, 0, sizeof(jack_port_id_t) * DRIVER_PORT_NUM);
    memset(fMonitorPortList, 0, sizeof(jack_port_id_t) * DRIVER_PORT_NUM);
    return JackDriver::Open(buffer_size, samplerate, capturing, playing, inchannels, outchannels,
        monitor, capture_driver_name, playback_driver_name, capture_latency, playback_latency);
}

void JackAudioDriver::UpdateLatencies()
{
    jack_latency_range_t input_range;
    jack_latency_range_t output_range;
    jack_latency_range_t monitor_range;

    for (int i = 0; i < fCaptureChannels; i++) {
        input_range.max = input_range.min = fEngineControl->fBufferSize + fCaptureLatency;
        fGraphManager->GetPort(fCapturePortList[i])->SetLatencyRange(JackCaptureLatency, &input_range);
    }

    for (int i = 0; i < fPlaybackChannels; i++) {
        output_range.max = output_range.min  = fPlaybackLatency;
        if (fEngineControl->fSyncMode) {
            output_range.max = output_range.min += fEngineControl->fBufferSize;
        } else {
            output_range.max = output_range.min += fEngineControl->fBufferSize * 2;
        }
        fGraphManager->GetPort(fPlaybackPortList[i])->SetLatencyRange(JackPlaybackLatency, &output_range);
        if (fWithMonitorPorts) {
            monitor_range.min = monitor_range.max = fEngineControl->fBufferSize;
            fGraphManager->GetPort(fMonitorPortList[i])->SetLatencyRange(JackCaptureLatency, &monitor_range);
        }
    }
}

int JackAudioDriver::Attach()
{
    JackPort* port;
    jack_port_id_t port_index;
    char name[REAL_JACK_PORT_NAME_SIZE+1];
    char alias[REAL_JACK_PORT_NAME_SIZE+1];
    int i;

    jack_log("JackAudioDriver::Attach fBufferSize = %ld fSampleRate = %ld", fEngineControl->fBufferSize, fEngineControl->fSampleRate);

    for (i = 0; i < fCaptureChannels; i++) {
        snprintf(alias, sizeof(alias), "%s:%s:out%d", fAliasName, fCaptureDriverName, i + 1);
        snprintf(name, sizeof(name), "%s:capture_%d", fClientControl.fName, i + 1);
        if (fEngine->PortRegister(fClientControl.fRefNum, name, JACK_DEFAULT_AUDIO_TYPE, CaptureDriverFlags, fEngineControl->fBufferSize, &port_index) < 0) {
            jack_error("driver: cannot register port for %s", name);
            return -1;
        }
        port = fGraphManager->GetPort(port_index);
        port->SetAlias(alias);
        fCapturePortList[i] = port_index;
        jack_log("JackAudioDriver::Attach fCapturePortList[i] port_index = %ld", port_index);
    }

    for (i = 0; i < fPlaybackChannels; i++) {
        snprintf(alias, sizeof(alias), "%s:%s:in%d", fAliasName, fPlaybackDriverName, i + 1);
        snprintf(name, sizeof(name), "%s:playback_%d", fClientControl.fName, i + 1);
        if (fEngine->PortRegister(fClientControl.fRefNum, name, JACK_DEFAULT_AUDIO_TYPE, PlaybackDriverFlags, fEngineControl->fBufferSize, &port_index) < 0) {
            jack_error("driver: cannot register port for %s", name);
            return -1;
        }
        port = fGraphManager->GetPort(port_index);
        port->SetAlias(alias);
        fPlaybackPortList[i] = port_index;
        jack_log("JackAudioDriver::Attach fPlaybackPortList[i] port_index = %ld", port_index);

        // Monitor ports
        if (fWithMonitorPorts) {
            jack_log("Create monitor port");
            snprintf(name, sizeof(name), "%s:monitor_%u", fClientControl.fName, i + 1);
            if (fEngine->PortRegister(fClientControl.fRefNum, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, fEngineControl->fBufferSize, &port_index) < 0) {
                jack_error("Cannot register monitor port for %s", name);
                return -1;
            } else {
                fMonitorPortList[i] = port_index;
            }
        }
    }

    UpdateLatencies();
    return 0;
}

int JackAudioDriver::Detach()
{
    int i;
    jack_log("JackAudioDriver::Detach");

    for (i = 0; i < fCaptureChannels; i++) {
        fEngine->PortUnRegister(fClientControl.fRefNum, fCapturePortList[i]);
    }

    for (i = 0; i < fPlaybackChannels; i++) {
        fEngine->PortUnRegister(fClientControl.fRefNum, fPlaybackPortList[i]);
        if (fWithMonitorPorts) {
            fEngine->PortUnRegister(fClientControl.fRefNum, fMonitorPortList[i]);
        }
    }

    return 0;
}

int JackAudioDriver::Write()
{
    for (int i = 0; i < fPlaybackChannels; i++) {
        if (fGraphManager->GetConnectionsNum(fPlaybackPortList[i]) > 0) {
            jack_default_audio_sample_t* buffer = GetOutputBuffer(i);
            int size = sizeof(jack_default_audio_sample_t) * fEngineControl->fBufferSize;
            // Monitor ports
            if (fWithMonitorPorts && fGraphManager->GetConnectionsNum(fMonitorPortList[i]) > 0) {
                memcpy(GetMonitorBuffer(i), buffer, size);
            }
        }
    }
    return 0;
}

int JackAudioDriver::Process()
{
    return (fEngineControl->fSyncMode) ? ProcessSync() : ProcessAsync();
}

/*
The driver "asynchronous" mode: output buffers computed at the *previous cycle* are used, the server does not
synchronize to the end of client graph execution.
*/

int JackAudioDriver::ProcessAsync()
{
    // Read input buffers for the current cycle
    if (Read() < 0) {
        jack_error("JackAudioDriver::ProcessAsync: read error, stopping...");
        return -1;
    }

    // Write output buffers from the previous cycle
    if (Write() < 0) {
        jack_error("JackAudioDriver::ProcessAsync: write error, stopping...");
        return -1;
    }

    // Process graph
    ProcessGraphAsync();

    // Keep end cycle time
    JackDriver::CycleTakeEndTime();
    return 0;
}

void JackAudioDriver::ProcessGraphAsync()
{
    // Process graph
    if (fIsMaster) {
        ProcessGraphAsyncMaster();
    } else {
        ProcessGraphAsyncSlave();
    }
}

/*
Used when the driver works in master mode.
*/

void JackAudioDriver::ProcessGraphAsyncMaster()
{
    // fBeginDateUst is set in the "low level" layer, fEndDateUst is from previous cycle
    if (!fEngine->Process(fBeginDateUst, fEndDateUst)) {
        jack_error("JackAudioDriver::ProcessGraphAsyncMaster: Process error");
    }

    if (ResumeRefNum() < 0) {
        jack_error("JackAudioDriver::ProcessGraphAsyncMaster: ResumeRefNum error");
    }

    if (ProcessReadSlaves() < 0) {
        jack_error("JackAudioDriver::ProcessGraphAsyncMaster: ProcessReadSlaves error");
    }

    if (ProcessWriteSlaves() < 0) {
        jack_error("JackAudioDriver::ProcessGraphAsyncMaster: ProcessWriteSlaves error");
    }

    // Does not wait on graph execution end
}

/*
Used when the driver works in slave mode.
*/

void JackAudioDriver::ProcessGraphAsyncSlave()
{
    if (ResumeRefNum() < 0) {
        jack_error("JackAudioDriver::ProcessGraphAsyncSlave: ResumeRefNum error");
    }
}

/*
The driver "synchronous" mode: the server does synchronize to the end of client graph execution,
if graph process succeed, output buffers computed at the *current cycle* are used.
*/

int JackAudioDriver::ProcessSync()
{
    // Read input buffers for the current cycle
    if (Read() < 0) {
        jack_error("JackAudioDriver::ProcessSync: read error, stopping...");
        return -1;
    }

    // Process graph
    ProcessGraphSync();

    // Write output buffers from the current cycle
    if (Write() < 0) {
        jack_error("JackAudioDriver::ProcessSync: write error, stopping...");
        return -1;
    }

    // Keep end cycle time
    JackDriver::CycleTakeEndTime();
    return 0;
}

void JackAudioDriver::ProcessGraphSync()
{
    // Process graph
    if (fIsMaster) {
        ProcessGraphSyncMaster();
     } else {
        ProcessGraphSyncSlave();
    }
}

/*
Used when the driver works in master mode.
*/

void JackAudioDriver::ProcessGraphSyncMaster()
{
    // fBeginDateUst is set in the "low level" layer, fEndDateUst is from previous cycle
    if (fEngine->Process(fBeginDateUst, fEndDateUst)) {

        if (ResumeRefNum() < 0) {
            jack_error("JackAudioDriver::ProcessGraphSyncMaster: ResumeRefNum error");
        }

        if (ProcessReadSlaves() < 0) {
            jack_error("JackAudioDriver::ProcessGraphSync: ProcessReadSlaves error, engine may now behave abnormally!!");
        }

        if (ProcessWriteSlaves() < 0) {
            jack_error("JackAudioDriver::ProcessGraphSync: ProcessWriteSlaves error, engine may now behave abnormally!!");
        }

        // Waits for graph execution end
        if (SuspendRefNum() < 0) {
            jack_error("JackAudioDriver::ProcessGraphSync: SuspendRefNum error, engine may now behave abnormally!!");
        }

    } else { // Graph not finished: do not activate it
        jack_error("JackAudioDriver::ProcessGraphSync: Process error");
    }
}

/*
Used when the driver works in slave mode.
*/

void JackAudioDriver::ProcessGraphSyncSlave()
{
    if (ResumeRefNum() < 0) {
        jack_error("JackAudioDriver::ProcessGraphSyncSlave: ResumeRefNum error");
    }
}

jack_default_audio_sample_t* JackAudioDriver::GetInputBuffer(int port_index)
{
    return fCapturePortList[port_index]
        ? (jack_default_audio_sample_t*)fGraphManager->GetBuffer(fCapturePortList[port_index], fEngineControl->fBufferSize)
        : NULL;
}

jack_default_audio_sample_t* JackAudioDriver::GetOutputBuffer(int port_index)
{
    return fPlaybackPortList[port_index]
        ? (jack_default_audio_sample_t*)fGraphManager->GetBuffer(fPlaybackPortList[port_index], fEngineControl->fBufferSize)
        : NULL;
}

jack_default_audio_sample_t* JackAudioDriver::GetMonitorBuffer(int port_index)
{
    return fPlaybackPortList[port_index]
        ? (jack_default_audio_sample_t*)fGraphManager->GetBuffer(fMonitorPortList[port_index], fEngineControl->fBufferSize)
        : NULL;
}

int JackAudioDriver::ClientNotify(int refnum, const char* name, int notify, int sync, const char* message, int value1, int value2)
{
    switch (notify) {

        case kLatencyCallback:
            HandleLatencyCallback(value1);
            break;

        default:
            JackDriver::ClientNotify(refnum, name, notify, sync, message, value1, value2);
            break;
    }

    return 0;
}

void JackAudioDriver::HandleLatencyCallback(int status)
{
    jack_latency_callback_mode_t mode = (status == 0) ? JackCaptureLatency : JackPlaybackLatency;

    for (int i = 0; i < fCaptureChannels; i++) {
        if (mode == JackPlaybackLatency) {
           fGraphManager->RecalculateLatency(fCapturePortList[i], mode);
		}
	}

    for (int i = 0; i < fPlaybackChannels; i++) {
        if (mode == JackCaptureLatency) {
            fGraphManager->RecalculateLatency(fPlaybackPortList[i], mode);
		}
	}
}

} // end of namespace
