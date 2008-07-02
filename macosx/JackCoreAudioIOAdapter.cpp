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

#include "JackCoreAudioIOAdapter.h"
#include "JackError.h"
#include <unistd.h>

namespace Jack
{

static void printError(OSStatus err)
{
    switch (err) {
        case kAudioHardwareNoError:
            jack_log("error code : kAudioHardwareNoError");
            break;
        case kAudioConverterErr_FormatNotSupported:
            jack_log("error code : kAudioConverterErr_FormatNotSupported");
            break;
        case kAudioConverterErr_OperationNotSupported:
            jack_log("error code : kAudioConverterErr_OperationNotSupported");
            break;
        case kAudioConverterErr_PropertyNotSupported:
            jack_log("error code : kAudioConverterErr_PropertyNotSupported");
            break;
        case kAudioConverterErr_InvalidInputSize:
            jack_log("error code : kAudioConverterErr_InvalidInputSize");
            break;
        case kAudioConverterErr_InvalidOutputSize:
            jack_log("error code : kAudioConverterErr_InvalidOutputSize");
            break;
        case kAudioConverterErr_UnspecifiedError:
            jack_log("error code : kAudioConverterErr_UnspecifiedError");
            break;
        case kAudioConverterErr_BadPropertySizeError:
            jack_log("error code : kAudioConverterErr_BadPropertySizeError");
            break;
        case kAudioConverterErr_RequiresPacketDescriptionsError:
            jack_log("error code : kAudioConverterErr_RequiresPacketDescriptionsError");
            break;
        case kAudioConverterErr_InputSampleRateOutOfRange:
            jack_log("error code : kAudioConverterErr_InputSampleRateOutOfRange");
            break;
        case kAudioConverterErr_OutputSampleRateOutOfRange:
            jack_log("error code : kAudioConverterErr_OutputSampleRateOutOfRange");
            break;
        case kAudioHardwareNotRunningError:
            jack_log("error code : kAudioHardwareNotRunningError");
            break;
        case kAudioHardwareUnknownPropertyError:
            jack_log("error code : kAudioHardwareUnknownPropertyError");
            break;
        case kAudioHardwareIllegalOperationError:
            jack_log("error code : kAudioHardwareIllegalOperationError");
            break;
        case kAudioHardwareBadDeviceError:
            jack_log("error code : kAudioHardwareBadDeviceError");
            break;
        case kAudioHardwareBadStreamError:
            jack_log("error code : kAudioHardwareBadStreamError");
            break;
        case kAudioDeviceUnsupportedFormatError:
            jack_log("error code : kAudioDeviceUnsupportedFormatError");
            break;
        case kAudioDevicePermissionsError:
            jack_log("error code : kAudioDevicePermissionsError");
            break;
        case kAudioHardwareBadObjectError:
            jack_log("error code : kAudioHardwareBadObjectError");
            break;
        case kAudioHardwareUnsupportedOperationError:
            jack_log("error code : kAudioHardwareUnsupportedOperationError");
            break;
        default:
            jack_log("error code : unknown");
            break;
    }
}

OSStatus JackCoreAudioIOAdapter::SRNotificationCallback(AudioDeviceID inDevice,
                                                        UInt32 inChannel,
                                                        Boolean	isInput,
                                                        AudioDevicePropertyID inPropertyID,
                                                        void* inClientData)
{
    JackCoreAudioIOAdapter* driver = static_cast<JackCoreAudioIOAdapter*>(inClientData);

    switch (inPropertyID) {

        case kAudioDevicePropertyNominalSampleRate: {
            jack_log("JackCoreAudioDriver::SRNotificationCallback kAudioDevicePropertyNominalSampleRate");
            driver->fState = true;
            break;
        }
    }

    return noErr;
}
            
OSStatus JackCoreAudioIOAdapter::Render(void *inRefCon,
                                        AudioUnitRenderActionFlags *ioActionFlags,
                                        const AudioTimeStamp *inTimeStamp,
                                        UInt32 inBusNumber,
                                        UInt32 inNumberFrames,
                                        AudioBufferList *ioData)
{
    JackCoreAudioIOAdapter* adapter = static_cast<JackCoreAudioIOAdapter*>(inRefCon);
     AudioUnitRender(adapter->fAUHAL, ioActionFlags, inTimeStamp, 1, inNumberFrames, adapter->fInputData);
      
    if (!adapter->fRunning) {
        adapter->fRunning = true;
        jack_time_t time = jack_get_time();
        adapter->fProducerDLL.Init(time);
        adapter->fConsumerDLL.Init(time);
    }
    
    // DLL 
    jack_time_t time = jack_get_time();
    adapter->fProducerDLL.IncFrame(time);
    jack_nframes_t time1 = adapter->fConsumerDLL.Time2Frames(time);
    jack_nframes_t time2 = adapter->fProducerDLL.Time2Frames(time);
    
    double src_ratio_output = double(time2) / double(time1);
    double src_ratio_input = double(time1) / double(time2);
 
    if (src_ratio_input < 0.7f || src_ratio_input > 1.3f) {
        jack_error("src_ratio_input = %f", src_ratio_input);
        src_ratio_input = 1;
        time1 = 1;
        time2 = 1;
    }
    
    if (src_ratio_output < 0.7f || src_ratio_output > 1.3f) {
        jack_error("src_ratio_output = %f", src_ratio_output);
        src_ratio_output = 1;
        time1 = 1;
        time2 = 1;
    }
  
    src_ratio_input = Range(0.7f, 1.3f, src_ratio_input);
    src_ratio_output = Range(0.7f, 1.3f, src_ratio_output);
    jack_log("Callback resampler src_ratio_input = %f src_ratio_output = %f", src_ratio_input, src_ratio_output);
     
    for (int i = 0; i < adapter->fCaptureChannels; i++) {
        adapter->fCaptureRingBuffer[i]->SetRatio(time1, time2);
        adapter->fCaptureRingBuffer[i]->WriteResample((float*)adapter->fInputData->mBuffers[i].mData, inNumberFrames);
    }
    
    for (int i = 0; i < adapter->fPlaybackChannels; i++) {
        adapter->fPlaybackRingBuffer[i]->SetRatio(time2, time1);
        adapter->fPlaybackRingBuffer[i]->ReadResample((float*)ioData->mBuffers[i].mData, inNumberFrames); 
    }
   
    return noErr;
}

OSStatus JackCoreAudioIOAdapter::GetDefaultDevice(AudioDeviceID* id)
{
    OSStatus res;
    UInt32 theSize = sizeof(UInt32);
    AudioDeviceID inDefault;
    AudioDeviceID outDefault;

    if ((res = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultInputDevice, &theSize, &inDefault)) != noErr)
        return res;

