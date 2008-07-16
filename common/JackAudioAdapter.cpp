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
#include "JackExports.h"
#include "JackTools.h"
#include "jslist.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

using namespace std;

namespace Jack
{
//static methods ***********************************************************
    int JackAudioAdapter::Process(jack_nframes_t frames, void* arg)
    {
        JackAudioAdapter* adapter = static_cast<JackAudioAdapter*>(arg);
        float* buffer;
        bool failure = false;
        int i;

        if (!adapter->fAudioAdapter->IsRunning())
            return 0;

        // DLL
        adapter->fAudioAdapter->SetCallbackTime(jack_get_time());

        // Push/pull from ringbuffer
        for (i = 0; i < adapter->fCaptureChannels; i++)
        {
            buffer = static_cast<float*>(jack_port_get_buffer(adapter->fCapturePortList[i], frames));
            if (adapter->fCaptureRingBuffer[i]->Read(buffer, frames) < frames)
                failure = true;
        }

        for (i = 0; i < adapter->fPlaybackChannels; i++)
        {
            buffer = static_cast<float*>(jack_port_get_buffer(adapter->fPlaybackPortList[i], frames));
            if (adapter->fPlaybackRingBuffer[i]->Write(buffer, frames) < frames)
                failure = true;
        }

        // Reset all ringbuffers in case of failure
        if (failure)
        {
            jack_error("JackCallbackAudioAdapter::Process ringbuffer failure... reset");
            adapter->Reset();
        }
        return 0;
    }

    int JackAudioAdapter::BufferSize(jack_nframes_t buffer_size, void* arg)
    {
        JackAudioAdapter* adapter = static_cast<JackAudioAdapter*>(arg);
        adapter->Reset();
        adapter->fAudioAdapter->SetBufferSize(buffer_size);
        return 0;
    }

    int JackAudioAdapter::SampleRate(jack_nframes_t sample_rate, void* arg)
    {
        JackAudioAdapter* adapter = static_cast<JackAudioAdapter*>(arg);
        adapter->Reset();
        adapter->fAudioAdapter->SetSampleRate(sample_rate);
        return 0;
    }

//JackAudioAdapter *********************************************************
    JackAudioAdapter::~JackAudioAdapter()
    {
        // When called, Close has already been used for the client, thus ports are already unregistered.
        int i;
        for (i = 0; i < fCaptureChannels; i++)
            delete(fCaptureRingBuffer[i]);
        for (i = 0; i < fPlaybackChannels; i++)
            delete(fPlaybackRingBuffer[i]);

        delete[] fCaptureRingBuffer;
        delete[] fPlaybackRingBuffer;
        delete fAudioAdapter;
    }

    void JackAudioAdapter::FreePorts()
    {
        int i;
        for (i = 0; i < fCaptureChannels; i++)
            if (fCapturePortList[i])
                jack_port_unregister(fJackClient, fCapturePortList[i]);
        for (i = 0; i < fCaptureChannels; i++)
            if (fPlaybackPortList[i])
                jack_port_unregister(fJackClient, fPlaybackPortList[i]);

        delete[] fCapturePortList;
        delete[] fPlaybackPortList;
    }

    void JackAudioAdapter::Reset()
    {
        int i;
        for (i = 0; i < fCaptureChannels; i++)
            fCaptureRingBuffer[i]->Reset();
        for (i = 0; i < fPlaybackChannels; i++)
            fPlaybackRingBuffer[i]->Reset();
        fAudioAdapter->Reset();
    }

    int JackAudioAdapter::Open()
    {
        jack_log("JackAudioAdapter::Open()");

        int i;
        char name[32];

        fCaptureChannels = fAudioAdapter->GetInputs();
        fPlaybackChannels = fAudioAdapter->GetOutputs();

        //ringbuffers
        fCaptureRingBuffer = new JackResampler*[fCaptureChannels];
        fPlaybackRingBuffer = new JackResampler*[fPlaybackChannels];
        for (i = 0; i < fCaptureChannels; i++)
            fCaptureRingBuffer[i] = new JackLibSampleRateResampler();
        for (i = 0; i < fPlaybackChannels; i++)
            fPlaybackRingBuffer[i] = new JackLibSampleRateResampler();
        fAudioAdapter->SetRingBuffers(fCaptureRingBuffer, fPlaybackRingBuffer);
        jack_log("ReadSpace = %ld", fCaptureRingBuffer[0]->ReadSpace());
        jack_log("WriteSpace = %ld", fPlaybackRingBuffer[0]->WriteSpace());

        //jack ports
        fCapturePortList = new jack_port_t* [fCaptureChannels];
        fPlaybackPortList = new jack_port_t* [fPlaybackChannels];

        for (i = 0; i < fCaptureChannels; i++)
        {
            sprintf(name, "capture_%d", i+1);
            if ((fCapturePortList[i] = jack_port_register(fJackClient, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0)) == NULL)
                goto fail;
        }

        for (i = 0; i < fPlaybackChannels; i++)
        {
            sprintf(name, "playback_%d", i+1);
            if ((fPlaybackPortList[i] = jack_port_register(fJackClient, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0)) == NULL)
                goto fail;
        }

        //callbacks and activation
        if (jack_set_process_callback(fJackClient, Process, this) < 0)
            goto fail;
        if (jack_set_buffer_size_callback(fJackClient, BufferSize, this) < 0)
            goto fail;
        if (jack_set_sample_rate_callback(fJackClient, SampleRate, this) < 0)
            goto fail;
        if (jack_activate(fJackClient) < 0)
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

#ifdef __cplusplus
extern "C"
{
#endif

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
        jack_log("Loading audioadapter");

        Jack::JackAudioAdapter* adapter;
        jack_nframes_t buffer_size = jack_get_buffer_size(jack_client);
        jack_nframes_t sample_rate = jack_get_sample_rate(jack_client);

#ifdef __linux__
        adapter = new Jack::JackAudioAdapter(jack_client, new Jack::JackAlsaAdapter(buffer_size, sample_rate, params));
#endif

#ifdef WIN32
        adapter = new Jack::JackAudioAdapter(jack_client, new Jack::JackPortAudioAdapter(buffer_size, sample_rate, params));
#endif

#ifdef __APPLE__
        adapter = new Jack::JackAudioAdapter(jack_client, new Jack::JackCoreAudioAdapter(buffer_size, sample_rate, params));
#endif

        assert(adapter);

        if (adapter->Open() == 0)
            return 0;
        else
        {
            delete adapter;
            return 1;
        }
    }

    EXPORT int jack_initialize(jack_client_t* jack_client, const char* load_init)
    {
        JSList* params = NULL;
        jack_driver_desc_t *desc = jack_get_descriptor();

        JackArgParser parser(load_init);

        if (parser.GetArgc() > 0)
        {
            if (parser.ParseParams(desc, &params) != 0)
                jack_error("Internal client : JackArgParser::ParseParams error.");
        }

        return jack_internal_initialize(jack_client, params);
    }

    EXPORT void jack_finish(void* arg)
    {
        Jack::JackAudioAdapter* adapter = static_cast<Jack::JackAudioAdapter*>(arg);

        if (adapter)
        {
            jack_log("Unloading audioadapter");
            adapter->Close();
            delete adapter;
        }
    }

#ifdef __cplusplus
}
#endif
