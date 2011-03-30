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
#include "JackError.h"
#include "JackTime.h"
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
        // Setup threadded based log function
        set_threaded_log_function();
        driver->CycleTakeBeginTime();
        return (driver->Process() == 0) ? paContinue : paAbort;
    }

    int JackPortAudioDriver::Read()
    {
        for (int i = 0; i < fCaptureChannels; i++)
            memcpy(GetInputBuffer(i), fInputBuffer[i], sizeof(jack_default_audio_sample_t) * fEngineControl->fBufferSize);
        return 0;
    }

    int JackPortAudioDriver::Write()
    {
        for (int i = 0; i < fPlaybackChannels; i++)
            memcpy(fOutputBuffer[i], GetOutputBuffer(i), sizeof(jack_default_audio_sample_t) * fEngineControl->fBufferSize);
        return 0;
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
        PaError err = paNoError;
        PaStreamParameters inputParameters;
        PaStreamParameters outputParameters;
        int in_max = 0;
        int out_max = 0;

        jack_log("JackPortAudioDriver::Open nframes = %ld in = %ld out = %ld capture name = %s playback name = %s samplerate = %ld",
                 buffer_size, inchannels, outchannels, capture_driver_uid, playback_driver_uid, samplerate);

        // Generic JackAudioDriver Open
        if (JackAudioDriver::Open(buffer_size, samplerate, capturing, playing, inchannels, outchannels, monitor, capture_driver_uid, playback_driver_uid, capture_latency, playback_latency) != 0)
            return -1;

        //get devices
        if (capturing)
        {
            if (fPaDevices->GetInputDeviceFromName(capture_driver_uid, fInputDevice, in_max) < 0)
                goto error;
        }
        if (playing)
        {
            if (fPaDevices->GetOutputDeviceFromName(playback_driver_uid, fOutputDevice, out_max) < 0)
                goto error;
        }

        jack_log("JackPortAudioDriver::Open fInputDevice = %d, fOutputDevice %d", fInputDevice, fOutputDevice);

        //default channels number required
        if (inchannels == 0)
        {
            jack_log("JackPortAudioDriver::Open setup max in channels = %ld", in_max);
            inchannels = in_max;
        }
        if (outchannels == 0)
        {
            jack_log("JackPortAudioDriver::Open setup max out channels = %ld", out_max);
            outchannels = out_max;
        }

        //too many channels required, take max available
        if (inchannels > in_max)
        {
            jack_error("This device has only %d available input channels.", in_max);
            inchannels = in_max;
        }
        if (outchannels > out_max)
        {
            jack_error("This device has only %d available output channels.", out_max);
            outchannels = out_max;
        }

        //in/out streams parametering
        inputParameters.device = fInputDevice;
        inputParameters.channelCount = inchannels;
        inputParameters.sampleFormat = paFloat32 | paNonInterleaved;		// 32 bit floating point output
        inputParameters.suggestedLatency = (fInputDevice != paNoDevice)		// TODO: check how to setup this on ASIO
										   ? fPaDevices->GetDeviceInfo(fInputDevice)->defaultLowInputLatency
                                           : 0;
        inputParameters.hostApiSpecificStreamInfo = NULL;

        outputParameters.device = fOutputDevice;
        outputParameters.channelCount = outchannels;
        outputParameters.sampleFormat = paFloat32 | paNonInterleaved;		// 32 bit floating point output
        outputParameters.suggestedLatency = (fOutputDevice != paNoDevice)	// TODO: check how to setup this on ASIO
                                            ? fPaDevices->GetDeviceInfo(fOutputDevice)->defaultLowOutputLatency
                                            : 0;
        outputParameters.hostApiSpecificStreamInfo = NULL;

        err = Pa_OpenStream(&fStream,
                            (fInputDevice == paNoDevice) ? 0 : &inputParameters,
                            (fOutputDevice == paNoDevice) ? 0 : &outputParameters,
                            samplerate,
                            buffer_size,
                            paNoFlag,  // Clipping is on...
                            Render,
                            this);
        if (err != paNoError)
        {
            jack_error("Pa_OpenStream error = %s", Pa_GetErrorText(err));
            goto error;
        }

#ifdef __APPLE__
        fEngineControl->fPeriod = fEngineControl->fPeriodUsecs * 1000;
        fEngineControl->fComputation = 500 * 1000;
        fEngineControl->fConstraint = fEngineControl->fPeriodUsecs * 1000;
#endif

        // Core driver may have changed the in/out values
        fCaptureChannels = inchannels;
        fPlaybackChannels = outchannels;

        assert(strlen(capture_driver_uid) < JACK_CLIENT_NAME_SIZE);
        assert(strlen(playback_driver_uid) < JACK_CLIENT_NAME_SIZE);

        strcpy(fCaptureDriverName, capture_driver_uid);
        strcpy(fPlaybackDriverName, playback_driver_uid);

        return 0;

error:
        JackAudioDriver::Close();
        jack_error("Can't open default PortAudio device : %s", Pa_GetErrorText(err));
        return -1;
    }

    int JackPortAudioDriver::Close()
    {
        // Generic audio driver close
        int res = JackAudioDriver::Close();

        jack_log("JackPortAudioDriver::Close");
        Pa_CloseStream(fStream);
        return res;
    }

    int JackPortAudioDriver::Start()
    {
        jack_log("JackPortAudioDriver::Start");
        if (JackAudioDriver::Start() >= 0) {
            PaError err = Pa_StartStream(fStream);
            if (err == paNoError) {
                return 0;
            }
            JackAudioDriver::Stop();
        }
        return -1;
    }

    int JackPortAudioDriver::Stop()
    {
        jack_log("JackPortAudioDriver::Stop");
        PaError err = Pa_StopStream(fStream);
        int res = (err == paNoError) ? 0 : -1;
        if (JackAudioDriver::Stop() < 0) {
            res = -1;
        }
        return res;
    }

    int JackPortAudioDriver::SetBufferSize(jack_nframes_t buffer_size)
    {
        PaError err;
        PaStreamParameters inputParameters;
        PaStreamParameters outputParameters;

        if ((err = Pa_CloseStream(fStream)) != paNoError)
        {
            jack_error("Pa_CloseStream error = %s", Pa_GetErrorText(err));
            return -1;
        }

        //change parametering
        inputParameters.device = fInputDevice;
        inputParameters.channelCount = fCaptureChannels;
        inputParameters.sampleFormat = paFloat32 | paNonInterleaved;		// 32 bit floating point output
        inputParameters.suggestedLatency = (fInputDevice != paNoDevice)		// TODO: check how to setup this on ASIO
                                           ? Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency
                                           : 0;
        inputParameters.hostApiSpecificStreamInfo = NULL;

        outputParameters.device = fOutputDevice;
        outputParameters.channelCount = fPlaybackChannels;
        outputParameters.sampleFormat = paFloat32 | paNonInterleaved;		// 32 bit floating point output
        outputParameters.suggestedLatency = (fOutputDevice != paNoDevice)	// TODO: check how to setup this on ASIO
                                            ? Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency
                                            : 0;
        outputParameters.hostApiSpecificStreamInfo = NULL;

        err = Pa_OpenStream(&fStream,
                            (fInputDevice == paNoDevice) ? 0 : &inputParameters,
                            (fOutputDevice == paNoDevice) ? 0 : &outputParameters,
                            fEngineControl->fSampleRate,
                            buffer_size,
                            paNoFlag,  // Clipping is on...
                            Render,
                            this);

        if (err != paNoError)
        {
            jack_error("Pa_OpenStream error = %s", Pa_GetErrorText(err));
            return -1;
        }
        else
        {
            // Only done when success
            return JackAudioDriver::SetBufferSize(buffer_size); // never fails
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
        jack_driver_desc_t *desc;
        unsigned int i;
        desc = (jack_driver_desc_t*)calloc(1, sizeof(jack_driver_desc_t));

        strcpy(desc->name, "portaudio");                             // size MUST be less then JACK_DRIVER_NAME_MAX + 1
        strcpy(desc->desc, "PortAudio API based audio backend");     // size MUST be less then JACK_DRIVER_PARAM_DESC + 1

        desc->nparams = 13;
        desc->params = (jack_driver_param_desc_t*)calloc(desc->nparams, sizeof(jack_driver_param_desc_t));

        i = 0;
        strcpy(desc->params[i].name, "channels");
        desc->params[i].character = 'c';
        desc->params[i].type = JackDriverParamInt;
        desc->params[i].value.ui = 0;
        strcpy(desc->params[i].short_desc, "Maximum number of channels");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "inchannels");
        desc->params[i].character = 'i';
        desc->params[i].type = JackDriverParamInt;
        desc->params[i].value.ui = 0;
        strcpy(desc->params[i].short_desc, "Maximum number of input channels");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "outchannels");
        desc->params[i].character = 'o';
        desc->params[i].type = JackDriverParamInt;
        desc->params[i].value.ui = 0;
        strcpy(desc->params[i].short_desc, "Maximum number of output channels");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "capture");
        desc->params[i].character = 'C';
        desc->params[i].type = JackDriverParamString;
        strcpy(desc->params[i].value.str, "will take default PortAudio input device");
        strcpy(desc->params[i].short_desc, "Provide capture ports. Optionally set PortAudio device name");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "playback");
        desc->params[i].character = 'P';
        desc->params[i].type = JackDriverParamString;
        strcpy(desc->params[i].value.str, "will take default PortAudio output device");
        strcpy(desc->params[i].short_desc, "Provide playback ports. Optionally set PortAudio device name");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy (desc->params[i].name, "monitor");
        desc->params[i].character = 'm';
        desc->params[i].type = JackDriverParamBool;
        desc->params[i].value.i = 0;
        strcpy(desc->params[i].short_desc, "Provide monitor ports for the output");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "duplex");
        desc->params[i].character = 'D';
        desc->params[i].type = JackDriverParamBool;
        desc->params[i].value.i = TRUE;
        strcpy(desc->params[i].short_desc, "Provide both capture and playback ports");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "rate");
        desc->params[i].character = 'r';
        desc->params[i].type = JackDriverParamUInt;
        desc->params[i].value.ui = 44100U;
        strcpy(desc->params[i].short_desc, "Sample rate");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "period");
        desc->params[i].character = 'p';
        desc->params[i].type = JackDriverParamUInt;
        desc->params[i].value.ui = 128U;
        strcpy(desc->params[i].short_desc, "Frames per period");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "device");
        desc->params[i].character = 'd';
        desc->params[i].type = JackDriverParamString;
        strcpy(desc->params[i].value.str, "will take default PortAudio device name");
        strcpy(desc->params[i].short_desc, "PortAudio device name");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "input-latency");
        desc->params[i].character = 'I';
        desc->params[i].type = JackDriverParamUInt;
        desc->params[i].value.i = 0;
        strcpy(desc->params[i].short_desc, "Extra input latency");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "output-latency");
        desc->params[i].character = 'O';
        desc->params[i].type = JackDriverParamUInt;
        desc->params[i].value.i = 0;
        strcpy(desc->params[i].short_desc, "Extra output latency");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "list-devices");
        desc->params[i].character = 'l';
        desc->params[i].type = JackDriverParamBool;
        desc->params[i].value.i = TRUE;
        strcpy(desc->params[i].short_desc, "Display available PortAudio devices");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

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

            switch (param->character)
            {

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
                break;
            }
        }

        // duplex is the default
        if (!capture && !playback) {
            capture = true;
            playback = true;
        }

        Jack::JackDriverClientInterface* driver = new Jack::JackPortAudioDriver("system", "portaudio", engine, table, pa_devices);
        if (driver->Open(frames_per_interrupt, srate, capture, playback, chan_in, chan_out, monitor, capture_pcm_name, playback_pcm_name, systemic_input_latency, systemic_output_latency) == 0)
        {
            return driver;
        }
        else
        {
            delete driver;
            return NULL;
        }
    }

#ifdef __cplusplus
}
#endif
