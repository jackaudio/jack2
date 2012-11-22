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
#include "JackTools.h"
#include "JackLockedEngine.h"
#include "JackAC3Encoder.h"

#include <sstream>
#include <iostream>
#include <CoreServices/CoreServices.h>
#include <CoreFoundation/CFNumber.h>

namespace Jack
{

static void Print4CharCode(const char* msg, long c)
{
    UInt32 __4CC_number = (c);
    char __4CC_string[5];
    *((SInt32*)__4CC_string) = EndianU32_NtoB(__4CC_number);
    __4CC_string[4] = 0;
    jack_log("%s'%s'", (msg), __4CC_string);
}

static void PrintStreamDesc(AudioStreamBasicDescription *inDesc)
{
    jack_log("- - - - - - - - - - - - - - - - - - - -");
    jack_log("  Sample Rate:%f", inDesc->mSampleRate);
    jack_log("  Format ID:%.*s", (int)sizeof(inDesc->mFormatID), (char*)&inDesc->mFormatID);
    jack_log("  Format Flags:%lX", inDesc->mFormatFlags);
    jack_log("  Bytes per Packet:%ld", inDesc->mBytesPerPacket);
    jack_log("  Frames per Packet:%ld", inDesc->mFramesPerPacket);
    jack_log("  Bytes per Frame:%ld", inDesc->mBytesPerFrame);
    jack_log("  Channels per Frame:%ld", inDesc->mChannelsPerFrame);
    jack_log("  Bits per Channel:%ld", inDesc->mBitsPerChannel);
    jack_log("- - - - - - - - - - - - - - - - - - - -");
}

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
            Print4CharCode("error code : unknown ", err);
            break;
    }
}

static bool CheckAvailableDeviceName(const char* device_name, AudioDeviceID* device_id)
{
    UInt32 size;
    Boolean isWritable;
    int i, deviceNum;
    OSStatus err;

    err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &size, &isWritable);
    if (err != noErr) {
        return false;
    }

    deviceNum = size / sizeof(AudioDeviceID);
    AudioDeviceID devices[deviceNum];

    err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &size, devices);
    if (err != noErr) {
        return false;
    }

    for (i = 0; i < deviceNum; i++) {
        char device_name_aux[256];

        size = 256;
        err = AudioDeviceGetProperty(devices[i], 0, false, kAudioDevicePropertyDeviceName, &size, device_name_aux);
        if (err != noErr) {
            return false;
        }

        if (strcmp(device_name_aux, device_name) == 0) {
            *device_id = devices[i];
            return true;
        }

    }

    return false;
}

static bool CheckAvailableDevice(AudioDeviceID device_id)
{
    UInt32 size;
    Boolean isWritable;
    int i, deviceNum;
    OSStatus err;

    err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &size, &isWritable);
    if (err != noErr) {
        return false;
    }

    deviceNum = size / sizeof(AudioDeviceID);
    AudioDeviceID devices[deviceNum];

    err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &size, devices);
    if (err != noErr) {
        return false;
    }

    for (i = 0; i < deviceNum; i++) {
        if (device_id == devices[i]) {
            return true;
        }
    }

    return false;
}


static OSStatus DisplayDeviceNames()
{
    UInt32 size;
    Boolean isWritable;
    int i, deviceNum;
    OSStatus err;
    CFStringRef UIname;

    err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &size, &isWritable);
    if (err != noErr) {
        return err;
    }

    deviceNum = size / sizeof(AudioDeviceID);
    AudioDeviceID devices[deviceNum];

    err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &size, devices);
    if (err != noErr) {
        return err;
    }

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
        if (err != noErr) {
            return err;
        }

        jack_info("Device ID = \'%d\' name = \'%s\', internal name = \'%s\' (to be used as -C, -P, or -d parameter)", devices[i], device_name, internal_name);
    }

    return noErr;

error:
    if (UIname != NULL) {
        CFRelease(UIname);
    }
    return err;
}

static CFStringRef GetDeviceName(AudioDeviceID id)
{
    UInt32 size = sizeof(CFStringRef);
    CFStringRef UIname;
    OSStatus err = AudioDeviceGetProperty(id, 0, false, kAudioDevicePropertyDeviceUID, &size, &UIname);
    return (err == noErr) ? UIname : NULL;
}

static void ParseChannelList(const string& list, vector<int>& result, int max_chan)
{
    stringstream ss(list);
    string token;
    int chan;
  
    while (ss >> token) {
        istringstream ins;
        ins.str(token);
        ins >> chan;
        if (chan < 0 || chan >= max_chan) {
            jack_error("Ignore incorrect channel mapping value = %d", chan);
        } else {
            result.push_back(chan);
        }
    }
}

OSStatus JackCoreAudioDriver::Render(void* inRefCon,
                                     AudioUnitRenderActionFlags* ioActionFlags,
                                     const AudioTimeStamp* inTimeStamp,
                                     UInt32 inBusNumber,
                                     UInt32 inNumberFrames,
                                     AudioBufferList* ioData)
{
    JackCoreAudioDriver* driver = (JackCoreAudioDriver*)inRefCon;
    driver->fActionFags = ioActionFlags;
    driver->fCurrentTime = inTimeStamp;
    driver->fDriverOutputData = ioData;

    // Setup threaded based log function et get RT thread parameters once...
    if (set_threaded_log_function()) {

        jack_log("JackCoreAudioDriver::Render : set_threaded_log_function");
        JackMachThread::GetParams(pthread_self(), &driver->fEngineControl->fPeriod, &driver->fEngineControl->fComputation, &driver->fEngineControl->fConstraint);

        if (driver->fComputationGrain > 0) {
            jack_log("JackCoreAudioDriver::Render : RT thread computation setup to %d percent of period", int(driver->fComputationGrain * 100));
            driver->fEngineControl->fComputation = driver->fEngineControl->fPeriod * driver->fComputationGrain;
        }
    }

    // Signal waiting start function...
    driver->fState = true;

    driver->CycleTakeBeginTime();

    if (driver->Process() < 0) {
        jack_error("Process error, stopping driver");
        driver->NotifyFailure(JackBackendError, "Process error, stopping driver");    // Message length limited to JACK_MESSAGE_SIZE
        driver->Stop();
        kill(JackTools::GetPID(), SIGINT);
        return kAudioHardwareUnsupportedOperationError;
    } else {
        return noErr;
    }
}

int JackCoreAudioDriver::Read()
{
    if (fCaptureChannels > 0)  { // Calling AudioUnitRender with no input returns a '????' error (callback setting issue ??), so hack to avoid it here...
        return (AudioUnitRender(fAUHAL, fActionFags, fCurrentTime, 1, fEngineControl->fBufferSize, fJackInputData) == noErr) ? 0 : -1;
    } else {
        return 0;
    }
}

int JackCoreAudioDriver::Write()
{
    if (fAC3Encoder) {
    
        // AC3 encoding and SPDIF write
        jack_default_audio_sample_t* AC3_inputs[MAX_AC3_CHANNELS];
        jack_default_audio_sample_t* AC3_outputs[2];
        for (int i = 0; i < fPlaybackChannels; i++) {
            AC3_inputs[i] = GetOutputBuffer(i);
            // If not connected, clear the buffer
            if (fGraphManager->GetConnectionsNum(fPlaybackPortList[i]) == 0) {
                memset(AC3_inputs[i], 0, sizeof(jack_default_audio_sample_t) * fEngineControl->fBufferSize);
            }
        }
        AC3_outputs[0] = (jack_default_audio_sample_t*)fDriverOutputData->mBuffers[0].mData;
        AC3_outputs[1] = (jack_default_audio_sample_t*)fDriverOutputData->mBuffers[1].mData;
        fAC3Encoder->Process(AC3_inputs, AC3_outputs, fEngineControl->fBufferSize);
        
    } else {
       
        // Standard write
        for (int i = 0; i < fPlaybackChannels; i++) {
            if (fGraphManager->GetConnectionsNum(fPlaybackPortList[i]) > 0) {
                jack_default_audio_sample_t* buffer = GetOutputBuffer(i);
                int size = sizeof(jack_default_audio_sample_t) * fEngineControl->fBufferSize;
                memcpy((jack_default_audio_sample_t*)fDriverOutputData->mBuffers[i].mData, buffer, size);
                // Monitor ports
                if (fWithMonitorPorts && fGraphManager->GetConnectionsNum(fMonitorPortList[i]) > 0) {
                    memcpy(GetMonitorBuffer(i), buffer, size);
                }
            } else {
                memset((jack_default_audio_sample_t*)fDriverOutputData->mBuffers[i].mData, 0, sizeof(jack_default_audio_sample_t) * fEngineControl->fBufferSize);
            }
        }
    }
    return 0;
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
            // Check new sample rate
            Float64 tmp_sample_rate;
            UInt32 outSize = sizeof(Float64);
            OSStatus err = AudioDeviceGetProperty(inDevice, 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyNominalSampleRate, &outSize, &tmp_sample_rate);
            if (err != noErr) {
                jack_error("Cannot get current sample rate");
                printError(err);
            } else {
                jack_log("JackCoreAudioDriver::SRNotificationCallback : checked sample rate = %f", tmp_sample_rate);
            }
            driver->fState = true;
            break;
        }
    }

    return noErr;
}

OSStatus JackCoreAudioDriver::BSNotificationCallback(AudioDeviceID inDevice,
                                                     UInt32 inChannel,
                                                     Boolean isInput,
                                                     AudioDevicePropertyID inPropertyID,
                                                     void* inClientData)
{
    JackCoreAudioDriver* driver = (JackCoreAudioDriver*)inClientData;

    switch (inPropertyID) {

        case kAudioDevicePropertyBufferFrameSize: {
            jack_log("JackCoreAudioDriver::BSNotificationCallback kAudioDevicePropertyBufferFrameSize");
            // Check new buffer size
            UInt32 tmp_buffer_size;
            UInt32 outSize = sizeof(UInt32);
            OSStatus err = AudioDeviceGetProperty(inDevice, 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyBufferFrameSize, &outSize, &tmp_buffer_size);
            if (err != noErr) {
                jack_error("Cannot get current buffer size");
                printError(err);
            } else {
                jack_log("JackCoreAudioDriver::BSNotificationCallback : checked buffer size = %d", tmp_buffer_size);
            }
            driver->fState = true;
            break;
        }
    }

    return noErr;
}

// A better implementation would possibly try to recover in case of hardware device change (see HALLAB HLFilePlayerWindowControllerAudioDevicePropertyListenerProc code)
OSStatus JackCoreAudioDriver::AudioHardwareNotificationCallback(AudioHardwarePropertyID inPropertyID, void* inClientData)
{
    JackCoreAudioDriver* driver = (JackCoreAudioDriver*)inClientData;

    switch (inPropertyID) {

        case kAudioHardwarePropertyDevices: {
            jack_log("JackCoreAudioDriver::AudioHardwareNotificationCallback kAudioHardwarePropertyDevices");
            DisplayDeviceNames();
            AudioDeviceID captureID, playbackID;
            if (CheckAvailableDevice(driver->fDeviceID) ||
                (CheckAvailableDeviceName(driver->fCaptureUID, &captureID)
                && CheckAvailableDeviceName(driver->fPlaybackUID, &playbackID))) {

            }
            break;
        }
    }

    return noErr;
}