    if ((res = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &theSize, &outDefault)) != noErr)
        return res;

    jack_log("GetDefaultDevice: input = %ld output = %ld", inDefault, outDefault);

    // Get the device only if default input and ouput are the same
    if (inDefault == outDefault) {
        *id = inDefault;
        return noErr;
    } else {
        jack_error("Default input and output devices are not the same !!");
        return kAudioHardwareBadDeviceError;
    }
}

OSStatus JackCoreAudioIOAdapter::GetTotalChannels(AudioDeviceID device, int* channelCount, bool isInput)
{
    OSStatus err = noErr;
    UInt32	outSize;
    Boolean	outWritable;
    AudioBufferList* bufferList = 0;

    *channelCount = 0;
    err = AudioDeviceGetPropertyInfo(device, 0, isInput, kAudioDevicePropertyStreamConfiguration, &outSize, &outWritable);
    if (err == noErr) {
        bufferList = (AudioBufferList*)malloc(outSize);
        err = AudioDeviceGetProperty(device, 0, isInput, kAudioDevicePropertyStreamConfiguration, &outSize, bufferList);
        if (err == noErr) {
            for (unsigned int i = 0; i < bufferList->mNumberBuffers; i++)
                *channelCount += bufferList->mBuffers[i].mNumberChannels;
        }

        if (bufferList)
            free(bufferList);
    }

    return err;
}

// Setup
int JackCoreAudioIOAdapter::SetupDevices(const char* capture_driver_uid,
                                        const char* playback_driver_uid,
                                        char* capture_driver_name,
                                        char* playback_driver_name)
{
    if (GetDefaultDevice(&fDeviceID) != noErr) {
        jack_error("Cannot open default device");
        return -1;
    }
    
    return 0;
}

