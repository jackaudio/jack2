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
#include "JackLibSampleRateResampler.h"
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
    int JackAudioAdapter::Process ( jack_nframes_t frames, void* arg )
    {
        JackAudioAdapter* adapter = static_cast<JackAudioAdapter*> ( arg );
        float* buffer;
        bool failure = false;
        int i;

        if ( !adapter->fAudioAdapter->IsRunning() )
            return 0;

        // DLL
        adapter->fAudioAdapter->SetCallbackTime (GetMicroSeconds());

        // Push/pull from ringbuffer
        for ( i = 0; i < adapter->fCaptureChannels; i++ )
        {
            buffer = static_cast<float*> ( jack_port_get_buffer ( adapter->fCapturePortList[i], frames ) );
            if ( adapter->fCaptureRingBuffer[i]->Read ( buffer, frames ) < frames )
                failure = true;
        }

        for ( i = 0; i < adapter->fPlaybackChannels; i++ )
        {
            buffer = static_cast<float*> ( jack_port_get_buffer ( adapter->fPlaybackPortList[i], frames ) );
            if ( adapter->fPlaybackRingBuffer[i]->Write ( buffer, frames ) < frames )
                failure = true;
        }

        // Reset all ringbuffers in case of failure
        if ( failure )
        {
            jack_error ( "JackCallbackAudioAdapter::Process ringbuffer failure... reset" );
            adapter->Reset();
        }
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
    JackAudioAdapter::~JackAudioAdapter()
    {
        // When called, Close has already been used for the client, thus ports are already unregistered.
        int i;
        for ( i = 0; i < fCaptureChannels; i++ )
            delete ( fCaptureRingBuffer[i] );
        for ( i = 0; i < fPlaybackChannels; i++ )
            delete ( fPlaybackRingBuffer[i] );

        delete[] fCaptureRingBuffer;
        delete[] fPlaybackRingBuffer;
        delete fAudioAdapter;
    }

    void JackAudioAdapter::FreePorts()
    {
        int i;
        for ( i = 0; i < fCaptureChannels; i++ )
            if ( fCapturePortList[i] )
                jack_port_unregister ( fJackClient, fCapturePortList[i] );
        for ( i = 0; i < fCaptureChannels; i++ )
            if ( fPlaybackPortList[i] )
                jack_port_unregister ( fJackClient, fPlaybackPortList[i] );

        delete[] fCapturePortList;
        delete[] fPlaybackPortList;
    }

    void JackAudioAdapter::Reset()
    {
        int i;
        for ( i = 0; i < fCaptureChannels; i++ )
            fCaptureRingBuffer[i]->Reset();
        for ( i = 0; i < fPlaybackChannels; i++ )
            fPlaybackRingBuffer[i]->Reset();
        fAudioAdapter->Reset();
    }

    int JackAudioAdapter::Open()
    {
        int i;
        char name[32];

        fCaptureChannels = fAudioAdapter->GetInputs();
        fPlaybackChannels = fAudioAdapter->GetOutputs();

        jack_log ( "JackAudioAdapter::Open fCaptureChannels %d fPlaybackChannels %d", fCaptureChannels, fPlaybackChannels );

        //ringbuffers
        fCaptureRingBuffer = new JackResampler*[fCaptureChannels];
        fPlaybackRingBuffer = new JackResampler*[fPlaybackChannels];
        for ( i = 0; i < fCaptureChannels; i++ )
            fCaptureRingBuffer[i] = new JackLibSampleRateResampler(fAudioAdapter->GetQuality());
        for ( i = 0; i < fPlaybackChannels; i++ )
            fPlaybackRingBuffer[i] = new JackLibSampleRateResampler(fAudioAdapter->GetQuality());
        fAudioAdapter->SetRingBuffers ( fCaptureRingBuffer, fPlaybackRingBuffer );
        if ( fCaptureChannels )
            jack_log ( "ReadSpace = %ld", fCaptureRingBuffer[0]->ReadSpace() );
        if ( fPlaybackChannels )
            jack_log ( "WriteSpace = %ld", fPlaybackRingBuffer[0]->WriteSpace() );

        //jack ports
        fCapturePortList = new jack_port_t* [fCaptureChannels];
        fPlaybackPortList = new jack_port_t* [fPlaybackChannels];

        for ( i = 0; i < fCaptureChannels; i++ )
        {
            sprintf ( name, "capture_%d", i+1 );
            if ( ( fCapturePortList[i] = jack_port_register ( fJackClient, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 ) ) == NULL )
                goto fail;
        }

        for ( i = 0; i < fPlaybackChannels; i++ )
        {
            sprintf ( name, "playback_%d", i+1 );
            if ( ( fPlaybackPortList[i] = jack_port_register ( fJackClient, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0 ) ) == NULL )
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

        //ringbuffers and jack clients are ok, we can now open the adapter driver interface
        return fAudioAdapter->Open();

    fail:
        FreePorts();
        return -1;
    }

    int JackAudioAdapter::Close()
    {
        return fAudioAdapter->Close();
    }

} //namespace