OSStatus JackCoreAudioDriver::DeviceNotificationCallback(AudioDeviceID inDevice,
                                                        UInt32 inChannel,
                                                        Boolean	isInput,
                                                        AudioDevicePropertyID inPropertyID,
                                                        void* inClientData)
{
    JackCoreAudioDriver* driver = (JackCoreAudioDriver*)inClientData;

    switch (inPropertyID) {

        case kAudioDevicePropertyDeviceIsRunning: {
            UInt32 isrunning = 0;
            UInt32 outsize = sizeof(UInt32);
            if (AudioDeviceGetProperty(driver->fDeviceID, 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyDeviceIsRunning, &outsize, &isrunning) == noErr) {
                jack_log("JackCoreAudioDriver::DeviceNotificationCallback kAudioDevicePropertyDeviceIsRunning = %d", isrunning);
            }
            break;
        }

        case kAudioDevicePropertyDeviceIsAlive: {
            UInt32 isalive = 0;
            UInt32 outsize = sizeof(UInt32);
            if (AudioDeviceGetProperty(driver->fDeviceID, 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyDeviceIsAlive, &outsize, &isalive) == noErr) {
                jack_log("JackCoreAudioDriver::DeviceNotificationCallback kAudioDevicePropertyDeviceIsAlive = %d", isalive);
            }
            break;
        }

        case kAudioDevicePropertyDeviceHasChanged: {
            UInt32 hachanged = 0;
            UInt32 outsize = sizeof(UInt32);
            if (AudioDeviceGetProperty(driver->fDeviceID, 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyDeviceHasChanged, &outsize, &hachanged) == noErr) {
                jack_log("JackCoreAudioDriver::DeviceNotificationCallback kAudioDevicePropertyDeviceHasChanged = %d", hachanged);
            }
            break;
        }

        case kAudioDeviceProcessorOverload: {
            jack_error("DeviceNotificationCallback kAudioDeviceProcessorOverload");
            jack_time_t cur_time = GetMicroSeconds();
            driver->NotifyXRun(cur_time, float(cur_time - driver->fBeginDateUst));   // Better this value than nothing...
            break;
        }

        case kAudioDevicePropertyStreamConfiguration: {
            jack_error("Cannot handle kAudioDevicePropertyStreamConfiguration : server will quit...");
            driver->NotifyFailure(JackBackendError, "Another application has changed the device configuration");   // Message length limited to JACK_MESSAGE_SIZE
            driver->CloseAUHAL();
            kill(JackTools::GetPID(), SIGINT);
            return kAudioHardwareUnsupportedOperationError;
        }

        case kAudioDevicePropertyNominalSampleRate: {
            Float64 sample_rate = 0;
            UInt32 outsize = sizeof(Float64);
            OSStatus err = AudioDeviceGetProperty(driver->fDeviceID, 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyNominalSampleRate, &outsize, &sample_rate);
            if (err != noErr) {
                return kAudioHardwareUnsupportedOperationError;
            }

            char device_name[256];
            const char* digidesign_name = "Digidesign";
            driver->GetDeviceNameFromID(driver->fDeviceID, device_name);

            if (sample_rate != driver->fEngineControl->fSampleRate) {

               // Digidesign hardware, so "special" code : change the SR again here
               if (strncmp(device_name, digidesign_name, 10) == 0) {

                    jack_log("JackCoreAudioDriver::DeviceNotificationCallback Digidesign HW = %s", device_name);

                    // Set sample rate again...
                    sample_rate = driver->fEngineControl->fSampleRate;
                    err = AudioDeviceSetProperty(driver->fDeviceID, NULL, 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyNominalSampleRate, outsize, &sample_rate);
                    if (err != noErr) {
                        jack_error("Cannot set sample rate = %f", sample_rate);
                        printError(err);
                    } else {
                        jack_log("JackCoreAudioDriver::DeviceNotificationCallback : set sample rate = %f", sample_rate);
                    }

                    // Check new sample rate again...
                    outsize = sizeof(Float64);
                    err = AudioDeviceGetProperty(inDevice, 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyNominalSampleRate, &outsize, &sample_rate);
                    if (err != noErr) {
                        jack_error("Cannot get current sample rate");
                        printError(err);
                    } else {
                        jack_log("JackCoreAudioDriver::DeviceNotificationCallback : checked sample rate = %f", sample_rate);
                    }
                    return noErr;

                } else {
                    driver->NotifyFailure(JackBackendError, "Another application has changed the sample rate");    // Message length limited to JACK_MESSAGE_SIZE
                    driver->CloseAUHAL();
                    kill(JackTools::GetPID(), SIGINT);
                    return kAudioHardwareUnsupportedOperationError;
                }
            }
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
        jack_log("JackCoreAudioDriver::GetDeviceIDFromUID %s %ld", UID, *id);
        return (*id == kAudioDeviceUnknown) ? kAudioHardwareBadDeviceError : res;
    }
}

OSStatus JackCoreAudioDriver::GetDefaultDevice(AudioDeviceID* id)
{
    OSStatus res;
    UInt32 theSize = sizeof(UInt32);
    AudioDeviceID inDefault;
    AudioDeviceID outDefault;

    if ((res = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultInputDevice, &theSize, &inDefault)) != noErr) {
        return res;
    }

    if ((res = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &theSize, &outDefault)) != noErr) {
        return res;
    }

    jack_log("JackCoreAudioDriver::GetDefaultDevice : input = %ld output = %ld", inDefault, outDefault);

    // Get the device only if default input and output are the same
    if (inDefault != outDefault) {
        jack_error("Default input and output devices are not the same !!");
        return kAudioHardwareBadDeviceError;
    } else if (inDefault == 0) {
        jack_error("Default input and output devices are null !!");
        return kAudioHardwareBadDeviceError;
    } else {
        *id = inDefault;
        return noErr;
    }
}

OSStatus JackCoreAudioDriver::GetDefaultInputDevice(AudioDeviceID* id)
{
    OSStatus res;
    UInt32 theSize = sizeof(UInt32);
    AudioDeviceID inDefault;

    if ((res = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultInputDevice, &theSize, &inDefault)) != noErr) {
        return res;
    }

    if (inDefault == 0) {
        jack_error("Error default input device is 0, please select a correct one !!");
        return -1;
    }
    jack_log("JackCoreAudioDriver::GetDefaultInputDevice : input = %ld ", inDefault);
    *id = inDefault;
    return noErr;
}

OSStatus JackCoreAudioDriver::GetDefaultOutputDevice(AudioDeviceID* id)
{
    OSStatus res;
    UInt32 theSize = sizeof(UInt32);
    AudioDeviceID outDefault;

    if ((res = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &theSize, &outDefault)) != noErr) {
        return res;
    }

    if (outDefault == 0) {
        jack_error("Error default output device is 0, please select a correct one !!");
        return -1;
    }
    jack_log("JackCoreAudioDriver::GetDefaultOutputDevice : output = %ld", outDefault);
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
    UInt32 outSize;
    Boolean	outWritable;

    channelCount = 0;
    err = AudioDeviceGetPropertyInfo(device, 0, isInput, kAudioDevicePropertyStreamConfiguration, &outSize, &outWritable);
    if (err == noErr) {
        int stream_count = outSize / sizeof(AudioBufferList);
        jack_log("JackCoreAudioDriver::GetTotalChannels stream_count = %d", stream_count);
        AudioBufferList bufferList[stream_count];
        err = AudioDeviceGetProperty(device, 0, isInput, kAudioDevicePropertyStreamConfiguration, &outSize, bufferList);
        if (err == noErr) {
            for (uint i = 0; i < bufferList->mNumberBuffers; i++) {
                channelCount += bufferList->mBuffers[i].mNumberChannels;
                jack_log("JackCoreAudioDriver::GetTotalChannels stream = %d channels = %d", i, bufferList->mBuffers[i].mNumberChannels);
            }
        }
    }
    return err;
}

OSStatus JackCoreAudioDriver::GetStreamLatencies(AudioDeviceID device, bool isInput, vector<int>& latencies)
{
    OSStatus err = noErr;
    UInt32 outSize1, outSize2, outSize3;
    Boolean	outWritable;

    err = AudioDeviceGetPropertyInfo(device, 0, isInput, kAudioDevicePropertyStreams, &outSize1, &outWritable);
    if (err == noErr) {
        int stream_count = outSize1 / sizeof(UInt32);
        AudioStreamID streamIDs[stream_count];
        AudioBufferList bufferList[stream_count];
        UInt32 streamLatency;
        outSize2 = sizeof(UInt32);

        err = AudioDeviceGetProperty(device, 0, isInput, kAudioDevicePropertyStreams, &outSize1, streamIDs);
        if (err != noErr) {
            jack_error("GetStreamLatencies kAudioDevicePropertyStreams err = %d", err);
            return err;
        }

        err = AudioDeviceGetPropertyInfo(device, 0, isInput, kAudioDevicePropertyStreamConfiguration, &outSize3, &outWritable);
        if (err != noErr) {
            jack_error("GetStreamLatencies kAudioDevicePropertyStreamConfiguration err = %d", err);
            return err;
        }

        for (int i = 0; i < stream_count; i++) {
            err = AudioStreamGetProperty(streamIDs[i], 0, kAudioStreamPropertyLatency, &outSize2, &streamLatency);
            if (err != noErr) {
                jack_error("GetStreamLatencies kAudioStreamPropertyLatency err = %d", err);
                return err;
            }
            err = AudioDeviceGetProperty(device, 0, isInput, kAudioDevicePropertyStreamConfiguration, &outSize3, bufferList);
            if (err != noErr) {
                jack_error("GetStreamLatencies kAudioDevicePropertyStreamConfiguration err = %d", err);
                return err;
            }
            // Push 'channel' time the stream latency
            for (uint k = 0; k < bufferList->mBuffers[i].mNumberChannels; k++) {
                latencies.push_back(streamLatency);
            }
        }
    }
    return err;
}

bool JackCoreAudioDriver::IsDigitalDevice(AudioDeviceID device)
{
    OSStatus err = noErr;
    UInt32 outSize1;
    bool is_digital = false;
    
    /* Get a list of all the streams on this device */
    AudioObjectPropertyAddress streamsAddress = { kAudioDevicePropertyStreams, kAudioDevicePropertyScopeOutput, kAudioObjectPropertyElementMaster };
    err = AudioObjectGetPropertyDataSize(device, &streamsAddress, 0, NULL, &outSize1);
    if (err != noErr) {
        jack_error("IsDigitalDevice kAudioDevicePropertyStreams err = %d", err);
        return false;
    }
    
    int stream_count = outSize1 / sizeof(AudioStreamID);
    AudioStreamID streamIDs[stream_count];
    
    err = AudioObjectGetPropertyData(device, &streamsAddress, 0, NULL, &outSize1, streamIDs);

    if (err != noErr) {
        jack_error("IsDigitalDevice kAudioDevicePropertyStreams list err = %d", err);
        return false;
    }
    
    AudioObjectPropertyAddress physicalFormatsAddress = { kAudioStreamPropertyAvailablePhysicalFormats, kAudioObjectPropertyScopeGlobal, 0 };
     
    for (int i = 0; i < stream_count ; i++) {
   
        /* Find a stream with a cac3 stream */
        int  format_num = 0;
    
        /* Retrieve all the stream formats supported by each output stream */
        err = AudioObjectGetPropertyDataSize(streamIDs[i], &physicalFormatsAddress, 0, NULL, &outSize1);
        
        if (err != noErr) {
            jack_error("IsDigitalDevice kAudioStreamPropertyAvailablePhysicalFormats err = %d", err);
            return false;
        }

        format_num = outSize1 / sizeof(AudioStreamRangedDescription);
        AudioStreamRangedDescription format_list[format_num];
      
        err = AudioObjectGetPropertyData(streamIDs[i], &physicalFormatsAddress, 0, NULL, &outSize1, format_list);
         
        if (err != noErr) {
            jack_error("IsDigitalDevice could not get the list of streamformats err = %d", err);
            return false;
        }
   
        /* Check if one of the supported formats is a digital format */
        for (int j = 0; j < format_num; j++) {
        
            PrintStreamDesc(&format_list[j].mFormat);
            
            if (format_list[j].mFormat.mFormatID == 'IAC3' ||
                format_list[j].mFormat.mFormatID == 'iac3' ||
                format_list[j].mFormat.mFormatID == kAudioFormat60958AC3 ||
                format_list[j].mFormat.mFormatID == kAudioFormatAC3)
            {
                is_digital = true;
                break;
            }
        }
    }
       
    return is_digital;
}

