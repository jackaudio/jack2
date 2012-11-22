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

#include "JackCoreAudioAdapter.h"
#include "JackError.h"
#include <unistd.h>

#include <CoreServices/CoreServices.h>

namespace Jack
{

static void PrintStreamDesc(AudioStreamBasicDescription *inDesc)
{
    jack_log("- - - - - - - - - - - - - - - - - - - -");
    jack_log("  Sample Rate:%f", inDesc->mSampleRate);
    jack_log("  Format ID:%.*s", (int) sizeof(inDesc->mFormatID), (char*)&inDesc->mFormatID);
    jack_log("  Format Flags:%lX", inDesc->mFormatFlags);
    jack_log("  Bytes per Packet:%ld", inDesc->mBytesPerPacket);
    jack_log("  Frames per Packet:%ld", inDesc->mFramesPerPacket);
    jack_log("  Bytes per Frame:%ld", inDesc->mBytesPerFrame);
    jack_log("  Channels per Frame:%ld", inDesc->mChannelsPerFrame);
    jack_log("  Bits per Channel:%ld", inDesc->mBitsPerChannel);
    jack_log("- - - - - - - - - - - - - - - - - - - -");
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

        jack_info("Device name = \'%s\', internal_name = \'%s\' (to be used as -C, -P, or -d parameter)", device_name, internal_name);
    }

    return noErr;

error:
    if (UIname != NULL) {
        CFRelease(UIname);
    }
    return err;
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
            jack_log("error code : unknown");
            break;
    }
}

OSStatus JackCoreAudioAdapter::AudioHardwareNotificationCallback(AudioHardwarePropertyID inPropertyID, void* inClientData)
{
    JackCoreAudioAdapter* driver = (JackCoreAudioAdapter*)inClientData;

    switch (inPropertyID) {

            case kAudioHardwarePropertyDevices: {
                jack_log("JackCoreAudioAdapter::AudioHardwareNotificationCallback kAudioHardwarePropertyDevices");
                DisplayDeviceNames();
                break;
            }
    }

    return noErr;
}

OSStatus JackCoreAudioAdapter::SRNotificationCallback(AudioDeviceID inDevice,
                                                        UInt32 inChannel,
                                                        Boolean	isInput,
                                                        AudioDevicePropertyID inPropertyID,
                                                        void* inClientData)
{
    JackCoreAudioAdapter* driver = static_cast<JackCoreAudioAdapter*>(inClientData);

    switch (inPropertyID) {

        case kAudioDevicePropertyNominalSampleRate: {
            jack_log("JackCoreAudioAdapter::SRNotificationCallback kAudioDevicePropertyNominalSampleRate");
            driver->fState = true;
            break;
        }
    }

    return noErr;
}

// A better implementation would try to recover in case of hardware device change (see HALLAB HLFilePlayerWindowControllerAudioDevicePropertyListenerProc code)
OSStatus JackCoreAudioAdapter::DeviceNotificationCallback(AudioDeviceID inDevice,
        UInt32 inChannel,
        Boolean	isInput,
        AudioDevicePropertyID inPropertyID,
        void* inClientData)
{

    switch (inPropertyID) {

        case kAudioDeviceProcessorOverload: {
            jack_error("JackCoreAudioAdapter::DeviceNotificationCallback kAudioDeviceProcessorOverload");
            break;
		}

        case kAudioDevicePropertyStreamConfiguration: {
            jack_error("Cannot handle kAudioDevicePropertyStreamConfiguration");
            return kAudioHardwareUnsupportedOperationError;
        }

        case kAudioDevicePropertyNominalSampleRate: {
            jack_error("Cannot handle kAudioDevicePropertyNominalSampleRate");
            return kAudioHardwareUnsupportedOperationError;
        }

    }
    return noErr;
}

int JackCoreAudioAdapter::AddListeners()
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

    return 0;
}

void JackCoreAudioAdapter::RemoveListeners()
{
    AudioDeviceRemovePropertyListener(fDeviceID, 0, true, kAudioDeviceProcessorOverload, DeviceNotificationCallback);
    AudioHardwareRemovePropertyListener(kAudioHardwarePropertyDevices, AudioHardwareNotificationCallback);
    AudioDeviceRemovePropertyListener(fDeviceID, 0, true, kAudioDevicePropertyNominalSampleRate, DeviceNotificationCallback);
    AudioDeviceRemovePropertyListener(fDeviceID, 0, true, kAudioDevicePropertyDeviceIsRunning, DeviceNotificationCallback);
    AudioDeviceRemovePropertyListener(fDeviceID, 0, true, kAudioDevicePropertyStreamConfiguration, DeviceNotificationCallback);
    AudioDeviceRemovePropertyListener(fDeviceID, 0, false, kAudioDevicePropertyStreamConfiguration, DeviceNotificationCallback);
}

