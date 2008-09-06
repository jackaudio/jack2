/*
Copyright (C) 2008 Romain Moret at Grame

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

#include "JackPortAudioDevices.h"
#include "JackError.h"

using namespace std;

PortAudioDevices::PortAudioDevices()
{
    PaError err;
	PaDeviceIndex id;
	printf("Initializing PortAudio...\n");
    if ( ( err = Pa_Initialize() ) == paNoError )
    {
        fNumHostApi = Pa_GetHostApiCount();
        fNumDevice = Pa_GetDeviceCount();
        fDeviceInfo = new PaDeviceInfo*[fNumDevice];
        for ( id = 0; id < fNumDevice; id++ )
            fDeviceInfo[id] = const_cast<PaDeviceInfo*>(Pa_GetDeviceInfo(id));
        fHostName = new string[fNumHostApi];
        for ( id = 0; id < fNumHostApi; id++ )
            fHostName[id] = string ( Pa_GetHostApiInfo(id)->name );
    }
    else
        printf("JackPortAudioDriver::Pa_Initialize error = %s\n", Pa_GetErrorText(err));
}

PortAudioDevices::~PortAudioDevices()
{
    Pa_Terminate();
    delete[] fDeviceInfo;
    delete[] fHostName;
}

PaDeviceIndex PortAudioDevices::GetNumDevice()
{
    return fNumDevice;
}

PaDeviceInfo* PortAudioDevices::GetDeviceInfo ( PaDeviceIndex id )
{
    return fDeviceInfo[id];
}

string PortAudioDevices::GetDeviceName ( PaDeviceIndex id )
{
    return string ( fDeviceInfo[id]->name );
}

string PortAudioDevices::GetHostFromDevice ( PaDeviceInfo* device )
{
    return fHostName[device->hostApi];
}

string PortAudioDevices::GetHostFromDevice ( PaDeviceIndex id )
{
    return fHostName[fDeviceInfo[id]->hostApi];
}

string PortAudioDevices::GetFullName ( PaDeviceIndex id )
{
    string hostname = GetHostFromDevice ( id );
    string devicename = GetDeviceName ( id );
    //some hostname are quite long...use shortcuts
    if ( hostname.compare ( "Windows DirectSound" ) == 0 )
        hostname = string ( "DirectSound" );
    return ( hostname + "::" + devicename );
}

string PortAudioDevices::GetFullName ( std::string hostname, std::string devicename )
{
    //some hostname are quite long...use shortcuts
    if ( hostname.compare ( "Windows DirectSound" ) == 0 )
        hostname = string ( "DirectSound" );
    return ( hostname + "::" + devicename );
}

PaDeviceInfo* PortAudioDevices::GetDeviceFromFullName ( string fullname, PaDeviceIndex& id )
{
    PaDeviceInfo* ret = NULL;
    //no driver to find
    if ( fullname.size() == 0 )
        return NULL;
    //first get host and device names from fullname
    string::size_type separator = fullname.find ( "::", 0 );
    if ( separator == 0 )
        return NULL;
    char* hostname = (char*)malloc(separator + 9);
    fill_n ( hostname, separator + 9, 0 );
    fullname.copy ( hostname, separator );
    //we need the entire hostname, replace shortcuts
    if ( strcmp ( hostname, "DirectSound" ) == 0 )
        strcpy ( hostname, "Windows DirectSound" );
    string devicename = fullname.substr ( separator + 2 );
    //then find the corresponding device
    for ( PaDeviceIndex dev_id = 0; dev_id < fNumDevice; dev_id++ )
    {
        if ( ( GetHostFromDevice(dev_id).compare(hostname) == 0 ) && ( GetDeviceName(dev_id).compare(devicename) == 0 ) )
        {
            id = dev_id;
            ret = fDeviceInfo[dev_id];
        }
    }
	free(hostname);
    return ret;
}

void PortAudioDevices::PrintSupportedStandardSampleRates(const PaStreamParameters* inputParameters, const PaStreamParameters* outputParameters)
{
    static double standardSampleRates[] =
    {
        8000.0, 9600.0, 11025.0, 12000.0, 16000.0, 22050.0, 24000.0, 32000.0,
        44100.0, 48000.0, 88200.0, 96000.0, 192000.0, -1 /* negative terminated  list */
    };
    int i, printCount;
    PaError err;

    printCount = 0;
    for (i = 0; standardSampleRates[i] > 0; i++)
    {
        err = Pa_IsFormatSupported(inputParameters, outputParameters, standardSampleRates[i]);
        if (err == paFormatIsSupported)
        {
            if (printCount == 0)
            {
                printf("\t%8.2f", standardSampleRates[i]);
                printCount = 1;
            }
            else if (printCount == 4)
            {
                printf(",\n\t%8.2f", standardSampleRates[i]);
                printCount = 1;
            }
            else
            {
                printf(", %8.2f", standardSampleRates[i]);
                ++printCount;
            }
        }
    }
    if (!printCount)
        printf("None\n");
    else
        printf("\n");
}