JackCoreAudioDriver::JackCoreAudioDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table)
        : JackAudioDriver(name, alias, engine, table),
        fAC3Encoder(NULL),
        fJackInputData(NULL),
        fDriverOutputData(NULL),
        fPluginID(0),
        fState(false),
        fHogged(false),
        fIOUsage(1.f),
        fComputationGrain(-1.f),
        fClockDriftCompensate(false),
        fDigitalPlayback(false)
{}

JackCoreAudioDriver::~JackCoreAudioDriver()
{
    delete fAC3Encoder;
}

OSStatus JackCoreAudioDriver::DestroyAggregateDevice()
{
    OSStatus osErr = noErr;
    AudioObjectPropertyAddress pluginAOPA;
    pluginAOPA.mSelector = kAudioPlugInDestroyAggregateDevice;
    pluginAOPA.mScope = kAudioObjectPropertyScopeGlobal;
    pluginAOPA.mElement = kAudioObjectPropertyElementMaster;
    UInt32 outDataSize;

    if (fPluginID > 0) {

        osErr = AudioObjectGetPropertyDataSize(fPluginID, &pluginAOPA, 0, NULL, &outDataSize);
        if (osErr != noErr) {
            jack_error("DestroyAggregateDevice : AudioObjectGetPropertyDataSize error");
            printError(osErr);
            return osErr;
        }

        osErr = AudioObjectGetPropertyData(fPluginID, &pluginAOPA, 0, NULL, &outDataSize, &fDeviceID);
        if (osErr != noErr) {
            jack_error("DestroyAggregateDevice : AudioObjectGetPropertyData error");
            printError(osErr);
            return osErr;
        }

    }

    return noErr;
}

OSStatus JackCoreAudioDriver::CreateAggregateDevice(AudioDeviceID captureDeviceID, AudioDeviceID playbackDeviceID, jack_nframes_t samplerate, AudioDeviceID* outAggregateDevice)
{
    OSStatus err = noErr;
    AudioObjectID sub_device[32];
    UInt32 outSize = sizeof(sub_device);

    err = AudioDeviceGetProperty(captureDeviceID, 0, kAudioDeviceSectionGlobal, kAudioAggregateDevicePropertyActiveSubDeviceList, &outSize, sub_device);
    vector<AudioDeviceID> captureDeviceIDArray;

    if (err != noErr) {
        jack_log("JackCoreAudioDriver::CreateAggregateDevice : input device does not have subdevices");
        captureDeviceIDArray.push_back(captureDeviceID);
    } else {
        int num_devices = outSize / sizeof(AudioObjectID);
        jack_log("JackCoreAudioDriver::CreateAggregateDevice :Input device has %d subdevices", num_devices);
        for (int i = 0; i < num_devices; i++) {
            captureDeviceIDArray.push_back(sub_device[i]);
        }
    }

    err = AudioDeviceGetProperty(playbackDeviceID, 0, kAudioDeviceSectionGlobal, kAudioAggregateDevicePropertyActiveSubDeviceList, &outSize, sub_device);
    vector<AudioDeviceID> playbackDeviceIDArray;

    if (err != noErr) {
        jack_log("JackCoreAudioDriver::CreateAggregateDevice : output device does not have subdevices");
        playbackDeviceIDArray.push_back(playbackDeviceID);
    } else {
        int num_devices = outSize / sizeof(AudioObjectID);
        jack_log("JackCoreAudioDriver::CreateAggregateDevice : output device has %d subdevices", num_devices);
        for (int i = 0; i < num_devices; i++) {
            playbackDeviceIDArray.push_back(sub_device[i]);
        }
    }

    return CreateAggregateDeviceAux(captureDeviceIDArray, playbackDeviceIDArray, samplerate, outAggregateDevice);
}

OSStatus JackCoreAudioDriver::CreateAggregateDeviceAux(vector<AudioDeviceID> captureDeviceID, vector<AudioDeviceID> playbackDeviceID, jack_nframes_t samplerate, AudioDeviceID* outAggregateDevice)
{
    OSStatus osErr = noErr;
    UInt32 outSize;
    Boolean outWritable;

    // Prepare sub-devices for clock drift compensation
    // Workaround for bug in the HAL : until 10.6.2
    AudioObjectPropertyAddress theAddressOwned = { kAudioObjectPropertyOwnedObjects, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster };
    AudioObjectPropertyAddress theAddressDrift = { kAudioSubDevicePropertyDriftCompensation, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster };
    UInt32 theQualifierDataSize = sizeof(AudioObjectID);
    AudioClassID inClass = kAudioSubDeviceClassID;
    void* theQualifierData = &inClass;
    UInt32 subDevicesNum = 0;

    //---------------------------------------------------------------------------
    // Setup SR of both devices otherwise creating AD may fail...
    //---------------------------------------------------------------------------
    UInt32 keptclockdomain = 0;
    UInt32 clockdomain = 0;
    outSize = sizeof(UInt32);
    bool need_clock_drift_compensation = false;

    for (UInt32 i = 0; i < captureDeviceID.size(); i++) {
        if (SetupSampleRateAux(captureDeviceID[i], samplerate) < 0) {
            jack_error("CreateAggregateDevice : cannot set SR of input device");
        } else  {
            // Check clock domain
            osErr = AudioDeviceGetProperty(captureDeviceID[i], 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyClockDomain, &outSize, &clockdomain);
            if (osErr != 0) {
                jack_error("CreateAggregateDevice : kAudioDevicePropertyClockDomain error");
                printError(osErr);
            } else {
                keptclockdomain = (keptclockdomain == 0) ? clockdomain : keptclockdomain;
                jack_log("JackCoreAudioDriver::CreateAggregateDevice : input clockdomain = %d", clockdomain);
                if (clockdomain != 0 && clockdomain != keptclockdomain) {
                    jack_error("CreateAggregateDevice : devices do not share the same clock!! clock drift compensation would be needed...");
                    need_clock_drift_compensation = true;
                }
            }
        }
    }

    for (UInt32 i = 0; i < playbackDeviceID.size(); i++) {
        if (SetupSampleRateAux(playbackDeviceID[i], samplerate) < 0) {
            jack_error("CreateAggregateDevice : cannot set SR of output device");
        } else {
            // Check clock domain
            osErr = AudioDeviceGetProperty(playbackDeviceID[i], 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyClockDomain, &outSize, &clockdomain);
            if (osErr != 0) {
                jack_error("CreateAggregateDevice : kAudioDevicePropertyClockDomain error");
                printError(osErr);
            } else {
                keptclockdomain = (keptclockdomain == 0) ? clockdomain : keptclockdomain;
                jack_log("JackCoreAudioDriver::CreateAggregateDevice : output clockdomain = %d", clockdomain);
                if (clockdomain != 0 && clockdomain != keptclockdomain) {
                    jack_error("CreateAggregateDevice : devices do not share the same clock!! clock drift compensation would be needed...");
                    need_clock_drift_compensation = true;
                }
            }
        }
    }

    // If no valid clock domain was found, then assume we have to compensate...
    if (keptclockdomain == 0) {
        need_clock_drift_compensation = true;
    }

    //---------------------------------------------------------------------------
    // Start to create a new aggregate by getting the base audio hardware plugin
    //---------------------------------------------------------------------------

    char device_name[256];
    for (UInt32 i = 0; i < captureDeviceID.size(); i++) {
        GetDeviceNameFromID(captureDeviceID[i], device_name);
        jack_info("Separated input = '%s' ", device_name);
    }

    for (UInt32 i = 0; i < playbackDeviceID.size(); i++) {
        GetDeviceNameFromID(playbackDeviceID[i], device_name);
        jack_info("Separated output = '%s' ", device_name);
    }

    osErr = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyPlugInForBundleID, &outSize, &outWritable);
    if (osErr != noErr) {
        jack_error("CreateAggregateDevice : AudioHardwareGetPropertyInfo kAudioHardwarePropertyPlugInForBundleID error");
        printError(osErr);
        return osErr;
    }

    AudioValueTranslation pluginAVT;

    CFStringRef inBundleRef = CFSTR("com.apple.audio.CoreAudio");

    pluginAVT.mInputData = &inBundleRef;
    pluginAVT.mInputDataSize = sizeof(inBundleRef);
    pluginAVT.mOutputData = &fPluginID;
    pluginAVT.mOutputDataSize = sizeof(fPluginID);

    osErr = AudioHardwareGetProperty(kAudioHardwarePropertyPlugInForBundleID, &outSize, &pluginAVT);
    if (osErr != noErr) {
        jack_error("CreateAggregateDevice : AudioHardwareGetProperty kAudioHardwarePropertyPlugInForBundleID error");
        printError(osErr);
        return osErr;
    }

    //-------------------------------------------------
    // Create a CFDictionary for our aggregate device
    //-------------------------------------------------

    CFMutableDictionaryRef aggDeviceDict = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    CFStringRef AggregateDeviceNameRef = CFSTR("JackDuplex");
    CFStringRef AggregateDeviceUIDRef = CFSTR("com.grame.JackDuplex");

    // add the name of the device to the dictionary
    CFDictionaryAddValue(aggDeviceDict, CFSTR(kAudioAggregateDeviceNameKey), AggregateDeviceNameRef);

    // add our choice of UID for the aggregate device to the dictionary
    CFDictionaryAddValue(aggDeviceDict, CFSTR(kAudioAggregateDeviceUIDKey), AggregateDeviceUIDRef);

    // add a "private aggregate key" to the dictionary
    int value = 1;
    CFNumberRef AggregateDeviceNumberRef = CFNumberCreate(NULL, kCFNumberIntType, &value);

    SInt32 system;
    Gestalt(gestaltSystemVersion, &system);

    jack_log("JackCoreAudioDriver::CreateAggregateDevice : system version = %x limit = %x", system, 0x00001054);

    // Starting with 10.5.4 systems, the AD can be internal... (better)
    if (system < 0x00001054) {
        jack_log("JackCoreAudioDriver::CreateAggregateDevice : public aggregate device....");
    } else {
        jack_log("JackCoreAudioDriver::CreateAggregateDevice : private aggregate device....");
        CFDictionaryAddValue(aggDeviceDict, CFSTR(kAudioAggregateDeviceIsPrivateKey), AggregateDeviceNumberRef);
    }

    // Prepare sub-devices for clock drift compensation
    CFMutableArrayRef subDevicesArrayClock = NULL;

    /*
    if (fClockDriftCompensate) {
        if (need_clock_drift_compensation) {
            jack_info("Clock drift compensation activated...");
            subDevicesArrayClock = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);

            for (UInt32 i = 0; i < captureDeviceID.size(); i++) {
                CFStringRef UID = GetDeviceName(captureDeviceID[i]);
                if (UID) {
                    CFMutableDictionaryRef subdeviceAggDeviceDict = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
                    CFDictionaryAddValue(subdeviceAggDeviceDict, CFSTR(kAudioSubDeviceUIDKey), UID);
                    CFDictionaryAddValue(subdeviceAggDeviceDict, CFSTR(kAudioSubDeviceDriftCompensationKey), AggregateDeviceNumberRef);
                    //CFRelease(UID);
                    CFArrayAppendValue(subDevicesArrayClock, subdeviceAggDeviceDict);
                }
            }

            for (UInt32 i = 0; i < playbackDeviceID.size(); i++) {
                CFStringRef UID = GetDeviceName(playbackDeviceID[i]);
                if (UID) {
                    CFMutableDictionaryRef subdeviceAggDeviceDict = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
                    CFDictionaryAddValue(subdeviceAggDeviceDict, CFSTR(kAudioSubDeviceUIDKey), UID);
                    CFDictionaryAddValue(subdeviceAggDeviceDict, CFSTR(kAudioSubDeviceDriftCompensationKey), AggregateDeviceNumberRef);
                    //CFRelease(UID);
                    CFArrayAppendValue(subDevicesArrayClock, subdeviceAggDeviceDict);
                }
            }

            // add sub-device clock array for the aggregate device to the dictionary
            CFDictionaryAddValue(aggDeviceDict, CFSTR(kAudioAggregateDeviceSubDeviceListKey), subDevicesArrayClock);
        } else {
            jack_info("Clock drift compensation was asked but is not needed (devices use the same clock domain)");
        }
    }
    */

    //-------------------------------------------------
    // Create a CFMutableArray for our sub-device list
    //-------------------------------------------------

    // we need to append the UID for each device to a CFMutableArray, so create one here
    CFMutableArrayRef subDevicesArray = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);

    vector<CFStringRef> captureDeviceUID;
    for (UInt32 i = 0; i < captureDeviceID.size(); i++) {
        CFStringRef ref = GetDeviceName(captureDeviceID[i]);
        if (ref == NULL) {
            return -1;
        }
        captureDeviceUID.push_back(ref);
        // input sub-devices in this example, so append the sub-device's UID to the CFArray
        CFArrayAppendValue(subDevicesArray, ref);
   }

    vector<CFStringRef> playbackDeviceUID;
    for (UInt32 i = 0; i < playbackDeviceID.size(); i++) {
        CFStringRef ref = GetDeviceName(playbackDeviceID[i]);
        if (ref == NULL) {
            return -1;
        }
        playbackDeviceUID.push_back(ref);
        // output sub-devices in this example, so append the sub-device's UID to the CFArray
        CFArrayAppendValue(subDevicesArray, ref);
    }

    //-----------------------------------------------------------------------
    // Feed the dictionary to the plugin, to create a blank aggregate device
    //-----------------------------------------------------------------------

    AudioObjectPropertyAddress pluginAOPA;
    pluginAOPA.mSelector = kAudioPlugInCreateAggregateDevice;
    pluginAOPA.mScope = kAudioObjectPropertyScopeGlobal;
    pluginAOPA.mElement = kAudioObjectPropertyElementMaster;
    UInt32 outDataSize;

    osErr = AudioObjectGetPropertyDataSize(fPluginID, &pluginAOPA, 0, NULL, &outDataSize);
    if (osErr != noErr) {
        jack_error("CreateAggregateDevice : AudioObjectGetPropertyDataSize error");
        printError(osErr);
        goto error;
    }

    osErr = AudioObjectGetPropertyData(fPluginID, &pluginAOPA, sizeof(aggDeviceDict), &aggDeviceDict, &outDataSize, outAggregateDevice);
    if (osErr != noErr) {
        jack_error("CreateAggregateDevice : AudioObjectGetPropertyData error");
        printError(osErr);
        goto error;
    }

    // pause for a bit to make sure that everything completed correctly
    // this is to work around a bug in the HAL where a new aggregate device seems to disappear briefly after it is created
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.1, false);

    //-------------------------
    // Set the sub-device list
    //-------------------------

    pluginAOPA.mSelector = kAudioAggregateDevicePropertyFullSubDeviceList;
    pluginAOPA.mScope = kAudioObjectPropertyScopeGlobal;
    pluginAOPA.mElement = kAudioObjectPropertyElementMaster;
    outDataSize = sizeof(CFMutableArrayRef);
    osErr = AudioObjectSetPropertyData(*outAggregateDevice, &pluginAOPA, 0, NULL, outDataSize, &subDevicesArray);
    if (osErr != noErr) {
        jack_error("CreateAggregateDevice : AudioObjectSetPropertyData for sub-device list error");
        printError(osErr);
        goto error;
    }

    // pause again to give the changes time to take effect
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.1, false);

    //-----------------------
    // Set the master device
    //-----------------------

    // set the master device manually (this is the device which will act as the master clock for the aggregate device)
    // pass in the UID of the device you want to use
    pluginAOPA.mSelector = kAudioAggregateDevicePropertyMasterSubDevice;
    pluginAOPA.mScope = kAudioObjectPropertyScopeGlobal;
    pluginAOPA.mElement = kAudioObjectPropertyElementMaster;
    outDataSize = sizeof(CFStringRef);
    osErr = AudioObjectSetPropertyData(*outAggregateDevice, &pluginAOPA, 0, NULL, outDataSize, &captureDeviceUID[0]);  // First apture is master...
    if (osErr != noErr) {
        jack_error("CreateAggregateDevice : AudioObjectSetPropertyData for master device error");
        printError(osErr);
        goto error;
    }

    // pause again to give the changes time to take effect
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.1, false);

    // Prepare sub-devices for clock drift compensation
    // Workaround for bug in the HAL : until 10.6.2

    if (fClockDriftCompensate) {
        if (need_clock_drift_compensation) {
            jack_info("Clock drift compensation activated...");

            // Get the property data size
            osErr = AudioObjectGetPropertyDataSize(*outAggregateDevice, &theAddressOwned, theQualifierDataSize, theQualifierData, &outSize);
            if (osErr != noErr) {
                jack_error("CreateAggregateDevice kAudioObjectPropertyOwnedObjects error");
                printError(osErr);
            }

            //	Calculate the number of object IDs
            subDevicesNum = outSize / sizeof(AudioObjectID);
            jack_info("JackCoreAudioDriver::CreateAggregateDevice clock drift compensation, number of sub-devices = %d", subDevicesNum);
            AudioObjectID subDevices[subDevicesNum];
            outSize = sizeof(subDevices);

            osErr = AudioObjectGetPropertyData(*outAggregateDevice, &theAddressOwned, theQualifierDataSize, theQualifierData, &outSize, subDevices);
            if (osErr != noErr) {
                jack_error("CreateAggregateDevice kAudioObjectPropertyOwnedObjects error");
                printError(osErr);
            }

            // Set kAudioSubDevicePropertyDriftCompensation property...
            for (UInt32 index = 0; index < subDevicesNum; ++index) {
                UInt32 theDriftCompensationValue = 1;
                osErr = AudioObjectSetPropertyData(subDevices[index], &theAddressDrift, 0, NULL, sizeof(UInt32), &theDriftCompensationValue);
                if (osErr != noErr) {
                    jack_error("CreateAggregateDevice kAudioSubDevicePropertyDriftCompensation error");
                    printError(osErr);
                }
            }
        } else {
            jack_info("Clock drift compensation was asked but is not needed (devices use the same clock domain)");
        }
    }

    // pause again to give the changes time to take effect
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.1, false);

    //----------
    // Clean up
    //----------

    // release the private AD key
    CFRelease(AggregateDeviceNumberRef);

    // release the CF objects we have created - we don't need them any more
    CFRelease(aggDeviceDict);
    CFRelease(subDevicesArray);

    if (subDevicesArrayClock) {
        CFRelease(subDevicesArrayClock);
    }

    // release the device UID
    for (UInt32 i = 0; i < captureDeviceUID.size(); i++) {
        CFRelease(captureDeviceUID[i]);
    }

    for (UInt32 i = 0; i < playbackDeviceUID.size(); i++) {
        CFRelease(playbackDeviceUID[i]);
    }

    jack_log("JackCoreAudioDriver::CreateAggregateDeviceAux : new aggregate device %ld", *outAggregateDevice);
    return noErr;

