/*
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

#include "JackDriverLoader.h"
#include "driver_interface.h"
#include "JackPortAudioDriver.h"
#include "JackEngineControl.h"
#include "JackGraphManager.h"
#include "JackError.h"
#include "JackTime.h"
#include "JackTools.h"
#include "JackCompilerDeps.h"
#include <iostream>
#include <assert.h>

using namespace std;

namespace Jack
{

int JackPortAudioDriver::Render(const void* inputBuffer, void* outputBuffer,
                                unsigned long framesPerBuffer,
                                const PaStreamCallbackTimeInfo* timeInfo,
                                PaStreamCallbackFlags statusFlags,
                                void* userData)
{
    JackPortAudioDriver* driver = (JackPortAudioDriver*)userData;
    driver->fInputBuffer = (jack_default_audio_sample_t**)inputBuffer;
    driver->fOutputBuffer = (jack_default_audio_sample_t**)outputBuffer;

    //MMCSSAcquireRealTime(GetCurrentThread());

    if (statusFlags) {
        if (statusFlags & paOutputUnderflow)
            jack_error("JackPortAudioDriver::Render paOutputUnderflow");
        if (statusFlags & paInputUnderflow)
            jack_error("JackPortAudioDriver::Render paInputUnderflow");
        if (statusFlags & paOutputOverflow)
            jack_error("JackPortAudioDriver::Render paOutputOverflow");
        if (statusFlags & paInputOverflow)
            jack_error("JackPortAudioDriver::Render paInputOverflow");
        if (statusFlags & paPrimingOutput)
            jack_error("JackPortAudioDriver::Render paOutputUnderflow");

        if (statusFlags != paPrimingOutput) {
            jack_time_t cur_time = GetMicroSeconds();
            driver->NotifyXRun(cur_time, float(cur_time - driver->fBeginDateUst));   // Better this value than nothing...
        }
    }

    // Setup threadded based log function
    set_threaded_log_function();
    driver->CycleTakeBeginTime();
    return (driver->Process() == 0) ? paContinue : paAbort;
}

int JackPortAudioDriver::Read()
{
    for (int i = 0; i < fCaptureChannels; i++) {
        memcpy(GetInputBuffer(i), fInputBuffer[i], sizeof(jack_default_audio_sample_t) * fEngineControl->fBufferSize);
    }
    return 0;
}

int JackPortAudioDriver::Write()
{
    for (int i = 0; i < fPlaybackChannels; i++) {
        memcpy(fOutputBuffer[i], GetOutputBuffer(i), sizeof(jack_default_audio_sample_t) * fEngineControl->fBufferSize);
    }
    return 0;
}

PaError JackPortAudioDriver::OpenStream(jack_nframes_t buffer_size)
{
    PaStreamParameters inputParameters;
    PaStreamParameters outputParameters;

    jack_log("JackPortAudioDriver::OpenStream buffer_size = %d", buffer_size);

    // Update parameters
    inputParameters.device = fInputDevice;
    inputParameters.channelCount = fCaptureChannels;
    inputParameters.sampleFormat = paFloat32 | paNonInterleaved;		// 32 bit floating point output
    inputParameters.suggestedLatency = (fInputDevice != paNoDevice)		// TODO: check how to setup this on ASIO
                                       ? ((fPaDevices->GetHostFromDevice(fInputDevice) == "ASIO") ? 0 : Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency)
                                       : 0;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    outputParameters.device = fOutputDevice;
    outputParameters.channelCount = fPlaybackChannels;
    outputParameters.sampleFormat = paFloat32 | paNonInterleaved;       // 32 bit floating point output
    outputParameters.suggestedLatency = (fOutputDevice != paNoDevice)   // TODO: check how to setup this on ASIO
                                        ? ((fPaDevices->GetHostFromDevice(fOutputDevice) == "ASIO") ? 0 : Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency)
                                        : 0;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    return Pa_OpenStream(&fStream,
                        (fInputDevice == paNoDevice) ? 0 : &inputParameters,
                        (fOutputDevice == paNoDevice) ? 0 : &outputParameters,
                        fEngineControl->fSampleRate,
                        buffer_size,
                        paNoFlag,  // Clipping is on...
                        Render,
                        this);
}

void JackPortAudioDriver::UpdateLatencies()
{
    jack_latency_range_t input_range;
    jack_latency_range_t output_range;
    jack_latency_range_t monitor_range;

    const PaStreamInfo* info = Pa_GetStreamInfo(fStream);
    assert(info);

    for (int i = 0; i < fCaptureChannels; i++) {
        input_range.max = input_range.min = fEngineControl->fBufferSize + (info->inputLatency * fEngineControl->fSampleRate) + fCaptureLatency;
        fGraphManager->GetPort(fCapturePortList[i])->SetLatencyRange(JackCaptureLatency, &input_range);
    }

    for (int i = 0; i < fPlaybackChannels; i++) {
        output_range.max = output_range.min = (info->outputLatency * fEngineControl->fSampleRate) + fPlaybackLatency;
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

int JackPortAudioDriver::Open(jack_nframes_t buffer_size,
                              jack_nframes_t samplerate,
                              bool capturing,
                              bool playing,
                              int inchannels,
                              int outchannels,
                              bool monitor,
                              const char* capture_driver_uid,
                              const char* playback_driver_uid,
                              jack_nframes_t capture_latency,
                              jack_nframes_t playback_latency)
{
    int in_max = 0;
    int out_max = 0;
    PaError err = paNoError;

    fCaptureLatency = capture_latency;
    fPlaybackLatency = playback_latency;
    
    jack_log("JackPortAudioDriver::Open nframes = %ld in = %ld out = %ld capture name = %s playback name = %s samplerate = %ld",
             buffer_size, inchannels, outchannels, capture_driver_uid, playback_driver_uid, samplerate);

    // Get devices
    if (capturing) {
        if (fPaDevices->GetInputDeviceFromName(capture_driver_uid, fInputDevice, in_max) < 0) {
            goto error;
        }
    }
    if (playing) {
        if (fPaDevices->GetOutputDeviceFromName(playback_driver_uid, fOutputDevice, out_max) < 0) {
            goto error;
        }
    }
    
    // If ASIO, request for preferred size (assuming fInputDevice and fOutputDevice are the same)
    if (buffer_size == 0) { 
        buffer_size = fPaDevices->GetPreferredBufferSize(fInputDevice);
        jack_log("JackPortAudioDriver::Open preferred buffer_size = %d", buffer_size);
    }
  
    // Generic JackAudioDriver Open
    if (JackAudioDriver::Open(buffer_size, samplerate, capturing, playing, inchannels, outchannels, monitor,
        capture_driver_uid, playback_driver_uid, capture_latency, playback_latency) != 0) {
        return -1;
    }

    jack_log("JackPortAudioDriver::Open fInputDevice = %d, fOutputDevice %d", fInputDevice, fOutputDevice);

    // Default channels number required
    if (inchannels == 0) {
        jack_log("JackPortAudioDriver::Open setup max in channels = %ld", in_max);
        inchannels = in_max;
    }
    if (outchannels == 0) {
        jack_log("JackPortAudioDriver::Open setup max out channels = %ld", out_max);
        outchannels = out_max;
    }

    // Too many channels required, take max available
    if (inchannels > in_max) {
        jack_error("This device has only %d available input channels.", in_max);
        inchannels = in_max;
    }
    if (outchannels > out_max) {
        jack_error("This device has only %d available output channels.", out_max);
        outchannels = out_max;
    }

    // Core driver may have changed the in/out values
    fCaptureChannels = inchannels;
    fPlaybackChannels = outchannels;

    err = OpenStream(buffer_size);
    if (err != paNoError) {
        jack_error("Pa_OpenStream error %d = %s", err, Pa_GetErrorText(err));
        goto error;
    }

#ifdef __APPLE__
    fEngineControl->fPeriod = fEngineControl->fPeriodUsecs * 1000;
    fEngineControl->fComputation = JackTools::ComputationMicroSec(fEngineControl->fBufferSize) * 1000;
    fEngineControl->fConstraint = fEngineControl->fPeriodUsecs * 1000;
#endif

    assert(strlen(capture_driver_uid) < JACK_CLIENT_NAME_SIZE);
    assert(strlen(playback_driver_uid) < JACK_CLIENT_NAME_SIZE);

    strcpy(fCaptureDriverName, capture_driver_uid);
    strcpy(fPlaybackDriverName, playback_driver_uid);

    return 0;

error:
    JackAudioDriver::Close();
    jack_error("Can't open default PortAudio device");
    return -1;
}

int JackPortAudioDriver::Close()
{
    // Generic audio driver close
    jack_log("JackPortAudioDriver::Close");
    int res = JackAudioDriver::Close();
    return (Pa_CloseStream(fStream) != paNoError) ? -1 : res;
}

int JackPortAudioDriver::Attach()
{
    if (JackAudioDriver::Attach() == 0) {

        const char* alias;

        if (fInputDevice != paNoDevice && fPaDevices->GetHostFromDevice(fInputDevice) == "ASIO") {
            for (int i = 0; i < fCaptureChannels; i++) {
                if (PaAsio_GetInputChannelName(fInputDevice, i, &alias) == paNoError) {
                    JackPort* port = fGraphManager->GetPort(fCapturePortList[i]);
                    port->SetAlias(alias);
                }
            }
        }

        if (fOutputDevice != paNoDevice && fPaDevices->GetHostFromDevice(fOutputDevice) == "ASIO") {
            for (int i = 0; i < fPlaybackChannels; i++) {
                if (PaAsio_GetOutputChannelName(fOutputDevice, i, &alias) == paNoError) {
                    JackPort* port = fGraphManager->GetPort(fPlaybackPortList[i]);
                    port->SetAlias(alias);
                }
            }
        }

        return 0;

    } else {
        return -1;
    }
}

int JackPortAudioDriver::Start()
{
    jack_log("JackPortAudioDriver::Start");
    if (JackAudioDriver::Start() >= 0) {
        if (Pa_StartStream(fStream) == paNoError) {
            return 0;
        }
        JackAudioDriver::Stop();
    }
    return -1;
}

int JackPortAudioDriver::Stop()
{
    jack_log("JackPortAudioDriver::Stop");
    int res = (Pa_StopStream(fStream) == paNoError) ? 0 : -1;
    if (JackAudioDriver::Stop() < 0) {
        res = -1;
    }
    return res;
}

int JackPortAudioDriver::SetBufferSize(jack_nframes_t buffer_size)
{
    PaError err;

    if ((err = Pa_CloseStream(fStream)) != paNoError) {
        jack_error("Pa_CloseStream error = %s", Pa_GetErrorText(err));
        return -1;
    }

    err = OpenStream(buffer_size);
    if (err != paNoError) {
        jack_error("Pa_OpenStream error %d = %s", err, Pa_GetErrorText(err));
        return -1;
    } else {
        JackAudioDriver::SetBufferSize(buffer_size); // Generic change, never fails
        return 0;
    }
}

} // end of namespace

#ifdef __cplusplus
extern "C"
{
#endif

#include "JackCompilerDeps.h"

    SERVER_EXPORT jack_driver_desc_t* driver_get_descriptor()
    {
        jack_driver_desc_t * desc;
        jack_driver_desc_filler_t filler;
        jack_driver_param_value_t value;

        desc = jack_driver_descriptor_construct("portaudio", JackDriverMaster, "PortAudio API based audio backend", &filler);

        value.ui = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "channels", 'c', JackDriverParamUInt, &value, NULL, "Maximum number of channels", NULL);
        jack_driver_descriptor_add_parameter(desc, &filler, "inchannels", 'i', JackDriverParamUInt, &value, NULL, "Maximum number of input channels", NULL);
        jack_driver_descriptor_add_parameter(desc, &filler, "outchannels", 'o', JackDriverParamUInt, &value, NULL, "Maximum number of output channels", NULL);

        jack_driver_descriptor_add_parameter(desc, &filler, "capture", 'C', JackDriverParamString, &value, NULL, "Provide capture ports. Optionally set PortAudio device name", NULL);

        jack_driver_descriptor_add_parameter(desc, &filler, "playback", 'P', JackDriverParamString, &value, NULL, "Provide playback ports. Optionally set PortAudio device name", NULL);

        value.i = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "monitor", 'm', JackDriverParamBool, &value, NULL, "Provide monitor ports for the output", NULL);

        value.i = TRUE;
        jack_driver_descriptor_add_parameter(desc, &filler, "duplex", 'D', JackDriverParamBool, &value, NULL, "Provide both capture and playback ports", NULL);

        value.ui = 44100U;
        jack_driver_descriptor_add_parameter(desc, &filler, "rate", 'r', JackDriverParamUInt, &value, NULL, "Sample rate", NULL);

        value.ui = 512U;
        jack_driver_descriptor_add_parameter(desc, &filler, "period", 'p', JackDriverParamUInt, &value, NULL, "Frames per period", "Frames per period. If 0 and ASIO driver, will take preferred value");

        jack_driver_descriptor_add_parameter(desc, &filler, "device", 'd', JackDriverParamString, &value, NULL, "PortAudio device name", NULL);

        value.ui = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "input-latency", 'I', JackDriverParamUInt, &value, NULL, "Extra input latency", NULL);
        jack_driver_descriptor_add_parameter(desc, &filler, "output-latency", 'O', JackDriverParamUInt, &value, NULL, "Extra output latency", NULL);

        value.i = TRUE;
        jack_driver_descriptor_add_parameter(desc, &filler, "list-devices", 'l', JackDriverParamBool, &value, NULL, "Display available PortAudio devices", NULL);

        return desc;
    }

    SERVER_EXPORT Jack::JackDriverClientInterface* driver_initialize(Jack::JackLockedEngine* engine, Jack::JackSynchro* table, const JSList* params)
    {
        jack_nframes_t srate = 44100;
        jack_nframes_t frames_per_interrupt = 512;
        const char* capture_pcm_name = "";
        const char* playback_pcm_name = "";
        bool capture = false;
        bool playback = false;
        int chan_in = 0;
        int chan_out = 0;
        bool monitor = false;
        const JSList *node;
        const jack_driver_param_t *param;
        jack_nframes_t systemic_input_latency = 0;
        jack_nframes_t systemic_output_latency = 0;
        PortAudioDevices* pa_devices = new PortAudioDevices();

        for (node = params; node; node = jack_slist_next(node))
        {
            param = (const jack_driver_param_t *) node->data;

            switch (param->character) {

            case 'd':
                capture_pcm_name = param->value.str;
                playback_pcm_name = param->value.str;
                break;

            case 'D':
                capture = true;
                playback = true;
                break;

            case 'c':
                chan_in = chan_out = (int)param->value.ui;
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
                playback = TRUE;
                if (strcmp(param->value.str, "none") != 0) {
                    playback_pcm_name = param->value.str;
                }
                break;

            case 'm':
                monitor = param->value.i;
                break;

            case 'r':
                srate = param->value.ui;
                break;

            case 'p':
                frames_per_interrupt = (unsigned int)param->value.ui;
                break;

            case 'I':
                systemic_input_latency = param->value.ui;
                break;

            case 'O':
                systemic_output_latency = param->value.ui;
                break;

            case 'l':
                pa_devices->DisplayDevicesNames();
                // Stops the server in this case
                return NULL;
            }
        }

        // duplex is the default
        if (!capture && !playback) {
            capture = true;
            playback = true;
        }

        Jack::JackDriverClientInterface* driver = new Jack::JackPortAudioDriver("system", "portaudio", engine, table, pa_devices);
        if (driver->Open(frames_per_interrupt, srate, capture, playback,
            chan_in, chan_out, monitor, capture_pcm_name,
            playback_pcm_name, systemic_input_latency,
            systemic_output_latency) == 0) {
            return driver;
        } else {
            delete driver;
            return NULL;
        }
    }

#ifdef __cplusplus
}
#endif
