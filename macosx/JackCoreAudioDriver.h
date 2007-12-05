/*
Copyright (C) 2004-2006 Grame

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

#include "/Developer/Examples/CoreAudio/PublicUtility/CALatencyLog.h"

namespace Jack
{

#define kVersion 102
#define LOG_SAMPLE_DURATION 3	// in millisecond

//#define IO_CPU 1

typedef	UInt8	CAAudioHardwareDeviceSectionID;
#define	kAudioDeviceSectionInput	((CAAudioHardwareDeviceSectionID)0x01)
#define	kAudioDeviceSectionOutput	((CAAudioHardwareDeviceSectionID)0x00)
#define	kAudioDeviceSectionGlobal	((CAAudioHardwareDeviceSectionID)0x00)
#define	kAudioDeviceSectionWildcard	((CAAudioHardwareDeviceSectionID)0xFF)


/*!
\brief The CoreAudio driver.
 
\todo hardware monitoring
*/

class JackCoreAudioDriver : public JackAudioDriver
{

    private:
	
	#ifdef DEBUG
        //CALatencyLog* fLogFile;
	#endif

        AudioUnit fAUHAL;

        AudioBufferList* fJackInputData;
        AudioBufferList* fDriverOutputData;

        AudioDeviceID fDeviceID;

        AudioUnitRenderActionFlags fActionFags;
        AudioTimeStamp fCurrentTime;
		
		bool fState;

        static	OSStatus Render(void *inRefCon,
                               AudioUnitRenderActionFlags *ioActionFlags,
                               const AudioTimeStamp *inTimeStamp,
                               UInt32 inBusNumber,
                               UInt32 inNumberFrames,
                               AudioBufferList *ioData);

        static OSStatus MeasureCallback(AudioDeviceID inDevice,
                                        const AudioTimeStamp* inNow,
                                        const AudioBufferList* inInputData,
                                        const AudioTimeStamp* inInputTime,
                                        AudioBufferList* outOutputData,
                                        const AudioTimeStamp* inOutputTime,
                                        void* inClientData);

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
													
        OSStatus GetDeviceIDFromUID(const char* UID, AudioDeviceID* id);
        OSStatus GetDefaultDevice(AudioDeviceID* id);
        OSStatus GetDefaultInputDevice(AudioDeviceID* id);
        OSStatus GetDefaultOutputDevice(AudioDeviceID* id);
        OSStatus GetDeviceNameFromID(AudioDeviceID id, char* name);
        OSStatus GetTotalChannels(AudioDeviceID device, long* channelCount, bool isInput);

    public:

        JackCoreAudioDriver(const char* name, JackEngine* engine, JackSynchro** table);
        virtual ~JackCoreAudioDriver();

        int Open(jack_nframes_t frames_per_cycle,
                 jack_nframes_t rate,
                 int capturing,
                 int playing,
                 int chan_in,
                 int chan_out,
                 bool monitor,
                 const char* capture_driver_name,
                 const char* playback_driver_name,
                 jack_nframes_t capture_latency,
                 jack_nframes_t playback_latency);

        int Close();

        int Attach();

        int Start();
        int Stop();

        int Read();
        int Write();

        int SetBufferSize(jack_nframes_t buffer_size);
};

} // end of namespace

#endif