OSStatus JackCoreAudioAdapter::Render(void *inRefCon,
                                    AudioUnitRenderActionFlags *ioActionFlags,
                                    const AudioTimeStamp *inTimeStamp,
                                    UInt32 inBusNumber,
                                    UInt32 inNumberFrames,
                                    AudioBufferList *ioData)
{
    JackCoreAudioAdapter* adapter = static_cast<JackCoreAudioAdapter*>(inRefCon);
    OSStatus err = AudioUnitRender(adapter->fAUHAL, ioActionFlags, inTimeStamp, 1, inNumberFrames, adapter->fInputData);

    if (err == noErr) {
        jack_default_audio_sample_t* inputBuffer[adapter->fCaptureChannels];
        jack_default_audio_sample_t* outputBuffer[adapter->fPlaybackChannels];

        for (int i = 0; i < adapter->fCaptureChannels; i++) {
            inputBuffer[i] = (jack_default_audio_sample_t*)adapter->fInputData->mBuffers[i].mData;
        }
        for (int i = 0; i < adapter->fPlaybackChannels; i++) {
            outputBuffer[i] = (jack_default_audio_sample_t*)ioData->mBuffers[i].mData;
        }

        adapter->PushAndPull((jack_default_audio_sample_t**)inputBuffer, (jack_default_audio_sample_t**)outputBuffer, inNumberFrames);
        return noErr;
    } else {
        return err;
    }
}

JackCoreAudioAdapter::JackCoreAudioAdapter(jack_nframes_t buffer_size, jack_nframes_t sample_rate, const JSList* params)
                :JackAudioAdapterInterface(buffer_size, sample_rate), fInputData(0), fCapturing(false), fPlaying(false), fState(false)
{
    const JSList* node;
    const jack_driver_param_t* param;
    int in_nChannels = 0;
    int out_nChannels = 0;
    char captureName[256];
    char playbackName[256];
    fCaptureUID[0] = 0;
    fPlaybackUID[0] = 0;
    fClockDriftCompensate = false;

    // Default values
    fCaptureChannels = -1;
    fPlaybackChannels = -1;

    SInt32 major;
    SInt32 minor;
    Gestalt(gestaltSystemVersionMajor, &major);
    Gestalt(gestaltSystemVersionMinor, &minor);

    // Starting with 10.6 systems, the HAL notification thread is created internally
    if (major == 10 && minor >= 6) {
        CFRunLoopRef theRunLoop = NULL;
        AudioObjectPropertyAddress theAddress = { kAudioHardwarePropertyRunLoop, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster };
        OSStatus theError = AudioObjectSetPropertyData (kAudioObjectSystemObject, &theAddress, 0, NULL, sizeof(CFRunLoopRef), &theRunLoop);
        if (theError != noErr) {
            jack_error("JackCoreAudioAdapter::Open kAudioHardwarePropertyRunLoop error");
        }
    }

    for (node = params; node; node = jack_slist_next(node)) {
        param = (const jack_driver_param_t*) node->data;

        switch (param->character) {

            case 'c' :
                fCaptureChannels = fPlaybackChannels = param->value.ui;
                break;

            case 'i':
                fCaptureChannels = param->value.ui;
                break;

            case 'o':
                fPlaybackChannels = param->value.ui;
                break;

            case 'C':
                fCapturing = true;
                strncpy(fCaptureUID, param->value.str, 256);
                break;

            case 'P':
                fPlaying = true;
                strncpy(fPlaybackUID, param->value.str, 256);
                break;

            case 'd':
                strncpy(fCaptureUID, param->value.str, 256);
                strncpy(fPlaybackUID, param->value.str, 256);
                break;

            case 'D':
                fCapturing = fPlaying = true;
                break;

            case 'r':
                SetAdaptedSampleRate(param->value.ui);
                break;

            case 'p':
                SetAdaptedBufferSize(param->value.ui);
                break;

            case 'l':
                DisplayDeviceNames();
                break;

            case 'q':
                fQuality = param->value.ui;
                break;

            case 'g':
                fRingbufferCurSize = param->value.ui;
                fAdaptative = false;
                break;

            case 's':
                fClockDriftCompensate = true;
                break;
        }
    }

    /* duplex is the default */
    if (!fCapturing && !fPlaying) {
        fCapturing = true;
        fPlaying = true;
    }

    if (SetupDevices(fCaptureUID, fPlaybackUID, captureName, playbackName, fAdaptedSampleRate) < 0) {
       throw std::bad_alloc();
    }

    if (SetupChannels(fCapturing, fPlaying, fCaptureChannels, fPlaybackChannels, in_nChannels, out_nChannels, true) < 0) {
        throw std::bad_alloc();
    }

    if (SetupBufferSize(fAdaptedBufferSize) < 0) {
        throw std::bad_alloc();
    }

    if (SetupSampleRate(fAdaptedSampleRate) < 0) {
        throw std::bad_alloc();
    }

    if (OpenAUHAL(fCapturing, fPlaying, fCaptureChannels, fPlaybackChannels, in_nChannels, out_nChannels, fAdaptedBufferSize, fAdaptedSampleRate) < 0) {
        throw std::bad_alloc();
    }

    if (fCapturing && fCaptureChannels > 0) {
        if (SetupBuffers(fCaptureChannels) < 0) {
            throw std::bad_alloc();
        }
    }

    if (AddListeners() < 0) {
        throw std::bad_alloc();
    }

    GetStreamLatencies(fDeviceID, true, fInputLatencies);
    GetStreamLatencies(fDeviceID, false, fOutputLatencies);
}