int JackCoreAudioIOAdapter::SetupChannels(bool capturing,
                                        bool playing,
                                        int& inchannels,
                                        int& outchannels,
                                        int& in_nChannels,
                                        int& out_nChannels,
                                        bool strict)
{
    OSStatus err = noErr;
    
    err = GetTotalChannels(fDeviceID, &in_nChannels, true);
    if (err != noErr) {
        jack_error("Cannot get input channel number");
        printError(err);
        return -1;
    }
    
    err = GetTotalChannels(fDeviceID, &out_nChannels, false);
    if (err != noErr) {
        jack_error("Cannot get output channel number");
        printError(err);
        return -1;
    }
    
    return 0;
}

int JackCoreAudioIOAdapter::SetupBufferSizeAndSampleRate(jack_nframes_t nframes, jack_nframes_t samplerate)
{
    OSStatus err = noErr;
    UInt32 outSize;
    Float64 sampleRate;

    // Setting buffer size
    outSize = sizeof(UInt32);
    err = AudioDeviceSetProperty(fDeviceID, NULL, 0, false, kAudioDevicePropertyBufferFrameSize, outSize, &nframes);
    if (err != noErr) {
        jack_error("Cannot set buffer size %ld", nframes);
        printError(err);
        return -1;
    }

    // Get sample rate
    outSize =  sizeof(Float64);
    err = AudioDeviceGetProperty(fDeviceID, 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyNominalSampleRate, &outSize, &sampleRate);
    if (err != noErr) {
        jack_error("Cannot get current sample rate");
        printError(err);
        return -1;
    }

    // If needed, set new sample rate
    if (samplerate != (jack_nframes_t)sampleRate) {
        sampleRate = (Float64)samplerate;

        // To get SR change notification
        err = AudioDeviceAddPropertyListener(fDeviceID, 0, true, kAudioDevicePropertyNominalSampleRate, SRNotificationCallback, this);
        if (err != noErr) {
            jack_error("Error calling AudioDeviceAddPropertyListener with kAudioDevicePropertyNominalSampleRate");
            printError(err);
            return -1;
        }
        err = AudioDeviceSetProperty(fDeviceID, NULL, 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyNominalSampleRate, outSize, &sampleRate);
        if (err != noErr) {
            jack_error("Cannot set sample rate = %ld", samplerate);
            printError(err);
            return -1;
        }

        // Waiting for SR change notification
        int count = 0;
        while (!fState && count++ < 100) {
            usleep(100000);
            jack_log("Wait count = %ld", count);
        }

        // Remove SR change notification
        AudioDeviceRemovePropertyListener(fDeviceID, 0, true, kAudioDevicePropertyNominalSampleRate, SRNotificationCallback);
    }

    return 0;
}

int JackCoreAudioIOAdapter::SetupBuffers(int inchannels, int outchannels)
{
    jack_log("JackCoreAudioIOAdapter::SetupBuffers: input = %ld output = %ld", inchannels, outchannels);
    
    // Prepare buffers
    fInputData = (AudioBufferList*)malloc(sizeof(UInt32) + inchannels * sizeof(AudioBuffer));
    if (fInputData == 0) {
        jack_error("Cannot allocate memory for input buffers");
        return -1;
    }
    fInputData->mNumberBuffers = inchannels;
    for (int i = 0; i < fCaptureChannels; i++) {
        fInputData->mBuffers[i].mNumberChannels = 1;
        fInputData->mBuffers[i].mDataByteSize = fBufferSize * sizeof(float);
        fInputData->mBuffers[i].mData = malloc(fBufferSize * sizeof(float));
    }
    return 0;
}

void JackCoreAudioIOAdapter::DisposeBuffers()
{
    if (fInputData) {
        for (int i = 0; i < fCaptureChannels; i++) 
            free(fInputData->mBuffers[i].mData);
        free(fInputData);
        fInputData = 0;
    }
}

