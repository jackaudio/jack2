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
    
    if (!adapter->fIOAdapter->IsRunning())
        return 0;
    
    for (i = 0; i < adapter->fCaptureChannels; i++) {
    
        buffer = static_cast<char*>(jack_port_get_buffer(adapter->fCapturePortList[i], frames));
        size_t len = jack_ringbuffer_read_space(adapter->fCaptureRingBuffer);
        
        if (len < frames * sizeof(float)) {
            jack_error("JackCallbackNetIOAdapter::Process : consumer too slow, skip frames = %d", (frames * sizeof(float)) - len);
            jack_ringbuffer_read(adapter->fCaptureRingBuffer, buffer, len);
        } else {
            jack_ringbuffer_read(adapter->fCaptureRingBuffer, buffer, frames * sizeof(float));
        }
    }
    
    for (i = 0; i < adapter->fPlaybackChannels; i++) {
    
        buffer = static_cast<char*>(jack_port_get_buffer(adapter->fPlaybackPortList[i], frames));
        size_t len = jack_ringbuffer_write_space(adapter->fPlaybackRingBuffer);
        
         if (len < frames * sizeof(float)) {
            jack_error("JackCallbackNetIOAdapter::Process : producer too slow, missing frames = %d", (frames * sizeof(float)) - len);
            jack_ringbuffer_write(adapter->fPlaybackRingBuffer, buffer, len);
        } else {
            jack_ringbuffer_write(adapter->fPlaybackRingBuffer, buffer, frames * sizeof(float));
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
   
    fCaptureRingBuffer = jack_ringbuffer_create(fCaptureChannels * sizeof(float) * DEFAULT_RB_SIZE);
    if (fCaptureRingBuffer == NULL)
        goto fail;

    fPlaybackRingBuffer = jack_ringbuffer_create(fPlaybackChannels * sizeof(float) * DEFAULT_RB_SIZE);
    if (fPlaybackRingBuffer == NULL)
        goto fail;

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
    if (fCaptureRingBuffer)
        jack_ringbuffer_free(fCaptureRingBuffer);
    
    if (fPlaybackRingBuffer)
        jack_ringbuffer_free(fPlaybackRingBuffer);
}

} //namespace
