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

#ifndef __JackCoreAudioIOAdapter__
#define __JackCoreAudioIOAdapter__

#include "JackIOAdapter.h"
#include "jack.h"
#include <AudioToolbox/AudioConverter.h>
#include <CoreAudio/CoreAudio.h>
#include <AudioUnit/AudioUnit.h>

namespace Jack
{

typedef	UInt8	CAAudioHardwareDeviceSectionID;
#define	kAudioDeviceSectionInput	((CAAudioHardwareDeviceSectionID)0x01)
#define	kAudioDeviceSectionOutput	((CAAudioHardwareDeviceSectionID)0x00)
#define	kAudioDeviceSectionGlobal	((CAAudioHardwareDeviceSectionID)0x00)
#define	kAudioDeviceSectionWildcard	((CAAudioHardwareDeviceSectionID)0xFF)


	class JackCoreAudioIOAdapter : public JackIOAdapterInterface
	{
    
		private:
        
            AudioUnit fAUHAL;
            
            AudioDeviceID fDeviceID;

            AudioUnitRenderActionFlags* fActionFags;
            AudioTimeStamp* fCurrentTime;
            
            static	OSStatus Render(void *inRefCon,
                               AudioUnitRenderActionFlags *ioActionFlags,
                               const AudioTimeStamp *inTimeStamp,
                               UInt32 inBusNumber,
                               UInt32 inNumberFrames,
                               AudioBufferList *ioData);

            OSStatus GetDeviceIDFromUID(const char* UID, AudioDeviceID* id);
            OSStatus GetDefaultDevice(AudioDeviceID* id);
            OSStatus GetDefaultInputDevice(AudioDeviceID* id);
            OSStatus GetDefaultOutputDevice(AudioDeviceID* id);
            OSStatus GetDeviceNameFromID(AudioDeviceID id, char* name);
            OSStatus GetTotalChannels(AudioDeviceID device, int* channelCount, bool isInput);

            // Setup
            int SetupDevices(const char* capture_driver_uid,
                             const char* playback_driver_uid,
                             char* capture_driver_name,
                             char* playback_driver_name);

            int SetupChannels(bool capturing,
                              bool playing,
                              int& inchannels,
                              int& outchannels,
                              int& in_nChannels,
                              int& out_nChannels,
                              bool strict);


            int SetupBufferSizeAndSampleRate(jack_nframes_t nframes, jack_nframes_t samplerate);
    
		public:
        
			JackCoreAudioIOAdapter(int input, int output, int buffer_size, float sample_rate)
                :JackIOAdapterInterface(input, output, buffer_size, sample_rate)
            {}
			~JackCoreAudioIOAdapter()
            {}
            
            virtual int Open();
            virtual int Close();
           
   	};
}

#endif
