/*
Copyright (C) 2009 Grame

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

#ifndef __JackAudioQueueAdapter__
#define __JackAudioQueueAdapter__

#include <AudioToolbox/AudioConverter.h>
#include <AudioToolbox/AudioQueue.h>

#include <jack/net.h>

namespace Jack
{

/*!
\brief Audio adapter using AudioQueue API.
*/

static const int kNumberBuffers = 3;

class JackAudioQueueAdapter 
{

    private:

        AudioQueueRef fCaptureQueue;
        AudioQueueBufferRef	fCaptureQueueBuffers[kNumberBuffers];
        
        AudioQueueRef fPlaybackQueue;
        AudioQueueBufferRef	fPlaybackQueueBuffers[kNumberBuffers];
        AudioStreamPacketDescription fPlaybackPacketDescs;
        
        
        jack_nframes_t fBufferSize;
        jack_nframes_t fSampleRate;

        int fCaptureChannels;
        int fPlaybackChannels;
        
        jack_adapter_t* fAdapter;
    
        static void CaptureCallback(void * inUserData,
									AudioQueueRef inAQ,
									AudioQueueBufferRef inBuffer,
									const AudioTimeStamp * inStartTime,
									UInt32 inNumPackets,
									const AudioStreamPacketDescription *inPacketDesc);


       static void PlaybackCallback(void * inUserData,
								AudioQueueRef inAQ,
								AudioQueueBufferRef inCompleteAQBuffer);

    public:

        JackAudioQueueAdapter(int inchan, int outchan, jack_nframes_t buffer_size, jack_nframes_t sample_rate, jack_adapter_t* adapter);
        ~JackAudioQueueAdapter();
    
        virtual int Open();
        virtual int Close();

        virtual int SetSampleRate(jack_nframes_t sample_rate);
        virtual int SetBufferSize(jack_nframes_t buffer_size);

};

}

#endif