int PortAudioDevices::GetInputDeviceFromName(const char* devicename, PaDeviceIndex& id, int& max_input)
{
    string fullname = string ( devicename );
    PaDeviceInfo* device = GetDeviceFromFullName ( fullname, id );
    if ( device )
        max_input = device->maxInputChannels;
    else
    {
        id = Pa_GetDefaultInputDevice();
        if ( fullname.size() )
            printf("Can't open %s, PortAudio will use default input device.\n", devicename);
        if ( id == paNoDevice )
            return -1;
        max_input = GetDeviceInfo(id)->maxInputChannels;
        devicename = strdup ( GetDeviceInfo(id)->name );
    }
    return id;
}

int PortAudioDevices::GetOutputDeviceFromName(const char* devicename, PaDeviceIndex& id, int& max_output)
{
    string fullname = string ( devicename );
    PaDeviceInfo* device = GetDeviceFromFullName ( fullname, id );
    if ( device )
        max_output = device->maxOutputChannels;
    else
    {
        id = Pa_GetDefaultOutputDevice();
        if ( fullname.size() )
            printf("Can't open %s, PortAudio will use default output device.\n", devicename);
        if ( id == paNoDevice )
            return -1;
        max_output = GetDeviceInfo(id)->maxOutputChannels;
        devicename = strdup ( GetDeviceInfo(id)->name );
    }
    return id;
}

void PortAudioDevices::DisplayDevicesNames()
{
    int def_display;
    PaDeviceIndex id;
    PaStreamParameters inputParameters, outputParameters;
    printf ( "********************** Devices list, %d detected **********************\n", fNumDevice );

    for ( id = 0; id < fNumDevice; id++ )
    {
        printf ( "-------- device #%d ------------------------------------------------\n", id );

        def_display = 0;
        if ( id == Pa_GetDefaultInputDevice() )
        {
            printf("[ Default Input");
            def_display = 1;
        }
        else if ( id == Pa_GetHostApiInfo ( fDeviceInfo[id]->hostApi)->defaultInputDevice )
        {
            const PaHostApiInfo *host_info = Pa_GetHostApiInfo ( fDeviceInfo[id]->hostApi );
            printf ( "[ Default %s Input", host_info->name );
            def_display = 1;
        }

        if ( id == Pa_GetDefaultOutputDevice() )
        {
            printf ( ( def_display ? "," : "[" ) );
            printf ( " Default Output" );
            def_display = 1;
        }
        else if ( id == Pa_GetHostApiInfo ( fDeviceInfo[id]->hostApi )->defaultOutputDevice )
        {
            const PaHostApiInfo *host_info = Pa_GetHostApiInfo ( fDeviceInfo[id]->hostApi );
            printf ( ( def_display ? "," : "[" ) );
            printf ( " Default %s Output", host_info->name );
            def_display = 1;
        }

        if ( def_display )
            printf ( " ]\n" );

        /* print device info fields */
        printf ( "Name                        = %s\n", GetFullName ( id ).c_str() );
        printf ( "Max inputs                  = %d\n", fDeviceInfo[id]->maxInputChannels );
        printf ( "Max outputs                 = %d\n", fDeviceInfo[id]->maxOutputChannels );

#ifdef WIN32
        /* ASIO specific latency information */
        if ( Pa_GetHostApiInfo(fDeviceInfo[id]->hostApi)->type == paASIO )
        {
            long minLatency, maxLatency, preferredLatency, granularity;

            PaAsio_GetAvailableLatencyValues ( id, &minLatency, &maxLatency, &preferredLatency, &granularity );

            printf ( "ASIO minimum buffer size    = %ld\n", minLatency );
            printf ( "ASIO maximum buffer size    = %ld\n", maxLatency );
            printf ( "ASIO preferred buffer size  = %ld\n", preferredLatency );

            if ( granularity == -1 )
                printf ( "ASIO buffer granularity     = power of 2\n" );
            else
                printf ( "ASIO buffer granularity     = %ld\n", granularity );
        }
#endif

        printf ( "Default sample rate         = %8.2f\n", fDeviceInfo[id]->defaultSampleRate );

        /* poll for standard sample rates */
        inputParameters.device = id;
        inputParameters.channelCount = fDeviceInfo[id]->maxInputChannels;
        inputParameters.sampleFormat = paInt16;
        inputParameters.suggestedLatency = 0; /* ignored by Pa_IsFormatSupported() */
        inputParameters.hostApiSpecificStreamInfo = NULL;

        outputParameters.device = id;
        outputParameters.channelCount = fDeviceInfo[id]->maxOutputChannels;
        outputParameters.sampleFormat = paInt16;
        outputParameters.suggestedLatency = 0; /* ignored by Pa_IsFormatSupported() */
        outputParameters.hostApiSpecificStreamInfo = NULL;
    }
    printf ( "**************************** End of list ****************************\n" );
}

bool PortAudioDevices::IsDuplex ( PaDeviceIndex id )
{
    //does the device has in and out facilities
    if ( fDeviceInfo[id]->maxInputChannels && fDeviceInfo[id]->maxOutputChannels )
        return true;
    //else is another complementary device ? (search in devices with the same name)
    for ( PaDeviceIndex i = 0; i < fNumDevice; i++ )
        if ( ( i != id ) && ( GetDeviceName ( i ) == GetDeviceName ( id ) ) )
            if ( ( fDeviceInfo[i]->maxInputChannels && fDeviceInfo[id]->maxOutputChannels )
                    || ( fDeviceInfo[i]->maxOutputChannels && fDeviceInfo[id]->maxInputChannels ) )
                return true;
    //then the device isn't full duplex
    return false;
}