error:
    DestroyAggregateDevice();
    return -1;
}

int JackCoreAudioDriver::SetupDevices(const char* capture_driver_uid,
                                      const char* playback_driver_uid,
                                      char* capture_driver_name,
                                      char* playback_driver_name,
                                      jack_nframes_t samplerate,
                                      bool ac3_encoding)
{
    capture_driver_name[0] = 0;
    playback_driver_name[0] = 0;

    // Duplex
    if (strcmp(capture_driver_uid, "") != 0 && strcmp(playback_driver_uid, "") != 0) {
        jack_log("JackCoreAudioDriver::SetupDevices : duplex");

        // Same device for capture and playback...
        if (strcmp(capture_driver_uid, playback_driver_uid) == 0)  {

            if (GetDeviceIDFromUID(playback_driver_uid, &fDeviceID) != noErr) {
                jack_log("JackCoreAudioDriver::SetupDevices : will take default in/out");
                if (GetDefaultDevice(&fDeviceID) != noErr) {
                    jack_error("Cannot open default device");
                    return -1;
                }
            }
            if (GetDeviceNameFromID(fDeviceID, capture_driver_name) != noErr || GetDeviceNameFromID(fDeviceID, playback_driver_name) != noErr) {
                jack_error("Cannot get device name from device ID");
                return -1;
            }
            
            if (fHogged) {
                if (!TakeHogAux(fDeviceID, false)) {
                    jack_error("Cannot take hog mode");
                } 
                if (ac3_encoding) {
                    fDigitalPlayback = IsDigitalDevice(fDeviceID);
                }
            }

        } else {

            // Creates aggregate device
            AudioDeviceID captureID = -1;
            AudioDeviceID playbackID = -1;
    
            if (GetDeviceIDFromUID(capture_driver_uid, &captureID) != noErr) {
                jack_log("JackCoreAudioDriver::SetupDevices : will take default input");
                if (GetDefaultInputDevice(&captureID) != noErr) {
                    jack_error("Cannot open default input device");
                    return -1;
                }
            }

            if (GetDeviceIDFromUID(playback_driver_uid, &playbackID) != noErr) {
                jack_log("JackCoreAudioDriver::SetupDevices : will take default output");
                if (GetDefaultOutputDevice(&playbackID) != noErr) {
                    jack_error("Cannot open default output device");
                    return -1;
                }
            }

            if (CreateAggregateDevice(captureID, playbackID, samplerate, &fDeviceID) != noErr) {
                return -1;
            }

            GetDeviceNameFromID(captureID, fCaptureUID);
            GetDeviceNameFromID(playbackID, fPlaybackUID);
            
            if (fHogged) {
                if (!TakeHogAux(captureID, true)) {
                    jack_error("Cannot take hog mode for capture device");
                }
                if (!TakeHogAux(playbackID, false)) {
                    jack_error("Cannot take hog mode for playback device");
                }
                if (ac3_encoding) {
                    fDigitalPlayback = IsDigitalDevice(playbackID);
                }
            }
            
        }

    // Capture only
    } else if (strcmp(capture_driver_uid, "") != 0) {
        jack_log("JackCoreAudioDriver::SetupDevices : capture only");
        if (GetDeviceIDFromUID(capture_driver_uid, &fDeviceID) != noErr) {
            jack_log("JackCoreAudioDriver::SetupDevices : will take default input");
            if (GetDefaultInputDevice(&fDeviceID) != noErr) {
                jack_error("Cannot open default input device");
                return -1;
            }
        }
        if (GetDeviceNameFromID(fDeviceID, capture_driver_name) != noErr) {
            jack_error("Cannot get device name from device ID");
            return -1;
        }
        
        if (fHogged) {
            if (!TakeHogAux(fDeviceID, true)) {
                jack_error("Cannot take hog mode for capture device");
            }
        }

    // Playback only
    } else if (strcmp(playback_driver_uid, "") != 0) {
        jack_log("JackCoreAudioDriver::SetupDevices : playback only");
        if (GetDeviceIDFromUID(playback_driver_uid, &fDeviceID) != noErr) {
            jack_log("JackCoreAudioDriver::SetupDevices : will take default output");
            if (GetDefaultOutputDevice(&fDeviceID) != noErr) {
                jack_error("Cannot open default output device");
                return -1;
            }
        }
        if (GetDeviceNameFromID(fDeviceID, playback_driver_name) != noErr) {
            jack_error("Cannot get device name from device ID");
            return -1;
        }
        
        if (fHogged) {
            if (!TakeHogAux(fDeviceID, false)) {
                jack_error("Cannot take hog mode for playback device");
            }
            if (ac3_encoding) {
                fDigitalPlayback = IsDigitalDevice(fDeviceID);
            }
        }

    // Use default driver in duplex mode
    } else {
        jack_log("JackCoreAudioDriver::SetupDevices : default driver");
        if (GetDefaultDevice(&fDeviceID) != noErr) {
            jack_error("Cannot open default device in duplex mode, so aggregate default input and default output");

            // Creates aggregate device
            AudioDeviceID captureID = -1;
            AudioDeviceID playbackID = -1;
            
            if (GetDeviceIDFromUID(capture_driver_uid, &captureID) != noErr) {
                jack_log("JackCoreAudioDriver::SetupDevices : will take default input");
                if (GetDefaultInputDevice(&captureID) != noErr) {
                    jack_error("Cannot open default input device");
                    return -1;
                }
            }

            if (GetDeviceIDFromUID(playback_driver_uid, &playbackID) != noErr) {
                jack_log("JackCoreAudioDriver::SetupDevices : will take default output");
                if (GetDefaultOutputDevice(&playbackID) != noErr) {
                    jack_error("Cannot open default output device");
                    return -1;
                }
            }

            if (CreateAggregateDevice(captureID, playbackID, samplerate, &fDeviceID) != noErr) {
                return -1;
            }

            GetDeviceNameFromID(captureID, fCaptureUID);
            GetDeviceNameFromID(playbackID, fPlaybackUID);
            
            if (fHogged) {
                if (!TakeHogAux(captureID, true)) {
                    jack_error("Cannot take hog mode for capture device");
                }
                if (!TakeHogAux(playbackID, false)) {
                    jack_error("Cannot take hog mode for playback device");
                }
                if (ac3_encoding) {
                    fDigitalPlayback = IsDigitalDevice(playbackID);
                }
            }
        }
    }
     
    return 0;
}

