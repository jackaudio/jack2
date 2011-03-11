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
#include "JackCompilerDeps.h"
#include "JackTools.h"
#include "JackTime.h"
#include "jslist.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

using namespace std;

namespace Jack
{

//static methods ***********************************************************
    int JackAudioAdapter::Process (jack_nframes_t frames, void* arg)
    {
        JackAudioAdapter* adapter = static_cast<JackAudioAdapter*>(arg);
        jack_default_audio_sample_t* inputBuffer[adapter->fAudioAdapter->GetInputs()];
        jack_default_audio_sample_t* outputBuffer[adapter->fAudioAdapter->GetOutputs()];

        // Always clear output
        for (int i = 0; i < adapter->fAudioAdapter->GetInputs(); i++) {
            inputBuffer[i] = (jack_default_audio_sample_t*)jack_port_get_buffer(adapter->fCapturePortList[i], frames);
            memset(inputBuffer[i], 0, frames * sizeof(jack_default_audio_sample_t));
        }

        for (int i = 0; i < adapter->fAudioAdapter->GetOutputs(); i++) {
            outputBuffer[i] = (jack_default_audio_sample_t*)jack_port_get_buffer(adapter->fPlaybackPortList[i], frames);
        }

        adapter->fAudioAdapter->PullAndPush(inputBuffer, outputBuffer, frames);
      	return 0;
    }

    int JackAudioAdapter::BufferSize ( jack_nframes_t buffer_size, void* arg )
    {
        JackAudioAdapter* adapter = static_cast<JackAudioAdapter*> ( arg );
        adapter->Reset();
        adapter->fAudioAdapter->SetHostBufferSize ( buffer_size );
        return 0;
    }

    int JackAudioAdapter::SampleRate ( jack_nframes_t sample_rate, void* arg )
    {
        JackAudioAdapter* adapter = static_cast<JackAudioAdapter*> ( arg );
        adapter->Reset();
        adapter->fAudioAdapter->SetHostSampleRate ( sample_rate );
        return 0;
    }

//JackAudioAdapter *********************************************************

     JackAudioAdapter::JackAudioAdapter (jack_client_t* jack_client, JackAudioAdapterInterface* audio_io, const JSList* params, bool system)
        :fJackClient(jack_client), fAudioAdapter(audio_io)
    {
        const JSList* node;
        const jack_driver_param_t* param;
        fAutoConnect = false;

        for (node = params; node; node = jack_slist_next(node)) {
            param = (const jack_driver_param_t*) node->data;
            switch (param->character) {
                case 'c':
                    fAutoConnect = true;
                    break;
            }
        }
    }

    JackAudioAdapter::~JackAudioAdapter()
    {
        // When called, Close has already been used for the client, thus ports are already unregistered.
        delete fAudioAdapter;
    }

    void JackAudioAdapter::FreePorts()
    {
        for (int i = 0; i < fAudioAdapter->GetInputs(); i++ )
            if ( fCapturePortList[i] )
                jack_port_unregister ( fJackClient, fCapturePortList[i] );
        for (int i = 0; i < fAudioAdapter->GetOutputs(); i++ )
            if ( fPlaybackPortList[i] )
                jack_port_unregister ( fJackClient, fPlaybackPortList[i] );

        delete[] fCapturePortList;
        delete[] fPlaybackPortList;
    }

    void JackAudioAdapter::ConnectPorts()
    {
        const char **ports;

        ports = jack_get_ports(fJackClient, NULL, NULL, JackPortIsPhysical | JackPortIsInput);
        if (ports != NULL) {
            for (int i = 0; i < fAudioAdapter->GetInputs() && ports[i]; i++) {
                jack_connect(fJackClient,jack_port_name(fCapturePortList[i]),  ports[i]);
            }
            free(ports);
        }

        ports = jack_get_ports(fJackClient, NULL, NULL, JackPortIsPhysical | JackPortIsOutput);
        if (ports != NULL) {
            for (int i = 0; i < fAudioAdapter->GetOutputs() && ports[i]; i++) {
                jack_connect(fJackClient, ports[i], jack_port_name(fPlaybackPortList[i]));
            }
            free(ports);
        }
    }

    void JackAudioAdapter::Reset()
    {
        fAudioAdapter->Reset();
    }

    int JackAudioAdapter::Open()
    {
        char name[32];
        jack_log("JackAudioAdapter::Open fCaptureChannels %d fPlaybackChannels %d", fAudioAdapter->GetInputs(), fAudioAdapter->GetOutputs());
        fAudioAdapter->Create();

        //jack ports
        fCapturePortList = new jack_port_t*[fAudioAdapter->GetInputs()];
        fPlaybackPortList = new jack_port_t*[fAudioAdapter->GetOutputs()];

        for (int i = 0; i < fAudioAdapter->GetInputs(); i++)
        {
            sprintf(name, "capture_%d", i + 1);
            if ((fCapturePortList[i] = jack_port_register(fJackClient, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0)) == NULL)
                goto fail;
        }

        for (int i = 0; i < fAudioAdapter->GetOutputs(); i++)
        {
            sprintf(name, "playback_%d", i + 1);
            if ((fPlaybackPortList[i] = jack_port_register(fJackClient, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0 )) == NULL)
                goto fail;
        }

        //callbacks and activation
        if ( jack_set_process_callback ( fJackClient, Process, this ) < 0 )
            goto fail;
        if ( jack_set_buffer_size_callback ( fJackClient, BufferSize, this ) < 0 )
            goto fail;
        if ( jack_set_sample_rate_callback ( fJackClient, SampleRate, this ) < 0 )
            goto fail;
        if ( jack_activate ( fJackClient ) < 0 )
            goto fail;

        if (fAutoConnect)
            ConnectPorts();

        // Ring buffer are now allocated..
        return fAudioAdapter->Open();

    fail:
        FreePorts();
        fAudioAdapter->Destroy();
        return -1;
    }

    int JackAudioAdapter::Close()
    {
        fAudioAdapter->Close();
        fAudioAdapter->Destroy();
        return 0;
    }

} //namespace
