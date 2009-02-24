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

#include "JackAudioQueueAdapter.h"
#include <CoreServices/CoreServices.h>

namespace Jack
{

// NOT YET WORKING....

static void Print4CharCode(char* msg, long c)
{
    UInt32 __4CC_number = (c);
    char __4CC_string[5];
    *((SInt32*)__4CC_string) = EndianU32_NtoB(__4CC_number);		
    __4CC_string[4] = 0;
    //printf("%s'%s'\n", (msg), __4CC_string);
    snprintf(__4CC_string, 5, "%s'%s'\n", (msg), __4CC_string);
}

void JackAudioQueueAdapter::CaptureCallback(void * inUserData,
									AudioQueueRef inAQ,
									AudioQueueBufferRef inBuffer,
									const AudioTimeStamp * inStartTime,
									UInt32 inNumPackets,
									const AudioStreamPacketDescription *inPacketDesc)
{
    JackAudioQueueAdapter* adapter = (JackAudioQueueAdapter*)inUserData;
    
    printf("JackAudioQueueAdapter::CaptureCallback\n");
    
    if (AudioQueueEnqueueBuffer(adapter->fCaptureQueue, inBuffer, 0, NULL) != noErr) {
        printf("JackAudioQueueAdapter::CaptureCallback error\n");
    }
    
    // Use the adapter to communicate with audio callback
    // jack_adapter_push_input(adapter, audio_output, audio_output_buffer);
}

void JackAudioQueueAdapter::PlaybackCallback(void *	inUserData,
								AudioQueueRef inAQ,
								AudioQueueBufferRef inCompleteAQBuffer)
{
    JackAudioQueueAdapter* adapter = (JackAudioQueueAdapter*)inUserData;
    
    printf("JackAudioQueueAdapter::PlaybackCallback\n");
    
    if (AudioQueueEnqueueBuffer(adapter->fPlaybackQueue, inCompleteAQBuffer, 0, &adapter->fPlaybackPacketDescs) != noErr) {
        printf("JackAudioQueueAdapter::PlaybackCallback error\n");
    }
    
    
    // Use the adapter to communicate with audio callback
    // jack_adapter_pull_output(adapter, audio_input, audio_input_buffer);
}


JackAudioQueueAdapter::JackAudioQueueAdapter(int inchan, int outchan, jack_nframes_t buffer_size, jack_nframes_t sample_rate, jack_adapter_t* adapter)
    :fCaptureChannels(inchan), fPlaybackChannels(outchan), fBufferSize(buffer_size), fSampleRate(sample_rate), fAdapter(adapter)
{}

JackAudioQueueAdapter::~JackAudioQueueAdapter()
{}

int  JackAudioQueueAdapter::Open()
{
   OSStatus err;
    AudioStreamBasicDescription captureDataFormat;
    
    /*
    captureDataFormat.mSampleRate = fSampleRate;
    captureDataFormat.mFormatID = kAudioFormatLinearPCM;
    //captureDataFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked | kLinearPCMFormatFlagIsNonInterleaved;
    captureDataFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
    captureDataFormat.mBytesPerPacket = sizeof(float);
    captureDataFormat.mFramesPerPacket = 1;
    captureDataFormat.mBytesPerFrame = sizeof(float);
    captureDataFormat.mChannelsPerFrame = fCaptureChannels;
    captureDataFormat.mBitsPerChannel = 32;
    */

    captureDataFormat.mSampleRate = fSampleRate;
	captureDataFormat.mFormatID = kAudioFormatLinearPCM;
	captureDataFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger  | kAudioFormatFlagIsPacked;
	captureDataFormat.mBytesPerPacket = 4;
	captureDataFormat.mFramesPerPacket = 1; // this means each packet in the AQ has two samples, one for each channel -> 4 bytes/frame/packet
	captureDataFormat.mBytesPerFrame = 4;
	captureDataFormat.mChannelsPerFrame = 2;
	captureDataFormat.mBitsPerChannel = 16;

    if ((err = AudioQueueNewInput(&captureDataFormat, CaptureCallback, this, CFRunLoopGetCurrent(), kCFRunLoopCommonModes, 0, &fCaptureQueue)) != noErr) {
        Print4CharCode("error code : unknown", err);
        return -1;
    }
    
    //AudioQueueSetProperty(fCaptureQueue, kAudioQueueProperty_MagicCookie, cookie, size)
    //AudioQueueSetProperty(fCaptureQueue, kAudioQueueProperty_ChannelLayout, acl, size
    
    AudioStreamBasicDescription playbackDataFormat;
    
    playbackDataFormat.mSampleRate = fSampleRate;
    playbackDataFormat.mFormatID = kAudioFormatLinearPCM;
    playbackDataFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked | kLinearPCMFormatFlagIsNonInterleaved;
    playbackDataFormat.mBytesPerPacket = sizeof(float);
    playbackDataFormat.mFramesPerPacket = 1;
    playbackDataFormat.mBytesPerFrame = sizeof(float);
    playbackDataFormat.mChannelsPerFrame = fPlaybackChannels;
    playbackDataFormat.mBitsPerChannel = 32;
    
    if ((err = AudioQueueNewOutput(&playbackDataFormat, PlaybackCallback, this, CFRunLoopGetCurrent(), kCFRunLoopCommonModes, 0, &fPlaybackQueue)) != noErr) {
        Print4CharCode("error code : unknown", err);
        return -1;
    }
    
  
    //AudioQueueSetProperty(fPlaybackQueue, kAudioQueueProperty_MagicCookie, cookie, size);
    //AudioQueueSetProperty(fPlaybackQueue, kAudioQueueProperty_ChannelLayout, acl, size);
    //AudioQueueSetParameter(fPlaybackQueue, kAudioQueueParam_Volume, volume
    
    //AudioQueueStart(fCaptureQueue, NULL);
    AudioQueueStart(fPlaybackQueue, NULL);
 
    return 0;
}
int  JackAudioQueueAdapter::Close()
{
    AudioQueueStop(fCaptureQueue, true);
    AudioQueueStop(fPlaybackQueue, true);
    
    AudioQueueDispose(fCaptureQueue, true);
    AudioQueueDispose(fPlaybackQueue, true);
    return 0;
}

int  JackAudioQueueAdapter::SetSampleRate(jack_nframes_t sample_rate)
{
    return 0;
}

int  JackAudioQueueAdapter::SetBufferSize(jack_nframes_t buffer_size)
{
    return 0;
}
   
};