/*
Return the max possible input channels in in_maxChannels and output channels in out_maxChannels.
*/
int JackCoreAudioDriver::SetupChannels(bool capturing, bool playing, int& inchannels, int& outchannels, int& in_maxChannels, int& out_maxChannels, bool strict)
{
    OSStatus err = noErr;

    if (capturing) {
        err = GetTotalChannels(fDeviceID, in_maxChannels, true);
        if (err != noErr) {
            jack_error("SetupChannels : cannot get input channel number");
            printError(err);
            return -1;
        } else {
            jack_log("JackCoreAudioDriver::SetupChannels : max input channels : %d", in_maxChannels);
        }
    }

    if (playing) {
        err = GetTotalChannels(fDeviceID, out_maxChannels, false);
        if (err != noErr) {
            jack_error("Cannot get output channel number");
            printError(err);
            return -1;
        } else {
            jack_log("JackCoreAudioDriver::SetupChannels : max output channels : %d", out_maxChannels);
        }
    }

    if (inchannels > in_maxChannels) {
        jack_error("This device hasn't required input channels inchannels = %d in_maxChannels = %d", inchannels, in_maxChannels);
        if (strict) {
            return -1;
        }
    }

    if (outchannels > out_maxChannels) {
        jack_error("This device hasn't required output channels outchannels = %d out_maxChannels = %d", outchannels, out_maxChannels);
        if (strict) {
            return -1;
        }
    }

    if (inchannels == -1) {
        jack_log("JackCoreAudioDriver::SetupChannels : setup max in channels = %d", in_maxChannels);
        inchannels = in_maxChannels;
    }

    if (outchannels == -1) {
        jack_log("JackCoreAudioDriver::SetupChannels : setup max out channels = %d", out_maxChannels);
        outchannels = out_maxChannels;
    }

    return 0;
}

int JackCoreAudioDriver::SetupBufferSize(jack_nframes_t buffer_size)
{
    // Setting buffer size
    OSStatus err = noErr;
    UInt32 tmp_buffer_size = buffer_size;
    UInt32 outSize = sizeof(UInt32);

    err = AudioDeviceGetProperty(fDeviceID, 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyBufferFrameSize, &outSize, &tmp_buffer_size);
    if (err != noErr) {
        jack_error("Cannot get buffer size %ld", buffer_size);
        printError(err);
        return -1;
    } else {
        jack_log("JackCoreAudioDriver::SetupBufferSize : current buffer size = %ld", tmp_buffer_size);
    }

    // If needed, set new buffer size
    if (buffer_size != tmp_buffer_size) {
        tmp_buffer_size = buffer_size;

        // To get BS change notification
        err = AudioDeviceAddPropertyListener(fDeviceID, 0, true, kAudioDevicePropertyBufferFrameSize, BSNotificationCallback, this);
        if (err != noErr) {
            jack_error("Error calling AudioDeviceAddPropertyListener with kAudioDevicePropertyBufferFrameSize");
            printError(err);
            return -1;
        }

        // Waiting for BS change notification
        int count = 0;
        fState = false;

        err = AudioDeviceSetProperty(fDeviceID, NULL, 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyBufferFrameSize, outSize, &tmp_buffer_size);
        if (err != noErr) {
            jack_error("SetupBufferSize : cannot set buffer size = %ld", tmp_buffer_size);
            printError(err);
            goto error;
        }

        while (!fState && count++ < WAIT_NOTIFICATION_COUNTER) {
            usleep(100000);
            jack_log("JackCoreAudioDriver::SetupBufferSize : wait count = %d", count);
        }

        if (count >= WAIT_NOTIFICATION_COUNTER) {
            jack_error("Did not get buffer size notification...");
            goto error;
        }

        // Check new buffer size
        outSize = sizeof(UInt32);
        err = AudioDeviceGetProperty(fDeviceID, 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyBufferFrameSize, &outSize, &tmp_buffer_size);
        if (err != noErr) {
            jack_error("Cannot get current buffer size");
            printError(err);
        } else {
            jack_log("JackCoreAudioDriver::SetupBufferSize : checked buffer size = %ld", tmp_buffer_size);
        }

        // Remove BS change notification
        AudioDeviceRemovePropertyListener(fDeviceID, 0, true, kAudioDevicePropertyBufferFrameSize, BSNotificationCallback);
    }

    return 0;

error:

    // Remove BS change notification
    AudioDeviceRemovePropertyListener(fDeviceID, 0, true, kAudioDevicePropertyBufferFrameSize, BSNotificationCallback);
    return -1;
}

int JackCoreAudioDriver::SetupSampleRate(jack_nframes_t sample_rate)
{
    return SetupSampleRateAux(fDeviceID, sample_rate);
}

int JackCoreAudioDriver::SetupSampleRateAux(AudioDeviceID inDevice, jack_nframes_t sample_rate)
{
    OSStatus err = noErr;
    UInt32 outSize;
    Float64 tmp_sample_rate;

    // Get sample rate
    outSize =  sizeof(Float64);
    err = AudioDeviceGetProperty(inDevice, 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyNominalSampleRate, &outSize, &tmp_sample_rate);
    if (err != noErr) {
        jack_error("Cannot get current sample rate");
        printError(err);
        return -1;
    } else {
        jack_log("JackCoreAudioDriver::SetupSampleRateAux : current sample rate = %f", tmp_sample_rate);
    }

    // If needed, set new sample rate
    if (sample_rate != (jack_nframes_t)tmp_sample_rate) {
        tmp_sample_rate = (Float64)sample_rate;

        // To get SR change notification
        err = AudioDeviceAddPropertyListener(inDevice, 0, true, kAudioDevicePropertyNominalSampleRate, SRNotificationCallback, this);
        if (err != noErr) {
            jack_error("Error calling AudioDeviceAddPropertyListener with kAudioDevicePropertyNominalSampleRate");
            printError(err);
            return -1;
        }

        // Waiting for SR change notification
        int count = 0;
        fState = false;

        err = AudioDeviceSetProperty(inDevice, NULL, 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyNominalSampleRate, outSize, &tmp_sample_rate);
        if (err != noErr) {
            jack_error("Cannot set sample rate = %ld", sample_rate);
            printError(err);
            goto error;
        }

        while (!fState && count++ < WAIT_NOTIFICATION_COUNTER) {
            usleep(100000);
            jack_log("JackCoreAudioDriver::SetupSampleRateAux : wait count = %d", count);
        }

        if (count >= WAIT_NOTIFICATION_COUNTER) {
            jack_error("Did not get sample rate notification...");
            goto error;
        }

        // Check new sample rate
        outSize = sizeof(Float64);
        err = AudioDeviceGetProperty(inDevice, 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyNominalSampleRate, &outSize, &tmp_sample_rate);
        if (err != noErr) {
            jack_error("Cannot get current sample rate");
            printError(err);
        } else {
            jack_log("JackCoreAudioDriver::SetupSampleRateAux : checked sample rate = %f", tmp_sample_rate);
        }

        // Remove SR change notification
        AudioDeviceRemovePropertyListener(inDevice, 0, true, kAudioDevicePropertyNominalSampleRate, SRNotificationCallback);
    }

    return 0;

error:

    // Remove SR change notification
    AudioDeviceRemovePropertyListener(inDevice, 0, true, kAudioDevicePropertyNominalSampleRate, SRNotificationCallback);
    return -1;
}

