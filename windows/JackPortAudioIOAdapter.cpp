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
    
    if (!adapter->fRunning) {
        adapter->fRunning = true;
        jack_time_t time = jack_get_time();
        adapter->fProducerDLL.Init(time);
        adapter->fConsumerDLL.Init(time);
    }
           
    // DLL
    jack_time_t time = jack_get_time();
    adapter->fProducerDLL.IncFrame(time);
    jack_nframes_t time1 = adapter->fConsumerDLL.Time2Frames(time);
    jack_nframes_t time2 = adapter->fProducerDLL.Time2Frames(time);
     
    double src_ratio_output = double(time2) / double(time1);
    double src_ratio_input = double(time1) / double(time2);
   
    if (src_ratio_input < 0.7f || src_ratio_input > 1.3f) {
        jack_error("src_ratio_input = %f", src_ratio_input);
        src_ratio_input = 1;
        time1 = 1;
        time2 = 1;
    }
    
    if (src_ratio_output < 0.7f || src_ratio_output > 1.3f) {
        jack_error("src_ratio_output = %f", src_ratio_output);
        src_ratio_output = 1;
        time1 = 1;
        time2 = 1;
    }  
    
    src_ratio_input = Range(0.7f, 1.3f, src_ratio_input);
    src_ratio_output = Range(0.7f, 1.3f, src_ratio_output);
    jack_log("Callback resampler src_ratio_input = %f src_ratio_output = %f", src_ratio_input, src_ratio_output);
  
    paBuffer = (float**)inputBuffer;
    for (int i = 0; i < adapter->fCaptureChannels; i++) {
        buffer = (float*)paBuffer[i];
        adapter->fCaptureRingBuffer[i]->SetRatio(time1, time2);
        adapter->fCaptureRingBuffer[i]->WriteResample(buffer, framesPerBuffer);
     }
    
    paBuffer = (float**)outputBuffer;
    for (int i = 0; i < adapter->fPlaybackChannels; i++) {
        buffer = (float*)paBuffer[i];
        adapter->fPlaybackRingBuffer[i]->SetRatio(time2, time1);
        adapter->fPlaybackRingBuffer[i]->ReadResample(buffer, framesPerBuffer); 
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

int JackPortAudioIOAdapter::SetBufferSize(int buffer_size)
{
    JackIOAdapterInterface::SetBufferSize(buffer_size);
    Close();
    return Open();
}

} // namespace