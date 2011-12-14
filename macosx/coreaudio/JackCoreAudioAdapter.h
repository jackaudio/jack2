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

#ifndef __JackCoreAudioAdapter__
#define __JackCoreAudioAdapter__

#include "JackAudioAdapterInterface.h"
#include "jack.h"
#include "jslist.h"
#include <AudioToolbox/AudioConverter.h>
#include <CoreAudio/CoreAudio.h>
#include <AudioUnit/AudioUnit.h>

#include <vector>

using namespace std;

namespace Jack
{

typedef	UInt8	CAAudioHardwareDeviceSectionID;
#define	kAudioDeviceSectionInput	((CAAudioHardwareDeviceSectionID)0x01)
#define	kAudioDeviceSectionOutput	((CAAudioHardwareDeviceSectionID)0x00)
#define	kAudioDeviceSectionGlobal	((CAAudioHardwareDeviceSectionID)0x00)
#define	kAudioDeviceSectionWildcard	((CAAudioHardwareDeviceSectionID)0xFF)

#define WAIT_COUNTER 60

/*!
\brief Audio adapter using CoreAudio API.
*/

class JackCoreAudioAdapter : public JackAudioAdapterInterface
{

    private:

        AudioUnit fAUHAL;
        AudioBufferList* fInputData;

        char fCaptureUID[256];
        char fPlaybackUID[256];

        bool fCapturing;
        bool fPlaying;

        AudioDeviceID fDeviceID;    // Used "duplex" device
        AudioObjectID fPluginID;    // Used for aggregate device

        vector<int> fInputLatencies;
        vector<int> fOutputLatencies;

        bool fState;

        AudioUnitRenderActionFlags* fActionFags;
        AudioTimeStamp* fCurrentTime;
        bool fClockDriftCompensate;

        static	OSStatus Render(void *inRefCon,
                                AudioUnitRenderActionFlags *ioActionFlags,
                                const AudioTimeStamp *inTimeStamp,
                                UInt32 inBusNumber,
                                UInt32 inNumberFrames,
                                AudioBufferList *ioData);

        static OSStatus AudioHardwareNotificationCallback(AudioHardwarePropertyID inPropertyID,void* inClientData);

        static OSStatus SRNotificationCallback(AudioDeviceID inDevice,
                                                UInt32 inChannel,
                                                Boolean	isInput,
                                                AudioDevicePropertyID inPropertyID,
                                                void* inClientData);
        static OSStatus DeviceNotificationCallback(AudioDeviceID inDevice,
                                                    UInt32 inChannel,
                                                    Boolean	isInput,
                                                    AudioDevicePropertyID inPropertyID,
                                                    void* inClientData);

        OSStatus GetDefaultDevice(AudioDeviceID* id);
        OSStatus GetTotalChannels(AudioDeviceID device, int& channelCount, bool isInput);
        OSStatus GetDeviceIDFromUID(const char* UID, AudioDeviceID* id);
        OSStatus GetDefaultInputDevice(AudioDeviceID* id);
        OSStatus GetDefaultOutputDevice(AudioDeviceID* id);
        OSStatus GetDeviceNameFromID(AudioDeviceID id, char* name);
        AudioDeviceID GetDeviceIDFromName(const char* name);

        // Setup
        OSStatus CreateAggregateDevice(AudioDeviceID captureDeviceID, AudioDeviceID playbackDeviceID, jack_nframes_t samplerate, AudioDeviceID* outAggregateDevice);
        OSStatus CreateAggregateDeviceAux(vector<AudioDeviceID> captureDeviceID, vector<AudioDeviceID> playbackDeviceID, jack_nframes_t samplerate, AudioDeviceID* outAggregateDevice);
        OSStatus DestroyAggregateDevice();
        bool IsAggregateDevice(AudioDeviceID device);

        int SetupDevices(const char* capture_driver_uid,
                         const char* playback_driver_uid,
                         char* capture_driver_name,
                         char* playback_driver_name,
                         jack_nframes_t samplerate);

        int SetupChannels(bool capturing,
                          bool playing,
                          int& inchannels,
                          int& outchannels,
                          int& in_nChannels,
                          int& out_nChannels,
                          bool strict);

        int OpenAUHAL(bool capturing,
                    bool playing,
                    int inchannels,
                    int outchannels,
                    int in_nChannels,
                    int out_nChannels,
                    jack_nframes_t buffer_size,
                    jack_nframes_t samplerate);

        int SetupBufferSize(jack_nframes_t buffer_size);
        int SetupSampleRate(jack_nframes_t samplerate);
        int SetupSampleRateAux(AudioDeviceID inDevice, jack_nframes_t samplerate);

        int SetupBuffers(int inchannels);
        void DisposeBuffers();
        void CloseAUHAL();

        int AddListeners();
        void RemoveListeners();

        int GetLatency(int port_index, bool input);
        OSStatus GetStreamLatencies(AudioDeviceID device, bool isInput, vector<int>& latencies);

    public:

        JackCoreAudioAdapter(jack_nframes_t buffer_size, jack_nframes_t sample_rate, const JSList* params);
        ~JackCoreAudioAdapter()
        {}

        virtual int Open();
        virtual int Close();

        virtual int SetSampleRate(jack_nframes_t sample_rate);
        virtual int SetBufferSize(jack_nframes_t buffer_size);

        virtual int GetInputLatency(int port_index);
        virtual int GetOutputLatency(int port_index);
};


} // end of namepace

#ifdef __cplusplus
extern "C"
{
#endif

#include "JackCompilerDeps.h"
#include "driver_interface.h"

SERVER_EXPORT jack_driver_desc_t* jack_get_descriptor();

#ifdef __cplusplus
}
#endif

#endif
