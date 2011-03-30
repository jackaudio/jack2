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

namespace Jack
{

JackAudioDriver::JackAudioDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table)
        : JackDriver(name, alias, engine, table),
        fCaptureChannels(0),
        fPlaybackChannels(0),
        fWithMonitorPorts(false)
{}

JackAudioDriver::~JackAudioDriver()
{}

int JackAudioDriver::SetBufferSize(jack_nframes_t buffer_size)
{
    fEngineControl->fBufferSize = buffer_size;
    fGraphManager->SetBufferSize(buffer_size);
    fEngineControl->fPeriodUsecs = jack_time_t(1000000.f / fEngineControl->fSampleRate * fEngineControl->fBufferSize);	// in microsec
    if (!fEngineControl->fTimeOut)
        fEngineControl->fTimeOutUsecs = jack_time_t(2.f * fEngineControl->fPeriodUsecs);
    return 0;
}

int JackAudioDriver::SetSampleRate(jack_nframes_t sample_rate)
{
    fEngineControl->fSampleRate = sample_rate;
    fEngineControl->fPeriodUsecs = jack_time_t(1000000.f / fEngineControl->fSampleRate * fEngineControl->fBufferSize);	// in microsec
    if (!fEngineControl->fTimeOut)
        fEngineControl->fTimeOutUsecs = jack_time_t(2.f * fEngineControl->fPeriodUsecs);
    return 0;
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
    return JackDriver::Open(buffer_size, samplerate, capturing, playing, inchannels, outchannels, monitor, capture_driver_name, playback_driver_name, capture_latency, playback_latency);
}

int JackAudioDriver::Open(bool capturing,
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
    return JackDriver::Open(capturing, playing, inchannels, outchannels, monitor, capture_driver_name, playback_driver_name, capture_latency, playback_latency);
}