int JackCoreAudioIOAdapter::OpenAUHAL(bool capturing,
                                    bool playing,
                                    int inchannels,
                                    int outchannels,
                                    int in_nChannels,
                                    int out_nChannels,
                                    jack_nframes_t nframes,
                                    jack_nframes_t samplerate,
                                    bool strict)
{
    ComponentResult err1;
    UInt32 enableIO;
    AudioStreamBasicDescription srcFormat, dstFormat;

    jack_log("OpenAUHAL capturing = %ld playing = %ld playing = %ld outchannels = %ld in_nChannels = %ld out_nChannels = %ld ", capturing, playing, inchannels, inchannels, in_nChannels, out_nChannels);

    // AUHAL
    ComponentDescription cd = {kAudioUnitType_Output, kAudioUnitSubType_HALOutput, kAudioUnitManufacturer_Apple, 0, 0};
    Component HALOutput = FindNextComponent(NULL, &cd);

    err1 = OpenAComponent(HALOutput, &fAUHAL);
    if (err1 != noErr) {
        jack_error("Error calling OpenAComponent");
        printError(err1);
        return -1;
    }

    err1 = AudioUnitInitialize(fAUHAL);
    if (err1 != noErr) {
        jack_error("Cannot initialize AUHAL unit");
        printError(err1);
        return -1;
    }

    // Start I/O
    enableIO = 1;
    if (capturing && inchannels > 0) {
        jack_log("Setup AUHAL input");
        err1 = AudioUnitSetProperty(fAUHAL, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, 1, &enableIO, sizeof(enableIO));
        if (err1 != noErr) {
            jack_error("Error calling AudioUnitSetProperty - kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input");
            printError(err1);
            if (strict)
                return -1;
        }
    }

    if (playing && outchannels > 0) {
        jack_log("Setup AUHAL output");
        err1 = AudioUnitSetProperty(fAUHAL, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, 0, &enableIO, sizeof(enableIO));
        if (err1 != noErr) {
            jack_error("Error calling AudioUnitSetProperty - kAudioOutputUnitProperty_EnableIO,kAudioUnitScope_Output");
            printError(err1);
            if (strict)
                return -1;
        }
    }

    // Setup up choosen device, in both input and output cases
    err1 = AudioUnitSetProperty(fAUHAL, kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Global, 0, &fDeviceID, sizeof(AudioDeviceID));
    if (err1 != noErr) {
        jack_error("Error calling AudioUnitSetProperty - kAudioOutputUnitProperty_CurrentDevice");
        printError(err1);
        if (strict)
            return -1;
    }

    // Set buffer size
    if (capturing && inchannels > 0) {
        err1 = AudioUnitSetProperty(fAUHAL, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 1, (UInt32*) & nframes, sizeof(UInt32));
        if (err1 != noErr) {
            jack_error("Error calling AudioUnitSetProperty - kAudioUnitProperty_MaximumFramesPerSlice");
            printError(err1);
            if (strict)
                return -1;
        }
    }

    if (playing && outchannels > 0) {
        err1 = AudioUnitSetProperty(fAUHAL, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, (UInt32*) & nframes, sizeof(UInt32));
        if (err1 != noErr) {
            jack_error("Error calling AudioUnitSetProperty - kAudioUnitProperty_MaximumFramesPerSlice");
            printError(err1);
            if (strict)
                return -1;
        }
    }

    // Setup channel map
    if (capturing && inchannels > 0 && inchannels < in_nChannels) {
        SInt32 chanArr[in_nChannels];
        for (int i = 0; i < in_nChannels; i++) {
            chanArr[i] = -1;
        }
        for (int i = 0; i < inchannels; i++) {
            chanArr[i] = i;
        }
        AudioUnitSetProperty(fAUHAL, kAudioOutputUnitProperty_ChannelMap , kAudioUnitScope_Input, 1, chanArr, sizeof(SInt32) * in_nChannels);
        if (err1 != noErr) {
            jack_error("Error calling AudioUnitSetProperty - kAudioOutputUnitProperty_ChannelMap 1");
            printError(err1);
        }
    }

    if (playing && outchannels > 0 && outchannels < out_nChannels) {
        SInt32 chanArr[out_nChannels];
        for (int i = 0;	i < out_nChannels; i++) {
            chanArr[i] = -1;
        }
        for (int i = 0; i < outchannels; i++) {
            chanArr[i] = i;
        }
        err1 = AudioUnitSetProperty(fAUHAL, kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Output, 0, chanArr, sizeof(SInt32) * out_nChannels);
        if (err1 != noErr) {
            jack_error("Error calling AudioUnitSetProperty - kAudioOutputUnitProperty_ChannelMap 0");
            printError(err1);
        }
    }

    // Setup stream converters
    jack_log("Setup AUHAL input stream converter SR = %ld", samplerate);
    srcFormat.mSampleRate = samplerate;
    srcFormat.mFormatID = kAudioFormatLinearPCM;
    srcFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked | kLinearPCMFormatFlagIsNonInterleaved;
    srcFormat.mBytesPerPacket = sizeof(float);
    srcFormat.mFramesPerPacket = 1;
    srcFormat.mBytesPerFrame = sizeof(float);
    srcFormat.mChannelsPerFrame = outchannels;
    srcFormat.mBitsPerChannel = 32;

    err1 = AudioUnitSetProperty(fAUHAL, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &srcFormat, sizeof(AudioStreamBasicDescription));
    if (err1 != noErr) {
        jack_error("Error calling AudioUnitSetProperty - kAudioUnitProperty_StreamFormat kAudioUnitScope_Input");
        printError(err1);
    }

    jack_log("Setup AUHAL output stream converter SR = %ld", samplerate);
    dstFormat.mSampleRate = samplerate;
    dstFormat.mFormatID = kAudioFormatLinearPCM;
    dstFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked | kLinearPCMFormatFlagIsNonInterleaved;
    dstFormat.mBytesPerPacket = sizeof(float);
    dstFormat.mFramesPerPacket = 1;
    dstFormat.mBytesPerFrame = sizeof(float);
    dstFormat.mChannelsPerFrame = inchannels;
    dstFormat.mBitsPerChannel = 32;

    err1 = AudioUnitSetProperty(fAUHAL, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 1, &dstFormat, sizeof(AudioStreamBasicDescription));
    if (err1 != noErr) {
        jack_error("Error calling AudioUnitSetProperty - kAudioUnitProperty_StreamFormat kAudioUnitScope_Output");
        printError(err1);
    }

    // Setup callbacks
    if (inchannels > 0 && outchannels == 0) {
        AURenderCallbackStruct output;
        output.inputProc = Render;
        output.inputProcRefCon = this;
        err1 = AudioUnitSetProperty(fAUHAL, kAudioOutputUnitProperty_SetInputCallback, kAudioUnitScope_Global, 0, &output, sizeof(output));
        if (err1 != noErr) {
            jack_error("Error calling  AudioUnitSetProperty - kAudioUnitProperty_SetRenderCallback 1");
            printError(err1);
            return -1;
        }
    } else {
        AURenderCallbackStruct output;
        output.inputProc = Render;
        output.inputProcRefCon = this;
        err1 = AudioUnitSetProperty(fAUHAL, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &output, sizeof(output));
        if (err1 != noErr) {
            jack_error("Error calling AudioUnitSetProperty - kAudioUnitProperty_SetRenderCallback 0");
            printError(err1);
            return -1;
        }
    }

    return 0;
}

