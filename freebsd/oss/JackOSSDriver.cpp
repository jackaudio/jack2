/*
Copyright (C) 2003-2007 Jussi Laako <jussi@sonarnerd.net>
Copyright (C) 2008 Grame & RTL 2008

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

#include "driver_interface.h"
#include "JackThreadedDriver.h"
#include "JackOSSDriver.h"
#include "JackEngineControl.h"
#include "JackGraphManager.h"
#include "JackError.h"
#include "JackTime.h"

#include <cstdint>
#include <stdio.h>

using namespace std;

namespace Jack
{

#ifdef JACK_MONITOR

#define CYCLE_POINTS 500000

struct OSSCycle {
    jack_time_t fBeforeRead;
    jack_time_t fAfterRead;
    jack_time_t fAfterReadConvert;
    jack_time_t fBeforeWrite;
    jack_time_t fAfterWrite;
    jack_time_t fBeforeWriteConvert;
};

struct OSSCycleTable {
    jack_time_t fBeforeFirstWrite;
    jack_time_t fAfterFirstWrite;
    OSSCycle fTable[CYCLE_POINTS];
};

OSSCycleTable gCycleTable;
int gCycleCount = 0;

#endif

int JackOSSDriver::Open(jack_nframes_t nframes,
                        int user_nperiods,
                        jack_nframes_t samplerate,
                        bool capturing,
                        bool playing,
                        int inchannels,
                        int outchannels,
                        bool excl,
                        bool monitor,
                        const char* capture_driver_uid,
                        const char* playback_driver_uid,
                        jack_nframes_t capture_latency,
                        jack_nframes_t playback_latency,
                        int bits,
                        bool ignorehwbuf)
{
    // Store local settings first.
    fCapture = capturing;
    fPlayback = playing;
    fBits = bits;
    fIgnoreHW = ignorehwbuf;
    fNperiods = user_nperiods;
    fExcl = excl;

    // Generic JackAudioDriver Open
    if (JackAudioDriver::Open(nframes, samplerate, capturing, playing, inchannels, outchannels, monitor,
        capture_driver_uid, playback_driver_uid, capture_latency, playback_latency) != 0) {
        return -1;
    } else {

#ifdef JACK_MONITOR
        // Force memory page in
        memset(&gCycleTable, 0, sizeof(gCycleTable));
#endif

        if (OpenAux() < 0) {
            Close();
            return -1;
        } else {
            fChannel.StartAssistThread(fEngineControl->fRealTime, fEngineControl->fServerPriority);
            return 0;
        }
    }
}

int JackOSSDriver::Close()
{
#ifdef JACK_MONITOR
    FILE* file = fopen("OSSProfiling.log", "w");

    if (file) {
        jack_info("Writing OSS driver timing data....");
        for (int i = 1; i < gCycleCount; i++) {
            int d1 = gCycleTable.fTable[i].fAfterRead - gCycleTable.fTable[i].fBeforeRead;
            int d2 = gCycleTable.fTable[i].fAfterReadConvert - gCycleTable.fTable[i].fAfterRead;
            int d3 = gCycleTable.fTable[i].fAfterWrite - gCycleTable.fTable[i].fBeforeWrite;
            int d4 = gCycleTable.fTable[i].fBeforeWrite - gCycleTable.fTable[i].fBeforeWriteConvert;
            fprintf(file, "%d \t %d \t %d \t %d \t \n", d1, d2, d3, d4);
        }
        fclose(file);
    } else {
        jack_error("JackOSSDriver::Close : cannot open OSSProfiling.log file");
    }

    file = fopen("TimingOSS.plot", "w");

    if (file == NULL) {
        jack_error("JackOSSDriver::Close cannot open TimingOSS.plot file");
    } else {

        fprintf(file, "set grid\n");
        fprintf(file, "set title \"OSS audio driver timing\"\n");
        fprintf(file, "set xlabel \"audio cycles\"\n");
        fprintf(file, "set ylabel \"usec\"\n");
        fprintf(file, "plot \"OSSProfiling.log\" using 1 title \"Driver read wait\" with lines, \
                            \"OSSProfiling.log\" using 2 title \"Driver read convert duration\" with lines, \
                            \"OSSProfiling.log\" using 3 title \"Driver write wait\" with lines, \
                            \"OSSProfiling.log\" using 4 title \"Driver write convert duration\" with lines\n");

        fprintf(file, "set output 'TimingOSS.pdf\n");
        fprintf(file, "set terminal pdf\n");

        fprintf(file, "set grid\n");
        fprintf(file, "set title \"OSS audio driver timing\"\n");
        fprintf(file, "set xlabel \"audio cycles\"\n");
        fprintf(file, "set ylabel \"usec\"\n");
        fprintf(file, "plot \"OSSProfiling.log\" using 1 title \"Driver read wait\" with lines, \
                            \"OSSProfiling.log\" using 2 title \"Driver read convert duration\" with lines, \
                            \"OSSProfiling.log\" using 3 title \"Driver write wait\" with lines, \
                            \"OSSProfiling.log\" using 4 title \"Driver write convert duration\" with lines\n");

        fclose(file);
    }
#endif

    fChannel.Lock();
    fChannel.StopAssistThread();
    fChannel.Unlock();

    int res = JackAudioDriver::Close();
    CloseAux();
    return res;
}


int JackOSSDriver::OpenAux()
{
    if (!fChannel.Lock()) {
        return -1;
    }

    // (Re-)Initialize runtime variables.
    fCycleEnd = 0;
    fLastRun = 0;
    fMaxRunGap = 0;

    if (!fChannel.InitialSetup(fEngineControl->fSampleRate)) {
        fChannel.Unlock();
        return -1;
    }

    if (fCapture) {
        if (!fChannel.OpenCapture(fCaptureDriverName, fExcl, fBits, fCaptureChannels)) {
            fChannel.Unlock();
            return -1;
        }
    }

    if (fPlayback) {
        if (!fChannel.OpenPlayback(fPlaybackDriverName, fExcl, fBits, fPlaybackChannels)) {
            fChannel.Unlock();
            return -1;
        }
    }

    if (!fChannel.StartChannels(fEngineControl->fBufferSize)) {
        fChannel.Unlock();
        return -1;
    }

    if (fCapture) {
        fChannel.Capture().log_device_info();
    }
    if (fPlayback) {
        fChannel.Playback().log_device_info();
    }

    if (size_t max_channels = std::max(fCaptureChannels, fPlaybackChannels)) {
        fSampleBuffers = new jack_sample_t * [max_channels];
    }

    if (!fChannel.Unlock()) {
        return -1;
    }

    return 0;
}

void JackOSSDriver::CloseAux()
{
    fChannel.Lock();

    fChannel.StopChannels();

    if (fSampleBuffers) {
        delete[] fSampleBuffers;
        fSampleBuffers = nullptr;
    }

    fChannel.Unlock();
}

int JackOSSDriver::Read()
{
#ifdef JACK_MONITOR
    gCycleTable.fTable[gCycleCount].fBeforeRead = GetMicroSeconds();
#endif

    if (!fChannel.Lock()) {
        return -1;
    }

    // Mark the end time of this cycle, in frames.
    fCycleEnd += fEngineControl->fBufferSize;

    // Process read and write channels at least once.
    std::int64_t channel_stamp = fChannel.FrameStamp();
    if (!fChannel.CheckTimeAndRun()) {
        fChannel.Unlock();
        return -1;
    }
    if (fChannel.FrameStamp() - fLastRun > fMaxRunGap) {
        fMaxRunGap = fChannel.FrameStamp() - fLastRun;
        std::int64_t channel_gap = fChannel.FrameStamp() - channel_stamp;
        jack_log("JackOSSDriver::Read max run gap %lld frames vs channel %lld.", fMaxRunGap, channel_gap);
    }

    // Check for over- and underruns.
    if (fChannel.XRunGap() > 0) {
        std::int64_t skip = fChannel.XRunGap() + fEngineControl->fBufferSize;
        NotifyXRun(GetMicroSeconds(), (float) (fChannel.FrameClock().frames_to_time(skip) / 1000));
        fCycleEnd += skip;
        jack_error("JackOSSDriver::Read(): XRun, late by %lld frames.", skip);
        fChannel.ResetBuffers(skip);
    }

    // Wait and process channels until read, or else write, buffer is finished.
    while ((fCapture && !fChannel.CaptureFinished()) ||
           (!fCapture && !fChannel.PlaybackFinished())) {
        if (!(fChannel.Sleep() && fChannel.CheckTimeAndRun())) {
            fChannel.Unlock();
            return -1;
        }
    }

    // Keep begin cycle time
    JackDriver::CycleTakeBeginTime();

    if (!fCapture) {
        if (!fChannel.Unlock()) {
            return -1;
        }
        return 0;
    }

    if ((fChannel.FrameStamp() / fEngineControl->fBufferSize) % ((5 * fEngineControl->fSampleRate) / fEngineControl->fBufferSize) == 0) {
        fChannel.Capture().log_state(fChannel.FrameStamp());
        fMaxRunGap = 0;
    }

#ifdef JACK_MONITOR
    gCycleTable.fTable[gCycleCount].fAfterRead = GetMicroSeconds();
#endif

    for (int i = 0; i < fCaptureChannels; i++) {
        fSampleBuffers[i] = nullptr;
        if (fGraphManager->GetConnectionsNum(fCapturePortList[i]) > 0) {
            fSampleBuffers[i] = GetInputBuffer(i);
        }
    }
    std::int64_t buffer_end = fCycleEnd + fEngineControl->fBufferSize;
    if (!fChannel.Read(fSampleBuffers, fEngineControl->fBufferSize, buffer_end)) {
        fChannel.Unlock();
        return -1;
    }

#ifdef JACK_MONITOR
    gCycleTable.fTable[gCycleCount].fAfterReadConvert = GetMicroSeconds();
#endif

    if (!fChannel.CheckTimeAndRun() || !fChannel.Unlock()) {
        return -1;
    }
    fLastRun = fChannel.FrameStamp();

    return 0;
}

int JackOSSDriver::Write()
{
    if (!fPlayback) {
        return 0;
    }

    if (!fChannel.Lock()) {
        return -1;
    }

    // Process read and write channels at least once.
    std::int64_t channel_stamp = fChannel.FrameStamp();
    if (!fChannel.CheckTimeAndRun()) {
        fChannel.Unlock();
        return -1;
    }
    if (fChannel.FrameStamp() - fLastRun > fMaxRunGap) {
        fMaxRunGap = fChannel.FrameStamp() - fLastRun;
        std::int64_t channel_gap = fChannel.FrameStamp() - channel_stamp;
        jack_log("JackOSSDriver::Write max run gap %lld frames vs channel %lld.", fMaxRunGap, channel_gap);
    }

    // Wait and process channels until write buffer is finished.
    while (!fChannel.PlaybackFinished()) {
        if (!(fChannel.Sleep() && fChannel.CheckTimeAndRun())) {
            fChannel.Unlock();
            return -1;
        }
    }

    if ((fChannel.FrameStamp() / fEngineControl->fBufferSize) % ((5 * fEngineControl->fSampleRate) / fEngineControl->fBufferSize) == 0) {
        fChannel.Playback().log_state(fChannel.FrameStamp());
        fMaxRunGap = 0;
    }

#ifdef JACK_MONITOR
    gCycleTable.fTable[gCycleCount].fBeforeWriteConvert = GetMicroSeconds();
#endif

    for (int i = 0; i < fPlaybackChannels; i++) {
        fSampleBuffers[i] = nullptr;
        if (fGraphManager->GetConnectionsNum(fPlaybackPortList[i]) > 0) {
            fSampleBuffers[i] = GetOutputBuffer(i);
        }
    }
    std::int64_t buffer_end = fCycleEnd + fEngineControl->fBufferSize;
    if (!fChannel.Write(fSampleBuffers, fEngineControl->fBufferSize, buffer_end)) {
        fChannel.Unlock();
        return -1;
    }

#ifdef JACK_MONITOR
    gCycleTable.fTable[gCycleCount].fBeforeWrite = GetMicroSeconds();
#endif

    // Do a processing step here.
    if (!fChannel.CheckTimeAndRun()) {
        fChannel.Unlock();
        return -1;
    }
    fLastRun = fChannel.FrameStamp();

#ifdef JACK_MONITOR
    gCycleTable.fTable[gCycleCount].fAfterWrite = GetMicroSeconds();
    gCycleCount = (gCycleCount == CYCLE_POINTS - 1) ? gCycleCount: gCycleCount + 1;
#endif

    if (!fChannel.Unlock()) {
        return -1;
    }

    return 0;
}

void JackOSSDriver::UpdateLatencies()
{
    // Reimplement from JackAudioDriver. Base latency is smaller, and there's
    // additional latency due to OSS playback buffer management.
    jack_latency_range_t input_range;
    jack_latency_range_t output_range;

    for (int i = 0; i < fCaptureChannels; i++) {
        input_range.max = input_range.min = (fEngineControl->fBufferSize / 2) + fCaptureLatency;
        fGraphManager->GetPort(fCapturePortList[i])->SetLatencyRange(JackCaptureLatency, &input_range);
    }

    for (int i = 0; i < fPlaybackChannels; i++) {
        // TODO: Move this half period to capture latency.
        output_range.max = (fEngineControl->fBufferSize / 2) + fPlaybackLatency;
        // Additional latency introduced by the OSS buffer.
        output_range.max += fNperiods * fEngineControl->fBufferSize;
        // Plus one period if in async mode.
        if (!fEngineControl->fSyncMode) {
            output_range.max += fEngineControl->fBufferSize;
        }
        output_range.min = output_range.max;
        fGraphManager->GetPort(fPlaybackPortList[i])->SetLatencyRange(JackPlaybackLatency, &output_range);
    }
}

int JackOSSDriver::SetBufferSize(jack_nframes_t buffer_size)
{
    // Close and reopen device, we have to adjust the OSS buffer management.
    CloseAux();
    JackAudioDriver::SetBufferSize(buffer_size); // Generic change, never fails
    return OpenAux();
}

} // end of namespace

#ifdef __cplusplus
extern "C"
{
#endif

SERVER_EXPORT jack_driver_desc_t* driver_get_descriptor()
{
    jack_driver_desc_t * desc;
    jack_driver_desc_filler_t filler;
    jack_driver_param_value_t value;

    desc = jack_driver_descriptor_construct("oss", JackDriverMaster, "OSS API based audio backend", &filler);

    value.ui = OSS_DRIVER_DEF_FS;
    jack_driver_descriptor_add_parameter(desc, &filler, "rate", 'r', JackDriverParamUInt, &value, NULL, "Sample rate", NULL);

    value.ui = OSS_DRIVER_DEF_BLKSIZE;
    jack_driver_descriptor_add_parameter(desc, &filler, "period", 'p', JackDriverParamUInt, &value, NULL, "Frames per period", NULL);

    value.ui = OSS_DRIVER_DEF_NPERIODS;
    jack_driver_descriptor_add_parameter(desc, &filler, "nperiods", 'n', JackDriverParamUInt, &value, NULL, "Number of periods to prefill output buffer", NULL);

    value.i = OSS_DRIVER_DEF_BITS;
    jack_driver_descriptor_add_parameter(desc, &filler, "wordlength", 'w', JackDriverParamInt, &value, NULL, "Word length", NULL);

    value.ui = OSS_DRIVER_DEF_INS;
    jack_driver_descriptor_add_parameter(desc, &filler, "inchannels", 'i', JackDriverParamUInt, &value, NULL, "Capture channels", NULL);

    value.ui = OSS_DRIVER_DEF_OUTS;
    jack_driver_descriptor_add_parameter(desc, &filler, "outchannels", 'o', JackDriverParamUInt, &value, NULL, "Playback channels", NULL);

    value.i = false;
    jack_driver_descriptor_add_parameter(desc, &filler, "excl", 'e', JackDriverParamBool, &value, NULL, "Exclusive and direct device access", NULL);

    strcpy(value.str, OSS_DRIVER_DEF_DEV);
    jack_driver_descriptor_add_parameter(desc, &filler, "capture", 'C', JackDriverParamString, &value, NULL, "Input device", NULL);
    jack_driver_descriptor_add_parameter(desc, &filler, "playback", 'P', JackDriverParamString, &value, NULL, "Output device", NULL);
    jack_driver_descriptor_add_parameter(desc, &filler, "device", 'd', JackDriverParamString, &value, NULL, "OSS device name", NULL);

    value.i = false;
    jack_driver_descriptor_add_parameter(desc, &filler, "ignorehwbuf", 'b', JackDriverParamBool, &value, NULL, "Ignore hardware period size", NULL);

    value.ui = 0;
    jack_driver_descriptor_add_parameter(desc, &filler, "input-latency", 'I', JackDriverParamUInt, &value, NULL, "Extra input latency", NULL);
    jack_driver_descriptor_add_parameter(desc, &filler, "output-latency", 'O', JackDriverParamUInt, &value, NULL, "Extra output latency", NULL);

    return desc;
}

SERVER_EXPORT Jack::JackDriverClientInterface* driver_initialize(Jack::JackLockedEngine* engine, Jack::JackSynchro* table, const JSList* params)
{
    int bits = OSS_DRIVER_DEF_BITS;
    jack_nframes_t srate = OSS_DRIVER_DEF_FS;
    jack_nframes_t frames_per_interrupt = OSS_DRIVER_DEF_BLKSIZE;
    const char* capture_pcm_name = OSS_DRIVER_DEF_DEV;
    const char* playback_pcm_name = OSS_DRIVER_DEF_DEV;
    bool capture = false;
    bool playback = false;
    int chan_in = 0;
    int chan_out = 0;
    bool monitor = false;
    bool excl = false;
    unsigned int nperiods = OSS_DRIVER_DEF_NPERIODS;
    const JSList *node;
    const jack_driver_param_t *param;
    bool ignorehwbuf = false;
    jack_nframes_t systemic_input_latency = 0;
    jack_nframes_t systemic_output_latency = 0;

    for (node = params; node; node = jack_slist_next(node)) {

        param = (const jack_driver_param_t *)node->data;

        switch (param->character) {

        case 'r':
            srate = param->value.ui;
            break;

        case 'p':
            frames_per_interrupt = (unsigned int)param->value.ui;
            break;

        case 'n':
            nperiods = (unsigned int)param->value.ui;
            break;

        case 'w':
            bits = param->value.i;
            break;

        case 'i':
            chan_in = (int)param->value.ui;
            break;

        case 'o':
            chan_out = (int)param->value.ui;
            break;

        case 'C':
            capture = true;
            if (strcmp(param->value.str, "none") != 0) {
                capture_pcm_name = param->value.str;
            }
            break;

        case 'P':
            playback = true;
            if (strcmp(param->value.str, "none") != 0) {
                playback_pcm_name = param->value.str;
            }
            break;

        case 'd':
            playback_pcm_name = param->value.str;
            capture_pcm_name = param->value.str;
            break;

        case 'b':
            ignorehwbuf = true;
            break;

        case 'e':
            excl = true;
            break;

        case 'I':
            systemic_input_latency = param->value.ui;
            break;

        case 'O':
            systemic_output_latency = param->value.ui;
            break;
        }
    }

    // duplex is the default
    if (!capture && !playback) {
        capture = true;
        playback = true;
    }

    Jack::JackOSSDriver* oss_driver = new Jack::JackOSSDriver("system", "oss", engine, table);
    Jack::JackDriverClientInterface* threaded_driver = new Jack::JackThreadedDriver(oss_driver);

    // Special open for OSS driver...
    if (oss_driver->Open(frames_per_interrupt, nperiods, srate, capture, playback, chan_in, chan_out,
        excl, monitor, capture_pcm_name, playback_pcm_name, systemic_input_latency, systemic_output_latency, bits, ignorehwbuf) == 0) {
        return threaded_driver;
    } else {
        delete threaded_driver; // Delete the decorated driver
        return NULL;
    }
}

#ifdef __cplusplus
}
#endif