OSStatus JackCoreAudioAdapter::GetDefaultDevice(AudioDeviceID* id)
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

    jack_log("GetDefaultDevice: input = %ld output = %ld", inDefault, outDefault);

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

OSStatus JackCoreAudioAdapter::GetTotalChannels(AudioDeviceID device, int& channelCount, bool isInput)
{
    OSStatus err = noErr;
    UInt32	outSize;
    Boolean	outWritable;

    channelCount = 0;
    err = AudioDeviceGetPropertyInfo(device, 0, isInput, kAudioDevicePropertyStreamConfiguration, &outSize, &outWritable);
    if (err == noErr) {
        AudioBufferList bufferList[outSize];
        err = AudioDeviceGetProperty(device, 0, isInput, kAudioDevicePropertyStreamConfiguration, &outSize, bufferList);
        if (err == noErr) {
            for (unsigned int i = 0; i < bufferList->mNumberBuffers; i++) {
                channelCount += bufferList->mBuffers[i].mNumberChannels;
            }
        }
    }

    return err;
}

OSStatus JackCoreAudioAdapter::GetDeviceIDFromUID(const char* UID, AudioDeviceID* id)
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

OSStatus JackCoreAudioAdapter::GetDefaultInputDevice(AudioDeviceID* id)
{
    OSStatus res;
    UInt32 theSize = sizeof(UInt32);
    AudioDeviceID inDefault;

    if ((res = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultInputDevice, &theSize, &inDefault)) != noErr) {
        return res;
    }

    if (inDefault == 0) {
        jack_error("Error: default input device is 0, please select a correct one !!");
        return -1;
    }
    jack_log("GetDefaultInputDevice: input = %ld ", inDefault);
    *id = inDefault;
    return noErr;
}

OSStatus JackCoreAudioAdapter::GetDefaultOutputDevice(AudioDeviceID* id)
{
    OSStatus res;
    UInt32 theSize = sizeof(UInt32);
    AudioDeviceID outDefault;

    if ((res = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &theSize, &outDefault)) != noErr) {
        return res;
    }

    if (outDefault == 0) {
        jack_error("Error: default output device is 0, please select a correct one !!");
        return -1;
    }
    jack_log("GetDefaultOutputDevice: output = %ld", outDefault);
    *id = outDefault;
    return noErr;
}

OSStatus JackCoreAudioAdapter::GetDeviceNameFromID(AudioDeviceID id, char* name)
{
    UInt32 size = 256;
    return AudioDeviceGetProperty(id, 0, false, kAudioDevicePropertyDeviceName, &size, name);
}

AudioDeviceID JackCoreAudioAdapter::GetDeviceIDFromName(const char* name)
{
    UInt32 size;
    Boolean isWritable;
    int i, deviceNum;

    OSStatus err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &size, &isWritable);
    if (err != noErr) {
        return -1;
    }

    deviceNum = size / sizeof(AudioDeviceID);
    AudioDeviceID devices[deviceNum];

    err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &size, devices);
    if (err != noErr) {
        return err;
    }

    for (i = 0; i < deviceNum; i++) {
        char device_name[256];
        size = 256;
        err = AudioDeviceGetProperty(devices[i], 0, false, kAudioDevicePropertyDeviceName, &size, device_name);
        if (err != noErr) {
            return -1;
        } else if (strcmp(device_name, name) == 0) {
            return devices[i];
        }
    }

    return -1;
}

