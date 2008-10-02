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

namespace Jack
{

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

OSStatus JackCoreAudioAdapter::SRNotificationCallback(AudioDeviceID inDevice,
                                                        UInt32 inChannel,
                                                        Boolean	isInput,
                                                        AudioDevicePropertyID inPropertyID,
                                                        void* inClientData)
{
    JackCoreAudioAdapter* driver = static_cast<JackCoreAudioAdapter*>(inClientData);

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

OSStatus JackCoreAudioAdapter::DeviceNotificationCallback(AudioDeviceID inDevice,
        UInt32 inChannel,
        Boolean	isInput,
        AudioDevicePropertyID inPropertyID,
        void* inClientData)
{
    JackCoreAudioAdapter* driver = (JackCoreAudioAdapter*)inClientData;
         
    switch (inPropertyID) {

        case kAudioDevicePropertyStreamConfiguration:
        case kAudioDevicePropertyNominalSampleRate: {

            UInt32 outSize = sizeof(Float64);
            Float64 sampleRate;
            int in_nChannels = 0;
            int out_nChannels = 0;
            char capture_driver_name[256];
            char playback_driver_name[256];
    
            // Stop and restart
            AudioOutputUnitStop(driver->fAUHAL);
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

            if (driver->SetupChannels(driver->fCapturing, driver->fPlaying, driver->fCaptureChannels, driver->fPlaybackChannels, in_nChannels, out_nChannels, false) < 0)
                return -1;

            if (driver->SetupBufferSizeAndSampleRate(driver->fAdaptedBufferSize, sampleRate)  < 0)
                return -1;
                
            driver->SetAdaptedSampleRate(sampleRate);

            if (driver->OpenAUHAL(driver->fCapturing,
                                  driver->fPlaying,
                                  driver->fCaptureChannels,
                                  driver->fPlaybackChannels,
                                  in_nChannels,
                                  out_nChannels,
                                  driver->fAdaptedBufferSize,
                                  sampleRate,
                                  false) < 0)
                goto error;

            if (driver->AddListeners() < 0)
                goto error;
       
            AudioOutputUnitStart(driver->fAUHAL);
            return noErr;
error:
            driver->CloseAUHAL();
            break;
        }
    }
    return noErr;
}

int JackCoreAudioAdapter::AddListeners()
{
    OSStatus err = noErr;

    // Add listeners
 
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

    return 0;
}


void JackCoreAudioAdapter::RemoveListeners()
{
    AudioDeviceRemovePropertyListener(fDeviceID, 0, true, kAudioHardwarePropertyDevices, DeviceNotificationCallback);
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
    AudioUnitRender(adapter->fAUHAL, ioActionFlags, inTimeStamp, 1, inNumberFrames, adapter->fInputData);
    bool failure = false;

    jack_nframes_t time1, time2;
    adapter->ResampleFactor(time1, time2);

    for (int i = 0; i < adapter->fCaptureChannels; i++) {
        adapter->fCaptureRingBuffer[i]->SetRatio(time1, time2);
        if (adapter->fCaptureRingBuffer[i]->WriteResample((float*)adapter->fInputData->mBuffers[i].mData, inNumberFrames) < inNumberFrames)
            failure = true;
    }

    for (int i = 0; i < adapter->fPlaybackChannels; i++) {
        adapter->fPlaybackRingBuffer[i]->SetRatio(time2, time1);
        if (adapter->fPlaybackRingBuffer[i]->ReadResample((float*)ioData->mBuffers[i].mData, inNumberFrames) < inNumberFrames)
             failure = true;
    }

#ifdef JACK_MONITOR
    adapter->fTable.Write(time1, time2,  double(time1) / double(time2), double(time2) / double(time1),
         adapter->fCaptureRingBuffer[0]->ReadSpace(),  adapter->fPlaybackRingBuffer[0]->WriteSpace());
#endif

    // Reset all ringbuffers in case of failure
    if (failure) {
        jack_error("JackCoreAudioAdapter::Render ringbuffer failure... reset");
        adapter->ResetRingBuffers();
    }

    return noErr;
}

 JackCoreAudioAdapter::JackCoreAudioAdapter(jack_nframes_t buffer_size, jack_nframes_t sample_rate, const JSList* params)
                :JackAudioAdapterInterface(buffer_size, sample_rate), fInputData(0), fCapturing(false), fPlaying(false), fState(false)
{

    const JSList* node;
    const jack_driver_param_t* param;
   
    char captureName[256];
    char playbackName[256];
        
    fCaptureUID[0] = 0;
    fPlaybackUID[0] = 0;
    
    // Default values
    fCaptureChannels = 0;
    fPlaybackChannels = 0;
  
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
        }
    }
    
    /* duplex is the default */
    if (!fCapturing && !fPlaying) {
        fCapturing = true;
        fPlaying = true;
    }
     
    int in_nChannels = 0;
    int out_nChannels = 0;
    
    if (SetupDevices(fCaptureUID, fPlaybackUID, captureName, playbackName) < 0)
        throw -1;
  
    if (SetupChannels(fCapturing, fPlaying, fCaptureChannels, fPlaybackChannels, in_nChannels, out_nChannels, true) < 0)
        throw -1;
 
    if (SetupBufferSizeAndSampleRate(fAdaptedBufferSize, fAdaptedSampleRate) < 0)
        throw -1;

    if (fCapturing && fCaptureChannels > 0)
        if (SetupBuffers(fCaptureChannels) < 0)
            throw -1;
            
