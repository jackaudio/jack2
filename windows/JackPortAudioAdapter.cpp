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

#include "JackPortAudioAdapter.h"
#include "portaudio.h"
#include "JackError.h"

namespace Jack
{

int JackPortAudioAdapter::Render(const void* inputBuffer, void* outputBuffer,
                                unsigned long framesPerBuffer,
                                const PaStreamCallbackTimeInfo* timeInfo,
                                PaStreamCallbackFlags statusFlags,
                                void* userData)
{
    JackPortAudioAdapter* adapter = static_cast<JackPortAudioAdapter*>(userData);
    float** paBuffer;
    float* buffer;
    bool failure = false;
    
    jack_nframes_t time1, time2; 
    adapter->ResampleFactor(time1, time2);
    
    paBuffer = (float**)inputBuffer;
    for (int i = 0; i < adapter->fCaptureChannels; i++) {
        buffer = (float*)paBuffer[i];
        adapter->fCaptureRingBuffer[i]->SetRatio(time1, time2);
        if (adapter->fCaptureRingBuffer[i]->WriteResample(buffer, framesPerBuffer) < framesPerBuffer)
            failure = true;
    }
    
    paBuffer = (float**)outputBuffer;
    for (int i = 0; i < adapter->fPlaybackChannels; i++) {
        buffer = (float*)paBuffer[i];
        adapter->fPlaybackRingBuffer[i]->SetRatio(time2, time1);
        if (adapter->fPlaybackRingBuffer[i]->ReadResample(buffer, framesPerBuffer) < framesPerBuffer)
            failure = true;
    }
    
#ifdef DEBUG    
    adapter->fTable.Write(time1, time2, double(time1) / double(time2), double(time2) / double(time1), 
         adapter->fCaptureRingBuffer[0]->ReadSpace(),  adapter->fPlaybackRingBuffer[0]->WriteSpace());
#endif

    // Reset all ringbuffers in case of failure
    if (failure) {
        jack_error("JackPortAudioAdapter::Render ringbuffer failure... reset");
        adapter->ResetRingBuffers();
    }
    return paContinue;
}
        
int JackPortAudioAdapter::Open()
{
    PaError err;
    PaStreamParameters inputParameters;
    PaStreamParameters outputParameters;
    PaDeviceIndex inputDevice;
    PaDeviceIndex outputDevice;
    
    if (JackAudioAdapterInterface::Open() < 0)
        return -1;
    
    err = Pa_Initialize();
    if (err != paNoError) {
        jack_error("JackPortAudioAdapter::Pa_Initialize error = %s\n", Pa_GetErrorText(err));
        goto error;
    }
    
    jack_log("JackPortAudioAdapter::Pa_GetDefaultInputDevice %ld", Pa_GetDefaultInputDevice());
    jack_log("JackPortAudioAdapter::Pa_GetDefaultOutputDevice %ld", Pa_GetDefaultOutputDevice());
    jack_log("JackPortAudioAdapter::Open fBufferSize = %ld fSampleRate %f", fBufferSize, fSampleRate);

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
    jack_log("JackPortAudioAdapter::Open OK");
    return 0;
     
error:
    Pa_Terminate();
    return -1;
}

int JackPortAudioAdapter::Close()
{
#ifdef DEBUG
    fTable.Save();
#endif
    jack_log("JackPortAudioAdapter::Close");
    Pa_StopStream(fStream);
    jack_log("JackPortAudioAdapter:: Pa_StopStream");
    Pa_CloseStream(fStream);
    jack_log("JackPortAudioAdapter:: Pa_CloseStream");
    Pa_Terminate();
    jack_log("JackPortAudioAdapter:: Pa_Terminate");
    return JackAudioAdapterInterface::Close();
}

int JackPortAudioAdapter::SetBufferSize(jack_nframes_t buffer_size)
{
    JackAudioAdapterInterface::SetBufferSize(buffer_size);
    Close();
    return Open();
}

} // namespace