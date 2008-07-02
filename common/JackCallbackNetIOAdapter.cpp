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

#include "JackCallbackNetIOAdapter.h"
#include "JackLibSampleRateResampler.h"
#include "JackError.h"
#include "JackExports.h"
#include <stdio.h>
#include <assert.h>

using namespace std;

namespace Jack
{

int JackCallbackNetIOAdapter::Process(jack_nframes_t frames, void* arg)
{
    JackCallbackNetIOAdapter* adapter = static_cast<JackCallbackNetIOAdapter*>(arg);
    float* buffer;
    int i;
    
    if (!adapter->fIOAdapter->IsRunning())
        return 0;
        
    // DLL
    adapter->fIOAdapter->SetCallbackTime(jack_get_time());
     
    for (i = 0; i < adapter->fCaptureChannels; i++) {
        buffer = static_cast<float*>(jack_port_get_buffer(adapter->fCapturePortList[i], frames));
        adapter->fCaptureRingBuffer[i]->Read(buffer, frames);
    }
    
    for (i = 0; i < adapter->fPlaybackChannels; i++) {
        buffer = static_cast<float*>(jack_port_get_buffer(adapter->fPlaybackPortList[i], frames));
        adapter->fPlaybackRingBuffer[i]->Write(buffer, frames);
    }
     
    return 0;
}

int JackCallbackNetIOAdapter::BufferSize(jack_nframes_t nframes, void *arg)
{
     JackCallbackNetIOAdapter* adapter = static_cast<JackCallbackNetIOAdapter*>(arg);
     adapter->fIOAdapter->SetBufferSize(nframes);
     return 0;
}

JackCallbackNetIOAdapter::JackCallbackNetIOAdapter(jack_client_t* jack_client, 
                                                JackIOAdapterInterface* audio_io, 
                                                int input, 
                                                int output)
                                                : JackNetIOAdapter(jack_client, audio_io, input, output)
{
    int i;
    fCurCallbackTime = 0;
    fLastCallbackTime = 0;
    
    fCaptureRingBuffer = new JackResampler*[fCaptureChannels];
    fPlaybackRingBuffer = new JackResampler*[fPlaybackChannels];
    
    for (i = 0; i < fCaptureChannels; i++) {
        fCaptureRingBuffer[i] = new JackLibSampleRateResampler();
    }
    
    for (i = 0; i < fPlaybackChannels; i++) {
        fPlaybackRingBuffer[i] = new JackLibSampleRateResampler();
   }
 
    fIOAdapter->SetRingBuffers(fCaptureRingBuffer, fPlaybackRingBuffer);
       
    jack_log("ReadSpace = %ld", fCaptureRingBuffer[0]->ReadSpace());
    jack_log("WriteSpace = %ld", fPlaybackRingBuffer[0]->WriteSpace());
 
    if (jack_set_process_callback(fJackClient, Process, this) < 0)
        goto fail;
        
    if (jack_set_buffer_size_callback(fJackClient, BufferSize, this) < 0)
        goto fail;
    
    if (jack_activate(fJackClient) < 0)
        goto fail;
       
    return;
        
fail:
     FreePorts();
}

JackCallbackNetIOAdapter::~JackCallbackNetIOAdapter()
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
