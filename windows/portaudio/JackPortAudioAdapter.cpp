/*
Copyright (C) 2008 Grame

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

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "JackPortAudioAdapter.h"
#include "JackError.h"

namespace Jack
{

    int JackPortAudioAdapter::Render(const void* inputBuffer,
                                    void* outputBuffer,
                                    unsigned long framesPerBuffer,
                                    const PaStreamCallbackTimeInfo* timeInfo,
                                    PaStreamCallbackFlags statusFlags,
                                    void* userData)
    {
        JackPortAudioAdapter* adapter = static_cast<JackPortAudioAdapter*>(userData);
        adapter->PushAndPull((jack_default_audio_sample_t**)inputBuffer, (jack_default_audio_sample_t**)outputBuffer, framesPerBuffer);
        return paContinue;
    }

    JackPortAudioAdapter::JackPortAudioAdapter ( jack_nframes_t buffer_size, jack_nframes_t sample_rate, const JSList* params )
            : JackAudioAdapterInterface ( buffer_size, sample_rate )
    {
        jack_log ( "JackPortAudioAdapter::JackPortAudioAdapter buffer_size = %d, sample_rate = %d", buffer_size, sample_rate );

        const JSList* node;
        const jack_driver_param_t* param;
        int in_max = 0;
        int out_max = 0;

        fInputDevice = Pa_GetDefaultInputDevice();
        fOutputDevice = Pa_GetDefaultOutputDevice();

        for (node = params; node; node = jack_slist_next(node))
        {
            param = (const jack_driver_param_t*) node->data;

            switch (param->character)
            {
            case 'i' :
                fCaptureChannels = param->value.ui;
                break;
            case 'o' :
                fPlaybackChannels = param->value.ui;
                break;
            case 'C' :
                if ( fPaDevices.GetInputDeviceFromName(param->value.str, fInputDevice, in_max) < 0 )
                {
                    jack_error ( "Can't use %s, taking default input device", param->value.str );
                    fInputDevice = Pa_GetDefaultInputDevice();
                }
                break;
            case 'P' :
                if ( fPaDevices.GetOutputDeviceFromName(param->value.str, fOutputDevice, out_max) < 0 )
                {
                    jack_error ( "Can't use %s, taking default output device", param->value.str );
                    fOutputDevice = Pa_GetDefaultOutputDevice();
                }
                break;
            case 'r' :
                SetAdaptedSampleRate ( param->value.ui );
                break;
            case 'p' :
                SetAdaptedBufferSize ( param->value.ui );
                break;
            case 'd' :
                if ( fPaDevices.GetInputDeviceFromName ( param->value.str, fInputDevice, in_max ) < 0 )
                    jack_error ( "Can't use %s, taking default input device", param->value.str );
                if ( fPaDevices.GetOutputDeviceFromName ( param->value.str, fOutputDevice, out_max ) < 0 )
                    jack_error ( "Can't use %s, taking default output device", param->value.str );
                break;
            case 'l' :
                fPaDevices.DisplayDevicesNames();
                break;
            case 'q':
                fQuality = param->value.ui;
                break;
            case 'g':
                fRingbufferCurSize = param->value.ui;
                fAdaptative = false;
                break;
            }
        }

        //max channels
        if ( in_max == 0 )
            in_max = fPaDevices.GetDeviceInfo ( fInputDevice )->maxInputChannels;
        if ( out_max == 0 )
            out_max = fPaDevices.GetDeviceInfo ( fOutputDevice )->maxOutputChannels;

        //effective channels
        if ( ( fCaptureChannels == 0 ) || ( fCaptureChannels > in_max ) )
            fCaptureChannels = in_max;
        if ( ( fPlaybackChannels == 0 ) || ( fPlaybackChannels > out_max ) )
            fPlaybackChannels = out_max;

        //set adapter interface channels
        SetInputs ( fCaptureChannels );
        SetOutputs ( fPlaybackChannels );
    }

    int JackPortAudioAdapter::Open()
    {
        PaError err;
        PaStreamParameters inputParameters;
        PaStreamParameters outputParameters;

        if ( JackAudioAdapterInterface::Open() < 0 )
            return -1;

        jack_log("JackPortAudioAdapter::Open fInputDevice = %d DeviceName %s", fInputDevice, fPaDevices.GetFullName(fInputDevice).c_str());
        jack_log("JackPortAudioAdapter::Open fOutputDevice = %d DeviceName %s", fOutputDevice, fPaDevices.GetFullName(fOutputDevice).c_str());
        jack_log("JackPortAudioAdapter::Open fAdaptedBufferSize = %u fAdaptedSampleRate %u", fAdaptedBufferSize, fAdaptedSampleRate);

        inputParameters.device = fInputDevice;
        inputParameters.channelCount = fCaptureChannels;
        inputParameters.sampleFormat = paFloat32 | paNonInterleaved;		// 32 bit floating point output
        inputParameters.suggestedLatency = ( fInputDevice != paNoDevice )   // TODO: check how to setup this on ASIO
                                           ? fPaDevices.GetDeviceInfo(fInputDevice)->defaultLowInputLatency
                                           : 0;
        inputParameters.hostApiSpecificStreamInfo = NULL;

        outputParameters.device = fOutputDevice;
        outputParameters.channelCount = fPlaybackChannels;
        outputParameters.sampleFormat = paFloat32 | paNonInterleaved;		// 32 bit floating point output
        outputParameters.suggestedLatency = ( fOutputDevice != paNoDevice )	// TODO: check how to setup this on ASIO
                                            ? fPaDevices.GetDeviceInfo(fOutputDevice)->defaultLowOutputLatency
                                            : 0;
        outputParameters.hostApiSpecificStreamInfo = NULL;

        err = Pa_OpenStream( &fStream,
                            ( fInputDevice == paNoDevice ) ? 0 : &inputParameters,
                            ( fOutputDevice == paNoDevice ) ? 0 : &outputParameters,
                            fAdaptedSampleRate,
                            fAdaptedBufferSize,
                            paNoFlag,  // Clipping is on...
                            Render,
                            this );

        if ( err != paNoError )
        {
            jack_error ( "Pa_OpenStream error = %s", Pa_GetErrorText ( err ) );
            return -1;
        }

        err = Pa_StartStream ( fStream );

        if ( err != paNoError )
        {
            jack_error ( "Pa_StartStream error = %s", Pa_GetErrorText ( err ) );
            return -1;
        }

        jack_log ( "JackPortAudioAdapter::Open OK" );
        return 0;
    }

    int JackPortAudioAdapter::Close()
    {
#ifdef JACK_MONITOR
        fTable.Save(fHostBufferSize, fHostSampleRate, fAdaptedSampleRate, fAdaptedBufferSize);
#endif
        jack_log ( "JackPortAudioAdapter::Close" );
        Pa_StopStream ( fStream );
        jack_log ( "JackPortAudioAdapter:: Pa_StopStream" );
        Pa_CloseStream ( fStream );
        jack_log ( "JackPortAudioAdapter:: Pa_CloseStream" );
        return JackAudioAdapterInterface::Close();
    }

    int JackPortAudioAdapter::SetSampleRate ( jack_nframes_t sample_rate )
    {
        JackAudioAdapterInterface::SetHostSampleRate ( sample_rate );
        Close();
        return Open();
    }

    int JackPortAudioAdapter::SetBufferSize ( jack_nframes_t buffer_size )
    {
        JackAudioAdapterInterface::SetHostBufferSize ( buffer_size );
        Close();
        return Open();
    }

} // namespace

#ifdef __cplusplus
extern "C"
{
#endif

    SERVER_EXPORT jack_driver_desc_t* jack_get_descriptor()
    {
        jack_driver_desc_t *desc;
        unsigned int i;
        desc = (jack_driver_desc_t*)calloc(1, sizeof(jack_driver_desc_t));

        strcpy(desc->name, "audioadapter");                            // size MUST be less then JACK_DRIVER_NAME_MAX + 1
        strcpy(desc->desc, "netjack audio <==> net backend adapter");  // size MUST be less then JACK_DRIVER_PARAM_DESC + 1

        desc->nparams = 10;
        desc->params = (jack_driver_param_desc_t*)calloc(desc->nparams, sizeof(jack_driver_param_desc_t));

        i = 0;
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
        strcpy(desc->params[i].value.str, "default input device");
        strcpy(desc->params[i].short_desc, "Provide capture ports. Optionally set PortAudio device name");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "playback");
        desc->params[i].character = 'P';
        desc->params[i].type = JackDriverParamString;
        strcpy(desc->params[i].value.str, "default output device");
        strcpy(desc->params[i].short_desc, "Provide playback ports. Optionally set PortAudio device name");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "rate");
        desc->params[i].character = 'r';
        desc->params[i].type = JackDriverParamUInt;
        desc->params[i].value.ui = 44100U;
        strcpy(desc->params[i].short_desc, "Sample rate");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "periodsize");
        desc->params[i].character = 'p';
        desc->params[i].type = JackDriverParamUInt;
        desc->params[i].value.ui = 512U;
        strcpy(desc->params[i].short_desc, "Period size");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "device");
        desc->params[i].character = 'd';
        desc->params[i].type = JackDriverParamString;
        desc->params[i].value.ui = 128U;
        strcpy(desc->params[i].value.str, "default device");
        strcpy(desc->params[i].short_desc, "PortAudio device name");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "list-devices");
        desc->params[i].character = 'l';
        desc->params[i].type = JackDriverParamBool;
        desc->params[i].value.i = true;
        strcpy(desc->params[i].short_desc, "Display available PortAudio devices");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "quality");
        desc->params[i].character = 'q';
        desc->params[i].type = JackDriverParamInt;
        desc->params[i].value.ui = 0;
        strcpy(desc->params[i].short_desc, "Resample algorithm quality (0 - 4)");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "ring-buffer");
        desc->params[i].character = 'g';
        desc->params[i].type = JackDriverParamInt;
        desc->params[i].value.ui = 32768;
        strcpy(desc->params[i].short_desc, "Fixed ringbuffer size");
        strcpy(desc->params[i].long_desc, "Fixed ringbuffer size (if not set => automatic adaptative)");

        return desc;
    }

#ifdef __cplusplus
}
#endif