int JackCoreAudioDriver::OpenAUHAL(bool capturing,
                                   bool playing,
                                   int inchannels,
                                   int outchannels,
                                   int in_maxChannels,
                                   int out_maxChannels,
                                   const vector<int>& chan_in_list,
                                   const vector<int>& chan_out_list,
                                   jack_nframes_t buffer_size,
                                   jack_nframes_t sample_rate)
{
    ComponentResult err1;
    UInt32 enableIO;
    AudioStreamBasicDescription srcFormat, dstFormat;
    AudioDeviceID currAudioDeviceID;
    UInt32 size;

    jack_log("JackCoreAudioDriver::OpenAUHAL : capturing = %d playing = %d inchannels = %d outchannels = %d in_maxChannels = %d out_maxChannels = %d chan_in_list = %d chan_out_list = %d",
        capturing, playing, inchannels, outchannels, in_maxChannels, out_maxChannels, chan_in_list.size(), chan_out_list.size());

    if (inchannels == 0 && outchannels == 0) {
        jack_error("No input and output channels...");
        return -1;
    }
    
    // AUHAL
#ifdef MAC_OS_X_VERSION_10_5
    ComponentDescription cd = {kAudioUnitType_Output, kAudioUnitSubType_HALOutput, kAudioUnitManufacturer_Apple, 0, 0};
    Component HALOutput = FindNextComponent(NULL, &cd);
    err1 = OpenAComponent(HALOutput, &fAUHAL);
    if (err1 != noErr) {
        jack_error("Error calling OpenAComponent");
        printError(err1);
        goto error;
    }
#else 
    AudioComponentDescription cd = {kAudioUnitType_Output, kAudioUnitSubType_HALOutput, kAudioUnitManufacturer_Apple, 0, 0};
    AudioComponent HALOutput = AudioComponentFindNext(NULL, &cd);
    err1 = AudioComponentInstanceNew(HALOutput, &fAUHAL);
    if (err1 != noErr) {
        jack_error("Error calling AudioComponentInstanceNew");
        printError(err1);
        goto error;
    }
#endif

    err1 = AudioUnitInitialize(fAUHAL);
    if (err1 != noErr) {
        jack_error("Cannot initialize AUHAL unit");
        printError(err1);
        goto error;
    }
    
    // Start I/O
    if (capturing && inchannels > 0) {
        enableIO = 1;
        jack_log("JackCoreAudioDriver::OpenAUHAL : setup AUHAL input on");
    } else {
        enableIO = 0;
        jack_log("JackCoreAudioDriver::OpenAUHAL : setup AUHAL input off");
    }

    err1 = AudioUnitSetProperty(fAUHAL, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, 1, &enableIO, sizeof(enableIO));
    if (err1 != noErr) {
        jack_error("Error calling AudioUnitSetProperty - kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input");
        printError(err1);
        goto error;
    }

    if (playing && outchannels > 0) {
        enableIO = 1;
        jack_log("JackCoreAudioDriver::OpenAUHAL : setup AUHAL output on");
    } else {
        enableIO = 0;
        jack_log("JackCoreAudioDriver::OpenAUHAL : setup AUHAL output off");
    }

    err1 = AudioUnitSetProperty(fAUHAL, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, 0, &enableIO, sizeof(enableIO));
    if (err1 != noErr) {
        jack_error("Error calling AudioUnitSetProperty - kAudioOutputUnitProperty_EnableIO,kAudioUnitScope_Output");
        printError(err1);
        goto error;
    }

    size = sizeof(AudioDeviceID);
    err1 = AudioUnitGetProperty(fAUHAL, kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Global, 0, &currAudioDeviceID, &size);
    if (err1 != noErr) {
        jack_error("Error calling AudioUnitGetProperty - kAudioOutputUnitProperty_CurrentDevice");
        printError(err1);
        goto error;
    } else {
        jack_log("JackCoreAudioDriver::OpenAUHAL : AudioUnitGetPropertyCurrentDevice = %d", currAudioDeviceID);
    }

    // Setup up choosen device, in both input and output cases
    err1 = AudioUnitSetProperty(fAUHAL, kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Global, 0, &fDeviceID, sizeof(AudioDeviceID));
    if (err1 != noErr) {
        jack_error("Error calling AudioUnitSetProperty - kAudioOutputUnitProperty_CurrentDevice");
        printError(err1);
        goto error;
    }

    // Set buffer size
    if (capturing && inchannels > 0) {
        err1 = AudioUnitSetProperty(fAUHAL, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 1, (UInt32*)&buffer_size, sizeof(UInt32));
        if (err1 != noErr) {
            jack_error("Error calling AudioUnitSetProperty - kAudioUnitProperty_MaximumFramesPerSlice");
            printError(err1);
            goto error;
        }
    }

    if (playing && outchannels > 0) {
        err1 = AudioUnitSetProperty(fAUHAL, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, (UInt32*)&buffer_size, sizeof(UInt32));
        if (err1 != noErr) {
            jack_error("Error calling AudioUnitSetProperty - kAudioUnitProperty_MaximumFramesPerSlice");
            printError(err1);
            goto error;
        }
    }

    // Setup input channel map
    if (capturing && inchannels > 0 && inchannels <= in_maxChannels) {
        SInt32 chanArr[in_maxChannels];
        for (int i = 0; i < in_maxChannels; i++) {
            chanArr[i] = -1;
        }
        // Explicit mapping
        if (chan_in_list.size() > 0) {
            for (uint i = 0; i < chan_in_list.size(); i++) {
                int chan = chan_in_list[i];
                if (chan < out_maxChannels) {
                    // The wanted JACK input index for the 'chan' channel value
                    chanArr[chan] = i;
                    jack_info("Input channel = %d ==> JACK input port = %d", chan, i);
                } else {
                    jack_info("Error input channel number is incorrect : %d", chan);
                    goto error;
                }
            }
        } else {
            for (int i = 0; i < inchannels; i++) {
                chanArr[i] = i;
                jack_info("Input channel = %d ==> JACK input port = %d", chanArr[i], i);
            }
        }

        AudioUnitSetProperty(fAUHAL, kAudioOutputUnitProperty_ChannelMap , kAudioUnitScope_Input, 1, chanArr, sizeof(SInt32) * in_maxChannels);
        if (err1 != noErr) {
            jack_error("Error calling AudioUnitSetProperty - kAudioOutputUnitProperty_ChannelMap for input");
            printError(err1);
            goto error;
        }
    }

    // Setup output channel map
    if (playing && outchannels > 0 && outchannels <= out_maxChannels) {
        SInt32 chanArr[out_maxChannels];
        for (int i = 0;	i < out_maxChannels; i++) {
            chanArr[i] = -1;
        }
        // Explicit mapping
        if (chan_out_list.size() > 0) {
            for (uint i = 0; i < chan_out_list.size(); i++) {
                int chan = chan_out_list[i];
                if (chan < out_maxChannels) {
                    // The wanted JACK output index for the 'chan' channel value
                    chanArr[chan] = i;
                    jack_info("JACK output port = %d ==> output channel = %d", i, chan);
                } else {
                    jack_info("Error output channel number is incorrect : %d", chan);
                    goto error;
                }
            }
        } else {
            for (int i = 0; i < outchannels; i++) {
                chanArr[i] = i;
                jack_info("JACK output port = %d ==> output channel = %d", i, chanArr[i]);
            }
        }

        err1 = AudioUnitSetProperty(fAUHAL, kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Output, 0, chanArr, sizeof(SInt32) * out_maxChannels);
        if (err1 != noErr) {
            jack_error("Error calling AudioUnitSetProperty - kAudioOutputUnitProperty_ChannelMap for output");
            printError(err1);
            goto error;
        }
    }

    // Setup stream converters
    if (capturing && inchannels > 0) {

        size = sizeof(AudioStreamBasicDescription);
        err1 = AudioUnitGetProperty(fAUHAL, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 1, &srcFormat, &size);
        if (err1 != noErr) {
            jack_error("Error calling AudioUnitGetProperty - kAudioUnitProperty_StreamFormat kAudioUnitScope_Output");
            printError(err1);
            goto error;
        }
        PrintStreamDesc(&srcFormat);

        jack_log("JackCoreAudioDriver::OpenAUHAL : setup AUHAL input stream converter SR = %ld", sample_rate);
        srcFormat.mSampleRate = sample_rate;
        srcFormat.mFormatID = kAudioFormatLinearPCM;
        srcFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked | kLinearPCMFormatFlagIsNonInterleaved;
        srcFormat.mBytesPerPacket = sizeof(jack_default_audio_sample_t);
        srcFormat.mFramesPerPacket = 1;
        srcFormat.mBytesPerFrame = sizeof(jack_default_audio_sample_t);
        srcFormat.mChannelsPerFrame = inchannels;
        srcFormat.mBitsPerChannel = 32;
        PrintStreamDesc(&srcFormat);

        err1 = AudioUnitSetProperty(fAUHAL, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 1, &srcFormat, sizeof(AudioStreamBasicDescription));
        if (err1 != noErr) {
            jack_error("Error calling AudioUnitSetProperty - kAudioUnitProperty_StreamFormat kAudioUnitScope_Output");
            printError(err1);
            goto error;
        }
    }

    if (playing && outchannels > 0) {

        size = sizeof(AudioStreamBasicDescription);
        err1 = AudioUnitGetProperty(fAUHAL, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &dstFormat, &size);
        if (err1 != noErr) {
            jack_error("Error calling AudioUnitGetProperty - kAudioUnitProperty_StreamFormat kAudioUnitScope_Input");
            printError(err1);
            goto error;
        }
        PrintStreamDesc(&dstFormat);

        jack_log("JackCoreAudioDriver::OpenAUHAL : setup AUHAL output stream converter SR = %ld", sample_rate);
        dstFormat.mSampleRate = sample_rate;
        dstFormat.mFormatID = kAudioFormatLinearPCM;
        dstFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked | kLinearPCMFormatFlagIsNonInterleaved;
        dstFormat.mBytesPerPacket = sizeof(jack_default_audio_sample_t);
        dstFormat.mFramesPerPacket = 1;
        dstFormat.mBytesPerFrame = sizeof(jack_default_audio_sample_t);
        dstFormat.mChannelsPerFrame = outchannels;
        dstFormat.mBitsPerChannel = 32;
        PrintStreamDesc(&dstFormat);

        err1 = AudioUnitSetProperty(fAUHAL, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &dstFormat, sizeof(AudioStreamBasicDescription));
        if (err1 != noErr) {
            jack_error("Error calling AudioUnitSetProperty - kAudioUnitProperty_StreamFormat kAudioUnitScope_Input");
            printError(err1);
            goto error;
        }
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
            goto error;
        }
    } else {
        AURenderCallbackStruct output;
        output.inputProc = Render;
        output.inputProcRefCon = this;
        err1 = AudioUnitSetProperty(fAUHAL, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &output, sizeof(output));
        if (err1 != noErr) {
            jack_error("Error calling AudioUnitSetProperty - kAudioUnitProperty_SetRenderCallback 0");
            printError(err1);
            goto error;
        }
    }

    return 0;

error:
    CloseAUHAL();
    return -1;
}

