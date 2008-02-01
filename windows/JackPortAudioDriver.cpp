/*
Copyright (C) 2004-2006 Grame

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

#ifdef WIN32 
#pragma warning (disable : 4786)
#endif

#include "pa_asio.h"
#include "JackDriverLoader.h"
#include "driver_interface.h"
#include "JackPortAudioDriver.h"
#include "JackEngineControl.h"
#include "JackError.h"
#include "JackTime.h"
#include <iostream>
#include <assert.h>

namespace Jack
{

void JackPortAudioDriver::PrintSupportedStandardSampleRates(const PaStreamParameters* inputParameters, const PaStreamParameters* outputParameters)
{
    static double standardSampleRates[] = {
                                              8000.0, 9600.0, 11025.0, 12000.0, 16000.0, 22050.0, 24000.0, 32000.0,
                                              44100.0, 48000.0, 88200.0, 96000.0, 192000.0, -1 /* negative terminated  list */
                                          };
    int i, printCount;
    PaError err;

    printCount = 0;
    for (i = 0; standardSampleRates[i] > 0; i++) {
        err = Pa_IsFormatSupported(inputParameters, outputParameters, standardSampleRates[i]);
        if (err == paFormatIsSupported) {
            if (printCount == 0) {
                printf("\t%8.2f", standardSampleRates[i]);
                printCount = 1;
            } else if (printCount == 4) {
                printf(",\n\t%8.2f", standardSampleRates[i]);
                printCount = 1;
            } else {
                printf(", %8.2f", standardSampleRates[i]);
                ++printCount;
            }
        }
    }
    if (!printCount)
        printf("None\n");
    else
        printf("\n");
}

bool JackPortAudioDriver::GetInputDeviceFromName(const char* name, PaDeviceIndex* device, int* in_max)
{
    const PaDeviceInfo* deviceInfo;
    PaDeviceIndex numDevices = Pa_GetDeviceCount();

    for (int i = 0; i < numDevices; i++) {
        deviceInfo = Pa_GetDeviceInfo(i);
        if (strcmp(name, deviceInfo->name) == 0) {
            *device = i;
            *in_max = deviceInfo->maxInputChannels;
            return true;
        }
	}

    return false;
}

bool JackPortAudioDriver::GetOutputDeviceFromName(const char* name, PaDeviceIndex* device, int* out_max)
{
    const PaDeviceInfo* deviceInfo;
    PaDeviceIndex numDevices = Pa_GetDeviceCount();

    for (int i = 0; i < numDevices; i++) {
        deviceInfo = Pa_GetDeviceInfo(i);
        if (strcmp(name, deviceInfo->name) == 0) {
            *device = i;
            *out_max = deviceInfo->maxOutputChannels;
            return true;
        }
    }

    return false;
}

