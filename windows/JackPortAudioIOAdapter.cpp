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

#include "JackPortAudioIOAdapter.h"
#include "portaudio.h"
#include "JackError.h"

namespace Jack
{

int JackPortAudioIOAdapter::Render(const void* inputBuffer, void* outputBuffer,
                                unsigned long framesPerBuffer,
                                const PaStreamCallbackTimeInfo* timeInfo,
                                PaStreamCallbackFlags statusFlags,
                                void* userData)
{
    JackPortAudioIOAdapter* adapter = static_cast<JackPortAudioIOAdapter*>(userData);
    float** paBuffer;
    char* buffer;
    
    jack_log("JackPortAudioIOAdapter::Render");
    
    paBuffer = (float**)inputBuffer;
    for (int i = 0; i < adapter->fCaptureChannels; i++) {
        
        buffer = (char*)paBuffer[i];
        size_t len = jack_ringbuffer_write_space(adapter->fCaptureRingBuffer);
        
        if (len < framesPerBuffer * sizeof(float)) {
            jack_error("JackPortAudioIOAdapter::Process : producer too slow, skip frames...");
            jack_ringbuffer_write(adapter->fCaptureRingBuffer, buffer, len);
        } else {
            jack_ringbuffer_write(adapter->fCaptureRingBuffer, buffer, framesPerBuffer * sizeof(float));
        }
    }
    
    paBuffer = (float**)outputBuffer;
    for (int i = 0; i < adapter->fPlaybackChannels; i++) {
        
        buffer =  (char*)paBuffer[i];
        size_t len = jack_ringbuffer_read_space(adapter->fPlaybackRingBuffer);
         
        if (len < framesPerBuffer * sizeof(float)) {
            jack_error("JackPortAudioIOAdapter::Process : consumer too slow, skip frames...");
            jack_ringbuffer_read(adapter->fPlaybackRingBuffer, buffer, len);
        } else {
            jack_ringbuffer_read(adapter->fPlaybackRingBuffer, buffer, framesPerBuffer * sizeof(float));
        }
    }
    
    return paContinue;
}
        
int JackPortAudioIOAdapter::Open()
{
    PaError err;
    PaStreamParameters inputParameters;
    PaStreamParameters outputParameters;
    PaDeviceIndex inputDevice;
    PaDeviceIndex outputDevice;
    
    err = Pa_Initialize();
    if (err != paNoError) {
        jack_error("JackPortAudioIOAdapter::Pa_Initialize error = %s\n", Pa_GetErrorText(err));
        goto error;
    }
    
    jack_log("JackPortAudioIOAdapter::Pa_GetDefaultInputDevice %ld", Pa_GetDefaultInputDevice());
    jack_log("JackPortAudioIOAdapter::Pa_GetDefaultOutputDevice %ld", Pa_GetDefaultOutputDevice());
    
    jack_log("JackPortAudioIOAdapter::Open fBufferSize = %ld fSampleRate %f", fBufferSize, fSampleRate);

    inputDevice = Pa_GetDefaultInputDevice();
    outputDevice = Pa_GetDefaultOutputDevice();
    
    inputParameters.device = inputDevice;
    inputParameters.channelCount = fCaptureChannels;
    inputParameters.sampleFormat = paFloat32 | paNonInterleaved;		// 32 bit floating point output
    inputParameters.suggestedLatency = (inputDevice != paNoDevice)		// TODO: check how to setup this on ASIO
                                       ? Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency
                                       : 0;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    outputParameters.device = outputDevice;
    outputParameters.channelCount = fPlaybackChannels;
    outputParameters.sampleFormat = paFloat32 | paNonInterleaved;		// 32 bit floating point output
    outputParameters.suggestedLatency = (outputDevice != paNoDevice)	// TODO: check how to setup this on ASIO
                                        ? Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency
                                        : 0;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(&fStream,
                        (inputDevice == paNoDevice) ? 0 : &inputParameters,
                        (outputDevice == paNoDevice) ? 0 : &outputParameters,
                        fSampleRate,
                        fBufferSize,
                        paNoFlag,  // Clipping is on...
                        Render,
                        this);
    if (err != paNoError) {
        jack_error("Pa_OpenStream error = %s", Pa_GetErrorText(err));
        goto error;
    }
    
    err = Pa_StartStream(fStream);
    if (err != paNoError) {
         jack_error("Pa_StartStream error = %s", Pa_GetErrorText(err));
         goto error;
    }
    jack_log("JackPortAudioIOAdapter::Open OK");
    return 0;
     
error:
    Pa_Terminate();
    return -1;
}

int JackPortAudioIOAdapter::Close()
{
    jack_log("JackPortAudioIOAdapter::Close");
    Pa_StopStream(fStream);
    jack_log("JackPortAudioIOAdapter:: Pa_StopStream");
    Pa_CloseStream(fStream);
    jack_log("JackPortAudioIOAdapter:: Pa_CloseStream");
    Pa_Terminate();
    jack_log("JackPortAudioIOAdapter:: Pa_Terminate");
    return 0;
}

} // namespace