int JackCoreAudioDriver::SetupBuffers(int inchannels)
{
    // Prepare buffers
    fJackInputData = (AudioBufferList*)malloc(sizeof(UInt32) + inchannels * sizeof(AudioBuffer));
    fJackInputData->mNumberBuffers = inchannels;
    for (int i = 0; i < inchannels; i++) {
        fJackInputData->mBuffers[i].mNumberChannels = 1;
        fJackInputData->mBuffers[i].mDataByteSize = fEngineControl->fBufferSize * sizeof(jack_default_audio_sample_t);
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
    AudioOutputUnitStop(fAUHAL);
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

    err = AudioHardwareAddPropertyListener(kAudioHardwarePropertyDevices, AudioHardwareNotificationCallback, this);
    if (err != noErr) {
        jack_error("Error calling AudioHardwareAddPropertyListener with kAudioHardwarePropertyDevices");
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

    err = AudioDeviceAddPropertyListener(fDeviceID, 0, true, kAudioDevicePropertyDeviceIsAlive, DeviceNotificationCallback, this);
    if (err != noErr) {
        jack_error("Error calling AudioDeviceAddPropertyListener with kAudioDevicePropertyDeviceIsAlive");
        printError(err);
        return -1;
    }

    err = AudioDeviceAddPropertyListener(fDeviceID, 0, true, kAudioDevicePropertyDeviceHasChanged, DeviceNotificationCallback, this);
    if (err != noErr) {
        jack_error("Error calling AudioDeviceAddPropertyListener with kAudioDevicePropertyDeviceHasChanged");
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
    AudioHardwareRemovePropertyListener(kAudioHardwarePropertyDevices, AudioHardwareNotificationCallback);
    AudioDeviceRemovePropertyListener(fDeviceID, 0, true, kAudioDevicePropertyNominalSampleRate, DeviceNotificationCallback);
    AudioDeviceRemovePropertyListener(fDeviceID, 0, true, kAudioDevicePropertyDeviceIsRunning, DeviceNotificationCallback);
    AudioDeviceRemovePropertyListener(fDeviceID, 0, true, kAudioDevicePropertyDeviceIsAlive, DeviceNotificationCallback);
    AudioDeviceRemovePropertyListener(fDeviceID, 0, true, kAudioDevicePropertyDeviceHasChanged, DeviceNotificationCallback);
    AudioDeviceRemovePropertyListener(fDeviceID, 0, true, kAudioDevicePropertyStreamConfiguration, DeviceNotificationCallback);
    AudioDeviceRemovePropertyListener(fDeviceID, 0, false, kAudioDevicePropertyStreamConfiguration, DeviceNotificationCallback);
}

int JackCoreAudioDriver::Open(jack_nframes_t buffer_size,
                              jack_nframes_t sample_rate,
                              bool capturing,
                              bool playing,
                              int inchannels,
                              int outchannels,
                              const char* chan_in_list,
                              const char* chan_out_list,
                              bool monitor,
                              const char* capture_driver_uid,
                              const char* playback_driver_uid,
                              jack_nframes_t capture_latency,
                              jack_nframes_t playback_latency,
                              int async_output_latency,
                              int computation_grain,
                              bool hogged,
                              bool clock_drift,
                              bool ac3_encoding,
                              int ac3_bitrate,
                              bool ac3_lfe)
{
    int in_maxChannels = 0;
    int out_maxChannels = 0;
    char capture_driver_name[256];
    char playback_driver_name[256];

    fCaptureLatency = capture_latency;
    fPlaybackLatency = playback_latency;
    fIOUsage = float(async_output_latency) / 100.f;
    fComputationGrain = float(computation_grain) / 100.f;
    fHogged = hogged;
    fClockDriftCompensate = clock_drift;

    SInt32 major;
    SInt32 minor;
    Gestalt(gestaltSystemVersionMajor, &major);
    Gestalt(gestaltSystemVersionMinor, &minor);

    vector<int> parsed_chan_in_list;
    vector<int> parsed_chan_out_list;
   
    // Starting with 10.6 systems, the HAL notification thread is created internally
    if (major == 10 && minor >= 6) {
        CFRunLoopRef theRunLoop = NULL;
        AudioObjectPropertyAddress theAddress = { kAudioHardwarePropertyRunLoop, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster };
        OSStatus osErr = AudioObjectSetPropertyData (kAudioObjectSystemObject, &theAddress, 0, NULL, sizeof(CFRunLoopRef), &theRunLoop);
        if (osErr != noErr) {
            jack_error("Open kAudioHardwarePropertyRunLoop error");
            printError(osErr);
        }
    }
  
    if (SetupDevices(capture_driver_uid, playback_driver_uid, capture_driver_name, playback_driver_name, sample_rate, ac3_encoding) < 0) {
        goto error;
    }

    // Generic JackAudioDriver Open
    if (JackAudioDriver::Open(buffer_size, sample_rate,
                            capturing, playing,
                            inchannels, outchannels,
                            monitor,
                            capture_driver_name,
                            playback_driver_name,
                            capture_latency,
                            playback_latency) != 0) {
        goto error;
    }

    if (SetupChannels(capturing, playing, inchannels, outchannels, in_maxChannels, out_maxChannels, !ac3_encoding) < 0) {
        goto error;
    }
    
    ParseChannelList(chan_in_list, parsed_chan_in_list, in_maxChannels);
    if (parsed_chan_in_list.size() > 0) {
        jack_info("Explicit input channel list size = %d", parsed_chan_in_list.size());
        inchannels = parsed_chan_in_list.size();
    }

    ParseChannelList(chan_out_list, parsed_chan_out_list, out_maxChannels);
    if (parsed_chan_out_list.size() > 0) {
        jack_info("Explicit output channel list size = %d", parsed_chan_out_list.size());
        outchannels = parsed_chan_out_list.size();
    }  

    if (SetupBufferSize(buffer_size) < 0) {
        goto error;
    }

    if (SetupSampleRate(sample_rate) < 0) {
        goto error;
    }
    
    if (ac3_encoding) {
          
        if (!fDigitalPlayback) {
            jack_error("AC3 encoding can only be used with a digital device");
            goto error;
        }
        
        JackAC3EncoderParams params;
        memset(&params, 0, sizeof(JackAC3EncoderParams));
        params.bitrate = ac3_bitrate;
        params.channels = outchannels;
        params.sample_rate = sample_rate;
        params.lfe = ac3_lfe;
        fAC3Encoder = new JackAC3Encoder(params);
        
        if (!fAC3Encoder || !fAC3Encoder->Init(sample_rate)) {
            jack_error("Cannot allocate or init AC3 encoder");
            goto error;
        }
        
        // Setup AC3 channel number
        fPlaybackChannels = outchannels;
        if (ac3_lfe) {
            fPlaybackChannels++;
        }
        
        if (fPlaybackChannels < 2 || fPlaybackChannels > 6) {
            jack_error("AC3 encoder channels must be between 2 and 6");
            goto error;
        }
        
        // Force real output channel number to 2
        outchannels = out_maxChannels = 2;
        
    } else {
        fPlaybackChannels = outchannels;
    }
    
    // Core driver may have changed the in/out values
    fCaptureChannels = inchannels;

    if (OpenAUHAL(capturing, playing, inchannels, outchannels, in_maxChannels, out_maxChannels, parsed_chan_in_list, parsed_chan_out_list, buffer_size, sample_rate) < 0) {
        goto error;
    }

    if (capturing && inchannels > 0) {
        if (SetupBuffers(inchannels) < 0) {
            goto error;
        }
    }

    if (AddListeners() < 0) {
        goto error;
    }
  
    return noErr;

error:
    Close();
    return -1;
}

int JackCoreAudioDriver::Close()
{
    jack_log("JackCoreAudioDriver::Close");

    // Generic audio driver close
    int res = JackAudioDriver::Close();

    RemoveListeners();
    DisposeBuffers();
    CloseAUHAL();
    DestroyAggregateDevice();
    return res;
}

void JackCoreAudioDriver::UpdateLatencies()
{
    UInt32 size;
    OSStatus err;
    jack_latency_range_t input_range;
    jack_latency_range_t output_range;
    jack_latency_range_t monitor_range;

    // Get Input latency
    size = sizeof(UInt32);
    UInt32 value1 = 0;
    UInt32 value2 = 0;
    err = AudioDeviceGetProperty(fDeviceID, 0, true, kAudioDevicePropertyLatency, &size, &value1);
    if (err != noErr) {
        jack_error("AudioDeviceGetProperty kAudioDevicePropertyLatency error");
    }
    err = AudioDeviceGetProperty(fDeviceID, 0, true, kAudioDevicePropertySafetyOffset, &size, &value2);
    if (err != noErr) {
        jack_error("AudioDeviceGetProperty kAudioDevicePropertySafetyOffset error");
    }

    input_range.min = input_range.max = fEngineControl->fBufferSize + value1 + value2 + fCaptureLatency;

    // Get input stream latencies
    vector<int> input_latencies;
    err = GetStreamLatencies(fDeviceID, true, input_latencies);

    for (int i = 0; i < fCaptureChannels; i++) {
        if (err != noErr) {
            input_range.min += input_latencies[i];
            input_range.max += input_latencies[i];
        }
        fGraphManager->GetPort(fCapturePortList[i])->SetLatencyRange(JackCaptureLatency, &input_range);
    }

    // Get Output latency
    size = sizeof(UInt32);
    value1 = 0;
    value2 = 0;
    err = AudioDeviceGetProperty(fDeviceID, 0, false, kAudioDevicePropertyLatency, &size, &value1);
    if (err != noErr) {
        jack_error("AudioDeviceGetProperty kAudioDevicePropertyLatency error");
    }
    err = AudioDeviceGetProperty(fDeviceID, 0, false, kAudioDevicePropertySafetyOffset, &size, &value2);
    if (err != noErr) {
        jack_error("AudioDeviceGetProperty kAudioDevicePropertySafetyOffset error");
    }

    // Get output stream latencies
    vector<int> output_latencies;
    err = GetStreamLatencies(fDeviceID, false, output_latencies);

    // Add more latency if "async" mode is used...
    output_range.min = output_range.max = fEngineControl->fBufferSize + ((fEngineControl->fSyncMode)
        ? 0 : fEngineControl->fBufferSize * fIOUsage) + value1 + value2 + fPlaybackLatency;

    for (int i = 0; i < fPlaybackChannels; i++) {
        if (err != noErr) {
            output_range.min += output_latencies[i];
            output_range.max += output_latencies[i];
        }
        fGraphManager->GetPort(fPlaybackPortList[i])->SetLatencyRange(JackPlaybackLatency, &output_range);

        // Monitor port
        if (fWithMonitorPorts) {
            monitor_range.min = monitor_range.max = fEngineControl->fBufferSize;
            fGraphManager->GetPort(fMonitorPortList[i])->SetLatencyRange(JackCaptureLatency, &monitor_range);
        }
    }
}

int JackCoreAudioDriver::Attach()
{
    OSStatus err;
    JackPort* port;
    jack_port_id_t port_index;
    UInt32 size;
    Boolean isWritable;
    char channel_name[64];
    char name[REAL_JACK_PORT_NAME_SIZE];
    char alias[REAL_JACK_PORT_NAME_SIZE];

    jack_log("JackCoreAudioDriver::Attach : fBufferSize %ld fSampleRate %ld", fEngineControl->fBufferSize, fEngineControl->fSampleRate);

    for (int i = 0; i < fCaptureChannels; i++) {

        err = AudioDeviceGetPropertyInfo(fDeviceID, i + 1, true, kAudioDevicePropertyChannelName, &size, &isWritable);
        if (err != noErr) {
            jack_log("JackCoreAudioDriver::Attach : AudioDeviceGetPropertyInfo kAudioDevicePropertyChannelName error");
        }
        if (err == noErr && size > 0) {
            err = AudioDeviceGetProperty(fDeviceID, i + 1, true, kAudioDevicePropertyChannelName, &size, channel_name);
            if (err != noErr) {
                jack_log("JackCoreAudioDriver::Attach : AudioDeviceGetProperty kAudioDevicePropertyChannelName error");
            }
            snprintf(alias, sizeof(alias), "%s:%s:out_%s%u", fAliasName, fCaptureDriverName, channel_name, i + 1);
        } else {
            snprintf(alias, sizeof(alias), "%s:%s:out%u", fAliasName, fCaptureDriverName, i + 1);
        }

        snprintf(name, sizeof(name), "%s:capture_%d", fClientControl.fName, i + 1);

        if (fEngine->PortRegister(fClientControl.fRefNum, name, JACK_DEFAULT_AUDIO_TYPE, CaptureDriverFlags, fEngineControl->fBufferSize, &port_index) < 0) {
            jack_error("Cannot register port for %s", name);
            return -1;
        }

        port = fGraphManager->GetPort(port_index);
        port->SetAlias(alias);
        fCapturePortList[i] = port_index;
    }

    for (int i = 0; i < fPlaybackChannels; i++) {

        err = AudioDeviceGetPropertyInfo(fDeviceID, i + 1, false, kAudioDevicePropertyChannelName, &size, &isWritable);
        if (err != noErr) {
            jack_log("JackCoreAudioDriver::Attach : AudioDeviceGetPropertyInfo kAudioDevicePropertyChannelName error");
        }
        if (err == noErr && size > 0) {
            err = AudioDeviceGetProperty(fDeviceID, i + 1, false, kAudioDevicePropertyChannelName, &size, channel_name);
            if (err != noErr) {
                jack_log("JackCoreAudioDriver::Attach : AudioDeviceGetProperty kAudioDevicePropertyChannelName error");
            }
            snprintf(alias, sizeof(alias), "%s:%s:in_%s%u", fAliasName, fPlaybackDriverName, channel_name, i + 1);
        } else {
            snprintf(alias, sizeof(alias), "%s:%s:in%u", fAliasName, fPlaybackDriverName, i + 1);
        }

        snprintf(name, sizeof(name), "%s:playback_%d", fClientControl.fName, i + 1);

        if (fEngine->PortRegister(fClientControl.fRefNum, name, JACK_DEFAULT_AUDIO_TYPE, PlaybackDriverFlags, fEngineControl->fBufferSize, &port_index) < 0) {
            jack_error("Cannot register port for %s", name);
            return -1;
        }

        port = fGraphManager->GetPort(port_index);
        port->SetAlias(alias);
        fPlaybackPortList[i] = port_index;

        // Monitor ports
        if (fWithMonitorPorts) {
            jack_log("JackCoreAudioDriver::Attach : create monitor port");
            snprintf(name, sizeof(name), "%s:monitor_%u", fClientControl.fName, i + 1);
            if (fEngine->PortRegister(fClientControl.fRefNum, name, JACK_DEFAULT_AUDIO_TYPE, MonitorDriverFlags, fEngineControl->fBufferSize, &port_index) < 0) {
                jack_error("Cannot register monitor port for %s", name);
                return -1;
            } else {
                fMonitorPortList[i] = port_index;
            }
        }
    }
    
    if (fAC3Encoder) {
        // Setup specific AC3 channels names
        for (int i = 0; i < fPlaybackChannels; i++) {
            fAC3Encoder->GetChannelName("coreaudio", "", alias, i);
            port = fGraphManager->GetPort(fPlaybackPortList[i]);
            port->SetAlias(alias);
        }
    }

    UpdateLatencies();

    // Input buffers do no change : prepare them only once
    for (int i = 0; i < fCaptureChannels; i++) {
        fJackInputData->mBuffers[i].mData = GetInputBuffer(i);
    }

    return 0;
}

int JackCoreAudioDriver::Start()
{
    jack_log("JackCoreAudioDriver::Start");
    if (JackAudioDriver::Start() == 0) {

        // Waiting for Render callback to be called (= driver has started)
        fState = false;
        int count = 0;

        OSStatus err = AudioOutputUnitStart(fAUHAL);
        if (err == noErr) {

            while (!fState && count++ < WAIT_COUNTER) {
                usleep(100000);
                jack_log("JackCoreAudioDriver::Start : wait count = %d", count);
            }

            if (count < WAIT_COUNTER) {
                jack_info("CoreAudio driver is running...");
                return 0;
            }

            jack_error("CoreAudio driver cannot start...");
        }
        JackAudioDriver::Stop();
    }
    return -1;
}

int JackCoreAudioDriver::Stop()
{
    jack_log("JackCoreAudioDriver::Stop");
    int res = (AudioOutputUnitStop(fAUHAL) == noErr) ? 0 : -1;
    if (JackAudioDriver::Stop() < 0) {
        res = -1;
    }
    return res;
}

int JackCoreAudioDriver::SetBufferSize(jack_nframes_t buffer_size)
{
    if (SetupBufferSize(buffer_size) < 0) {
        return -1;
    }

    JackAudioDriver::SetBufferSize(buffer_size); // Generic change, never fails

    // CoreAudio specific
    UpdateLatencies();

    // Input buffers do no change : prepare them only once
    for (int i = 0; i < fCaptureChannels; i++) {
        fJackInputData->mBuffers[i].mNumberChannels = 1;
        fJackInputData->mBuffers[i].mDataByteSize = fEngineControl->fBufferSize * sizeof(jack_default_audio_sample_t);
        fJackInputData->mBuffers[i].mData = GetInputBuffer(i);
    }

    return 0;
}

bool JackCoreAudioDriver::TakeHogAux(AudioDeviceID deviceID, bool isInput)
{
    pid_t hog_pid;
    OSStatus err;

    UInt32 propSize = sizeof(hog_pid);
    err = AudioDeviceGetProperty(deviceID, 0, isInput, kAudioDevicePropertyHogMode, &propSize, &hog_pid);
    if (err) {
        jack_error("Cannot read hog state...");
        printError(err);
    }
    
    jack_log("JackCoreAudioDriver::TakeHogAux : deviceID = %d", deviceID);

    if (hog_pid != getpid()) {
        hog_pid = getpid();
        err = AudioDeviceSetProperty(deviceID, 0, 0, isInput, kAudioDevicePropertyHogMode, propSize, &hog_pid);
        if (err != noErr) {
            jack_error("Can't hog device = %d because it's being hogged by another program or cannot be hogged", deviceID);
            return false;
        }
    }

    return true;
}

bool JackCoreAudioDriver::TakeHog()
{
    OSStatus err = noErr;
    AudioObjectID sub_device[32];
    UInt32 outSize = sizeof(sub_device);
    err = AudioDeviceGetProperty(fDeviceID, 0, kAudioDeviceSectionGlobal, kAudioAggregateDevicePropertyActiveSubDeviceList, &outSize, sub_device);

    if (err != noErr) {
        jack_log("JackCoreAudioDriver::TakeHog : device does not have subdevices");
        return TakeHogAux(fDeviceID, true);
    } else {
        int num_devices = outSize / sizeof(AudioObjectID);
        jack_log("JackCoreAudioDriver::TakeHog : device does has %d subdevices", num_devices);
        for (int i = 0; i < num_devices; i++) {
            if (!TakeHogAux(sub_device[i], true)) {
                return false;
            }
        }
        return true;
    }
}

bool JackCoreAudioDriver::IsAggregateDevice(AudioDeviceID device)
{
    UInt32 deviceType, outSize = sizeof(UInt32);
    OSStatus err = AudioDeviceGetProperty(device, 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyTransportType, &outSize, &deviceType);

    if (err != noErr) {
        jack_log("JackCoreAudioDriver::IsAggregateDevice kAudioDevicePropertyTransportType error");
        return false;
    } else {
        return (deviceType == kAudioDeviceTransportTypeAggregate);
    }
}


} // end of namespace


#ifdef __cplusplus
extern "C"
{
#endif

    SERVER_EXPORT jack_driver_desc_t* driver_get_descriptor()
    {
        jack_driver_desc_t * desc;
        jack_driver_desc_filler_t filler;
        jack_driver_param_value_t value;

        desc = jack_driver_descriptor_construct("coreaudio", JackDriverMaster, "Apple CoreAudio API based audio backend", &filler);

        value.i = -1;
        jack_driver_descriptor_add_parameter(desc, &filler, "channels", 'c', JackDriverParamInt, &value, NULL, "Maximum number of channels", "Maximum number of channels. If -1, max possible number of channels will be used");
        jack_driver_descriptor_add_parameter(desc, &filler, "in-channels", 'i', JackDriverParamInt, &value, NULL, "Maximum number of input channels", "Maximum number of input channels. If -1, max possible number of input channels will be used");
        jack_driver_descriptor_add_parameter(desc, &filler, "out-channels", 'o', JackDriverParamInt, &value, NULL, "Maximum number of output channels", "Maximum number of output channels. If -1, max possible number of output channels will be used");

        value.str[0] = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "input-list", 'n', JackDriverParamString, &value, NULL, "Input channel list for channel mapping", "List of input channel number to be opened (syntax like : \"0 3 2\")");
        jack_driver_descriptor_add_parameter(desc, &filler, "output-list", 'N', JackDriverParamString, &value, NULL, "Output channel list for channel mapping", "List of output channel number to be opened (syntax like : \"0 3 2\")");

        value.str[0] = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "capture", 'C', JackDriverParamString, &value, NULL, "Input CoreAudio device name", NULL);
        jack_driver_descriptor_add_parameter(desc, &filler, "playback", 'P', JackDriverParamString, &value, NULL, "Output CoreAudio device name", NULL);

        value.i  = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "monitor", 'm', JackDriverParamBool, &value, NULL, "Provide monitor ports for the output", NULL);
        
#ifndef __ppc__
        value.i  = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "AC3-encoding", 'a', JackDriverParamBool, &value, NULL, "AC3 multi-channels encoding", NULL);
        
        value.i  = 448;
        jack_driver_descriptor_add_parameter(desc, &filler, "AC3-bitrate", 'b', JackDriverParamUInt, &value, NULL, "AC3 bitrate", NULL);
       
        value.i  = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "AC3-LFE", 'f', JackDriverParamBool, &value, NULL, "AC3 LFE channel", NULL);