static void DisplayDeviceNames()
{
    PaError err;
    const PaDeviceInfo* deviceInfo;
    PaStreamParameters inputParameters, outputParameters;
    int defaultDisplayed;

    err = Pa_Initialize();
    if (err != paNoError)
        return ;

    PaDeviceIndex numDevices = Pa_GetDeviceCount();
    printf("Number of devices = %d\n", numDevices);

    for (int i = 0; i < numDevices; i++) {
        deviceInfo = Pa_GetDeviceInfo(i);
        printf( "--------------------------------------- device #%d\n", i );

        /* Mark global and API specific default devices */
        defaultDisplayed = 0;
        if (i == Pa_GetDefaultInputDevice()) {
            printf("[ Default Input");
            defaultDisplayed = 1;
        } else if (i == Pa_GetHostApiInfo(deviceInfo->hostApi)->defaultInputDevice) {
            const PaHostApiInfo *hostInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
            printf("[ Default %s Input", hostInfo->name);
            defaultDisplayed = 1;
        }

        if (i == Pa_GetDefaultOutputDevice()) {
            printf((defaultDisplayed ? "," : "["));
            printf(" Default Output");
            defaultDisplayed = 1;
        } else if (i == Pa_GetHostApiInfo(deviceInfo->hostApi)->defaultOutputDevice) {
            const PaHostApiInfo *hostInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
            printf((defaultDisplayed ? "," : "["));
            printf(" Default %s Output", hostInfo->name);
            defaultDisplayed = 1;
        }

        if (defaultDisplayed)
            printf(" ]\n");

        /* print device info fields */
        printf("Name                        = %s\n", deviceInfo->name);
        printf("Host API                    = %s\n", Pa_GetHostApiInfo(deviceInfo->hostApi)->name);
        printf("Max inputs = %d", deviceInfo->maxInputChannels);
        printf(", Max outputs = %d\n", deviceInfo->maxOutputChannels);
        /*
              printf("Default low input latency   = %8.3f\n", deviceInfo->defaultLowInputLatency);
              printf("Default low output latency  = %8.3f\n", deviceInfo->defaultLowOutputLatency);
              printf("Default high input latency  = %8.3f\n", deviceInfo->defaultHighInputLatency);
              printf("Default high output latency = %8.3f\n", deviceInfo->defaultHighOutputLatency);
        */

#ifdef WIN32
#ifndef PA_NO_ASIO 
        /* ASIO specific latency information */
        if (Pa_GetHostApiInfo(deviceInfo->hostApi)->type == paASIO) {
            long minLatency, maxLatency, preferredLatency, granularity;

            err = PaAsio_GetAvailableLatencyValues(i, &minLatency, &maxLatency, &preferredLatency, &granularity);

            printf("ASIO minimum buffer size    = %ld\n", minLatency);
            printf("ASIO maximum buffer size    = %ld\n", maxLatency);
            printf("ASIO preferred buffer size  = %ld\n", preferredLatency);

            if (granularity == -1)
                printf("ASIO buffer granularity     = power of 2\n");
            else
                printf("ASIO buffer granularity     = %ld\n", granularity);
        }
#endif /* !PA_NO_ASIO */
#endif /* WIN32 */

        printf("Default sample rate         = %8.2f\n", deviceInfo->defaultSampleRate);

        /* poll for standard sample rates */
        inputParameters.device = i;
        inputParameters.channelCount = deviceInfo->maxInputChannels;
        inputParameters.sampleFormat = paInt16;
        inputParameters.suggestedLatency = 0; /* ignored by Pa_IsFormatSupported() */
        inputParameters.hostApiSpecificStreamInfo = NULL;

        outputParameters.device = i;
        outputParameters.channelCount = deviceInfo->maxOutputChannels;
        outputParameters.sampleFormat = paInt16;
        outputParameters.suggestedLatency = 0; /* ignored by Pa_IsFormatSupported() */
        outputParameters.hostApiSpecificStreamInfo = NULL;

        /*
              if (inputParameters.channelCount > 0) {
                  printf("Supported standard sample rates\n for half-duplex 16 bit %d channel input = \n", inputParameters.channelCount);
                  PrintSupportedStandardSampleRates(&inputParameters, NULL);
              }

              if (outputParameters.channelCount > 0) {
                  printf("Supported standard sample rates\n for half-duplex 16 bit %d channel output = \n", outputParameters.channelCount);
                  PrintSupportedStandardSampleRates(NULL, &outputParameters);
              }

              if (inputParameters.channelCount > 0 && outputParameters.channelCount > 0) {
                  printf("Supported standard sample rates\n for full-duplex 16 bit %d channel input, %d channel output = \n",
                          inputParameters.channelCount, outputParameters.channelCount );
                  PrintSupportedStandardSampleRates(&inputParameters, &outputParameters);
              }
        */
    }

    Pa_Terminate();
}

int JackPortAudioDriver::Render(const void* inputBuffer, void* outputBuffer,
                                unsigned long framesPerBuffer,
                                const PaStreamCallbackTimeInfo* timeInfo,
                                PaStreamCallbackFlags statusFlags,
                                void* userData)
{
    JackPortAudioDriver* driver = (JackPortAudioDriver*)userData;
    driver->fLastWaitUst = GetMicroSeconds(); // Take callback date here
    driver->fInputBuffer = (float**)inputBuffer;
    driver->fOutputBuffer = (float**)outputBuffer;
    return (driver->Process() == 0) ? paContinue : paAbort;
}

int JackPortAudioDriver::Read()
{
    for (int i = 0; i < fCaptureChannels; i++) {
        memcpy(GetInputBuffer(i), fInputBuffer[i], sizeof(float) * fEngineControl->fBufferSize);
    }
    return 0;
}

int JackPortAudioDriver::Write()
{
    for (int i = 0; i < fPlaybackChannels; i++) {
        memcpy(fOutputBuffer[i], GetOutputBuffer(i), sizeof(float) * fEngineControl->fBufferSize);
    }
    return 0;
}

