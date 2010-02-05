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
//#include <CoreServices/CoreServices.h>

namespace Jack
{

// NOT YET WORKING....

static void Print4CharCode(const char* msg, long c)
{
    UInt32 __4CC_number = (c);
    char __4CC_string[5];
    //*((SInt32*)__4CC_string) = EndianU32_NtoB(__4CC_number);		
    __4CC_string[4] = 0;
    //printf("%s'%s'\n", (msg), __4CC_string);
    snprintf(__4CC_string, 5, "%s'%s'\n", (msg), __4CC_string);
}
    
static int ComputeRecordBufferSize(AudioQueueRef mQueue, const AudioStreamBasicDescription *format, float seconds)
{
    OSStatus err;
    int packets, frames, bytes = 0;
    frames = (int)ceil(seconds * format->mSampleRate);
    
    if (format->mBytesPerFrame > 0)
        bytes = frames * format->mBytesPerFrame;
    else {
        UInt32 maxPacketSize;
        if (format->mBytesPerPacket > 0) {
            maxPacketSize = format->mBytesPerPacket;	// constant packet size
        } else {
            UInt32 propertySize = sizeof(maxPacketSize);
            if ((err = AudioQueueGetProperty(mQueue, kAudioQueueProperty_MaximumOutputPacketSize, &maxPacketSize, &propertySize)) != noErr) {
                printf("Couldn't get queue's maximum output packet size\n");
                return 0;
            }
        }
        if (format->mFramesPerPacket > 0)
            packets = frames / format->mFramesPerPacket;
        else
            packets = frames;	// worst-case scenario: 1 frame in a packet
        if (packets == 0)		// sanity check
            packets = 1;
        bytes = packets * maxPacketSize;
    }
    return bytes;
}

void JackAudioQueueAdapter::CaptureCallback(void * inUserData,
                                            AudioQueueRef inAQ,
                                            AudioQueueBufferRef inBuffer,
                                            const AudioTimeStamp * inStartTime,
                                            UInt32 inNumPackets,
                                            const AudioStreamPacketDescription *inPacketDesc)
{
    JackAudioQueueAdapter* adapter = (JackAudioQueueAdapter*)inUserData;
    OSStatus err;
    printf("JackAudioQueueAdapter::CaptureCallback\n");
    
    // Use the adapter to communicate with audio callback
    // jack_adapter_push_input(adapter, audio_output, audio_output_buffer);
    
    if ((err = AudioQueueEnqueueBuffer(adapter->fCaptureQueue, inBuffer, 0, NULL)) != noErr) {
        printf("JackAudioQueueAdapter::CaptureCallback error %d\n", err);
    } else {
        printf("JackAudioQueueAdapter::CaptureCallback enqueue buffer\n");
    }
}

void JackAudioQueueAdapter::PlaybackCallback(void *	inUserData,
                                            AudioQueueRef inAQ,
                                            AudioQueueBufferRef inCompleteAQBuffer)
{
    JackAudioQueueAdapter* adapter = (JackAudioQueueAdapter*)inUserData;
    OSStatus err;
    printf("JackAudioQueueAdapter::PlaybackCallback\n");
    
    
    // Use the adapter to communicate with audio callback
    // jack_adapter_pull_output(adapter, audio_input, audio_input_buffer);
    
    //if (AudioQueueEnqueueBuffer(adapter->fPlaybackQueue, inCompleteAQBuffer, 0, &adapter->fPlaybackPacketDescs) != noErr) {
    if ((err = AudioQueueEnqueueBuffer(inAQ, inCompleteAQBuffer, 0, NULL)) != noErr) {    
        printf("JackAudioQueueAdapter::PlaybackCallback error %d\n", err);
    } else {
        printf("JackAudioQueueAdapter::PlaybackCallback enqueue buffer\n");
    }
}

void JackAudioQueueAdapter::InterruptionListener(void* inClientData, UInt32 inInterruptionState)
{
	JackAudioQueueAdapter* obj = (JackAudioQueueAdapter*)inClientData;
}

void JackAudioQueueAdapter::PropListener(void* inClientData,
                                        AudioSessionPropertyID inID,
                                        UInt32 inDataSize,
                                        const void* inData)
{}


JackAudioQueueAdapter::JackAudioQueueAdapter(int inchan, int outchan, jack_nframes_t buffer_size, jack_nframes_t sample_rate, jack_adapter_t* adapter)
    :fCaptureChannels(inchan), fPlaybackChannels(outchan), fBufferSize(buffer_size), fSampleRate(sample_rate), fAdapter(adapter)
{}

JackAudioQueueAdapter::~JackAudioQueueAdapter()
{}

int JackAudioQueueAdapter::Open()
{
    OSStatus err;
    int bufferByteSize;
    UInt32 size;
    
    /*
    err = AudioSessionInitialize(NULL, NULL, InterruptionListener, this);
	if (err != noErr) {
        fprintf(stderr, "AudioSessionInitialize error %d\n", err);
        return -1;
    }
    
    err = AudioSessionAddPropertyListener(kAudioSessionProperty_AudioRouteChange, PropListener, this);
    if (err) {
        fprintf(stderr, "kAudioSessionProperty_AudioRouteChange error %d\n", err);
    }
    
    UInt32 inputAvailable = 0;
    UInt32 size = sizeof(inputAvailable);
    
    // we do not want to allow recording if input is not available
    err = AudioSessionGetProperty(kAudioSessionProperty_AudioInputAvailable, &size, &inputAvailable);
    if (err != noErr) {
        fprintf(stderr, "kAudioSessionProperty_AudioInputAvailable error %d\n", err);
    }
     
    // we also need to listen to see if input availability changes
    err = AudioSessionAddPropertyListener(kAudioSessionProperty_AudioInputAvailable, PropListener, this);
    if (err != noErr) { 
        fprintf(stderr, "kAudioSessionProperty_AudioInputAvailable error %d\n", err);
    }

    err = AudioSessionSetActive(true); 
    if (err != noErr) {
        fprintf(stderr, "AudioSessionSetActive (true) failed %d", err);
        return -1;
    }
    */
	
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
	captureDataFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    captureDataFormat.mBytesPerPacket = 4;
	captureDataFormat.mFramesPerPacket = 1; // this means each packet in the AQ has two samples, one for each channel -> 4 bytes/frame/packet
	captureDataFormat.mBytesPerFrame = 4;
	captureDataFormat.mChannelsPerFrame = 2;
	captureDataFormat.mBitsPerChannel = 16;
    

    if ((err = AudioQueueNewInput(&captureDataFormat, CaptureCallback, this, CFRunLoopGetCurrent(), kCFRunLoopCommonModes, 0, &fCaptureQueue)) != noErr) {
        Print4CharCode("error code : unknown", err);
        return -1;
    }
    
    size = sizeof(captureDataFormat.mSampleRate);
	if ((err = AudioSessionGetProperty(kAudioSessionProperty_CurrentHardwareSampleRate, &size, &captureDataFormat.mSampleRate)) != noErr) {
        printf("couldn't get hardware sample rate\n");
    }
    
    size = sizeof(captureDataFormat.mChannelsPerFrame);
    if ((err = AudioSessionGetProperty(kAudioSessionProperty_CurrentHardwareInputNumberChannels, &size, &captureDataFormat.mChannelsPerFrame)) != noErr) {
        printf("couldn't get input channel count\n");
    }
    
    size = sizeof(captureDataFormat);
    if ((err = AudioQueueGetProperty(fCaptureQueue, kAudioQueueProperty_StreamDescription,	&captureDataFormat, &size)) != noErr) {
        printf("couldn't get queue's format\n");
    }
    
    bufferByteSize = ComputeRecordBufferSize(fCaptureQueue, &captureDataFormat, kBufferDurationSeconds);	// enough bytes for half a second
    for (int i = 0; i < kNumberBuffers; ++i) {
        if ((err = AudioQueueAllocateBuffer(fCaptureQueue, bufferByteSize, &fCaptureQueueBuffers[i])) != noErr) {
            printf("Capture AudioQueueAllocateBuffer failed\n");
        }
        if ((err = AudioQueueEnqueueBuffer(fCaptureQueue, fCaptureQueueBuffers[i], 0, NULL)) != noErr) {
             printf("Capture AudioQueueEnqueueBuffer failed\n");
        }
    }
    
    
    //AudioQueueSetProperty(fCaptureQueue, kAudioQueueProperty_MagicCookie, cookie, size)
    //AudioQueueSetProperty(fCaptureQueue, kAudioQueueProperty_ChannelLayout, acl, size
    
    AudioStreamBasicDescription playbackDataFormat;
    
    /*
    playbackDataFormat.mSampleRate = fSampleRate;
    playbackDataFormat.mFormatID = kAudioFormatLinearPCM;
    playbackDataFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked | kLinearPCMFormatFlagIsNonInterleaved;
    playbackDataFormat.mBytesPerPacket = sizeof(float);
    playbackDataFormat.mFramesPerPacket = 1;
    playbackDataFormat.mBytesPerFrame = sizeof(float);
    playbackDataFormat.mChannelsPerFrame = fPlaybackChannels;
    playbackDataFormat.mBitsPerChannel = 32;
     */
    
    playbackDataFormat.mSampleRate = fSampleRate;
    playbackDataFormat.mFormatID = kAudioFormatLinearPCM;
    playbackDataFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    playbackDataFormat.mBytesPerPacket = 4;
    playbackDataFormat.mFramesPerPacket = 1;
    playbackDataFormat.mBytesPerFrame = 4;
    playbackDataFormat.mChannelsPerFrame = fPlaybackChannels;
    playbackDataFormat.mBitsPerChannel = 16;
    
    
    if ((err = AudioQueueNewOutput(&playbackDataFormat, PlaybackCallback, this, CFRunLoopGetCurrent(), kCFRunLoopCommonModes, 0, &fPlaybackQueue)) != noErr) {
        Print4CharCode("error code : unknown", err);
        return -1;
    }
    
    for (int i = 0; i < kNumberBuffers; ++i) {
        if ((err = AudioQueueAllocateBuffer(fPlaybackQueue, bufferByteSize, &fPlaybackQueueBuffers[i])) != noErr) {
            printf("Playback AudioQueueAllocateBuffer failed %d\n", err);
        }
        //if ((err = AudioQueueEnqueueBuffer(fPlaybackQueue, fPlaybackQueueBuffers[i], 0, NULL)) != noErr) {
        //    printf("Playback AudioQueueEnqueueBuffer failed %d\n", err);
        //}
    }
    
    AudioQueueSetParameter(fPlaybackQueue, kAudioQueueParam_Volume, 1.0);
    
  
    //AudioQueueSetProperty(fPlaybackQueue, kAudioQueueProperty_MagicCookie, cookie, size);
    //AudioQueueSetProperty(fPlaybackQueue, kAudioQueueProperty_ChannelLayout, acl, size);
    //AudioQueueSetParameter(fPlaybackQueue, kAudioQueueParam_Volume, volume
    
    return 0;
}
int JackAudioQueueAdapter::Close()
{
    //AudioSessionSetActive(false); 
    
    AudioQueueDispose(fCaptureQueue, true);
    AudioQueueDispose(fPlaybackQueue, true);
    return 0;
}
  
int JackAudioQueueAdapter::Start()
{
    for (int i = 0; i < kNumberBuffers; ++i) {
		PlaybackCallback(this, fPlaybackQueue, fPlaybackQueueBuffers[i]);			
	}

    AudioQueueStart(fCaptureQueue, NULL);
    AudioQueueStart(fPlaybackQueue, NULL);
    
    return 0;
}
    
int JackAudioQueueAdapter::Stop()
{
        
    AudioQueueStop(fCaptureQueue, NULL);
    AudioQueueStop(fPlaybackQueue, NULL);
        
    return 0;
}


int JackAudioQueueAdapter::SetSampleRate(jack_nframes_t sample_rate)
{
    return 0;
}

int JackAudioQueueAdapter::SetBufferSize(jack_nframes_t buffer_size)
{
    return 0;
}
   
};