// Setup
int JackCoreAudioAdapter::SetupDevices(const char* capture_driver_uid,
                                        const char* playback_driver_uid,
                                        char* capture_driver_name,
                                        char* playback_driver_name,
                                        jack_nframes_t samplerate)
{
    capture_driver_name[0] = 0;
    playback_driver_name[0] = 0;

    // Duplex
    if (strcmp(capture_driver_uid, "") != 0 && strcmp(playback_driver_uid, "") != 0) {
        jack_log("JackCoreAudioDriver::Open duplex");

        // Same device for capture and playback...
        if (strcmp(capture_driver_uid, playback_driver_uid) == 0)  {

            if (GetDeviceIDFromUID(playback_driver_uid, &fDeviceID) != noErr) {
                jack_log("Will take default in/out");
                if (GetDefaultDevice(&fDeviceID) != noErr) {
                    jack_error("Cannot open default device");
                    return -1;
                }
            }
            if (GetDeviceNameFromID(fDeviceID, capture_driver_name) != noErr || GetDeviceNameFromID(fDeviceID, playback_driver_name) != noErr) {
                jack_error("Cannot get device name from device ID");
                return -1;
            }

        } else {

            // Creates aggregate device
            AudioDeviceID captureID, playbackID;

            if (GetDeviceIDFromUID(capture_driver_uid, &captureID) != noErr) {
                jack_log("Will take default input");
                if (GetDefaultInputDevice(&captureID) != noErr) {
                    jack_error("Cannot open default input device");
                    return -1;
                }
            }

            if (GetDeviceIDFromUID(playback_driver_uid, &playbackID) != noErr) {
                jack_log("Will take default output");
                if (GetDefaultOutputDevice(&playbackID) != noErr) {
                    jack_error("Cannot open default output device");
                    return -1;
                }
            }

            if (CreateAggregateDevice(captureID, playbackID, samplerate, &fDeviceID) != noErr) {
                return -1;
            }
        }

    // Capture only
    } else if (strcmp(capture_driver_uid, "") != 0) {
        jack_log("JackCoreAudioAdapter::Open capture only");
        if (GetDeviceIDFromUID(capture_driver_uid, &fDeviceID) != noErr) {
            if (GetDefaultInputDevice(&fDeviceID) != noErr) {
                jack_error("Cannot open default input device");
                return -1;
            }
        }
        if (GetDeviceNameFromID(fDeviceID, capture_driver_name) != noErr) {
            jack_error("Cannot get device name from device ID");
            return -1;
        }

    // Playback only
    } else if (strcmp(playback_driver_uid, "") != 0) {
        jack_log("JackCoreAudioAdapter::Open playback only");
        if (GetDeviceIDFromUID(playback_driver_uid, &fDeviceID) != noErr) {
            if (GetDefaultOutputDevice(&fDeviceID) != noErr) {
                jack_error("Cannot open default output device");
                return -1;
            }
        }
        if (GetDeviceNameFromID(fDeviceID, playback_driver_name) != noErr) {
            jack_error("Cannot get device name from device ID");
            return -1;
        }

    // Use default driver in duplex mode
    } else {
        jack_log("JackCoreAudioAdapter::Open default driver");
        if (GetDefaultDevice(&fDeviceID) != noErr) {
            jack_error("Cannot open default device in duplex mode, so aggregate default input and default output");

            // Creates aggregate device
            AudioDeviceID captureID = -1, playbackID = -1;

            if (GetDeviceIDFromUID(capture_driver_uid, &captureID) != noErr) {
                jack_log("Will take default input");
                if (GetDefaultInputDevice(&captureID) != noErr) {
                    jack_error("Cannot open default input device");
                    goto built_in;
                }
            }

            if (GetDeviceIDFromUID(playback_driver_uid, &playbackID) != noErr) {
                jack_log("Will take default output");
                if (GetDefaultOutputDevice(&playbackID) != noErr) {
                    jack_error("Cannot open default output device");
                    goto built_in;
                }
            }

            if (captureID > 0 && playbackID > 0) {
                if (CreateAggregateDevice(captureID, playbackID, samplerate, &fDeviceID) != noErr) {
                    goto built_in;
                }
            } else {
                jack_error("Cannot use default input/output");
                goto built_in;
            }
        }
    }

    return 0;

built_in:

    // Aggregate built-in input and output
    AudioDeviceID captureID = GetDeviceIDFromName("Built-in Input");
    AudioDeviceID playbackID = GetDeviceIDFromName("Built-in Output");

    if (captureID > 0 && playbackID > 0) {
        if (CreateAggregateDevice(captureID, playbackID, samplerate, &fDeviceID) != noErr) {
            return -1;
        }
    } else {
        jack_error("Cannot aggregate built-in input and output");
        return -1;
    }

    return 0;
}

int JackCoreAudioAdapter::SetupChannels(bool capturing,
                                        bool playing,
                                        int& inchannels,
                                        int& outchannels,
                                        int& in_nChannels,
                                        int& out_nChannels,
                                        bool strict)
{
    OSStatus err = noErr;

    if (capturing) {
        err = GetTotalChannels(fDeviceID, in_nChannels, true);
        if (err != noErr) {
            jack_error("Cannot get input channel number");
            printError(err);
            return -1;
        } else {
            jack_log("Max input channels : %d", in_nChannels);
        }
    }

    if (playing) {
        err = GetTotalChannels(fDeviceID, out_nChannels, false);
        if (err != noErr) {
            jack_error("Cannot get output channel number");
            printError(err);
            return -1;
        } else {
            jack_log("Max output channels : %d", out_nChannels);
        }
    }

    if (inchannels > in_nChannels) {
        jack_error("This device hasn't required input channels inchannels = %ld in_nChannels = %ld", inchannels, in_nChannels);
        if (strict) {
            return -1;
        }
    }

    if (outchannels > out_nChannels) {
        jack_error("This device hasn't required output channels outchannels = %ld out_nChannels = %ld", outchannels, out_nChannels);
        if (strict) {
            return -1;
        }
    }

    if (inchannels == -1) {
        jack_log("Setup max in channels = %ld", in_nChannels);
        inchannels = in_nChannels;
    }

    if (outchannels == -1) {
        jack_log("Setup max out channels = %ld", out_nChannels);
        outchannels = out_nChannels;
    }

    return 0;
}

int JackCoreAudioAdapter::SetupBufferSize(jack_nframes_t buffer_size)
{
    // Setting buffer size
    UInt32 outSize = sizeof(UInt32);
    OSStatus err = AudioDeviceSetProperty(fDeviceID, NULL, 0, false, kAudioDevicePropertyBufferFrameSize, outSize, &buffer_size);
    if (err != noErr) {
        jack_error("Cannot set buffer size %ld", buffer_size);
        printError(err);
        return -1;
    }

    return 0;
}

int JackCoreAudioAdapter::SetupSampleRate(jack_nframes_t samplerate)
{
    return SetupSampleRateAux(fDeviceID, samplerate);
}

