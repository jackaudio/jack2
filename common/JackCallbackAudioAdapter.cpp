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

#include "JackCallbackAudioAdapter.h"
#include "JackLibSampleRateResampler.h"
#include "JackError.h"
#include "JackExports.h"
#include <stdio.h>
#include <assert.h>

using namespace std;

namespace Jack
{

int JackCallbackAudioAdapter::Process(jack_nframes_t frames, void* arg)
{
    JackCallbackAudioAdapter* adapter = static_cast<JackCallbackAudioAdapter*>(arg);
    float* buffer;
    bool failure = false;
    int i;
    
    if (!adapter->fAudioAdapter->IsRunning())
        return 0;
        
    // DLL
    adapter->fAudioAdapter->SetCallbackTime(jack_get_time());
     
    // Push/pull from ringbuffer
    for (i = 0; i < adapter->fCaptureChannels; i++) {
        buffer = static_cast<float*>(jack_port_get_buffer(adapter->fCapturePortList[i], frames));
        if (adapter->fCaptureRingBuffer[i]->Read(buffer, frames) < frames) 
            failure = true;
    }
    
    for (i = 0; i < adapter->fPlaybackChannels; i++) {
        buffer = static_cast<float*>(jack_port_get_buffer(adapter->fPlaybackPortList[i], frames));
        if (adapter->fPlaybackRingBuffer[i]->Write(buffer, frames) < frames)
            failure = true;
    }
     
    // Reset all ringbuffers in case of failure
    if (failure) {
        jack_error("JackCallbackAudioAdapter::Process ringbuffer failure... reset");
        adapter->Reset();
    }
    return 0;
}

int JackCallbackAudioAdapter::BufferSize(jack_nframes_t buffer_size, void* arg)
{
    JackCallbackAudioAdapter* adapter = static_cast<JackCallbackAudioAdapter*>(arg);
    adapter->Reset();
    adapter->fAudioAdapter->SetBufferSize(buffer_size);
    return 0;
}

int JackCallbackAudioAdapter::SampleRate(jack_nframes_t sample_rate, void* arg)
{
    JackCallbackAudioAdapter* adapter = static_cast<JackCallbackAudioAdapter*>(arg);
    adapter->Reset();
    adapter->fAudioAdapter->SetSampleRate(sample_rate);
    return 0;
}

void JackCallbackAudioAdapter::Reset()
{
    int i;
        
    for (i = 0; i < fCaptureChannels; i++) {
        fCaptureRingBuffer[i]->Reset();
    }
    
    for (i = 0; i < fPlaybackChannels; i++) {
        fPlaybackRingBuffer[i]->Reset();
    }
        
    fAudioAdapter->Reset();
}

JackCallbackAudioAdapter::JackCallbackAudioAdapter(jack_client_t* jack_client, JackAudioAdapterInterface* audio_io)
                                                : JackAudioAdapter(jack_client, audio_io)
{
    int i;
     
    fCaptureRingBuffer = new JackResampler*[fCaptureChannels];
    fPlaybackRingBuffer = new JackResampler*[fPlaybackChannels];
    
    for (i = 0; i < fCaptureChannels; i++) {
        fCaptureRingBuffer[i] = new JackLibSampleRateResampler();
    }
    
    for (i = 0; i < fPlaybackChannels; i++) {
        fPlaybackRingBuffer[i] = new JackLibSampleRateResampler();
   }
 
    fAudioAdapter->SetRingBuffers(fCaptureRingBuffer, fPlaybackRingBuffer);
       
    jack_log("ReadSpace = %ld", fCaptureRingBuffer[0]->ReadSpace());
    jack_log("WriteSpace = %ld", fPlaybackRingBuffer[0]->WriteSpace());
 
    if (jack_set_process_callback(fJackClient, Process, this) < 0)
        goto fail;
        
    if (jack_set_buffer_size_callback(fJackClient, BufferSize, this) < 0)
        goto fail;
        
    if (jack_set_sample_rate_callback(fJackClient, SampleRate, this) < 0)
        goto fail;
    
    if (jack_activate(fJackClient) < 0)
        goto fail;
       
    return;
        
fail:
     FreePorts();
}

JackCallbackAudioAdapter::~JackCallbackAudioAdapter()
{
    int i;
    
    for (i = 0; i < fCaptureChannels; i++) {
       delete(fCaptureRingBuffer[i]);
    }
    
    for (i = 0; i < fPlaybackChannels; i++) {
         delete(fPlaybackRingBuffer[i]);
    }
    
    delete[] fCaptureRingBuffer;
    delete[] fPlaybackRingBuffer;
}

} //namespace
