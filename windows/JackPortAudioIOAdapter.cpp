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
    float* buffer;
    
    adapter->fLastCallbackTime = adapter->fCurCallbackTime;
    adapter->fCurCallbackTime = jack_get_time();
    
    adapter->fConsumerFilter.AddValue(adapter->fCurCallbackTime - adapter->fLastCallbackTime);
    adapter->fProducerFilter.AddValue(adapter->fDeltaTime);
    
    jack_log("JackPortAudioIOAdapter::Render delta %ld", adapter->fCurCallbackTime - adapter->fLastCallbackTime);
     
    if (!adapter->fRunning) 
        adapter->fRunning = true;
           
    /*
    double src_ratio_output = double(adapter->fCurCallbackTime - adapter->fLastCallbackTime) / double(adapter->fDeltaTime);
    double src_ratio_input = double(adapter->fDeltaTime) / double(adapter->fCurCallbackTime - adapter->fLastCallbackTime);
    */
    jack_time_t val2 = adapter->fConsumerFilter.GetVal();
    jack_time_t val1 = adapter->fProducerFilter.GetVal();
    double src_ratio_output = double(val1) / double(val2);
    double src_ratio_input = double(val2) / double(val1);
  
    if (src_ratio_input < 0.8f || src_ratio_input > 1.2f) {
        jack_error("src_ratio_input = %f", src_ratio_input);
        src_ratio_input = 1;
    }
    
    
    if (src_ratio_output < 0.8f || src_ratio_output > 1.2f) {
        jack_error("src_ratio_output = %f", src_ratio_output);
        src_ratio_output = 1;
    }  
    src_ratio_input = Range(0.8f, 1.2f, src_ratio_input);
    src_ratio_output = Range(0.8f, 1.2f, src_ratio_output);
    
    //jack_log("Callback resampler src_ratio_input = %f src_ratio_output = %f", src_ratio_input, src_ratio_output);
    printf("Callback resampler src_ratio_input = %f src_ratio_output = %f\n", src_ratio_input, src_ratio_output);
    
    paBuffer = (float**)inputBuffer;
    for (int i = 0; i < adapter->fCaptureChannels; i++) {
        buffer = (float*)paBuffer[i];
        adapter->fCaptureRingBuffer[i].SetRatio(src_ratio_input);
        //adapter->fCaptureRingBuffer[i].WriteResample(buffer, framesPerBuffer);
        adapter->fCaptureRingBuffer[i].Write(buffer, framesPerBuffer);
    }
    
    paBuffer = (float**)outputBuffer;
    for (int i = 0; i < adapter->fPlaybackChannels; i++) {
        buffer = (float*)paBuffer[i];
        adapter->fPlaybackRingBuffer[i].SetRatio(src_ratio_output);
        //adapter->fPlaybackRingBuffer[i].ReadResample(buffer, framesPerBuffer); 
        adapter->fPlaybackRingBuffer[i].Read(buffer, framesPerBuffer);    
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
    
    if (JackIOAdapterInterface::Open() < 0)
        return -1;
    
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
    
    return JackIOAdapterInterface::Close();
}

void JackPortAudioIOAdapter::SetBufferSize(int buffer_size)
{
    JackIOAdapterInterface::SetBufferSize(buffer_size);
    Close();
    Open();
}

} // namespace