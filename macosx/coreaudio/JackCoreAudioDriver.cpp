/*
Copyright (C) 2004-2008 Grame

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

#include "JackCoreAudioDriver.h"
#include "JackEngineControl.h"
#include "JackMachThread.h"
#include "JackGraphManager.h"
#include "JackError.h"
#include "JackClientControl.h"
#include "JackDriverLoader.h"
#include "JackGlobals.h"
#include "JackCompilerDeps.h"
#include <iostream>

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

static OSStatus DisplayDeviceNames()
{
    UInt32 size;
    Boolean isWritable;
    int i, deviceNum;
    OSStatus err;
    CFStringRef UIname;

    err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &size, &isWritable);
    if (err != noErr)
        return err;

    deviceNum = size / sizeof(AudioDeviceID);
    AudioDeviceID devices[deviceNum];

    err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &size, devices);
    if (err != noErr)
        return err;

    for (i = 0; i < deviceNum; i++) {
        char device_name[256];
        char internal_name[256];

        size = sizeof(CFStringRef);
        UIname = NULL;
        err = AudioDeviceGetProperty(devices[i], 0, false, kAudioDevicePropertyDeviceUID, &size, &UIname);
        if (err == noErr) {
            CFStringGetCString(UIname, internal_name, 256, CFStringGetSystemEncoding());
        } else {
            goto error;
        }

        size = 256;
        err = AudioDeviceGetProperty(devices[i], 0, false, kAudioDevicePropertyDeviceName, &size, device_name);
        if (err != noErr)
            return err;

        jack_info("Device name = \'%s\', internal_name = \'%s\' (to be used as -C, -P, or -d parameter)", device_name, internal_name);
    }

    return noErr;

error:
    if (UIname != NULL)
        CFRelease(UIname);
    return err;
}

OSStatus JackCoreAudioDriver::Render(void *inRefCon,
                                     AudioUnitRenderActionFlags *ioActionFlags,
                                     const AudioTimeStamp *inTimeStamp,
                                     UInt32 inBusNumber,
                                     UInt32 inNumberFrames,
                                     AudioBufferList *ioData)
{
    JackCoreAudioDriver* driver = (JackCoreAudioDriver*)inRefCon;
    driver->fActionFags = ioActionFlags;
    driver->fCurrentTime = (AudioTimeStamp *)inTimeStamp;
    driver->fDriverOutputData = ioData;
    driver->CycleTakeBeginTime();
    return driver->Process();
}

int JackCoreAudioDriver::Read()
{
    AudioUnitRender(fAUHAL, fActionFags, fCurrentTime, 1, fEngineControl->fBufferSize, fJackInputData);
    return 0;
}

int JackCoreAudioDriver::Write()
{
    for (int i = 0; i < fPlaybackChannels; i++) {
        if (fGraphManager->GetConnectionsNum(fPlaybackPortList[i]) > 0) {
            float* buffer = GetOutputBuffer(i);
            int size = sizeof(float) * fEngineControl->fBufferSize;
            memcpy((float*)fDriverOutputData->mBuffers[i].mData, buffer, size);
            // Monitor ports
            if (fWithMonitorPorts && fGraphManager->GetConnectionsNum(fMonitorPortList[i]) > 0)
                memcpy(GetMonitorBuffer(i), buffer, size);
        } else {
            memset((float*)fDriverOutputData->mBuffers[i].mData, 0, sizeof(float) * fEngineControl->fBufferSize);
        }
    }
    return 0;
}

// Will run only once
OSStatus JackCoreAudioDriver::MeasureCallback(AudioDeviceID inDevice,
        const AudioTimeStamp* inNow,
        const AudioBufferList* inInputData,
        const AudioTimeStamp* inInputTime,
        AudioBufferList* outOutputData,
        const AudioTimeStamp* inOutputTime,
        void* inClientData)
{
    JackCoreAudioDriver* driver = (JackCoreAudioDriver*)inClientData;
    AudioDeviceStop(driver->fDeviceID, MeasureCallback);
    
    jack_log("JackCoreAudioDriver::MeasureCallback called");
    JackMachThread::GetParams(&driver->fEngineControl->fPeriod, &driver->fEngineControl->fComputation, &driver->fEngineControl->fConstraint);
    
    // Setup threadded based log function
    set_threaded_log_function();
    return noErr;
}

OSStatus JackCoreAudioDriver::SRNotificationCallback(AudioDeviceID inDevice,
        UInt32 inChannel,
        Boolean	isInput,
        AudioDevicePropertyID inPropertyID,
        void* inClientData)
{
    JackCoreAudioDriver* driver = (JackCoreAudioDriver*)inClientData;

    switch (inPropertyID) {

        case kAudioDevicePropertyNominalSampleRate: {
            jack_log("JackCoreAudioDriver::SRNotificationCallback kAudioDevicePropertyNominalSampleRate");
            driver->fState = true;
            break;
        }
    }

    return noErr;
}

// A better implementation would try to recover in case of hardware device change (see HALLAB HLFilePlayerWindowControllerAudioDevicePropertyListenerProc code)

OSStatus JackCoreAudioDriver::DeviceNotificationCallback(AudioDeviceID inDevice,
        UInt32 inChannel,
        Boolean	isInput,
        AudioDevicePropertyID inPropertyID,
        void* inClientData)
{
    JackCoreAudioDriver* driver = (JackCoreAudioDriver*)inClientData;
         
    switch (inPropertyID) {

        case kAudioDeviceProcessorOverload:
            jack_error("JackCoreAudioDriver::DeviceNotificationCallback kAudioDeviceProcessorOverload");
            jack_time_t cur_time = GetMicroSeconds();
            driver->NotifyXRun(cur_time, float(cur_time - driver->fBeginDateUst));   // Better this value than nothing... 
            break;

        case kAudioDevicePropertyStreamConfiguration:
        case kAudioDevicePropertyNominalSampleRate: {

            UInt32 outSize = sizeof(Float64);
            Float64 sampleRate;
            int in_nChannels = 0;
            int out_nChannels = 0;
            char capture_driver_name[256];
            char playback_driver_name[256];
            CFStringRef ref;

            // Stop and restart
            driver->Stop();
            driver->RemoveListeners();
            driver->CloseAUHAL();

            OSStatus err = AudioDeviceGetProperty(driver->fDeviceID, 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyNominalSampleRate, &outSize, &sampleRate);
            if (err != noErr) {
                jack_error("Cannot get current sample rate");
                printError(err);
            }
            jack_log("JackCoreAudioDriver::DeviceNotificationCallback kAudioDevicePropertyNominalSampleRate %ld", long(sampleRate));

            if (driver->SetupDevices(driver->fCaptureUID, driver->fPlaybackUID, capture_driver_name, playback_driver_name) < 0)
                return -1;

            if (driver->SetupChannels(driver->fCapturing, driver->fPlaying, driver->fInChannels, driver->fOutChannels, in_nChannels, out_nChannels, false) < 0)
                return -1;

            if (driver->SetupBufferSizeAndSampleRate(driver->fEngineControl->fBufferSize, sampleRate)  < 0)
                return -1;

            if (driver->OpenAUHAL(driver->fCapturing,
                                  driver->fPlaying,
                                  driver->fInChannels,
                                  driver->fOutChannels,
                                  in_nChannels,
                                  out_nChannels,
                                  driver->fEngineControl->fBufferSize,
                                  sampleRate,
                                  false) < 0)
                goto error;

            if (driver->AddListeners() < 0)
                goto error;

            driver->Start();

            // Send notification to be used in JackPilot or JackRouter plugin
            jack_error("Device restart...");
            ref = CFStringCreateWithCString(NULL, driver->fEngineControl->fServerName, kCFStringEncodingMacRoman);
            CFNotificationCenterPostNotificationWithOptions(CFNotificationCenterGetDistributedCenter(),
                    CFSTR("com.grame.jackserver.restart"),
                    ref,
                    NULL,
                    kCFNotificationDeliverImmediately | kCFNotificationPostToAllSessions);
            CFRelease(ref);
            return noErr;
error:
            driver->CloseAUHAL();
            break;
        }
    }
    return noErr;
}

OSStatus JackCoreAudioDriver::GetDeviceIDFromUID(const char* UID, AudioDeviceID* id)
{
    UInt32 size = sizeof(AudioValueTranslation);
    CFStringRef inIUD = CFStringCreateWithCString(NULL, UID, CFStringGetSystemEncoding());
    AudioValueTranslation value = { &inIUD, sizeof(CFStringRef), id, sizeof(AudioDeviceID) };

    if (inIUD == NULL) {
        return kAudioHardwareUnspecifiedError;
    } else {
        OSStatus res = AudioHardwareGetProperty(kAudioHardwarePropertyDeviceForUID, &size, &value);
        CFRelease(inIUD);
        jack_log("GetDeviceIDFromUID %s %ld", UID, *id);
        return (*id == kAudioDeviceUnknown) ? kAudioHardwareBadDeviceError : res;
    }
}

OSStatus JackCoreAudioDriver::GetDefaultDevice(AudioDeviceID* id)
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

OSStatus JackCoreAudioDriver::GetDefaultInputDevice(AudioDeviceID* id)
{
    OSStatus res;
    UInt32 theSize = sizeof(UInt32);
    AudioDeviceID inDefault;

    if ((res = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultInputDevice, &theSize, &inDefault)) != noErr)
        return res;

    jack_log("GetDefaultInputDevice: input = %ld ", inDefault);
    *id = inDefault;
    return noErr;
}

OSStatus JackCoreAudioDriver::GetDefaultOutputDevice(AudioDeviceID* id)
{
    OSStatus res;
    UInt32 theSize = sizeof(UInt32);
    AudioDeviceID outDefault;

    if ((res = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &theSize, &outDefault)) != noErr)
        return res;

    jack_log("GetDefaultOutputDevice: output = %ld", outDefault);
    *id = outDefault;
    return noErr;
}

OSStatus JackCoreAudioDriver::GetDeviceNameFromID(AudioDeviceID id, char* name)
{
    UInt32 size = 256;
    return AudioDeviceGetProperty(id, 0, false, kAudioDevicePropertyDeviceName, &size, name);
}

OSStatus JackCoreAudioDriver::GetTotalChannels(AudioDeviceID device, int& channelCount, bool isInput)
{
    OSStatus err = noErr;
    UInt32	outSize;
    Boolean	outWritable;
    AudioBufferList* bufferList = 0;

    channelCount = 0;
    err = AudioDeviceGetPropertyInfo(device, 0, isInput, kAudioDevicePropertyStreamConfiguration, &outSize, &outWritable);
    if (err == noErr) {
        bufferList = (AudioBufferList*)malloc(outSize);
        err = AudioDeviceGetProperty(device, 0, isInput, kAudioDevicePropertyStreamConfiguration, &outSize, bufferList);
        if (err == noErr) {
            for (unsigned int i = 0; i < bufferList->mNumberBuffers; i++)
                channelCount += bufferList->mBuffers[i].mNumberChannels;
        }

        if (bufferList)
            free(bufferList);
    }

    return err;
}

JackCoreAudioDriver::JackCoreAudioDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table)
        : JackAudioDriver(name, alias, engine, table), fJackInputData(NULL), fDriverOutputData(NULL), fState(false), fIOUsage(1.f)
{}

JackCoreAudioDriver::~JackCoreAudioDriver()
{}

int JackCoreAudioDriver::SetupDevices(const char* capture_driver_uid, const char* playback_driver_uid, char* capture_driver_name, char* playback_driver_name)
{
    capture_driver_name[0] = 0;
    playback_driver_name[0] = 0;

    // Duplex
    if (strcmp(capture_driver_uid, "") != 0 && strcmp(playback_driver_uid, "") != 0) {
        jack_log("JackCoreAudioDriver::Open duplex");
        if (GetDeviceIDFromUID(playback_driver_uid, &fDeviceID) != noErr) {
            if (GetDefaultDevice(&fDeviceID) != noErr) {
                jack_error("Cannot open default device");
                return -1;
            }
        }
        if (GetDeviceNameFromID(fDeviceID, capture_driver_name) != noErr || GetDeviceNameFromID(fDeviceID, playback_driver_name) != noErr) {
            jack_error("Cannot get device name from device ID");
            return -1;
        }

        // Capture only
    } else if (strcmp(capture_driver_uid, "") != 0) {
        jack_log("JackCoreAudioDriver::Open capture only");
        if (GetDeviceIDFromUID(capture_driver_uid, &fDeviceID) != noErr) {
            if (GetDefaultInputDevice(&fDeviceID) != noErr) {
                jack_error("Cannot open default device");
                return -1;
            }
        }
        if (GetDeviceNameFromID(fDeviceID, capture_driver_name) != noErr) {
            jack_error("Cannot get device name from device ID");
            return -1;
        }

        // Playback only
    } else if (strcmp(playback_driver_uid, "") != 0) {
        jack_log("JackCoreAudioDriver::Open playback only");
        if (GetDeviceIDFromUID(playback_driver_uid, &fDeviceID) != noErr) {
            if (GetDefaultOutputDevice(&fDeviceID) != noErr) {
                jack_error("Cannot open default device");
                return -1;
            }
        }
        if (GetDeviceNameFromID(fDeviceID, playback_driver_name) != noErr) {
            jack_error("Cannot get device name from device ID");
            return -1;
        }

        // Use default driver in duplex mode
    } else {
        jack_log("JackCoreAudioDriver::Open default driver");
        if (GetDefaultDevice(&fDeviceID) != noErr) {
            jack_error("Cannot open default device");
            return -1;
        }
        if (GetDeviceNameFromID(fDeviceID, capture_driver_name) != noErr || GetDeviceNameFromID(fDeviceID, playback_driver_name) != noErr) {
            jack_error("Cannot get device name from device ID");
            return -1;
        }
    }

    return 0;
}

int JackCoreAudioDriver::SetupChannels(bool capturing, bool playing, int& inchannels, int& outchannels, int& in_nChannels, int& out_nChannels, bool strict)
{
    OSStatus err = noErr;

    if (capturing) {
        err = GetTotalChannels(fDeviceID, in_nChannels, true);
        if (err != noErr) {
            jack_error("Cannot get input channel number");
            printError(err);
            return -1;
        }
    }

    if (playing) {
        err = GetTotalChannels(fDeviceID, out_nChannels, false);
        if (err != noErr) {
            jack_error("Cannot get output channel number");
            printError(err);
            return -1;
        }
    }

    if (inchannels > in_nChannels) {
        jack_error("This device hasn't required input channels inchannels = %ld in_nChannels = %ld", inchannels, in_nChannels);
        if (strict)
            return -1;
    }

    if (outchannels > out_nChannels) {
        jack_error("This device hasn't required output channels outchannels = %ld out_nChannels = %ld", outchannels, out_nChannels);
        if (strict)
            return -1;
    }

    if (inchannels == 0) {
        jack_log("Setup max in channels = %ld", in_nChannels);
        inchannels = in_nChannels;
    }

    if (outchannels == 0) {
        jack_log("Setup max out channels = %ld", out_nChannels);
        outchannels = out_nChannels;
    }

    return 0;
}

int JackCoreAudioDriver::SetupBufferSizeAndSampleRate(jack_nframes_t buffer_size, jack_nframes_t samplerate)
{
    OSStatus err = noErr;
    UInt32 outSize;
    Float64 sampleRate;

    // Setting buffer size
    outSize = sizeof(UInt32);
    err = AudioDeviceSetProperty(fDeviceID, NULL, 0, false, kAudioDevicePropertyBufferFrameSize, outSize, &buffer_size);
    if (err != noErr) {
        jack_error("Cannot set buffer size %ld", buffer_size);
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

int JackCoreAudioDriver::OpenAUHAL(bool capturing,
                                   bool playing,
                                   int inchannels,
                                   int outchannels,
                                   int in_nChannels,
                                   int out_nChannels,
                                   jack_nframes_t buffer_size,
                                   jack_nframes_t samplerate,
                                   bool strict)
{
    ComponentResult err1;
    UInt32 enableIO;
    AudioStreamBasicDescription srcFormat, dstFormat;

    jack_log("OpenAUHAL capturing = %ld playing = %ld inchannels = %ld outchannels = %ld in_nChannels = %ld out_nChannels = %ld ", capturing, playing, inchannels, outchannels, in_nChannels, out_nChannels);

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
        err1 = AudioUnitSetProperty(fAUHAL, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 1, (UInt32*) & buffer_size, sizeof(UInt32));
        if (err1 != noErr) {
            jack_error("Error calling AudioUnitSetProperty - kAudioUnitProperty_MaximumFramesPerSlice");
            printError(err1);
            if (strict)
                return -1;
        }
    }

    if (playing && outchannels > 0) {
        err1 = AudioUnitSetProperty(fAUHAL, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, (UInt32*) & buffer_size, sizeof(UInt32));
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

int JackCoreAudioDriver::SetupBuffers(int inchannels)
{
    // Prepare buffers
    fJackInputData = (AudioBufferList*)malloc(sizeof(UInt32) + inchannels * sizeof(AudioBuffer));
    if (fJackInputData == 0) {
        jack_error("Cannot allocate memory for input buffers");
        return -1;
    }
    fJackInputData->mNumberBuffers = inchannels;
    for (int i = 0; i < fCaptureChannels; i++) {
        fJackInputData->mBuffers[i].mNumberChannels = 1;
        fJackInputData->mBuffers[i].mDataByteSize = fEngineControl->fBufferSize * sizeof(float);
    }
    return 0;
}

void JackCoreAudioDriver::DisposeBuffers()
{
    if (fJackInputData) {
        free(fJackInputData);
        fJackInputData = 0;
    }
}

void JackCoreAudioDriver::CloseAUHAL()
{
    AudioUnitUninitialize(fAUHAL);
    CloseComponent(fAUHAL);
}

int JackCoreAudioDriver::AddListeners()
{
    OSStatus err = noErr;

    // Add listeners
    err = AudioDeviceAddPropertyListener(fDeviceID, 0, true, kAudioDeviceProcessorOverload, DeviceNotificationCallback, this);
    if (err != noErr) {
        jack_error("Error calling AudioDeviceAddPropertyListener with kAudioDeviceProcessorOverload");
        printError(err);
        return -1;
    }

    err = AudioDeviceAddPropertyListener(fDeviceID, 0, true, kAudioHardwarePropertyDevices, DeviceNotificationCallback, this);
    if (err != noErr) {
        jack_error("Error calling AudioDeviceAddPropertyListener with kAudioHardwarePropertyDevices");
        printError(err);
        return -1;
    }

    err = AudioDeviceAddPropertyListener(fDeviceID, 0, true, kAudioDevicePropertyNominalSampleRate, DeviceNotificationCallback, this);
    if (err != noErr) {
        jack_error("Error calling AudioDeviceAddPropertyListener with kAudioDevicePropertyNominalSampleRate");
        printError(err);
        return -1;
    }

    err = AudioDeviceAddPropertyListener(fDeviceID, 0, true, kAudioDevicePropertyDeviceIsRunning, DeviceNotificationCallback, this);
    if (err != noErr) {
        jack_error("Error calling AudioDeviceAddPropertyListener with kAudioDevicePropertyDeviceIsRunning");
        printError(err);
        return -1;
    }

    err = AudioDeviceAddPropertyListener(fDeviceID, 0, true, kAudioDevicePropertyStreamConfiguration, DeviceNotificationCallback, this);
    if (err != noErr) {
        jack_error("Error calling AudioDeviceAddPropertyListener with kAudioDevicePropertyStreamConfiguration");
        printError(err);
        return -1;
    }

    err = AudioDeviceAddPropertyListener(fDeviceID, 0, false, kAudioDevicePropertyStreamConfiguration, DeviceNotificationCallback, this);
    if (err != noErr) {
        jack_error("Error calling AudioDeviceAddPropertyListener with kAudioDevicePropertyStreamConfiguration");
        printError(err);
        return -1;
    }

    if (!fEngineControl->fSyncMode && fIOUsage != 1.f) {
        UInt32 outSize = sizeof(float);
        err = AudioDeviceSetProperty(fDeviceID, NULL, 0, false, kAudioDevicePropertyIOCycleUsage, outSize, &fIOUsage);
        if (err != noErr) {
            jack_error("Error calling AudioDeviceSetProperty kAudioDevicePropertyIOCycleUsage");
            printError(err);
        }
    }

    return 0;
}

void JackCoreAudioDriver::RemoveListeners()
{
    AudioDeviceRemovePropertyListener(fDeviceID, 0, true, kAudioDeviceProcessorOverload, DeviceNotificationCallback);
    AudioDeviceRemovePropertyListener(fDeviceID, 0, true, kAudioHardwarePropertyDevices, DeviceNotificationCallback);
    AudioDeviceRemovePropertyListener(fDeviceID, 0, true, kAudioDevicePropertyNominalSampleRate, DeviceNotificationCallback);
    AudioDeviceRemovePropertyListener(fDeviceID, 0, true, kAudioDevicePropertyDeviceIsRunning, DeviceNotificationCallback);
    AudioDeviceRemovePropertyListener(fDeviceID, 0, true, kAudioDevicePropertyStreamConfiguration, DeviceNotificationCallback);
    AudioDeviceRemovePropertyListener(fDeviceID, 0, false, kAudioDevicePropertyStreamConfiguration, DeviceNotificationCallback);
}

int JackCoreAudioDriver::Open(jack_nframes_t buffer_size,
                              jack_nframes_t samplerate,
                              bool capturing,
                              bool playing,
                              int inchannels,
                              int outchannels,
                              bool monitor,
                              const char* capture_driver_uid,
                              const char* playback_driver_uid,
                              jack_nframes_t capture_latency,
                              jack_nframes_t playback_latency,
                              int async_output_latency)
{
    int in_nChannels = 0;
    int out_nChannels = 0;
    char capture_driver_name[256];
    char playback_driver_name[256];

    // Keep initial state
    fCapturing = capturing;
    fPlaying = playing;
    fInChannels = inchannels;
    fOutChannels = outchannels;
    fMonitor = monitor;
    strcpy(fCaptureUID, capture_driver_uid);
    strcpy(fPlaybackUID, playback_driver_uid);
    fCaptureLatency = capture_latency;
    fPlaybackLatency = playback_latency;
    fIOUsage = float(async_output_latency)/ 100.f;

    if (SetupDevices(capture_driver_uid, playback_driver_uid, capture_driver_name, playback_driver_name) < 0)
        return -1;

    // Generic JackAudioDriver Open
    if (JackAudioDriver::Open(buffer_size, samplerate, capturing, playing, inchannels, outchannels, monitor, capture_driver_name, playback_driver_name, capture_latency, playback_latency) != 0)
        return -1;

    if (SetupChannels(capturing, playing, inchannels, outchannels, in_nChannels, out_nChannels, true) < 0)
        return -1;

    if (SetupBufferSizeAndSampleRate(buffer_size, samplerate)  < 0)
        return -1;

    if (OpenAUHAL(capturing, playing, inchannels, outchannels, in_nChannels, out_nChannels, buffer_size, samplerate, true) < 0)
        goto error;

    if (capturing && inchannels > 0)
        if (SetupBuffers(inchannels) < 0)
            goto error;

    if (AddListeners() < 0)
        goto error;

    // Core driver may have changed the in/out values
    fCaptureChannels = inchannels;
    fPlaybackChannels = outchannels;
    return noErr;

error:
    Close();
    return -1;
}

int JackCoreAudioDriver::Close()
{
    jack_log("JackCoreAudioDriver::Close");
    Stop();
    JackAudioDriver::Close();
    RemoveListeners();
    DisposeBuffers();
    CloseAUHAL();
    return 0;
}

int JackCoreAudioDriver::Attach()
{
    OSStatus err;
    JackPort* port;
    jack_port_id_t port_index;
    UInt32 size;
    Boolean isWritable;
    char channel_name[64];
    char name[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
    char alias[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
    unsigned long port_flags = JackPortIsOutput | JackPortIsPhysical | JackPortIsTerminal;

    jack_log("JackCoreAudioDriver::Attach fBufferSize %ld fSampleRate %ld", fEngineControl->fBufferSize, fEngineControl->fSampleRate);

    for (int i = 0; i < fCaptureChannels; i++) {

        err = AudioDeviceGetPropertyInfo(fDeviceID, i + 1, true, kAudioDevicePropertyChannelName, &size, &isWritable);
        if (err != noErr)
            jack_log("AudioDeviceGetPropertyInfo kAudioDevicePropertyChannelName error ");
        if (err == noErr && size > 0) {
            err = AudioDeviceGetProperty(fDeviceID, i + 1, true, kAudioDevicePropertyChannelName, &size, channel_name);
            if (err != noErr)
                jack_log("AudioDeviceGetProperty kAudioDevicePropertyChannelName error ");
            snprintf(alias, sizeof(alias) - 1, "%s:%s:out_%s%u", fAliasName, fCaptureDriverName, channel_name, i + 1);
        } else {
            snprintf(alias, sizeof(alias) - 1, "%s:%s:out%u", fAliasName, fCaptureDriverName, i + 1);
        }

        snprintf(name, sizeof(name) - 1, "%s:capture_%d", fClientControl.fName, i + 1);

        if ((port_index = fGraphManager->AllocatePort(fClientControl.fRefNum, name, JACK_DEFAULT_AUDIO_TYPE, (JackPortFlags)port_flags, fEngineControl->fBufferSize)) == NO_PORT) {
            jack_error("Cannot register port for %s", name);
            return -1;
        }

        size = sizeof(UInt32);
        UInt32 value1 = 0;
        UInt32 value2 = 0;
        err = AudioDeviceGetProperty(fDeviceID, 0, true, kAudioDevicePropertyLatency, &size, &value1);
        if (err != noErr)
            jack_log("AudioDeviceGetProperty kAudioDevicePropertyLatency error ");
        err = AudioDeviceGetProperty(fDeviceID, 0, true, kAudioDevicePropertySafetyOffset, &size, &value2);
        if (err != noErr)
            jack_log("AudioDeviceGetProperty kAudioDevicePropertySafetyOffset error ");

        port = fGraphManager->GetPort(port_index);
        port->SetAlias(alias);
        port->SetLatency(fEngineControl->fBufferSize + value1 + value2 + fCaptureLatency);
        fCapturePortList[i] = port_index;
    }

    port_flags = JackPortIsInput | JackPortIsPhysical | JackPortIsTerminal;

    for (int i = 0; i < fPlaybackChannels; i++) {

        err = AudioDeviceGetPropertyInfo(fDeviceID, i + 1, false, kAudioDevicePropertyChannelName, &size, &isWritable);
        if (err != noErr)
            jack_log("AudioDeviceGetPropertyInfo kAudioDevicePropertyChannelName error ");
        if (err == noErr && size > 0) {
            err = AudioDeviceGetProperty(fDeviceID, i + 1, false, kAudioDevicePropertyChannelName, &size, channel_name);
            if (err != noErr)
                jack_log("AudioDeviceGetProperty kAudioDevicePropertyChannelName error ");
            snprintf(alias, sizeof(alias) - 1, "%s:%s:in_%s%u", fAliasName, fPlaybackDriverName, channel_name, i + 1);
        } else {
            snprintf(alias, sizeof(alias) - 1, "%s:%s:in%u", fAliasName, fPlaybackDriverName, i + 1);
        }

        snprintf(name, sizeof(name) - 1, "%s:playback_%d", fClientControl.fName, i + 1);

        if ((port_index = fGraphManager->AllocatePort(fClientControl.fRefNum, name, JACK_DEFAULT_AUDIO_TYPE, (JackPortFlags)port_flags, fEngineControl->fBufferSize)) == NO_PORT) {
            jack_error("Cannot register port for %s", name);
            return -1;
        }

        size = sizeof(UInt32);
        UInt32 value1 = 0;
        UInt32 value2 = 0;
        err = AudioDeviceGetProperty(fDeviceID, 0, false, kAudioDevicePropertyLatency, &size, &value1);
        if (err != noErr)
            jack_log("AudioDeviceGetProperty kAudioDevicePropertyLatency error ");
        err = AudioDeviceGetProperty(fDeviceID, 0, false, kAudioDevicePropertySafetyOffset, &size, &value2);
        if (err != noErr)
            jack_log("AudioDeviceGetProperty kAudioDevicePropertySafetyOffset error ");

        port = fGraphManager->GetPort(port_index);
        port->SetAlias(alias);
        // Add more latency if "async" mode is used...
        port->SetLatency(fEngineControl->fBufferSize + ((fEngineControl->fSyncMode) ? 0 : fEngineControl->fBufferSize * fIOUsage) + value1 + value2 + fPlaybackLatency);
        fPlaybackPortList[i] = port_index;

        // Monitor ports
        if (fWithMonitorPorts) {
            jack_log("Create monitor port ");
            snprintf(name, sizeof(name) - 1, "%s:%s:monitor_%u", fAliasName, fPlaybackDriverName, i + 1);
            if ((port_index = fGraphManager->AllocatePort(fClientControl.fRefNum, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, fEngineControl->fBufferSize)) == NO_PORT) {
                jack_error("Cannot register monitor port for %s", name);
                return -1;
            } else {
                port = fGraphManager->GetPort(port_index);
                port->SetLatency(fEngineControl->fBufferSize);
                fMonitorPortList[i] = port_index;
            }
        }
    }

    // Input buffers do no change : prepare them only once
    for (int i = 0; i < fCaptureChannels; i++) {
        fJackInputData->mBuffers[i].mData = GetInputBuffer(i);
    }

    return 0;
}

int JackCoreAudioDriver::Start()
{
    jack_log("JackCoreAudioDriver::Start");
    JackAudioDriver::Start();
/*
#ifdef MAC_OS_X_VERSION_10_5
    OSStatus err = AudioDeviceCreateIOProcID(fDeviceID, MeasureCallback, this, &fMesureCallbackID);
#else
    OSStatus err = AudioDeviceAddIOProc(fDeviceID, MeasureCallback, this);
#endif
*/
    OSStatus err = AudioDeviceAddIOProc(fDeviceID, MeasureCallback, this);
    
    if (err != noErr)
        return -1;

    err = AudioOutputUnitStart(fAUHAL);
    if (err != noErr)
        return -1;

    if ((err = AudioDeviceStart(fDeviceID, MeasureCallback)) != noErr) {
        jack_error("Cannot start MeasureCallback");
        printError(err);
        return -1;
    }

    return 0;
}