void JackCoreAudioIOAdapter::CloseAUHAL()
{
    AudioUnitUninitialize(fAUHAL);
    CloseComponent(fAUHAL);
}
  
int JackCoreAudioIOAdapter::Open()
{
    OSStatus err;
    int in_nChannels = 0;
    int out_nChannels = 0;
   
    if (SetupDevices("", "", "", "") < 0)
        return -1;

    if (SetupChannels(true, true, fCaptureChannels, fPlaybackChannels, in_nChannels, out_nChannels, true) < 0)
        return -1;

    if (SetupBufferSizeAndSampleRate(fBufferSize, fSampleRate)  < 0)
        return -1;

    if (OpenAUHAL(true, true, fCaptureChannels, fPlaybackChannels, in_nChannels, out_nChannels, fBufferSize, fSampleRate, true) < 0)
        goto error;

    if (SetupBuffers(fCaptureChannels, fPlaybackChannels) < 0)
        goto error;
        
    err = AudioOutputUnitStart(fAUHAL);
    if (err != noErr)
        goto error;
  
    return 0;

error:
    Close();
    return -1;
}

int JackCoreAudioIOAdapter::Close()
{
    AudioOutputUnitStop(fAUHAL);
    DisposeBuffers();
    CloseAUHAL();
    return 0;
}

}
