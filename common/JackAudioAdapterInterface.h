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

#ifndef __JackAudioAdapterInterface__
#define __JackAudioAdapterInterface__

#include "JackResampler.h"
#include "JackFilters.h"
#include <stdio.h>

namespace Jack
{

#ifdef JACK_MONITOR

#define TABLE_MAX 100000

    struct Measure
    {
        int delta;
        int time1;
        int time2;
        float r1;
        float r2;
        int pos1;
        int pos2;
    };

    struct MeasureTable
    {

        Measure fTable[TABLE_MAX];
        int fCount;

        MeasureTable() :fCount(0)
        {}

        void Write(int time1, int time2, float r1, float r2, int pos1, int pos2);
        void Save(unsigned int fHostBufferSize, unsigned int fHostSampleRate, unsigned int fAdaptedSampleRate, unsigned int fAdaptedBufferSize);

    };

#endif

    /*!
    \brief Base class for audio adapters.
    */

    class JackAudioAdapterInterface
    {

    protected:

#ifdef JACK_MONITOR
        MeasureTable fTable;
#endif
        //channels
        int fCaptureChannels;
        int fPlaybackChannels;

        //host parameters
        jack_nframes_t fHostBufferSize;
        jack_nframes_t fHostSampleRate;

        //adapted parameters
        jack_nframes_t fAdaptedBufferSize;
        jack_nframes_t fAdaptedSampleRate;

        //PI controler
        JackPIControler fPIControler;

        JackResampler** fCaptureRingBuffer;
        JackResampler** fPlaybackRingBuffer;

        unsigned int fQuality;
        unsigned int fRingbufferCurSize;
        jack_time_t fPullAndPushTime;

        bool fRunning;
        bool fAdaptative;

        void ResetRingBuffers();
        void AdaptRingBufferSize();
        void GrowRingBufferSize();

    public:

        JackAudioAdapterInterface(jack_nframes_t buffer_size, jack_nframes_t sample_rate, jack_nframes_t ring_buffer_size = DEFAULT_ADAPTATIVE_SIZE):
                                fCaptureChannels(0),
                                fPlaybackChannels(0),
                                fHostBufferSize(buffer_size),
                                fHostSampleRate(sample_rate),
                                fAdaptedBufferSize(buffer_size),
                                fAdaptedSampleRate(sample_rate),
                                fPIControler(sample_rate / sample_rate, 256),
                                fCaptureRingBuffer(NULL), fPlaybackRingBuffer(NULL),
                                fQuality(0),
                                fRingbufferCurSize(ring_buffer_size),
                                fPullAndPushTime(0),
                                fRunning(false),
                                fAdaptative(true)
        {}

        JackAudioAdapterInterface(jack_nframes_t host_buffer_size,
                                jack_nframes_t host_sample_rate,
                                jack_nframes_t adapted_buffer_size,
                                jack_nframes_t adapted_sample_rate,
                                jack_nframes_t ring_buffer_size = DEFAULT_ADAPTATIVE_SIZE) :
                                fCaptureChannels(0),
                                fPlaybackChannels(0),
                                fHostBufferSize(host_buffer_size),
                                fHostSampleRate(host_sample_rate),
                                fAdaptedBufferSize(adapted_buffer_size),
                                fAdaptedSampleRate(adapted_sample_rate),
                                fPIControler(host_sample_rate / host_sample_rate, 256),
                                fQuality(0),
                                fRingbufferCurSize(ring_buffer_size),
                                fPullAndPushTime(0),
                                fRunning(false),
                                fAdaptative(true)
        {}

        virtual ~JackAudioAdapterInterface()
        {}

        virtual void Reset();

        virtual void Create();
        virtual void Destroy();

        virtual int Open()
        {
            return 0;
        }

        virtual int Close()
        {
            return 0;
        }

        virtual int SetHostBufferSize(jack_nframes_t buffer_size);
        virtual int SetAdaptedBufferSize(jack_nframes_t buffer_size);
        virtual int SetBufferSize(jack_nframes_t buffer_size);
        virtual int SetHostSampleRate(jack_nframes_t sample_rate);
        virtual int SetAdaptedSampleRate(jack_nframes_t sample_rate);
        virtual int SetSampleRate(jack_nframes_t sample_rate);
        void SetInputs(int inputs);
        void SetOutputs(int outputs);
        int GetInputs();
        int GetOutputs();

        virtual int GetInputLatency(int port_index) { return 0; }
        virtual int GetOutputLatency(int port_index) { return 0; }

        int PushAndPull(jack_default_audio_sample_t** inputBuffer, jack_default_audio_sample_t** outputBuffer, unsigned int frames);
        int PullAndPush(jack_default_audio_sample_t** inputBuffer, jack_default_audio_sample_t** outputBuffer, unsigned int frames);

    };

}

#endif