#endif
        value.i  = TRUE;
        jack_driver_descriptor_add_parameter(desc, &filler, "duplex", 'D', JackDriverParamBool, &value, NULL, "Provide both capture and playback ports", NULL);

        value.ui  = 44100U;
        jack_driver_descriptor_add_parameter(desc, &filler, "rate", 'r', JackDriverParamUInt, &value, NULL, "Sample rate", NULL);

        value.ui  = 256U;
        jack_driver_descriptor_add_parameter(desc, &filler, "period", 'p', JackDriverParamUInt, &value, NULL, "Frames per period", NULL);

        value.str[0] = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "device", 'd', JackDriverParamString, &value, NULL, "CoreAudio device name", NULL);

        value.ui  = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "input-latency", 'I', JackDriverParamUInt, &value, NULL, "Extra input latency (frames)", NULL);
        jack_driver_descriptor_add_parameter(desc, &filler, "output-latency", 'O', JackDriverParamUInt, &value, NULL, "Extra output latency (frames)", NULL);

        value.i  = FALSE;
        jack_driver_descriptor_add_parameter(desc, &filler, "list-devices", 'l', JackDriverParamBool, &value, NULL, "Display available CoreAudio devices", NULL);

        value.i  = FALSE;
        jack_driver_descriptor_add_parameter(desc, &filler, "hog", 'H', JackDriverParamBool, &value, NULL, "Take exclusive access of the audio device", NULL);

        value.ui  = 100;
        jack_driver_descriptor_add_parameter(desc, &filler, "async-latency", 'L', JackDriverParamUInt, &value, NULL, "Extra output latency in asynchronous mode (percent)", NULL);

        value.ui  = 100;
        jack_driver_descriptor_add_parameter(desc, &filler, "grain", 'G', JackDriverParamUInt, &value, NULL, "Computation grain in RT thread (percent)", NULL);

        value.i = FALSE;
        jack_driver_descriptor_add_parameter(desc, &filler, "clock-drift", 's', JackDriverParamBool, &value, NULL, "Clock drift compensation", "Whether to compensate clock drift in dynamically created aggregate device");

        return desc;
    }

    SERVER_EXPORT Jack::JackDriverClientInterface* driver_initialize(Jack::JackLockedEngine* engine, Jack::JackSynchro* table, const JSList* params)
    {
        jack_nframes_t srate = 44100;
        jack_nframes_t frames_per_interrupt = 256;
        bool capture = false;
        bool playback = false;
        int chan_in = -1;   // Default: if not explicitely set, then max possible will be used...
        int chan_out = -1;  // Default: if not explicitely set, then max possible will be used...
        const char* chan_in_list = "";
        const char* chan_out_list = "";
        bool monitor = false;
        const char* capture_driver_uid = "";
        const char* playback_driver_uid = "";
        const JSList *node;
        const jack_driver_param_t *param;
        jack_nframes_t systemic_input_latency = 0;
        jack_nframes_t systemic_output_latency = 0;
        int async_output_latency = 100;
        int computation_grain = -1;
        bool hogged = false;
        bool clock_drift = false;
        bool ac3_encoding = false;
        int ac3_bitrate = 448;
        bool ac3_lfe = false;

        for (node = params; node; node = jack_slist_next(node)) {
            param = (const jack_driver_param_t *) node->data;

            switch (param->character) {

                case 'd':
                    capture_driver_uid = param->value.str;
                    playback_driver_uid = param->value.str;
                    break;

                case 'D':
                    capture = true;
                    playback = true;
                    break;

                case 'c':
                    chan_in = chan_out = param->value.i;
                    break;

                case 'i':
                    chan_in = param->value.i;
                    break;

                case 'o':
                    chan_out = param->value.i;
                    break;

                case 'n':
                    chan_in_list = param->value.str;
                    break;

                case 'N':
                    chan_out_list = param->value.str;
                    break;

                case 'C':
                    capture = true;
                    if (strcmp(param->value.str, "none") != 0) {
                        capture_driver_uid = param->value.str;
                    }
                    break;

                case 'P':
                    playback = true;
                    if (strcmp(param->value.str, "none") != 0) {
                        playback_driver_uid = param->value.str;
                    }
                    break;

                case 'm':
                    monitor = param->value.i;
                    break;
                    
            #ifndef __ppc__            
                case 'a':
                    ac3_encoding = param->value.i;
                    break;
                     
                case 'b':
                    ac3_bitrate = param->value.i;
                    break;
                    
                case 'f':
                    ac3_lfe = param->value.i;
                    break;
            #endif

                case 'r':
                    srate = param->value.ui;
                    break;

                case 'p':
                    frames_per_interrupt = (unsigned int)param->value.ui;
                    break;

                case 'I':
                    systemic_input_latency = param->value.ui;
                    break;

                case 'O':
                    systemic_output_latency = param->value.ui;
                    break;

                case 'l':
                    Jack::DisplayDeviceNames();
                    // Stops the server in this case
                    return NULL;

                case 'H':
                    hogged = true;
                    break;

                case 'L':
                    async_output_latency = param->value.ui;
                    break;

                case 'G':
                    computation_grain = param->value.ui;
                    break;

                case 's':
                    clock_drift = true;
                    break;
            }
        }

        /* duplex is the default */
        if (!capture && !playback) {
            capture = true;
            playback = true;
        }

        if (strcmp(chan_in_list, "") != 0 && chan_in >= 0) {
            printf("Input channel list and in channels are both specified, input channel list will take over...\n");
        }

        if (strcmp(chan_out_list, "") != 0 && chan_out >= 0) {
            printf("Output channel list and out channels are both specified, output channel list will take over...\n");
        }

        Jack::JackCoreAudioDriver* driver = new Jack::JackCoreAudioDriver("system", "coreaudio", engine, table);
        if (driver->Open(frames_per_interrupt,
                        srate, capture,
                        playback, chan_in,
                        chan_out, chan_in_list,
                        chan_out_list, monitor,
                        capture_driver_uid,
                        playback_driver_uid,
                        systemic_input_latency,
                        systemic_output_latency,
                        async_output_latency,
                        computation_grain,
                        hogged, clock_drift, 
                        ac3_encoding, ac3_bitrate, ac3_lfe) == 0) {
            return driver;
        } else {
            delete driver;
            return NULL;
        }
    }

#ifdef __cplusplus
}
#endif