int JackPortAudioDriver::Open(jack_nframes_t nframes,
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
    PaError err;
    PaStreamParameters inputParameters;
    PaStreamParameters outputParameters;
    const PaDeviceInfo* deviceInfo;
    int in_max = 0;
    int out_max = 0;

    JackLog("JackPortAudioDriver::Open nframes = %ld in = %ld out = %ld capture name = %s playback name = %s samplerate = %ld\n",
            nframes, inchannels, outchannels, capture_driver_uid, playback_driver_uid, samplerate);

    // Generic JackAudioDriver Open
    if (JackAudioDriver::Open(nframes, samplerate, capturing, playing, inchannels, outchannels, monitor, capture_driver_uid, playback_driver_uid, capture_latency, playback_latency) != 0) {
        return -1;
    }

    err = Pa_Initialize();
    if (err != paNoError) {
        jack_error("JackPortAudioDriver::Pa_Initialize error = %s\n", Pa_GetErrorText(err));
        goto error;
    }

    JackLog("JackPortAudioDriver::Pa_GetDefaultInputDevice %ld\n", Pa_GetDefaultInputDevice());
    JackLog("JackPortAudioDriver::Pa_GetDefaultOutputDevice %ld\n", Pa_GetDefaultOutputDevice());

    if (capturing) {
        if (!GetInputDeviceFromName(capture_driver_uid, &fInputDevice, &in_max)) {
			JackLog("JackPortAudioDriver::GetInputDeviceFromName cannot open %s\n", capture_driver_uid);
            fInputDevice = Pa_GetDefaultInputDevice();
			if (fInputDevice == paNoDevice)
				goto error;
            deviceInfo = Pa_GetDeviceInfo(fInputDevice);
            in_max = deviceInfo->maxInputChannels;
            capture_driver_uid = strdup(deviceInfo->name);
        }
    }

    if (inchannels > in_max) {
        jack_error("This device hasn't required input channels inchannels = %ld in_max = %ld", inchannels, in_max);
        goto error;
    }

    if (playing) {
        if (!GetOutputDeviceFromName(playback_driver_uid, &fOutputDevice, &out_max)) {
            JackLog("JackPortAudioDriver::GetOutputDeviceFromName cannot open %s\n", playback_driver_uid);
			fOutputDevice = Pa_GetDefaultOutputDevice();
			if (fOutputDevice == paNoDevice)
				goto error;
            deviceInfo = Pa_GetDeviceInfo(fOutputDevice);
            out_max = deviceInfo->maxOutputChannels;
            playback_driver_uid = strdup(deviceInfo->name);
        }
    }

    if (outchannels > out_max) {
        jack_error("This device hasn't required output channels outchannels = %ld out_max = %ld", outchannels, out_max);
        goto error;
    }

    if (inchannels == 0) {
        JackLog("JackPortAudioDriver::Open setup max in channels = %ld\n", in_max);
        inchannels = in_max;
    }

    if (outchannels == 0) {
        JackLog("JackPortAudioDriver::Open setup max out channels = %ld\n", out_max);
        outchannels = out_max;
    }

    inputParameters.device = fInputDevice;
    inputParameters.channelCount = inchannels;
    inputParameters.sampleFormat = paFloat32 | paNonInterleaved;		// 32 bit floating point output
    inputParameters.suggestedLatency = (fInputDevice != paNoDevice)		// TODO: check how to setup this on ASIO
                                       ? Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency
                                       : 0;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    outputParameters.device = fOutputDevice;
    outputParameters.channelCount = outchannels;
    outputParameters.sampleFormat = paFloat32 | paNonInterleaved;		// 32 bit floating point output
    outputParameters.suggestedLatency = (fOutputDevice != paNoDevice)	// TODO: check how to setup this on ASIO
                                        ? Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency
                                        : 0;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(&fStream,
                        (fInputDevice == paNoDevice) ? 0 : &inputParameters,
                        (fOutputDevice == paNoDevice) ? 0 : &outputParameters,
                        samplerate,
                        nframes,
                        paNoFlag,  // Clipping is on...
                        Render,
                        this);
    if (err != paNoError) {
        jack_error("Pa_OpenStream error = %s\n", Pa_GetErrorText(err));
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
    Pa_Terminate();
    return -1;
}

int JackPortAudioDriver::Close()
{
    JackAudioDriver::Close();
	JackLog("JackPortAudioDriver::Close\n");
    Pa_CloseStream(fStream);
    Pa_Terminate();
    return 0;
}

int JackPortAudioDriver::Start()
{
    JackLog("JackPortAudioDriver::Start\n");
    JackAudioDriver::Start();
    PaError err = Pa_StartStream(fStream);
    return (err == paNoError) ? 0 : -1;
}

int JackPortAudioDriver::Stop()
{
    JackLog("JackPortAudioDriver::Stop\n");
    PaError err = Pa_StopStream(fStream);
    return (err == paNoError) ? 0 : -1;
}

int JackPortAudioDriver::SetBufferSize(jack_nframes_t buffer_size)
{
    PaError err;
    PaStreamParameters inputParameters;
    PaStreamParameters outputParameters;

    if ((err = Pa_CloseStream(fStream)) != paNoError) {
        jack_error("Pa_CloseStream error = %s\n", Pa_GetErrorText(err));
        return -1;
    }

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
						
    if (err != paNoError) {
        jack_error("Pa_OpenStream error = %s\n", Pa_GetErrorText(err));
        return -1;
    } else {
        // Only done when success
		return JackAudioDriver::SetBufferSize(buffer_size); // never fails
     }
}

} // end of namespace