int JackAudioDriver::Attach()
{
    JackPort* port;
    jack_port_id_t port_index;
    char name[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
    char alias[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
    jack_latency_range_t range;
    int i;

    jack_log("JackAudioDriver::Attach fBufferSize = %ld fSampleRate = %ld", fEngineControl->fBufferSize, fEngineControl->fSampleRate);

    for (i = 0; i < fCaptureChannels; i++) {
        snprintf(alias, sizeof(alias) - 1, "%s:%s:out%d", fAliasName, fCaptureDriverName, i + 1);
        snprintf(name, sizeof(name) - 1, "%s:capture_%d", fClientControl.fName, i + 1);
        if ((port_index = fGraphManager->AllocatePort(fClientControl.fRefNum, name, JACK_DEFAULT_AUDIO_TYPE, CaptureDriverFlags, fEngineControl->fBufferSize)) == NO_PORT) {
            jack_error("driver: cannot register port for %s", name);
            return -1;
        }
        port = fGraphManager->GetPort(port_index);
        port->SetAlias(alias);
        range.min = range.max = fEngineControl->fBufferSize + fCaptureLatency;
        port->SetLatencyRange(JackCaptureLatency, &range);
        fCapturePortList[i] = port_index;
        jack_log("JackAudioDriver::Attach fCapturePortList[i] port_index = %ld", port_index);
    }

    for (i = 0; i < fPlaybackChannels; i++) {
        snprintf(alias, sizeof(alias) - 1, "%s:%s:in%d", fAliasName, fPlaybackDriverName, i + 1);
        snprintf(name, sizeof(name) - 1, "%s:playback_%d", fClientControl.fName, i + 1);
        if ((port_index = fGraphManager->AllocatePort(fClientControl.fRefNum, name, JACK_DEFAULT_AUDIO_TYPE, PlaybackDriverFlags, fEngineControl->fBufferSize)) == NO_PORT) {
            jack_error("driver: cannot register port for %s", name);
            return -1;
        }
        port = fGraphManager->GetPort(port_index);
        port->SetAlias(alias);
        // Add more latency if "async" mode is used...
        range.min = range.max = fEngineControl->fBufferSize + ((fEngineControl->fSyncMode) ? 0 : fEngineControl->fBufferSize) + fPlaybackLatency;
        port->SetLatencyRange(JackPlaybackLatency, &range);
        fPlaybackPortList[i] = port_index;
        jack_log("JackAudioDriver::Attach fPlaybackPortList[i] port_index = %ld", port_index);

        // Monitor ports
        if (fWithMonitorPorts) {
            jack_log("Create monitor port");
            snprintf(name, sizeof(name) - 1, "%s:monitor_%u", fClientControl.fName, i + 1);
            if ((port_index = fGraphManager->AllocatePort(fClientControl.fRefNum, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, fEngineControl->fBufferSize)) == NO_PORT) {
                jack_error("Cannot register monitor port for %s", name);
                return -1;
            } else {
                port = fGraphManager->GetPort(port_index);
                range.min = range.max = fEngineControl->fBufferSize;
                port->SetLatencyRange(JackCaptureLatency, &range);
                fMonitorPortList[i] = port_index;
            }
        }
    }

    return 0;
}

int JackAudioDriver::Detach()
{
    int i;
    jack_log("JackAudioDriver::Detach");

    for (i = 0; i < fCaptureChannels; i++) {
        fGraphManager->ReleasePort(fClientControl.fRefNum, fCapturePortList[i]);
    }

    for (i = 0; i < fPlaybackChannels; i++) {
        fGraphManager->ReleasePort(fClientControl.fRefNum, fPlaybackPortList[i]);
        if (fWithMonitorPorts)
            fGraphManager->ReleasePort(fClientControl.fRefNum, fMonitorPortList[i]);
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
            if (fWithMonitorPorts && fGraphManager->GetConnectionsNum(fMonitorPortList[i]) > 0)
                memcpy(GetMonitorBuffer(i), buffer, size);
        }
    }
    return 0;
}

int JackAudioDriver::ProcessNull()
{
    // Keep begin cycle time
    JackDriver::CycleTakeBeginTime();

    if (fEngineControl->fSyncMode) {
        ProcessGraphSync();
    } else {
        ProcessGraphAsync();
    }

    // Keep end cycle time
    JackDriver::CycleTakeEndTime();
    WaitUntilNextCycle();
    return 0;
}

int JackAudioDriver::Process()
{
    return (fEngineControl->fSyncMode) ? ProcessSync() : ProcessAsync();
}

/*
The driver ASYNC mode: output buffers computed at the *previous cycle* are used, the server does not
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
    if (fIsMaster) {
        ProcessGraphAsync();
    } else {
        fGraphManager->ResumeRefNum(&fClientControl, fSynchroTable);
    }

    // Keep end cycle time
    JackDriver::CycleTakeEndTime();
    return 0;
}

/*
The driver SYNC mode: the server does synchronize to the end of client graph execution,
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
    if (fIsMaster) {
        if (ProcessGraphSync() < 0) {
            jack_error("JackAudioDriver::ProcessSync: process error, skip cycle...");
            goto end;
        }
    } else {
        if (fGraphManager->ResumeRefNum(&fClientControl, fSynchroTable) < 0) {
            jack_error("JackAudioDriver::ProcessSync: process error, skip cycle...");
            goto end;
        }
    }

    // Write output buffers from the current cycle
    if (Write() < 0) {
        jack_error("JackAudioDriver::ProcessSync: write error, stopping...");
        return -1;
    }

end:

    // Keep end cycle time
    JackDriver::CycleTakeEndTime();
    return 0;
}

void JackAudioDriver::ProcessGraphAsync()
{
    // fBeginDateUst is set in the "low level" layer, fEndDateUst is from previous cycle
    if (!fEngine->Process(fBeginDateUst, fEndDateUst))
        jack_error("JackAudioDriver::ProcessGraphAsync: Process error");
    fGraphManager->ResumeRefNum(&fClientControl, fSynchroTable);
    if (ProcessSlaves() < 0)
        jack_error("JackAudioDriver::ProcessGraphAsync: ProcessSlaves error");
}

int JackAudioDriver::ProcessGraphSync()
{
    int res = 0;

    // fBeginDateUst is set in the "low level" layer, fEndDateUst is from previous cycle
    if (fEngine->Process(fBeginDateUst, fEndDateUst)) {
        fGraphManager->ResumeRefNum(&fClientControl, fSynchroTable);
        if (ProcessSlaves() < 0) {
            jack_error("JackAudioDriver::ProcessGraphSync: ProcessSlaves error, engine may now behave abnormally!!");
            res = -1;
        }
        if (fGraphManager->SuspendRefNum(&fClientControl, fSynchroTable, DRIVER_TIMEOUT_FACTOR * fEngineControl->fTimeOutUsecs) < 0) {
            jack_error("JackAudioDriver::ProcessGraphSync: SuspendRefNum error, engine may now behave abnormally!!");
            res = -1;
        }
    } else { // Graph not finished: do not activate it
        jack_error("JackAudioDriver::ProcessGraphSync: Process error");
        res = -1;
    }

    return res;
}

int JackAudioDriver::Start()
{
    int res = JackDriver::Start();
    if ((res >= 0) && fIsMaster) {
        res = StartSlaves();
    }
    return res;
}

int JackAudioDriver::Stop()
{
    int res = JackDriver::Stop();
    if (fIsMaster) {
        if (StopSlaves() < 0) {
            res = -1;
        }
    }
    return res;
}

void JackAudioDriver::WaitUntilNextCycle()
{
    int wait_time_usec = (int((float(fEngineControl->fBufferSize) / (float(fEngineControl->fSampleRate))) * 1000000.0f));
    wait_time_usec = int(wait_time_usec - (GetMicroSeconds() - fBeginDateUst));
	if (wait_time_usec > 0)
		JackSleep(wait_time_usec);
}

jack_default_audio_sample_t* JackAudioDriver::GetInputBuffer(int port_index)
{
    assert(fCapturePortList[port_index]);
    return (jack_default_audio_sample_t*)fGraphManager->GetBuffer(fCapturePortList[port_index], fEngineControl->fBufferSize);
}

jack_default_audio_sample_t* JackAudioDriver::GetOutputBuffer(int port_index)
{
    assert(fPlaybackPortList[port_index]);
    return (jack_default_audio_sample_t*)fGraphManager->GetBuffer(fPlaybackPortList[port_index], fEngineControl->fBufferSize);
}

jack_default_audio_sample_t* JackAudioDriver::GetMonitorBuffer(int port_index)
{
    assert(fPlaybackPortList[port_index]);
    return (jack_default_audio_sample_t*)fGraphManager->GetBuffer(fMonitorPortList[port_index], fEngineControl->fBufferSize);
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