int JackCoreAudioDriver::Stop()
{
    jack_log("JackCoreAudioDriver::Stop");
    AudioDeviceStop(fDeviceID, MeasureCallback);
/*
#ifdef MAC_OS_X_VERSION_10_5
    AudioDeviceDestroyIOProcID(fDeviceID, fMesureCallbackID);
#else
    AudioDeviceRemoveIOProc(fDeviceID, MeasureCallback);
#endif
*/
    AudioDeviceRemoveIOProc(fDeviceID, MeasureCallback);
    return (AudioOutputUnitStop(fAUHAL) == noErr) ? 0 : -1;
}

int JackCoreAudioDriver::SetBufferSize(jack_nframes_t buffer_size)
{
    OSStatus err;
    UInt32 outSize = sizeof(UInt32);

    err = AudioDeviceSetProperty(fDeviceID, NULL, 0, false, kAudioDevicePropertyBufferFrameSize, outSize, &buffer_size);
    if (err != noErr) {
        jack_error("Cannot set buffer size %ld", buffer_size);
        printError(err);
        return -1;
    }

    JackAudioDriver::SetBufferSize(buffer_size); // never fails

    // Input buffers do no change : prepare them only once
    for (int i = 0; i < fCaptureChannels; i++) {
        fJackInputData->mBuffers[i].mNumberChannels = 1;
        fJackInputData->mBuffers[i].mDataByteSize = fEngineControl->fBufferSize * sizeof(float);
        fJackInputData->mBuffers[i].mData = GetInputBuffer(i);
    }

    return 0;
}

} // end of namespace


