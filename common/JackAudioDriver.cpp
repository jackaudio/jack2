/*
Copyright (C) 2001 Paul Davis
Copyright (C) 2004-2008 Grame

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

#ifdef WIN32
#pragma warning (disable : 4786)
#endif

#include "JackAudioDriver.h"
#include "JackTime.h"
#include "JackError.h"
#include "JackEngineControl.h"
#include "JackClientControl.h"
#include "JackPort.h"
#include "JackGraphManager.h"
#include "JackEngine.h"
#include <assert.h>

namespace Jack
{

JackAudioDriver::JackAudioDriver(const char* name, JackEngine* engine, JackSynchro** table)
        : JackDriver(name, engine, table),
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

int JackAudioDriver::Open(jack_nframes_t nframes,
                          jack_nframes_t samplerate,
                          int capturing,
                          int playing,
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
    return JackDriver::Open(nframes, samplerate, capturing, playing, inchannels, outchannels, monitor, capture_driver_name, playback_driver_name, capture_latency, playback_latency);
}

int JackAudioDriver::Attach()
{
    JackPort* port;
    jack_port_id_t port_index;
    char name[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
    char alias[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
    unsigned long port_flags = JackPortIsOutput | JackPortIsPhysical | JackPortIsTerminal;
    int i;

    jack_log("JackAudioDriver::Attach fBufferSize = %ld fSampleRate = %ld", fEngineControl->fBufferSize, fEngineControl->fSampleRate);

    for (i = 0; i < fCaptureChannels; i++) {
        snprintf(alias, sizeof(alias) - 1, "%s:%s:out%d", fClientControl->fName, fCaptureDriverName, i + 1);
        snprintf(name, sizeof(name) - 1, "system:capture_%d", i + 1);
        if ((port_index = fGraphManager->AllocatePort(fClientControl->fRefNum, name, JACK_DEFAULT_AUDIO_TYPE, (JackPortFlags)port_flags, fEngineControl->fBufferSize)) == NO_PORT) {
            jack_error("driver: cannot register port for %s", name);
            return -1;
        }
        port = fGraphManager->GetPort(port_index);
        port->SetAlias(alias);
        port->SetLatency(fEngineControl->fBufferSize + fCaptureLatency);
        fCapturePortList[i] = port_index;
        jack_log("JackAudioDriver::Attach fCapturePortList[i] port_index = %ld", port_index);
    }

    port_flags = JackPortIsInput | JackPortIsPhysical | JackPortIsTerminal;

    for (i = 0; i < fPlaybackChannels; i++) {
        snprintf(alias, sizeof(alias) - 1, "%s:%s:in%d", fClientControl->fName, fPlaybackDriverName, i + 1);
        snprintf(name, sizeof(name) - 1, "system:playback_%d", i + 1);
        if ((port_index = fGraphManager->AllocatePort(fClientControl->fRefNum, name, JACK_DEFAULT_AUDIO_TYPE, (JackPortFlags)port_flags, fEngineControl->fBufferSize)) == NO_PORT) {
            jack_error("driver: cannot register port for %s", name);
            return -1;
        }
        port = fGraphManager->GetPort(port_index);
        port->SetAlias(alias);
        // Add more latency if "async" mode is used...
        port->SetLatency(fEngineControl->fBufferSize + ((fEngineControl->fSyncMode) ? 0 : fEngineControl->fBufferSize) + fPlaybackLatency);
        fPlaybackPortList[i] = port_index;
        jack_log("JackAudioDriver::Attach fPlaybackPortList[i] port_index = %ld", port_index);

        // Monitor ports
        if (fWithMonitorPorts) {
            jack_log("Create monitor port ");
            snprintf(name, sizeof(name) - 1, "%s:%s:monitor_%u", fClientControl->fName, fPlaybackDriverName, i + 1);
            if ((port_index = fGraphManager->AllocatePort(fClientControl->fRefNum, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, fEngineControl->fBufferSize)) == NO_PORT) {
                jack_error("Cannot register monitor port for %s", name);
                return -1;
            } else {
                port = fGraphManager->GetPort(port_index);
                port->SetLatency(fEngineControl->fBufferSize);
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
        fGraphManager->ReleasePort(fClientControl->fRefNum, fCapturePortList[i]);
    }

    for (i = 0; i < fPlaybackChannels; i++) {
        fGraphManager->ReleasePort(fClientControl->fRefNum, fPlaybackPortList[i]);
        if (fWithMonitorPorts)
            fGraphManager->ReleasePort(fClientControl->fRefNum, fMonitorPortList[i]);
    }

    return 0;
}

int JackAudioDriver::Write()
{
    for (int i = 0; i < fPlaybackChannels; i++) {
        if (fGraphManager->GetConnectionsNum(fPlaybackPortList[i]) > 0) {
            float* buffer = GetOutputBuffer(i);
            int size = sizeof(float) * fEngineControl->fBufferSize;
            // Monitor ports
            if (fWithMonitorPorts && fGraphManager->GetConnectionsNum(fMonitorPortList[i]) > 0)
                memcpy(GetMonitorBuffer(i), buffer, size);
        }
    }
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
    if (Read() < 0) { // Read input buffers for the current cycle
        jack_error("ProcessAsync: read error");
        return 0;
    }

    if (Write() < 0) { // Write output buffers from the previous cycle
        jack_error("ProcessAsync: write error");
        return 0;
    }

    if (fIsMaster) {
        if (!fEngine->Process(fLastWaitUst)) // fLastWaitUst is set in the "low level" layer
            jack_error("JackAudioDriver::ProcessAsync Process error");
        fGraphManager->ResumeRefNum(fClientControl, fSynchroTable);
        if (ProcessSlaves() < 0)
            jack_error("JackAudioDriver::ProcessAsync ProcessSlaves error");
    } else {
        fGraphManager->ResumeRefNum(fClientControl, fSynchroTable);
    }
    return 0;
}

/*
The driver SYNC mode: the server does synchronize to the end of client graph execution,
output buffers computed at the *current cycle* are used.
*/

int JackAudioDriver::ProcessSync()
{
    if (Read() < 0) { // Read input buffers for the current cycle
        jack_error("ProcessSync: read error");
        return 0;
    }

    if (fIsMaster) {

        if (fEngine->Process(fLastWaitUst)) { // fLastWaitUst is set in the "low level" layer
            fGraphManager->ResumeRefNum(fClientControl, fSynchroTable);
            if (ProcessSlaves() < 0)
                jack_error("JackAudioDriver::ProcessSync ProcessSlaves error, engine may now behave abnormally!!");
            if (fGraphManager->SuspendRefNum(fClientControl, fSynchroTable, fEngineControl->fTimeOutUsecs) < 0)
                jack_error("JackAudioDriver::ProcessSync SuspendRefNum error, engine may now behave abnormally!!");
        } else { // Graph not finished: do not activate it
            jack_error("ProcessSync: error");
        }

        if (Write() < 0)  // Write output buffers for the current cycle
            jack_error("ProcessSync: write error");

    } else {
        fGraphManager->ResumeRefNum(fClientControl, fSynchroTable);
    }
    return 0;
}

void JackAudioDriver::NotifyXRun(jack_time_t callback_usecs)
{
    fEngine->NotifyXRun(callback_usecs);
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

} // end of namespace
