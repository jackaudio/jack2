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

#include "JackNetAudioAdapter.h"
#include "JackError.h"
#include "JackExports.h"
#include <stdio.h>

using namespace std;

namespace Jack
{

#define DEFAULT_RB_SIZE 16384		/* ringbuffer size in frames */

int JackNetAudioAdapter::Process(jack_nframes_t frames, void* arg)
{
    JackNetAudioAdapter* adapter = static_cast<JackNetAudioAdapter*>(arg);
    char* buffer;
    int i;
    
    for (i = 0; i < adapter->fCaptureChannels; i++) {
    
        buffer = static_cast<char*>(jack_port_get_buffer(adapter->fCapturePortList[i], frames));
        size_t len = jack_ringbuffer_write_space(adapter->fCaptureRingBuffer);
        
        if (len <  frames * sizeof(float)) {
            jack_error("JackNetAudioAdapter::Process : consumer too slow, skip frames...");
            jack_ringbuffer_write(adapter->fCaptureRingBuffer, buffer, len);
        } else {
            jack_ringbuffer_write(adapter->fCaptureRingBuffer, buffer, frames * sizeof(float));
        }
    }
    
    for (i = 0; i < adapter->fPlaybackChannels; i++) {
    
        buffer = static_cast<char*>(jack_port_get_buffer(adapter->fPlaybackPortList[i], frames));
        size_t len = jack_ringbuffer_read_space(adapter->fPlaybackRingBuffer);
        
        if (len <  frames * sizeof(float)) {
            jack_error("JackNetAudioAdapter::Process : producer too slow, missing frames...");
            jack_ringbuffer_read(adapter->fPlaybackRingBuffer, buffer, len);
        } else {
            jack_ringbuffer_read(adapter->fPlaybackRingBuffer, buffer, frames * sizeof(float));
        }
    }
     
    return 0;
}

JackNetAudioAdapter::JackNetAudioAdapter(jack_client_t* jack_client)
{
    int i;
    char name[32];
    fJackClient = jack_client;
    fCaptureChannels = 2;
    fPlaybackChannels = 2;
    
    fCapturePortList = new jack_port_t* [fCaptureChannels];
    fPlaybackPortList = new jack_port_t* [fPlaybackChannels];
    
    fCaptureRingBuffer = jack_ringbuffer_create(fCaptureChannels * sizeof(float) * DEFAULT_RB_SIZE);
    if (fCaptureRingBuffer == NULL)
        goto fail;

    fPlaybackRingBuffer = jack_ringbuffer_create(fPlaybackChannels * sizeof(float) * DEFAULT_RB_SIZE);
    if (fPlaybackRingBuffer == NULL)
        goto fail;

    for (i = 0; i < fCaptureChannels; i++) {
        sprintf(name, "in_%d", i+1);
        if ((fCapturePortList[i] = jack_port_register(fJackClient, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0)) == NULL) 
            goto fail;
    }
    
    for (i = 0; i < fPlaybackChannels; i++) {
        sprintf(name, "out_%d", i+1);
        if ((fPlaybackPortList[i] = jack_port_register(fJackClient, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0)) == NULL) 
            goto fail;
    }
          
    if (jack_set_process_callback(fJackClient, Process, this) < 0)
        goto fail;
    
    if (jack_activate(fJackClient) < 0)
        goto fail;
       
    return;
        
fail:
     FreePorts();
}

JackNetAudioAdapter::~JackNetAudioAdapter()
{
    FreePorts();
}

void JackNetAudioAdapter::FreePorts()
{
    int i;
    
    if (fCaptureRingBuffer)
        jack_ringbuffer_free(fCaptureRingBuffer);
    
    if (fPlaybackRingBuffer)
        jack_ringbuffer_free(fPlaybackRingBuffer);
    
    for (i = 0; i < fCaptureChannels; i++) {
        if (fCapturePortList[i])
            jack_port_unregister(fJackClient, fCapturePortList[i]);
    }
    
    for (i = 0; i < fCaptureChannels; i++) {
        if (fPlaybackPortList[i])
            jack_port_unregister(fJackClient, fPlaybackPortList[i]);
    }

    delete[] fCapturePortList;
    delete[] fPlaybackPortList;
}

} //namespace

static Jack::JackNetAudioAdapter* adapter = NULL;

#ifdef __cplusplus
extern "C"
{
#endif

	EXPORT int jack_initialize(jack_client_t* jack_client, const char* load_init)
	{
		if (adapter) {
			jack_error("NetAudio Adapter already loaded");
			return 1;
		} else {
			jack_log("Loading NetAudio Adapter");
			adapter = new Jack::JackNetAudioAdapter(jack_client);
			return (adapter) ? 0 : 1;
		}
	}

	EXPORT void jack_finish(void* arg)
	{
		if (adapter) {
			jack_log("Unloading  NetAudio Adapter");
			delete adapter;
			adapter = NULL;
		}
	}
    
#ifdef __cplusplus
}
#endif