    if (OpenAUHAL(fCapturing, fPlaying, fCaptureChannels, fPlaybackChannels, in_nChannels, out_nChannels, fAdaptedBufferSize, fAdaptedSampleRate, true) < 0)
        throw -1;
        
    if (AddListeners() < 0)
        throw -1;
}

OSStatus JackCoreAudioAdapter::GetDefaultDevice(AudioDeviceID* id)
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

OSStatus JackCoreAudioAdapter::GetTotalChannels(AudioDeviceID device, int& channelCount, bool isInput)
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

    if ((res = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultInputDevice, &theSize, &inDefault)) != noErr)
        return res;

    jack_log("GetDefaultInputDevice: input = %ld ", inDefault);
    *id = inDefault;
    return noErr;
}

OSStatus JackCoreAudioAdapter::GetDefaultOutputDevice(AudioDeviceID* id)
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

OSStatus JackCoreAudioAdapter::GetDeviceNameFromID(AudioDeviceID id, char* name)
{
    UInt32 size = 256;
    return AudioDeviceGetProperty(id, 0, false, kAudioDevicePropertyDeviceName, &size, name);
}

// Setup
int JackCoreAudioAdapter::SetupDevices(const char* capture_driver_uid,
                                        const char* playback_driver_uid,
                                        char* capture_driver_name,
                                        char* playback_driver_name)
{
    capture_driver_name[0] = 0;
    playback_driver_name[0] = 0;

    // Duplex
    if (strcmp(capture_driver_uid, "") != 0 && strcmp(playback_driver_uid, "") != 0) {
        jack_log("JackCoreAudioAdapter::Open duplex");
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
        jack_log("JackCoreAudioAdapter::Open capture only");
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
        jack_log("JackCoreAudioAdapter::Open playback only");
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
        jack_log("JackCoreAudioAdapter::Open default driver");
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

int JackCoreAudioAdapter::SetupBufferSizeAndSampleRate(jack_nframes_t nframes, jack_nframes_t samplerate)
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

int JackCoreAudioAdapter::SetupBuffers(int inchannels)
{
    jack_log("JackCoreAudioAdapter::SetupBuffers: input = %ld", inchannels);

    // Prepare buffers
    fInputData = (AudioBufferList*)malloc(sizeof(UInt32) + inchannels * sizeof(AudioBuffer));
    if (fInputData == 0) {
        jack_error("Cannot allocate memory for input buffers");
        return -1;
    }
    fInputData->mNumberBuffers = inchannels;
    for (int i = 0; i < fCaptureChannels; i++) {
        fInputData->mBuffers[i].mNumberChannels = 1;
        fInputData->mBuffers[i].mDataByteSize = fAdaptedBufferSize * sizeof(float);
        fInputData->mBuffers[i].mData = malloc(fAdaptedBufferSize * sizeof(float));
    }
    return 0;
}

void JackCoreAudioAdapter::DisposeBuffers()
{
    if (fInputData) {
        for (int i = 0; i < fCaptureChannels; i++)
            free(fInputData->mBuffers[i].mData);
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
                                    jack_nframes_t nframes,
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
    fTable.Save();
#endif
    AudioOutputUnitStop(fAUHAL);
    DisposeBuffers();
    CloseAUHAL();
    RemoveListeners();
    return 0;
}

int JackCoreAudioAdapter::SetSampleRate ( jack_nframes_t sample_rate ) {
    JackAudioAdapterInterface::SetHostSampleRate ( sample_rate );
    Close();
    return Open();
}

int JackCoreAudioAdapter::SetBufferSize ( jack_nframes_t buffer_size ) {
    JackAudioAdapterInterface::SetHostBufferSize ( buffer_size );
    Close();
    return Open();
}

} // namespace

#ifdef __cplusplus
extern "C"
{
#endif

   SERVER_EXPORT jack_driver_desc_t* jack_get_descriptor()
   {
        jack_driver_desc_t *desc;
        unsigned int i;
        desc = (jack_driver_desc_t*)calloc(1, sizeof(jack_driver_desc_t));

        strcpy(desc->name, "audioadapter");                            // size MUST be less then JACK_DRIVER_NAME_MAX + 1
        strcpy(desc->desc, "netjack audio <==> net backend adapter");  // size MUST be less then JACK_DRIVER_PARAM_DESC + 1
     
        desc->nparams = 10;
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
        strcpy(desc->params[i].name, "rate");
        desc->params[i].character = 'r';
        desc->params[i].type = JackDriverParamUInt;
        desc->params[i].value.ui = 44100U;
        strcpy(desc->params[i].short_desc, "Sample rate");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "periodsize");
        desc->params[i].character = 'p';
        desc->params[i].type = JackDriverParamUInt;
        desc->params[i].value.ui = 512U;
        strcpy(desc->params[i].short_desc, "Period size");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "duplex");
        desc->params[i].character = 'D';
        desc->params[i].type = JackDriverParamBool;
        desc->params[i].value.i = TRUE;
        strcpy(desc->params[i].short_desc, "Provide both capture and playback ports");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "device");
        desc->params[i].character = 'd';
        desc->params[i].type = JackDriverParamString;
        strcpy(desc->params[i].value.str, "will take default CoreAudio device name");
        strcpy(desc->params[i].short_desc, "CoreAudio device name");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "list-devices");
        desc->params[i].character = 'l';
        desc->params[i].type = JackDriverParamBool;
        desc->params[i].value.i = TRUE;
        strcpy(desc->params[i].short_desc, "Display available CoreAudio devices");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        return desc;
    }


#ifdef __cplusplus
}
#endif

