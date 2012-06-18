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

#ifndef __JackCoreAudioDriver__
#define __JackCoreAudioDriver__

#include <AudioToolbox/AudioConverter.h>
#include <CoreAudio/CoreAudio.h>
#include <AudioUnit/AudioUnit.h>
#include "JackAudioDriver.h"
#include "JackTime.h"

#include <vector>

using namespace std;

namespace Jack
{

#define kVersion 102

typedef	UInt8	CAAudioHardwareDeviceSectionID;
#define	kAudioDeviceSectionInput	((CAAudioHardwareDeviceSectionID)0x01)
#define	kAudioDeviceSectionOutput	((CAAudioHardwareDeviceSectionID)0x00)
#define	kAudioDeviceSectionGlobal	((CAAudioHardwareDeviceSectionID)0x00)
#define	kAudioDeviceSectionWildcard	((CAAudioHardwareDeviceSectionID)0xFF)

#define WAIT_COUNTER 60
#define WAIT_NOTIFICATION_COUNTER 30

/*!
\brief The CoreAudio driver.

\todo hardware monitoring
*/

class JackAC3Encoder;

class JackCoreAudioDriver : public JackAudioDriver
{

    private:
    
        JackAC3Encoder* fAC3Encoder;

        AudioUnit fAUHAL;

        AudioBufferList* fJackInputData;
        AudioBufferList* fDriverOutputData;

        AudioDeviceID fDeviceID;    // Used "duplex" device
        AudioObjectID fPluginID;    // Used for aggregate device
    
        AudioUnitRenderActionFlags* fActionFags;
        const AudioTimeStamp* fCurrentTime;

        bool fState;
        bool fHogged;

        char fCaptureUID[256];
        char fPlaybackUID[256];

        float fIOUsage;
        float fComputationGrain;
        bool fClockDriftCompensate;
        bool fDigitalPlayback;
   
        static OSStatus Render(void *inRefCon,
                               AudioUnitRenderActionFlags *ioActionFlags,
                               const AudioTimeStamp *inTimeStamp,
                               UInt32 inBusNumber,
                               UInt32 inNumberFrames,
                               AudioBufferList *ioData);
    
        static OSStatus AudioHardwareNotificationCallback(AudioHardwarePropertyID inPropertyID,void* inClientData);
   
        static OSStatus DeviceNotificationCallback(AudioDeviceID inDevice,
                                                    UInt32 inChannel,
                                                    Boolean	isInput,
                                                    AudioDevicePropertyID inPropertyID,
                                                    void* inClientData);

        static OSStatus SRNotificationCallback(AudioDeviceID inDevice,
                                               UInt32 inChannel,
                                               Boolean	isInput,
                                               AudioDevicePropertyID inPropertyID,
                                               void* inClientData);

        static OSStatus BSNotificationCallback(AudioDeviceID inDevice,
                                                UInt32 inChannel,
                                                Boolean	isInput,
                                                AudioDevicePropertyID inPropertyID,
                                                void* inClientData);

        OSStatus GetDeviceIDFromUID(const char* UID, AudioDeviceID* id);
        OSStatus GetDefaultDevice(AudioDeviceID* id);
        OSStatus GetDefaultInputDevice(AudioDeviceID* id);
        OSStatus GetDefaultOutputDevice(AudioDeviceID* id);
        OSStatus GetDeviceNameFromID(AudioDeviceID id, char* name);
        OSStatus GetTotalChannels(AudioDeviceID device, int& channelCount, bool isInput);
        OSStatus GetStreamLatencies(AudioDeviceID device, bool isInput, vector<int>& latencies);

        // Setup
        OSStatus CreateAggregateDevice(AudioDeviceID captureDeviceID, AudioDeviceID playbackDeviceID, jack_nframes_t samplerate, AudioDeviceID* outAggregateDevice);
        OSStatus CreateAggregateDeviceAux(vector<AudioDeviceID> captureDeviceID, vector<AudioDeviceID> playbackDeviceID, jack_nframes_t samplerate, AudioDeviceID* outAggregateDevice);
        OSStatus DestroyAggregateDevice();
        bool IsAggregateDevice(AudioDeviceID device);

        int SetupDevices(const char* capture_driver_uid,
                         const char* playback_driver_uid,
                         char* capture_driver_name,
                         char* playback_driver_name,
                         jack_nframes_t samplerate,
                         bool ac3_encoding);

        int SetupChannels(bool capturing,
                          bool playing,
                          int& inchannels,
                          int& outchannels,
                          int& in_nChannels,
                          int& out_nChannels,
                          bool strict);

        int SetupBuffers(int inchannels);
        void DisposeBuffers();

        int SetupBufferSize(jack_nframes_t buffer_size);
        int SetupSampleRate(jack_nframes_t samplerate);
        int SetupSampleRateAux(AudioDeviceID inDevice, jack_nframes_t samplerate);

        int OpenAUHAL(bool capturing,
                      bool playing,
                      int inchannels,
                      int outchannels,
                      int in_nChannels,
                      int out_nChannels,
                      const vector<int>& chan_in_list,
                      const vector<int>& chan_out_list,
                      jack_nframes_t nframes,
                      jack_nframes_t samplerate);
        void CloseAUHAL();

        int AddListeners();
        void RemoveListeners();

        bool TakeHogAux(AudioDeviceID deviceID, bool isInput);
        bool TakeHog();

        void UpdateLatencies();
        
        bool IsDigitalDevice(AudioDeviceID device);

    public:

        JackCoreAudioDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table);
        virtual ~JackCoreAudioDriver();

        int Open(jack_nframes_t buffer_size,
                 jack_nframes_t samplerate,
                 bool capturing,
                 bool playing,
                 int inchannels,
                 int outchannels,
                 const char* chan_in_list,
                 const char* chan_out_list,
                 bool monitor,
                 const char* capture_driver_name,
                 const char* playback_driver_name,
                 jack_nframes_t capture_latency,
                 jack_nframes_t playback_latency,
                 int async_output_latency,
                 int computation_grain,
                 bool hogged,
                 bool clock_drift,
                 bool ac3_encoding,
                 int ac3_bitrate, 
                 bool ac3_lfe);
        int Close();

        int Attach();

        int Start();
        int Stop();

        int Read();
        int Write();

        // BufferSize can be changed
        bool IsFixedBufferSize()
        {
            return false;
        }

        int SetBufferSize(jack_nframes_t buffer_size);
};

} // end of namespace

#endif