int JackCoreAudioAdapter::SetupSampleRateAux(AudioDeviceID inDevice, jack_nframes_t samplerate)
{
    OSStatus err = noErr;
    UInt32 outSize;
    Float64 sampleRate;

    // Get sample rate
    outSize =  sizeof(Float64);
    err = AudioDeviceGetProperty(inDevice, 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyNominalSampleRate, &outSize, &sampleRate);
    if (err != noErr) {
        jack_error("Cannot get current sample rate");
        printError(err);
        return -1;
    } else {
        jack_log("Current sample rate = %f", sampleRate);
    }

    // If needed, set new sample rate
    if (samplerate != (jack_nframes_t)sampleRate) {
        sampleRate = (Float64)samplerate;

        // To get SR change notification
        err = AudioDeviceAddPropertyListener(inDevice, 0, true, kAudioDevicePropertyNominalSampleRate, SRNotificationCallback, this);
        if (err != noErr) {
            jack_error("Error calling AudioDeviceAddPropertyListener with kAudioDevicePropertyNominalSampleRate");
            printError(err);
            return -1;
        }
        err = AudioDeviceSetProperty(inDevice, NULL, 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyNominalSampleRate, outSize, &sampleRate);
        if (err != noErr) {
            jack_error("Cannot set sample rate = %ld", samplerate);
            printError(err);
            return -1;
        }

        // Waiting for SR change notification
        int count = 0;
        while (!fState && count++ < WAIT_COUNTER) {
            usleep(100000);
            jack_log("Wait count = %d", count);
        }

        // Remove SR change notification
        AudioDeviceRemovePropertyListener(inDevice, 0, true, kAudioDevicePropertyNominalSampleRate, SRNotificationCallback);
    }

    return 0;
}

int JackCoreAudioAdapter::SetupBuffers(int inchannels)
{
    jack_log("JackCoreAudioAdapter::SetupBuffers: input = %ld", inchannels);

    // Prepare buffers
    fInputData = (AudioBufferList*)malloc(sizeof(UInt32) + inchannels * sizeof(AudioBuffer));
    fInputData->mNumberBuffers = inchannels;
    for (int i = 0; i < fCaptureChannels; i++) {
        fInputData->mBuffers[i].mNumberChannels = 1;
        fInputData->mBuffers[i].mDataByteSize = fAdaptedBufferSize * sizeof(jack_default_audio_sample_t);
        fInputData->mBuffers[i].mData = malloc(fAdaptedBufferSize * sizeof(jack_default_audio_sample_t));
    }
    return 0;
}

void JackCoreAudioAdapter::DisposeBuffers()
{
    if (fInputData) {
        for (int i = 0; i < fCaptureChannels; i++) {
            free(fInputData->mBuffers[i].mData);
        }
        free(fInputData);
        fInputData = 0;
    }
}

