/*
Copyright (C) 2008-2011 Romain Moret at Grame

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

#ifndef __PortAudioDevices__
#define __PortAudioDevices__

#include <cstdio>
#include <string>

#include "portaudio.h"
#include "pa_asio.h"

/*!
\brief A PortAudio Devices manager.
*/

class PortAudioDevices
{
    private:

        PaHostApiIndex fNumHostApi;     //number of hosts
        PaDeviceIndex fNumDevice;       //number of devices
        PaDeviceInfo** fDeviceInfo;     //array of device info
        std::string* fHostName;         //array of host names (matched with host id's)

    public:

        PortAudioDevices();
        ~PortAudioDevices();

        PaDeviceIndex GetNumDevice();
        PaDeviceInfo* GetDeviceInfo(PaDeviceIndex id);
        std::string GetDeviceName(PaDeviceIndex id);
        std::string GetHostFromDevice(PaDeviceInfo* device);
        std::string GetHostFromDevice(PaDeviceIndex id);
        std::string GetFullName(PaDeviceIndex id);
        std::string GetFullName(std::string hostname, std::string devicename);
        PaDeviceInfo* GetDeviceFromFullName(std::string fullname, PaDeviceIndex& id, bool isInput);
        void PrintSupportedStandardSampleRates(const PaStreamParameters* inputParameters, const PaStreamParameters* outputParameters);
        int GetInputDeviceFromName(const char* name, PaDeviceIndex& device, int& in_max);
        int GetOutputDeviceFromName(const char* name, PaDeviceIndex& device, int& out_max);
        int GetPreferredBufferSize(PaDeviceIndex id);
        void DisplayDevicesNames();
        bool IsDuplex(PaDeviceIndex id);

};

#endif
