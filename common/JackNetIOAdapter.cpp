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

#include "JackNetIOAdapter.h"
#include "JackError.h"
#include "JackExports.h"
#include <stdio.h>
#include <assert.h>

using namespace std;

namespace Jack
{

JackNetIOAdapter::JackNetIOAdapter(jack_client_t* jack_client, 
                                    JackIOAdapterInterface* audio_io, 
                                    int input, 
                                    int output)
{
    int i;
    char name[32];
    fJackClient = jack_client;
    fCaptureChannels = input;
    fPlaybackChannels = output;
    fIOAdapter = audio_io;
    
    fCapturePortList = new jack_port_t* [fCaptureChannels];
    fPlaybackPortList = new jack_port_t* [fPlaybackChannels];
   
    for (i = 0; i < fCaptureChannels; i++) {
        sprintf(name, "capture_%d", i+1);
        if ((fCapturePortList[i] = jack_port_register(fJackClient, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0)) == NULL) 
            goto fail;
    }
    
    for (i = 0; i < fPlaybackChannels; i++) {
        sprintf(name, "playback_%d", i+1);
        if ((fPlaybackPortList[i] = jack_port_register(fJackClient, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0)) == NULL) 
            goto fail;
    }
    
    return;
        
fail:
     FreePorts();
}

JackNetIOAdapter::~JackNetIOAdapter()
{
    // When called, Close has already been used for the client, thus ports are already unregistered.
    delete fIOAdapter;
}

void JackNetIOAdapter::FreePorts()
{
    int i;
    
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

int JackNetIOAdapter::Open()
{
    return fIOAdapter->Open();
}

int JackNetIOAdapter::Close()
{
    return fIOAdapter->Close();
}

} //namespace

static Jack::JackNetIOAdapter* adapter = NULL;

#ifdef __cplusplus
extern "C"
{
#endif

#include "JackCoreAudioIOAdapter.h"
#include "JackPortAudioIOAdapter.h"
#include "JackCallbackNetIOAdapter.h"

	EXPORT int jack_initialize(jack_client_t* jack_client, const char* load_init)
	{
		if (adapter) {
			jack_error("NetAudio Adapter already loaded");
			return 1;
		} else {
			jack_log("Loading NetAudio Adapter");
            adapter = new Jack::JackCallbackNetIOAdapter(jack_client, 
                new Jack::JackCoreAudioIOAdapter(2, 2, jack_get_buffer_size(jack_client), jack_get_sample_rate(jack_client)), 2, 2);
            //adapter = new Jack::JackCallbackNetIOAdapter(jack_client, 
            //    new Jack::JackPortAudioIOAdapter(2, 2, jack_get_buffer_size(jack_client), jack_get_sample_rate(jack_client)), 2, 2);
            assert(adapter);
            
            if (adapter->Open() == 0) {
                return 0;
            } else {
                delete adapter;
                adapter = NULL;
                return 1;
            }
		}
	}

	EXPORT void jack_finish(void* arg)
	{
		if (adapter) {
			jack_log("Unloading NetAudio Adapter");
            adapter->Close();
			delete adapter;
			adapter = NULL;
		}
	}
    
#ifdef __cplusplus
}
#endif