int JackCoreAudioAdapter::OpenAUHAL(bool capturing,
                                   bool playing,
                                   int inchannels,
                                   int outchannels,
                                   int in_nChannels,
                                   int out_nChannels,
                                   jack_nframes_t buffer_size,
                                   jack_nframes_t samplerate)
{
    ComponentResult err1;
    UInt32 enableIO;
    AudioStreamBasicDescription srcFormat, dstFormat;
    AudioDeviceID currAudioDeviceID;
    UInt32 size;

    jack_log("OpenAUHAL capturing = %d playing = %d inchannels = %d outchannels = %d in_nChannels = %d out_nChannels = %d", capturing, playing, inchannels, outchannels, in_nChannels, out_nChannels);

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
        jack_log("Setup AUHAL input on");
    } else {
        enableIO = 0;
        jack_log("Setup AUHAL input off");
    }

    err1 = AudioUnitSetProperty(fAUHAL, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, 1, &enableIO, sizeof(enableIO));
    if (err1 != noErr) {
        jack_error("Error calling AudioUnitSetProperty - kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input");
        printError(err1);
        goto error;
    }

    if (playing && outchannels > 0) {
        enableIO = 1;
        jack_log("Setup AUHAL output on");
    } else {
        enableIO = 0;
        jack_log("Setup AUHAL output off");
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
        jack_log("AudioUnitGetPropertyCurrentDevice = %d", currAudioDeviceID);
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

    // Setup channel map
    if (capturing && inchannels > 0 && inchannels <= in_nChannels) {
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
            goto error;
        }
    }

    if (playing && outchannels > 0 && outchannels <= out_nChannels) {
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
            goto error;
        }
    }

    // Setup stream converters
    if (capturing && inchannels > 0) {

        size = sizeof(AudioStreamBasicDescription);
        err1 = AudioUnitGetProperty(fAUHAL, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &srcFormat, &size);
        if (err1 != noErr) {
            jack_error("Error calling AudioUnitGetProperty - kAudioUnitProperty_StreamFormat kAudioUnitScope_Input");
            printError(err1);
            goto error;
        }
        PrintStreamDesc(&srcFormat);

        jack_log("Setup AUHAL input stream converter SR = %ld", samplerate);
        srcFormat.mSampleRate = samplerate;
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
            jack_error("Error calling AudioUnitSetProperty - kAudioUnitProperty_StreamFormat kAudioUnitScope_Input");
            printError(err1);
            goto error;
        }
    }

    if (playing && outchannels > 0) {

        size = sizeof(AudioStreamBasicDescription);
        err1 = AudioUnitGetProperty(fAUHAL, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 1, &dstFormat, &size);
        if (err1 != noErr) {
            jack_error("Error calling AudioUnitGetProperty - kAudioUnitProperty_StreamFormat kAudioUnitScope_Output");
            printError(err1);
            goto error;
        }
        PrintStreamDesc(&dstFormat);

        jack_log("Setup AUHAL output stream converter SR = %ld", samplerate);
        dstFormat.mSampleRate = samplerate;
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
            jack_error("Error calling AudioUnitSetProperty - kAudioUnitProperty_StreamFormat kAudioUnitScope_Output");
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

OSStatus JackCoreAudioAdapter::DestroyAggregateDevice()
{
    OSStatus osErr = noErr;
    AudioObjectPropertyAddress pluginAOPA;
    pluginAOPA.mSelector = kAudioPlugInDestroyAggregateDevice;
    pluginAOPA.mScope = kAudioObjectPropertyScopeGlobal;
    pluginAOPA.mElement = kAudioObjectPropertyElementMaster;
    UInt32 outDataSize;

    osErr = AudioObjectGetPropertyDataSize(fPluginID, &pluginAOPA, 0, NULL, &outDataSize);
    if (osErr != noErr) {
        jack_error("JackCoreAudioAdapter::DestroyAggregateDevice : AudioObjectGetPropertyDataSize error");
        printError(osErr);
        return osErr;
    }

    osErr = AudioObjectGetPropertyData(fPluginID, &pluginAOPA, 0, NULL, &outDataSize, &fDeviceID);
    if (osErr != noErr) {
        jack_error("JackCoreAudioAdapter::DestroyAggregateDevice : AudioObjectGetPropertyData error");
        printError(osErr);
        return osErr;
    }

    return noErr;
}

static CFStringRef GetDeviceName(AudioDeviceID id)
{
    UInt32 size = sizeof(CFStringRef);
    CFStringRef UIname;
    OSStatus err = AudioDeviceGetProperty(id, 0, false, kAudioDevicePropertyDeviceUID, &size, &UIname);
    return (err == noErr) ? UIname : NULL;
}

OSStatus JackCoreAudioAdapter::CreateAggregateDevice(AudioDeviceID captureDeviceID, AudioDeviceID playbackDeviceID, jack_nframes_t samplerate, AudioDeviceID* outAggregateDevice)
{
    OSStatus err = noErr;
    AudioObjectID sub_device[32];
    UInt32 outSize = sizeof(sub_device);

    err = AudioDeviceGetProperty(captureDeviceID, 0, kAudioDeviceSectionGlobal, kAudioAggregateDevicePropertyActiveSubDeviceList, &outSize, sub_device);
    vector<AudioDeviceID> captureDeviceIDArray;

    if (err != noErr) {
        jack_log("Input device does not have subdevices");
        captureDeviceIDArray.push_back(captureDeviceID);
    } else {
        int num_devices = outSize / sizeof(AudioObjectID);
        jack_log("Input device has %d subdevices", num_devices);
        for (int i = 0; i < num_devices; i++) {
            captureDeviceIDArray.push_back(sub_device[i]);
        }
    }

    err = AudioDeviceGetProperty(playbackDeviceID, 0, kAudioDeviceSectionGlobal, kAudioAggregateDevicePropertyActiveSubDeviceList, &outSize, sub_device);
    vector<AudioDeviceID> playbackDeviceIDArray;

    if (err != noErr) {
        jack_log("Output device does not have subdevices");
        playbackDeviceIDArray.push_back(playbackDeviceID);
    } else {
        int num_devices = outSize / sizeof(AudioObjectID);
        jack_log("Output device has %d subdevices", num_devices);
        for (int i = 0; i < num_devices; i++) {
            playbackDeviceIDArray.push_back(sub_device[i]);
        }
    }

    return CreateAggregateDeviceAux(captureDeviceIDArray, playbackDeviceIDArray, samplerate, outAggregateDevice);
}

OSStatus JackCoreAudioAdapter::CreateAggregateDeviceAux(vector<AudioDeviceID> captureDeviceID, vector<AudioDeviceID> playbackDeviceID, jack_nframes_t samplerate, AudioDeviceID* outAggregateDevice)
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
            jack_error("JackCoreAudioAdapter::CreateAggregateDevice : cannot set SR of input device");
        } else  {
            // Check clock domain
            osErr = AudioDeviceGetProperty(captureDeviceID[i], 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyClockDomain, &outSize, &clockdomain);
            if (osErr != 0) {
                jack_error("JackCoreAudioAdapter::CreateAggregateDevice : kAudioDevicePropertyClockDomain error");
                printError(osErr);
            } else {
                keptclockdomain = (keptclockdomain == 0) ? clockdomain : keptclockdomain;
                jack_log("JackCoreAudioAdapter::CreateAggregateDevice : input clockdomain = %d", clockdomain);
                if (clockdomain != 0 && clockdomain != keptclockdomain) {
                    jack_error("JackCoreAudioAdapter::CreateAggregateDevice : devices do not share the same clock!! clock drift compensation would be needed...");
                    need_clock_drift_compensation = true;
                }
            }
        }
    }

    for (UInt32 i = 0; i < playbackDeviceID.size(); i++) {
        if (SetupSampleRateAux(playbackDeviceID[i], samplerate) < 0) {
            jack_error("JackCoreAudioAdapter::CreateAggregateDevice : cannot set SR of output device");
        } else {
            // Check clock domain
            osErr = AudioDeviceGetProperty(playbackDeviceID[i], 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyClockDomain, &outSize, &clockdomain);
            if (osErr != 0) {
                jack_error("JackCoreAudioAdapter::CreateAggregateDevice : kAudioDevicePropertyClockDomain error");
                printError(osErr);
            } else {
                keptclockdomain = (keptclockdomain == 0) ? clockdomain : keptclockdomain;
                jack_log("JackCoreAudioAdapter::CreateAggregateDevice : output clockdomain = %d", clockdomain);
                if (clockdomain != 0 && clockdomain != keptclockdomain) {
                    jack_error("JackCoreAudioAdapter::CreateAggregateDevice : devices do not share the same clock!! clock drift compensation would be needed...");
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
        jack_error("JackCoreAudioAdapter::CreateAggregateDevice : AudioHardwareGetPropertyInfo kAudioHardwarePropertyPlugInForBundleID error");
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
        jack_error("JackCoreAudioAdapter::CreateAggregateDevice : AudioHardwareGetProperty kAudioHardwarePropertyPlugInForBundleID error");
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

    jack_log("JackCoreAudioAdapter::CreateAggregateDevice : system version = %x limit = %x", system, 0x00001054);

    // Starting with 10.5.4 systems, the AD can be internal... (better)
    if (system < 0x00001054) {
        jack_log("JackCoreAudioAdapter::CreateAggregateDevice : public aggregate device....");
    } else {
        jack_log("JackCoreAudioAdapter::CreateAggregateDevice : private aggregate device....");
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
        jack_error("JackCoreAudioAdapter::CreateAggregateDevice : AudioObjectGetPropertyDataSize error");
        printError(osErr);
        goto error;
    }

    osErr = AudioObjectGetPropertyData(fPluginID, &pluginAOPA, sizeof(aggDeviceDict), &aggDeviceDict, &outDataSize, outAggregateDevice);
    if (osErr != noErr) {
        jack_error("JackCoreAudioAdapter::CreateAggregateDevice : AudioObjectGetPropertyData error");
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
        jack_error("JackCoreAudioAdapter::CreateAggregateDevice : AudioObjectSetPropertyData for sub-device list error");
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
        jack_error("JackCoreAudioAdapter::CreateAggregateDevice : AudioObjectSetPropertyData for master device error");
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
                jack_error("JackCoreAudioAdapter::CreateAggregateDevice kAudioObjectPropertyOwnedObjects error");
                printError(osErr);
            }

            //	Calculate the number of object IDs
            subDevicesNum = outSize / sizeof(AudioObjectID);
            jack_info("JackCoreAudioAdapter::CreateAggregateDevice clock drift compensation, number of sub-devices = %d", subDevicesNum);
            AudioObjectID subDevices[subDevicesNum];
            outSize = sizeof(subDevices);

            osErr = AudioObjectGetPropertyData(*outAggregateDevice, &theAddressOwned, theQualifierDataSize, theQualifierData, &outSize, subDevices);
            if (osErr != noErr) {
                jack_error("JackCoreAudioAdapter::CreateAggregateDevice kAudioObjectPropertyOwnedObjects error");
                printError(osErr);
            }

            // Set kAudioSubDevicePropertyDriftCompensation property...
            for (UInt32 index = 0; index < subDevicesNum; ++index) {
                UInt32 theDriftCompensationValue = 1;
                osErr = AudioObjectSetPropertyData(subDevices[index], &theAddressDrift, 0, NULL, sizeof(UInt32), &theDriftCompensationValue);
                if (osErr != noErr) {
                    jack_error("JackCoreAudioAdapter::CreateAggregateDevice kAudioSubDevicePropertyDriftCompensation error");
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

    jack_log("New aggregate device %ld", *outAggregateDevice);
    return noErr;

error:
    DestroyAggregateDevice();
    return -1;
}


bool JackCoreAudioAdapter::IsAggregateDevice(AudioDeviceID device)
{
    OSStatus err = noErr;
    AudioObjectID sub_device[32];
    UInt32 outSize = sizeof(sub_device);
    err = AudioDeviceGetProperty(device, 0, kAudioDeviceSectionGlobal, kAudioAggregateDevicePropertyActiveSubDeviceList, &outSize, sub_device);

    if (err != noErr) {
        jack_log("Device does not have subdevices");
        return false;
    } else {
        int num_devices = outSize / sizeof(AudioObjectID);
        jack_log("Device does has %d subdevices", num_devices);
        return true;
    }
}

void JackCoreAudioAdapter::CloseAUHAL()
{
    AudioUnitUninitialize(fAUHAL);
    CloseComponent(fAUHAL);
}

int JackCoreAudioAdapter::Open()
{
    return (AudioOutputUnitStart(fAUHAL) != noErr)  ? -1 : 0;
}

int JackCoreAudioAdapter::Close()
{
#ifdef JACK_MONITOR
    fTable.Save(fHostBufferSize, fHostSampleRate, fAdaptedSampleRate, fAdaptedBufferSize);
#endif
    AudioOutputUnitStop(fAUHAL);
    DisposeBuffers();
    CloseAUHAL();
    RemoveListeners();
    if (fPluginID > 0) {
        DestroyAggregateDevice();
    }
    return 0;
}

int JackCoreAudioAdapter::SetSampleRate(jack_nframes_t sample_rate)
{
    JackAudioAdapterInterface::SetHostSampleRate(sample_rate);
    Close();
    return Open();
}

int JackCoreAudioAdapter::SetBufferSize(jack_nframes_t buffer_size)
{
    JackAudioAdapterInterface::SetHostBufferSize(buffer_size);
    Close();
    return Open();
}

OSStatus JackCoreAudioAdapter::GetStreamLatencies(AudioDeviceID device, bool isInput, vector<int>& latencies)
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

int JackCoreAudioAdapter::GetLatency(int port_index, bool input)
{
    UInt32 size = sizeof(UInt32);
    UInt32 value1 = 0;
    UInt32 value2 = 0;

    OSStatus err = AudioDeviceGetProperty(fDeviceID, 0, input, kAudioDevicePropertyLatency, &size, &value1);
    if (err != noErr) {
        jack_log("AudioDeviceGetProperty kAudioDevicePropertyLatency error");
    }
    err = AudioDeviceGetProperty(fDeviceID, 0, input, kAudioDevicePropertySafetyOffset, &size, &value2);
    if (err != noErr) {
        jack_log("AudioDeviceGetProperty kAudioDevicePropertySafetyOffset error");
    }

    // TODO : add stream latency

    return value1 + value2 + fAdaptedBufferSize;
}

int JackCoreAudioAdapter::GetInputLatency(int port_index)
{
    if (port_index < int(fInputLatencies.size())) {
        return GetLatency(port_index, true) + fInputLatencies[port_index];
    } else {
        // No stream latency
        return GetLatency(port_index, true);
    }
}

int JackCoreAudioAdapter::GetOutputLatency(int port_index)
{
    if (port_index < int(fOutputLatencies.size())) {
        return GetLatency(port_index, false) + fOutputLatencies[port_index];
    } else {
        // No stream latency
        return GetLatency(port_index, false);
    }
}

} // namespace

#ifdef __cplusplus
extern "C"
{
#endif

   SERVER_EXPORT jack_driver_desc_t* jack_get_descriptor()
   {
        jack_driver_desc_t * desc;
        jack_driver_desc_filler_t filler;
        jack_driver_param_value_t value;

        desc = jack_driver_descriptor_construct("audioadapter", JackDriverNone, "netjack audio <==> net backend adapter", &filler);

        value.i = -1;
        jack_driver_descriptor_add_parameter(desc, &filler, "channels", 'c', JackDriverParamInt, &value, NULL, "Maximum number of channels", "Maximum number of channels. If -1, max possible number of channels will be used");
        jack_driver_descriptor_add_parameter(desc, &filler, "in-channels", 'i', JackDriverParamInt, &value, NULL, "Maximum number of input channels", "Maximum number of input channels. If -1, max possible number of input channels will be used");
        jack_driver_descriptor_add_parameter(desc, &filler, "out-channels", 'o', JackDriverParamInt, &value, NULL, "Maximum number of output channels", "Maximum number of output channels. If -1, max possible number of output channels will be used");

        value.str[0] = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "capture", 'C', JackDriverParamString, &value, NULL, "Input CoreAudio device name", NULL);
        jack_driver_descriptor_add_parameter(desc, &filler, "playback", 'P', JackDriverParamString, &value, NULL, "Output CoreAudio device name", NULL);

        value.ui = 44100U;
        jack_driver_descriptor_add_parameter(desc, &filler, "rate", 'r', JackDriverParamUInt, &value, NULL, "Sample rate", NULL);

        value.ui = 512U;
        jack_driver_descriptor_add_parameter(desc, &filler, "period", 'p', JackDriverParamUInt, &value, NULL, "Frames per period", NULL);

        value.i = TRUE;
        jack_driver_descriptor_add_parameter(desc, &filler, "duplex", 'D', JackDriverParamBool, &value, NULL, "Provide both capture and playback ports", NULL);

        value.str[0] = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "device", 'd', JackDriverParamString, &value, NULL, "CoreAudio device name", NULL);

        value.i = TRUE;
        jack_driver_descriptor_add_parameter(desc, &filler, "list-devices", 'l', JackDriverParamBool, &value, NULL, "Display available CoreAudio devices", NULL);

        value.ui = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "quality", 'q', JackDriverParamInt, &value, NULL, "Resample algorithm quality (0 - 4)", NULL);

        value.ui = 32768;
        jack_driver_descriptor_add_parameter(desc, &filler, "ring-buffer", 'g', JackDriverParamInt, &value, NULL, "Fixed ringbuffer size", "Fixed ringbuffer size (if not set => automatic adaptative)");

        value.i = FALSE;
        jack_driver_descriptor_add_parameter(desc, &filler, "clock-drift", 's', JackDriverParamBool, &value, NULL, "Clock drift compensation", "Whether to compensate clock drift in dynamically created aggregate device");

        return desc;
    }


#ifdef __cplusplus
}
#endif

