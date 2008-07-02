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

namespace Jack
{
            
OSStatus JackCoreAudioIOAdapter::Render(void *inRefCon,
                   AudioUnitRenderActionFlags *ioActionFlags,
                   const AudioTimeStamp *inTimeStamp,
                   UInt32 inBusNumber,
                   UInt32 inNumberFrames,
                   AudioBufferList *ioData)
{
    return noErr;
}

OSStatus JackCoreAudioIOAdapter::GetDeviceIDFromUID(const char* UID, AudioDeviceID* id)
{
    return noErr;
}
OSStatus JackCoreAudioIOAdapter::GetDefaultDevice(AudioDeviceID* id)
{
    return noErr;
}
OSStatus JackCoreAudioIOAdapter::GetDefaultInputDevice(AudioDeviceID* id)
{
    return noErr;
}
OSStatus JackCoreAudioIOAdapter::GetDefaultOutputDevice(AudioDeviceID* id)
{
    return noErr;
}
OSStatus JackCoreAudioIOAdapter::GetDeviceNameFromID(AudioDeviceID id, char* name)
{
    return noErr;
}
OSStatus JackCoreAudioIOAdapter::GetTotalChannels(AudioDeviceID device, int* channelCount, bool isInput)
{
    return noErr;
}

// Setup
int JackCoreAudioIOAdapter::SetupDevices(const char* capture_driver_uid,
                 const char* playback_driver_uid,
                 char* capture_driver_name,
                 char* playback_driver_name)
{
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
    return 0;
}


int JackCoreAudioIOAdapter::SetupBufferSizeAndSampleRate(jack_nframes_t nframes, jack_nframes_t samplerate)
{
    return 0;
}
  
  
}