#ifdef __cplusplus
extern "C"
{
#endif

#include "JackExports.h"

    EXPORT jack_driver_desc_t* driver_get_descriptor() {
        jack_driver_desc_t *desc;
        unsigned int i;
        desc = (jack_driver_desc_t*)calloc(1, sizeof(jack_driver_desc_t));

        strcpy(desc->name, "portaudio");

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
        desc->params[i].value.ui = 128U;
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

    EXPORT Jack::JackDriverClientInterface* driver_initialize(Jack::JackEngine* engine, Jack::JackSynchro** table, const JSList* params) {
        jack_nframes_t srate = 44100;
        jack_nframes_t frames_per_interrupt = 512;
        int capture = FALSE;
        int playback = FALSE;
        int chan_in = 0;
        int chan_out = 0;
        bool monitor = false;
        char* capture_pcm_name = "winmme";
        char* playback_pcm_name = "winmme";
        const JSList *node;
        const jack_driver_param_t *param;
        jack_nframes_t systemic_input_latency = 0;
        jack_nframes_t systemic_output_latency = 0;

        for (node = params; node; node = jack_slist_next(node)) {
            param = (const jack_driver_param_t *) node->data;

            switch (param->character) {

                case 'd':
                    capture_pcm_name = strdup(param->value.str);
                    playback_pcm_name = strdup(param->value.str);
                    break;

                case 'D':
                    capture = TRUE;
                    playback = TRUE;
                    break;

                case 'c':
                    chan_in = chan_out = (int) param->value.ui;
                    break;

                case 'i':
                    chan_in = (int) param->value.ui;
                    break;

                case 'o':
                    chan_out = (int) param->value.ui;
                    break;

                case 'C':
                    capture = TRUE;
                    if (strcmp(param->value.str, "none") != 0) {
                        capture_pcm_name = strdup(param->value.str);
                    }
                    break;

                case 'P':
                    playback = TRUE;
                    if (strcmp(param->value.str, "none") != 0) {
                        playback_pcm_name = strdup(param->value.str);
                    }
                    break;

                case 'm':
                    monitor = param->value.i;
                    break;

                case 'r':
                    srate = param->value.ui;
                    break;

                case 'p':
                    frames_per_interrupt = (unsigned int) param->value.ui;
                    break;

                case 'I':
                    systemic_input_latency = param->value.ui;
                    break;

                case 'O':
                    systemic_output_latency = param->value.ui;
                    break;

                case 'l':
                    Jack::DisplayDeviceNames();
                    break;
            }
        }

        // duplex is the default
        if (!capture && !playback) {
            capture = TRUE;
            playback = TRUE;
        }

        Jack::JackDriverClientInterface* driver = new Jack::JackPortAudioDriver("portaudio", engine, table);
        if (driver->Open(frames_per_interrupt, srate, capture, playback, chan_in, chan_out, monitor, capture_pcm_name, playback_pcm_name, systemic_input_latency, systemic_output_latency) == 0) {
            return driver;
        } else {
            delete driver;
            return NULL;
        }
    }

#ifdef __cplusplus
}
#endif