#ifdef __cplusplus
extern "C"
{
#endif

    SERVER_EXPORT jack_driver_desc_t* driver_get_descriptor() 
    {
        jack_driver_desc_t *desc;
        unsigned int i;
        desc = (jack_driver_desc_t*)calloc(1, sizeof(jack_driver_desc_t));

        strcpy(desc->name, "coreaudio");                                    // size MUST be less then JACK_DRIVER_NAME_MAX + 1
        strcpy(desc->desc, "Apple CoreAudio API based audio backend");      // size MUST be less then JACK_DRIVER_PARAM_DESC + 1
        
        desc->nparams = 14;
        desc->params = (jack_driver_param_desc_t*)calloc(desc->nparams, sizeof(jack_driver_param_desc_t));

        i = 0;
        strcpy(desc->params[i].name, "channels");
        desc->params[i].character = 'c';
        desc->params[i].type = JackDriverParamInt;
        desc->params[i].value.ui = 0;
        strcpy(desc->params[i].short_desc, "Maximum number of channels");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "inchannels");
        desc->params[i].character = 'i';
        desc->params[i].type = JackDriverParamInt;
        desc->params[i].value.ui = 0;
        strcpy(desc->params[i].short_desc, "Maximum number of input channels");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "outchannels");
        desc->params[i].character = 'o';
        desc->params[i].type = JackDriverParamInt;
        desc->params[i].value.ui = 0;
        strcpy(desc->params[i].short_desc, "Maximum number of output channels");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "capture");
        desc->params[i].character = 'C';
        desc->params[i].type = JackDriverParamString;
        strcpy(desc->params[i].value.str, "will take default CoreAudio input device");
        strcpy(desc->params[i].short_desc, "Provide capture ports. Optionally set CoreAudio device name");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "playback");
        desc->params[i].character = 'P';
        desc->params[i].type = JackDriverParamString;
        strcpy(desc->params[i].value.str, "will take default CoreAudio output device");
        strcpy(desc->params[i].short_desc, "Provide playback ports. Optionally set CoreAudio device name");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy (desc->params[i].name, "monitor");
        desc->params[i].character = 'm';
        desc->params[i].type = JackDriverParamBool;
        desc->params[i].value.i = 0;
        strcpy(desc->params[i].short_desc, "Provide monitor ports for the output");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "duplex");
        desc->params[i].character = 'D';
        desc->params[i].type = JackDriverParamBool;
        desc->params[i].value.i = TRUE;
        strcpy(desc->params[i].short_desc, "Provide both capture and playback ports");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "rate");
        desc->params[i].character = 'r';
        desc->params[i].type = JackDriverParamUInt;
        desc->params[i].value.ui = 44100U;
        strcpy(desc->params[i].short_desc, "Sample rate");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "period");
        desc->params[i].character = 'p';
        desc->params[i].type = JackDriverParamUInt;
        desc->params[i].value.ui = 128U;
        strcpy(desc->params[i].short_desc, "Frames per period");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "device");
        desc->params[i].character = 'd';
        desc->params[i].type = JackDriverParamString;
        strcpy(desc->params[i].value.str, "will take default CoreAudio device name");
        strcpy(desc->params[i].short_desc, "CoreAudio device name");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "input-latency");
        desc->params[i].character = 'I';
        desc->params[i].type = JackDriverParamUInt;
        desc->params[i].value.i = 0;
        strcpy(desc->params[i].short_desc, "Extra input latency (frames)");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "output-latency");
        desc->params[i].character = 'O';
        desc->params[i].type = JackDriverParamUInt;
        desc->params[i].value.i = 0;
        strcpy(desc->params[i].short_desc, "Extra output latency (frames)");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "list-devices");
        desc->params[i].character = 'l';
        desc->params[i].type = JackDriverParamBool;
        desc->params[i].value.i = TRUE;
        strcpy(desc->params[i].short_desc, "Display available CoreAudio devices");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);
        
        i++;
        strcpy(desc->params[i].name, "async-latency");
        desc->params[i].character = 'L';
        desc->params[i].type = JackDriverParamUInt;
        desc->params[i].value.i = 100;
        strcpy(desc->params[i].short_desc, "Extra output latency in aynchronous mode (percent)");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        return desc;
    }

    SERVER_EXPORT Jack::JackDriverClientInterface* driver_initialize(Jack::JackLockedEngine* engine, Jack::JackSynchro* table, const JSList* params) 
    {
        jack_nframes_t srate = 44100;
        jack_nframes_t frames_per_interrupt = 128;
        int capture = FALSE;
        int playback = FALSE;
        int chan_in = 0;
        int chan_out = 0;
        bool monitor = false;
        const char* capture_driver_uid = "";
        const char* playback_driver_uid = "";
        const JSList *node;
        const jack_driver_param_t *param;
        jack_nframes_t systemic_input_latency = 0;
        jack_nframes_t systemic_output_latency = 0;
        int async_output_latency = 100;
    
        for (node = params; node; node = jack_slist_next(node)) {
            param = (const jack_driver_param_t *) node->data;
       
            switch (param->character) {

                case 'd':
                    capture_driver_uid = strdup(param->value.str);
                    playback_driver_uid = strdup(param->value.str);
                    break;

                case 'D':
                    capture = TRUE;
                    playback = TRUE;
                    break;

                case 'c':
                    chan_in = chan_out = (int) param->value.ui;
                    break;

                case 'i':
                    chan_in = (int) param->value.ui;
                    break;

                case 'o':
                    chan_out = (int) param->value.ui;
                    break;

                case 'C':
                    capture = TRUE;
                    if (strcmp(param->value.str, "none") != 0) {
                        capture_driver_uid = strdup(param->value.str);
                    }
                    break;

                case 'P':
                    playback = TRUE;
                    if (strcmp(param->value.str, "none") != 0) {
                        playback_driver_uid = strdup(param->value.str);
                    }
                    break;

                case 'm':
                    monitor = param->value.i;
                    break;

                case 'r':
                    srate = param->value.ui;
                    break;

                case 'p':
                    frames_per_interrupt = (unsigned int) param->value.ui;
                    break;

                case 'I':
                    systemic_input_latency = param->value.ui;
                    break;

                case 'O':
                    systemic_output_latency = param->value.ui;
                    break;

                case 'l':
                    Jack::DisplayDeviceNames();
                    break;
                    
                case 'L':
                    async_output_latency = param->value.ui;
                    break;
            }
        }

        /* duplex is the default */
        if (!capture && !playback) {
            capture = TRUE;
            playback = TRUE;
        }

        Jack::JackCoreAudioDriver* driver = new Jack::JackCoreAudioDriver("system", "coreaudio", engine, table);
        if (driver->Open(frames_per_interrupt, srate, capture, playback, chan_in, chan_out, monitor, capture_driver_uid, 
            playback_driver_uid, systemic_input_latency, systemic_output_latency, async_output_latency) == 0) {
            return driver;
        } else {
            delete driver;
            return NULL;
        }
    }

#ifdef __cplusplus
}
#endif


