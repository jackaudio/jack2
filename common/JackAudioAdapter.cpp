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

#include "JackAudioAdapter.h"
#include "JackError.h"
#include "JackExports.h"
#include "JackTools.h"
#include "jslist.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

using namespace std;

namespace Jack
{

JackAudioAdapter::JackAudioAdapter(jack_client_t* jack_client, 
                                    JackAudioAdapterInterface* audio_io)
{
    int i;
    char name[32];
    fJackClient = jack_client;
    fCaptureChannels = audio_io->GetInputs();
    fPlaybackChannels = audio_io->GetOutputs();
    fAudioAdapter = audio_io;
    
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

JackAudioAdapter::~JackAudioAdapter()
{
    // When called, Close has already been used for the client, thus ports are already unregistered.
    delete fAudioAdapter;
}

void JackAudioAdapter::FreePorts()
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

int JackAudioAdapter::Open()
{
    return fAudioAdapter->Open();
}

int JackAudioAdapter::Close()
{
    return fAudioAdapter->Close();
}

} //namespace

#ifdef __cplusplus
extern "C"
{
#endif

#include "JackCallbackAudioAdapter.h"
#include "driver_interface.h"

#ifdef __linux__
#include "JackAlsaAdapter.h"
#endif

#ifdef __APPLE__
#include "JackCoreAudioAdapter.h"
#endif

#ifdef WIN32
#include "JackPortAudioAdapter.h"
#endif

using namespace Jack;

    EXPORT int jack_internal_initialize(jack_client_t* jack_client, const JSList* params)
    {
        Jack::JackAudioAdapter* adapter;
        jack_log("Loading audioadapter");
         
    #ifdef __linux__
        adapter = new Jack::JackCallbackAudioAdapter(jack_client, 
            new Jack::JackAlsaAdapter(jack_get_buffer_size(jack_client), jack_get_sample_rate(jack_client), params));
    #endif

    #ifdef WIN32
        adapter = new Jack::JackCallbackAudioAdapter(jack_client, 
            new Jack::JackPortAudioAdapter(jack_get_buffer_size(jack_client), jack_get_sample_rate(jack_client), params));
    #endif
        
    #ifdef __APPLE__
        adapter = new Jack::JackCallbackAudioAdapter(jack_client, 
            new Jack::JackCoreAudioAdapter(jack_get_buffer_size(jack_client), jack_get_sample_rate(jack_client), params));
    #endif
        
        assert(adapter);
        
        if (adapter->Open() == 0) {
            return 0;
        } else {
            delete adapter;
            return 1;
        }
    }

	EXPORT int jack_initialize(jack_client_t* jack_client, const char* load_init)
	{
        JSList* params = NULL;
        jack_driver_desc_t *desc = jack_get_descriptor();
        JackArgParser parser(load_init);
        
        if (parser.GetArgc() > 0) {
            if (jack_parse_driver_params(desc, parser.GetArgc(), (char**)parser.GetArgv(), &params) != 0) 
                jack_error("Internal client jack_parse_driver_params error");
        }
         
        return jack_internal_initialize(jack_client, params);
    }

	EXPORT void jack_finish(void* arg)
	{
        Jack::JackCallbackAudioAdapter* adapter = static_cast<Jack::JackCallbackAudioAdapter*>(arg); 
        
      	if (adapter) {
			jack_log("Unloading audioadapter");
            adapter->Close();
			delete adapter;
		}
	}
    
#ifdef __cplusplus
}
#endif
