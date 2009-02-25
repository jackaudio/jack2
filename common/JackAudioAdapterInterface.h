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
#include "JackConstants.h"

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

        MeasureTable() :fCount ( 0 )
        {}

        void Write ( int time1, int time2, float r1, float r2, int pos1, int pos2 );
        void Save();

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

        //delay locked loop
        JackAtomicDelayLockedLoop fHostDLL;
        JackAtomicDelayLockedLoop fAdaptedDLL;

        JackResampler** fCaptureRingBuffer;
        JackResampler** fPlaybackRingBuffer;
        
        unsigned int fQuality;
        unsigned int fRingbufferSize;
      
        bool fRunning;

    public:

        JackAudioAdapterInterface ( jack_nframes_t buffer_size, jack_nframes_t sample_rate ) :
                fCaptureChannels ( 0 ),
                fPlaybackChannels ( 0 ),
                fHostBufferSize ( buffer_size ),
                fHostSampleRate ( sample_rate ),
                fAdaptedBufferSize ( buffer_size),
                fAdaptedSampleRate ( sample_rate ),
                fHostDLL ( buffer_size, sample_rate ),
                fAdaptedDLL ( buffer_size, sample_rate ),
                fQuality(0), fRingbufferSize(DEFAULT_RB_SIZE),
                fRunning(false)
        {}

        virtual ~JackAudioAdapterInterface()
        {}

        void SetRingBuffers ( JackResampler** input, JackResampler** output )
        {
            fCaptureRingBuffer = input;
            fPlaybackRingBuffer = output;
        }

        bool IsRunning()
        {
            return fRunning;
        }

        virtual void Reset()
        {
            fRunning = false;
        }

        void ResetRingBuffers();
        
        unsigned int GetRingbufferSize()
        {
            return fRingbufferSize;
        }
        
        unsigned int GetQuality()
        {
            return fQuality;
        }

        virtual int Open();
        virtual int Close();

        virtual int SetHostBufferSize ( jack_nframes_t buffer_size )
        {
            fHostBufferSize = buffer_size;
            fHostDLL.Init ( fHostBufferSize, fHostSampleRate );
            return 0;
        }

        virtual int SetAdaptedBufferSize ( jack_nframes_t buffer_size )
        {
            fAdaptedBufferSize = buffer_size;
            fAdaptedDLL.Init ( fAdaptedBufferSize, fAdaptedSampleRate );
            return 0;
        }

        virtual int SetBufferSize ( jack_nframes_t buffer_size )
        {
            SetHostBufferSize ( buffer_size );
            SetAdaptedBufferSize ( buffer_size );
            return 0;
        }

        virtual int SetHostSampleRate ( jack_nframes_t sample_rate )
        {
            fHostSampleRate = sample_rate;
            fHostDLL.Init ( fHostBufferSize, fHostSampleRate );
            return 0;
        }

        virtual int SetAdaptedSampleRate ( jack_nframes_t sample_rate )
        {
            fAdaptedSampleRate = sample_rate;
            fAdaptedDLL.Init ( fAdaptedBufferSize, fAdaptedSampleRate );
            return 0;
        }

        virtual int SetSampleRate ( jack_nframes_t sample_rate )
        {
            SetHostSampleRate ( sample_rate );
            SetAdaptedSampleRate ( sample_rate );
            return 0;
        }

        virtual void SetCallbackTime ( jack_time_t callback_usec )
        {
            fHostDLL.IncFrame ( callback_usec );
        }

        void ResampleFactor ( jack_time_t& frame1, jack_time_t& frame2 );

        void SetInputs ( int inputs )
        {
            jack_log ( "JackAudioAdapterInterface::SetInputs %d", inputs );
            fCaptureChannels = inputs;
        }

        void SetOutputs ( int outputs )
        {
            jack_log ( "JackAudioAdapterInterface::SetOutputs %d", outputs );
            fPlaybackChannels = outputs;
        }

        int GetInputs()
        {
            jack_log ( "JackAudioAdapterInterface::GetInputs %d", fCaptureChannels );
            return fCaptureChannels;
        }

        int GetOutputs()
        {
            jack_log ( "JackAudioAdapterInterface::GetOutputs %d", fPlaybackChannels );
            return fPlaybackChannels;
        }
        
        void PushAndPull(float** inputBuffer, float** outputBuffer, unsigned int inNumberFrames);

    };

}

#endif
