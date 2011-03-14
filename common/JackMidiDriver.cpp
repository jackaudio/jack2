/*
Copyright (C) 2009 Grame.

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
#include "JackMidiDriver.h"
#include "JackTime.h"
#include "JackError.h"
#include "JackEngineControl.h"
#include "JackPort.h"
#include "JackGraphManager.h"
#include "JackException.h"
#include <assert.h>

namespace Jack
{

JackMidiDriver::JackMidiDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table)
        : JackDriver(name, alias, engine, table),
        fCaptureChannels(0),
        fPlaybackChannels(0)
{
    for (int i = 0; i < DRIVER_PORT_NUM; i++) {
        fRingBuffer[i] = NULL;
    }
}

JackMidiDriver::~JackMidiDriver()
{
    for (int i = 0; i < fCaptureChannels; i++) {
        if (fRingBuffer[i])
            jack_ringbuffer_free(fRingBuffer[i]);
    }
}

int JackMidiDriver::Open(bool capturing,
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

    for (int i = 0; i < fCaptureChannels; i++) {
        fRingBuffer[i] = jack_ringbuffer_create(sizeof(jack_default_audio_sample_t) * BUFFER_SIZE_MAX);
    }

    return JackDriver::Open(capturing, playing, inchannels, outchannels, monitor, capture_driver_name, playback_driver_name, capture_latency, playback_latency);
}

int JackMidiDriver::Attach()
{
    JackPort* port;
    jack_port_id_t port_index;
    char name[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
    char alias[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
    int i;

    jack_log("JackMidiDriver::Attach fBufferSize = %ld fSampleRate = %ld", fEngineControl->fBufferSize, fEngineControl->fSampleRate);

    for (i = 0; i < fCaptureChannels; i++) {
        snprintf(alias, sizeof(alias) - 1, "%s:%s:out%d", fAliasName, fCaptureDriverName, i + 1);
        snprintf(name, sizeof(name) - 1, "%s:capture_%d", fClientControl.fName, i + 1);
        if ((port_index = fGraphManager->AllocatePort(fClientControl.fRefNum, name, JACK_DEFAULT_MIDI_TYPE, CaptureDriverFlags, fEngineControl->fBufferSize)) == NO_PORT) {
            jack_error("driver: cannot register port for %s", name);
            return -1;
        }
        port = fGraphManager->GetPort(port_index);
        port->SetAlias(alias);
        fCapturePortList[i] = port_index;
        jack_log("JackMidiDriver::Attach fCapturePortList[i] port_index = %ld", port_index);
    }

    for (i = 0; i < fPlaybackChannels; i++) {
        snprintf(alias, sizeof(alias) - 1, "%s:%s:in%d", fAliasName, fPlaybackDriverName, i + 1);
        snprintf(name, sizeof(name) - 1, "%s:playback_%d", fClientControl.fName, i + 1);
        if ((port_index = fGraphManager->AllocatePort(fClientControl.fRefNum, name, JACK_DEFAULT_MIDI_TYPE, PlaybackDriverFlags, fEngineControl->fBufferSize)) == NO_PORT) {
            jack_error("driver: cannot register port for %s", name);
            return -1;
        }
        port = fGraphManager->GetPort(port_index);
        port->SetAlias(alias);
        fPlaybackPortList[i] = port_index;
        jack_log("JackMidiDriver::Attach fPlaybackPortList[i] port_index = %ld", port_index);
    }

    return 0;
}

int JackMidiDriver::Detach()
{
    int i;
    jack_log("JackMidiDriver::Detach");

    for (i = 0; i < fCaptureChannels; i++) {
        fGraphManager->ReleasePort(fClientControl.fRefNum, fCapturePortList[i]);
    }

    for (i = 0; i < fPlaybackChannels; i++) {
        fGraphManager->ReleasePort(fClientControl.fRefNum, fPlaybackPortList[i]);
    }

    return 0;
}

int JackMidiDriver::Read()
{
    return 0;
}

int JackMidiDriver::Write()
{
    return 0;
}

int JackMidiDriver::ProcessNull()
{
    return 0;
}

int JackMidiDriver::Process()
{
    // Read input buffers for the current cycle
    if (Read() < 0) {
        jack_error("JackMidiDriver::Process: read error, skip cycle");
        return 0;   // Skip cycle, but continue processing...
    }

    fGraphManager->ResumeRefNum(&fClientControl, fSynchroTable);
    if (fEngineControl->fSyncMode) {
        if (fGraphManager->SuspendRefNum(&fClientControl, fSynchroTable, fEngineControl->fTimeOutUsecs) < 0) {
            jack_error("JackFreewheelDriver::ProcessSync SuspendRefNum error");
            return -1;
        }
    }

    // Write output buffers for the current cycle
    if (Write() < 0) {
        jack_error("JackMidiDriver::Process: write error, skip cycle");
        return 0;   // Skip cycle, but continue processing...
    }

    return 0;
}

JackMidiBuffer* JackMidiDriver::GetInputBuffer(int port_index)
{
    assert(fCapturePortList[port_index]);
    return (JackMidiBuffer*)fGraphManager->GetBuffer(fCapturePortList[port_index], fEngineControl->fBufferSize);
}

JackMidiBuffer* JackMidiDriver::GetOutputBuffer(int port_index)
{
    assert(fPlaybackPortList[port_index]);
    return (JackMidiBuffer*)fGraphManager->GetBuffer(fPlaybackPortList[port_index], fEngineControl->fBufferSize);
}

} // end of namespace
