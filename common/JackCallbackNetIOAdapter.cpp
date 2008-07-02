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
#include "JackError.h"
#include "JackExports.h"
#include <stdio.h>
#include <assert.h>

using namespace std;

namespace Jack
{

#define DEFAULT_RB_SIZE 16384	

int JackCallbackNetIOAdapter::Process(jack_nframes_t frames, void* arg)
{
    JackCallbackNetIOAdapter* adapter = static_cast<JackCallbackNetIOAdapter*>(arg);
    char* buffer;
    int i;
    
    adapter->fLastCallbackTime = adapter->fCurCallbackTime;
    adapter->fCurCallbackTime = jack_get_time();
    
    if (!adapter->fIOAdapter->IsRunning())
        return 0;
        
    adapter->fIOAdapter->SetCallbackDeltaTime(adapter->fCurCallbackTime - adapter->fLastCallbackTime);
    
    for (i = 0; i < adapter->fCaptureChannels; i++) {
    
        buffer = static_cast<char*>(jack_port_get_buffer(adapter->fCapturePortList[i], frames));
        size_t len = jack_ringbuffer_read_space(adapter->fCaptureRingBuffer[i]);
        
        jack_log("JackCallbackNetIOAdapter::Process INPUT available = %ld", len / sizeof(float));
        
        if (len < frames * sizeof(float)) {
            jack_error("JackCallbackNetIOAdapter::Process : producer too slow, skip frames = %d", frames);
            //jack_ringbuffer_read(adapter->fCaptureRingBuffer, buffer, len);
        } else {
            jack_ringbuffer_read(adapter->fCaptureRingBuffer[i], buffer, frames * sizeof(float));
        }
    }
    
    for (i = 0; i < adapter->fPlaybackChannels; i++) {
    
        buffer = static_cast<char*>(jack_port_get_buffer(adapter->fPlaybackPortList[i], frames));
        size_t len = jack_ringbuffer_write_space(adapter->fPlaybackRingBuffer[i]);
        
        jack_log("JackCallbackNetIOAdapter::Process OUTPUT available = %ld", len / sizeof(float));
        
         if (len < frames * sizeof(float)) {
            jack_error("JackCallbackNetIOAdapter::Process : consumer too slow, missing frames = %d", frames);
            //jack_ringbuffer_write(adapter->fPlaybackRingBuffer, buffer, len);
        } else {
            jack_ringbuffer_write(adapter->fPlaybackRingBuffer[i], buffer, frames * sizeof(float));
        }
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
    
    fCurCallbackTime = 0;
    fLastCallbackTime = 0;
    
    fCaptureRingBuffer = new jack_ringbuffer_t*[fCaptureChannels];
    fPlaybackRingBuffer = new jack_ringbuffer_t*[fPlaybackChannels];
    
    for (int i = 0; i < fCaptureChannels; i++) {
        fCaptureRingBuffer[i] = jack_ringbuffer_create(sizeof(float) * DEFAULT_RB_SIZE);
        if (fCaptureRingBuffer[i] == NULL)
            goto fail;
    }

    for (int i = 0; i < fPlaybackChannels; i++) {
        fPlaybackRingBuffer[i] = jack_ringbuffer_create(sizeof(float) * DEFAULT_RB_SIZE);
        if (fPlaybackRingBuffer[i] == NULL)
            goto fail;
    }

    fIOAdapter->SetRingBuffers(fCaptureRingBuffer, fPlaybackRingBuffer);
     
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
    for (int i = 0; i < fCaptureChannels; i++) {
        if (fCaptureRingBuffer[i])
            jack_ringbuffer_free(fCaptureRingBuffer[i]);
    }

    for (int i = 0; i < fPlaybackChannels; i++) {
        if (fPlaybackRingBuffer[i])
            jack_ringbuffer_free(fPlaybackRingBuffer[i]);
    }
        
    delete[] fCaptureRingBuffer;
    delete[] fPlaybackRingBuffer;
}

} //namespace
