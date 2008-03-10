/*
Copyright (C) 2006 Grame

Portable Audio I/O Library for ASIO Drivers
Author: Stephane Letz
Based on the Open Source API proposed by Ross Bencina
Copyright (c) 2000-2002 Stephane Letz, Phil Burk, Ross Bencina

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

#include "pa_asio.h"
#include "JackDriverLoader.h"
#include "driver_interface.h"

#include "JackASIODriver.h"
#include "JackEngineControl.h"
#include "JackGraphManager.h"
#include "JackError.h"
#include "JackClientControl.h"
#include "JackGlobals.h"
#include <iostream>


#include <windows.h>
#include <mmsystem.h>

#include "asiosys.h"
#include "asio.h"
#include "asiodrivers.h"
#include "iasiothiscallresolver.h"


/* external references */
extern AsioDrivers* asioDrivers ;
bool loadAsioDriver(char *name);


namespace Jack
{


/*
    load the asio driver named by <driverName> and return statistics about
    the driver in info. If no error occurred, the driver will remain open
    and must be closed by the called by calling ASIOExit() - if an error
    is returned the driver will already be closed.
*/
static PaError LoadAsioDriver( const char *driverName,
                               PaAsioDriverInfo *driverInfo, void *systemSpecific )
{
    PaError result = paNoError;
    ASIOError asioError;
    int asioIsInitialized = 0;

    if ( !loadAsioDriver(const_cast<char*>(driverName))) {
        result = paUnanticipatedHostError;
        PA_ASIO_SET_LAST_HOST_ERROR( 0, "Failed to load ASIO driver" );
        goto error;
    }

    memset( &driverInfo->asioDriverInfo, 0, sizeof(ASIODriverInfo) );
    driverInfo->asioDriverInfo.asioVersion = 2;
    driverInfo->asioDriverInfo.sysRef = systemSpecific;
    if ( (asioError = ASIOInit( &driverInfo->asioDriverInfo )) != ASE_OK ) {
        result = paUnanticipatedHostError;
        PA_ASIO_SET_LAST_ASIO_ERROR( asioError );
        goto error;
    } else {
        asioIsInitialized = 1;
    }

    if ( (asioError = ASIOGetChannels(&driverInfo->inputChannelCount,
                                      &driverInfo->outputChannelCount)) != ASE_OK ) {
        result = paUnanticipatedHostError;
        PA_ASIO_SET_LAST_ASIO_ERROR( asioError );
        goto error;
    }

    if ( (asioError = ASIOGetBufferSize(&driverInfo->bufferMinSize,
                                        &driverInfo->bufferMaxSize, &driverInfo->bufferPreferredSize,
                                        &driverInfo->bufferGranularity)) != ASE_OK ) {
        result = paUnanticipatedHostError;
        PA_ASIO_SET_LAST_ASIO_ERROR( asioError );
        goto error;
    }

    if ( ASIOOutputReady() == ASE_OK )
        driverInfo->postOutput = true;
    else
        driverInfo->postOutput = false;

    return result;

error:
    if ( asioIsInitialized )
        ASIOExit();

    return result;
}


int JackASIODriver::bufferSwitch(long index, ASIOBool directProcess)
{
    JackASIODriver* driver = (JackASIODriver*)userData;

    // the actual processing callback.
    // Beware that this is normally in a seperate thread, hence be sure that
    // you take care about thread synchronization. This is omitted here for
    // simplicity.

    // as this is a "back door" into the bufferSwitchTimeInfo a timeInfo needs
    // to be created though it will only set the timeInfo.samplePosition and
    // timeInfo.systemTime fields and the according flags

    ASIOTime  timeInfo;
    memset(&timeInfo, 0, sizeof(timeInfo));

    // get the time stamp of the buffer, not necessary if no
    // synchronization to other media is required
    if ( ASIOGetSamplePosition(&timeInfo.timeInfo.samplePosition, &timeInfo.timeInfo.systemTime) == ASE_OK)
        timeInfo.timeInfo.flags = kSystemTimeValid | kSamplePositionValid;


    driver->fLastWaitUst = GetMicroSeconds(); // Take callback date here
    driver->fInputBuffer = (float**)inputBuffer;
    driver->fOutputBuffer = (float**)outputBuffer;

    // Call the real callback
    bufferSwitchTimeInfo( &timeInfo, index, directProcess );

    return driver->Process();
}

int JackASIODriver::Read()
{
    return 0;
}

int JackASIODriver::Write()
{
    return 0;
}


int JackASIODriver::Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex hostApiIndex )
{
    PaError result = paNoError;
    int i, driverCount;
    PaAsioHostApiRepresentation *asioHostApi;
    PaAsioDeviceInfo *deviceInfoArray;
    char **names;
    PaAsioDriverInfo paAsioDriverInfo;

    asioHostApi = (PaAsioHostApiRepresentation*)PaUtil_AllocateMemory( sizeof(PaAsioHostApiRepresentation) );
    if ( !asioHostApi ) {
        result = paInsufficientMemory;
        goto error;
    }

    asioHostApi->allocations = PaUtil_CreateAllocationGroup();
    if ( !asioHostApi->allocations ) {
        result = paInsufficientMemory;
        goto error;
    }

    asioHostApi->systemSpecific = 0;
    asioHostApi->openAsioDeviceIndex = paNoDevice;

    *hostApi = &asioHostApi->inheritedHostApiRep;
    (*hostApi)->info.structVersion = 1;

    (*hostApi)->info.type = paASIO;
    (*hostApi)->info.name = "ASIO";
    (*hostApi)->info.deviceCount = 0;

#ifdef WINDOWS
    /* use desktop window as system specific ptr */
    asioHostApi->systemSpecific = GetDesktopWindow();
    CoInitialize(NULL);
#endif

    /* MUST BE CHECKED : to force fragments loading on Mac */
    loadAsioDriver( "dummy" );

    /* driverCount is the number of installed drivers - not necessarily
        the number of installed physical devices. */
#if MAC
    driverCount = asioDrivers->getNumFragments();
#elif WINDOWS
    driverCount = asioDrivers->asioGetNumDev();
#endif

    if ( driverCount > 0 ) {
        names = GetAsioDriverNames( asioHostApi->allocations, driverCount );
        if ( !names ) {
            result = paInsufficientMemory;
            goto error;
        }


        /* allocate enough space for all drivers, even if some aren't installed */

        (*hostApi)->deviceInfos = (PaDeviceInfo**)PaUtil_GroupAllocateMemory(
                                      asioHostApi->allocations, sizeof(PaDeviceInfo*) * driverCount );
        if ( !(*hostApi)->deviceInfos ) {
            result = paInsufficientMemory;
            goto error;
        }

        /* allocate all device info structs in a contiguous block */
        deviceInfoArray = (PaAsioDeviceInfo*)PaUtil_GroupAllocateMemory(
                              asioHostApi->allocations, sizeof(PaAsioDeviceInfo) * driverCount );
        if ( !deviceInfoArray ) {
            result = paInsufficientMemory;
            goto error;
        }

        for ( i = 0; i < driverCount; ++i ) {

            PA_DEBUG(("ASIO names[%d]:%s\n", i, names[i]));

            // Since portaudio opens ALL ASIO drivers, and no one else does that,
            // we face fact that some drivers were not meant for it, drivers which act
            // like shells on top of REAL drivers, for instance.
            // so we get duplicate handles, locks and other problems.
            // so lets NOT try to load any such wrappers.
            // The ones i [davidv] know of so far are:

            if (   strcmp (names[i], "ASIO DirectX Full Duplex Driver") == 0
                    || strcmp (names[i], "ASIO Multimedia Driver")          == 0
                    || strncmp(names[i], "Premiere", 8)                      == 0 //"Premiere Elements Windows Sound 1.0"
                    || strncmp(names[i], "Adobe", 5)                         == 0 ) //"Adobe Default Windows Sound 1.5"
            {
                PA_DEBUG(("BLACKLISTED!!!\n"));
                continue;
            }


            /* Attempt to load the asio driver... */
            if ( LoadAsioDriver( names[i], &paAsioDriverInfo, asioHostApi->systemSpecific ) == paNoError ) {
                PaAsioDeviceInfo *asioDeviceInfo = &deviceInfoArray[ (*hostApi)->info.deviceCount ];
                PaDeviceInfo *deviceInfo = &asioDeviceInfo->commonDeviceInfo;

                deviceInfo->structVersion = 2;
                deviceInfo->hostApi = hostApiIndex;

                deviceInfo->name = names[i];
                PA_DEBUG(("PaAsio_Initialize: drv:%d name = %s\n",  i, deviceInfo->name));
                PA_DEBUG(("PaAsio_Initialize: drv:%d inputChannels       = %d\n", i, paAsioDriverInfo.inputChannelCount));
                PA_DEBUG(("PaAsio_Initialize: drv:%d outputChannels      = %d\n", i, paAsioDriverInfo.outputChannelCount));
                PA_DEBUG(("PaAsio_Initialize: drv:%d bufferMinSize       = %d\n", i, paAsioDriverInfo.bufferMinSize));
                PA_DEBUG(("PaAsio_Initialize: drv:%d bufferMaxSize       = %d\n", i, paAsioDriverInfo.bufferMaxSize));
                PA_DEBUG(("PaAsio_Initialize: drv:%d bufferPreferredSize = %d\n", i, paAsioDriverInfo.bufferPreferredSize));
                PA_DEBUG(("PaAsio_Initialize: drv:%d bufferGranularity   = %d\n", i, paAsioDriverInfo.bufferGranularity));

                deviceInfo->maxInputChannels  = paAsioDriverInfo.inputChannelCount;
                deviceInfo->maxOutputChannels = paAsioDriverInfo.outputChannelCount;

                deviceInfo->defaultSampleRate = 0.;
                bool foundDefaultSampleRate = false;
                for ( int j = 0; j < PA_DEFAULTSAMPLERATESEARCHORDER_COUNT_; ++j ) {
                    ASIOError asioError = ASIOCanSampleRate( defaultSampleRateSearchOrder_[j] );
                    if ( asioError != ASE_NoClock && asioError != ASE_NotPresent ) {
                        deviceInfo->defaultSampleRate = defaultSampleRateSearchOrder_[j];
                        foundDefaultSampleRate = true;
                        break;
                    }
                }

                PA_DEBUG(("PaAsio_Initialize: drv:%d defaultSampleRate = %f\n", i, deviceInfo->defaultSampleRate));

                if ( foundDefaultSampleRate ) {

                    /* calculate default latency values from bufferPreferredSize
                        for default low latency, and bufferPreferredSize * 3
                        for default high latency.
                        use the default sample rate to convert from samples to
                        seconds. Without knowing what sample rate the user will
                        use this is the best we can do.
                    */

                    double defaultLowLatency =
                        paAsioDriverInfo.bufferPreferredSize / deviceInfo->defaultSampleRate;

                    deviceInfo->defaultLowInputLatency = defaultLowLatency;
                    deviceInfo->defaultLowOutputLatency = defaultLowLatency;

                    long defaultHighLatencyBufferSize =
                        paAsioDriverInfo.bufferPreferredSize * 3;

                    if ( defaultHighLatencyBufferSize > paAsioDriverInfo.bufferMaxSize )
                        defaultHighLatencyBufferSize = paAsioDriverInfo.bufferMaxSize;

                    double defaultHighLatency =
                        defaultHighLatencyBufferSize / deviceInfo->defaultSampleRate;

                    if ( defaultHighLatency < defaultLowLatency )
                        defaultHighLatency = defaultLowLatency; /* just incase the driver returns something strange */

                    deviceInfo->defaultHighInputLatency = defaultHighLatency;
                    deviceInfo->defaultHighOutputLatency = defaultHighLatency;

                } else {

                    deviceInfo->defaultLowInputLatency = 0.;
                    deviceInfo->defaultLowOutputLatency = 0.;
                    deviceInfo->defaultHighInputLatency = 0.;
                    deviceInfo->defaultHighOutputLatency = 0.;
                }

                PA_DEBUG(("PaAsio_Initialize: drv:%d defaultLowInputLatency = %f\n", i, deviceInfo->defaultLowInputLatency));
                PA_DEBUG(("PaAsio_Initialize: drv:%d defaultLowOutputLatency = %f\n", i, deviceInfo->defaultLowOutputLatency));
                PA_DEBUG(("PaAsio_Initialize: drv:%d defaultHighInputLatency = %f\n", i, deviceInfo->defaultHighInputLatency));
                PA_DEBUG(("PaAsio_Initialize: drv:%d defaultHighOutputLatency = %f\n", i, deviceInfo->defaultHighOutputLatency));

                asioDeviceInfo->minBufferSize = paAsioDriverInfo.bufferMinSize;
                asioDeviceInfo->maxBufferSize = paAsioDriverInfo.bufferMaxSize;
                asioDeviceInfo->preferredBufferSize = paAsioDriverInfo.bufferPreferredSize;
                asioDeviceInfo->bufferGranularity = paAsioDriverInfo.bufferGranularity;


                asioDeviceInfo->asioChannelInfos = (ASIOChannelInfo*)PaUtil_GroupAllocateMemory(
                                                       asioHostApi->allocations,
                                                       sizeof(ASIOChannelInfo) * (deviceInfo->maxInputChannels
                                                                                  + deviceInfo->maxOutputChannels) );
                if ( !asioDeviceInfo->asioChannelInfos ) {
                    result = paInsufficientMemory;
                    goto error;
                }

                int a;

                for ( a = 0; a < deviceInfo->maxInputChannels; ++a ) {
                    asioDeviceInfo->asioChannelInfos[a].channel = a;
                    asioDeviceInfo->asioChannelInfos[a].isInput = ASIOTrue;
                    ASIOError asioError = ASIOGetChannelInfo( &asioDeviceInfo->asioChannelInfos[a] );
                    if ( asioError != ASE_OK ) {
                        result = paUnanticipatedHostError;
                        PA_ASIO_SET_LAST_ASIO_ERROR( asioError );
                        goto error;
                    }
                }

                for ( a = 0; a < deviceInfo->maxOutputChannels; ++a ) {
                    int b = deviceInfo->maxInputChannels + a;
                    asioDeviceInfo->asioChannelInfos[b].channel = a;
                    asioDeviceInfo->asioChannelInfos[b].isInput = ASIOFalse;
                    ASIOError asioError = ASIOGetChannelInfo( &asioDeviceInfo->asioChannelInfos[b] );
                    if ( asioError != ASE_OK ) {
                        result = paUnanticipatedHostError;
                        PA_ASIO_SET_LAST_ASIO_ERROR( asioError );
                        goto error;
                    }
                }


                /* unload the driver */
                ASIOExit();

                (*hostApi)->deviceInfos[ (*hostApi)->info.deviceCount ] = deviceInfo;
                ++(*hostApi)->info.deviceCount;
            }
        }
    }

    if ( (*hostApi)->info.deviceCount > 0 ) {
        (*hostApi)->info.defaultInputDevice = 0;
        (*hostApi)->info.defaultOutputDevice = 0;
    } else {
        (*hostApi)->info.defaultInputDevice = paNoDevice;
        (*hostApi)->info.defaultOutputDevice = paNoDevice;
    }


    (*hostApi)->Terminate = Terminate;
    (*hostApi)->OpenStream = OpenStream;
    (*hostApi)->IsFormatSupported = IsFormatSupported;

    PaUtil_InitializeStreamInterface( &asioHostApi->callbackStreamInterface, CloseStream, StartStream,
                                      StopStream, AbortStream, IsStreamStopped, IsStreamActive,
                                      GetStreamTime, GetStreamCpuLoad,
                                      PaUtil_DummyRead, PaUtil_DummyWrite,
                                      PaUtil_DummyGetReadAvailable, PaUtil_DummyGetWriteAvailable );

    PaUtil_InitializeStreamInterface( &asioHostApi->blockingStreamInterface, CloseStream, StartStream,
                                      StopStream, AbortStream, IsStreamStopped, IsStreamActive,
                                      GetStreamTime, PaUtil_DummyGetCpuLoad,
                                      ReadStream, WriteStream, GetStreamReadAvailable, GetStreamWriteAvailable );

    return result;

error:
    if ( asioHostApi ) {
        if ( asioHostApi->allocations ) {
            PaUtil_FreeAllAllocations( asioHostApi->allocations );
            PaUtil_DestroyAllocationGroup( asioHostApi->allocations );
        }

        PaUtil_FreeMemory( asioHostApi );
    }
    return result;
}



void JackASIODriverTerminate( struct PaUtilHostApiRepresentation *hostApi )
{
    PaAsioHostApiRepresentation *asioHostApi = (PaAsioHostApiRepresentation*)hostApi;

    /*
        IMPLEMENT ME:
            - clean up any resources not handled by the allocation group
    */

    if ( asioHostApi->allocations )
    {
        PaUtil_FreeAllAllocations( asioHostApi->allocations );
        PaUtil_DestroyAllocationGroup( asioHostApi->allocations );
    }

    PaUtil_FreeMemory( asioHostApi );
}


int JackASIODriver::Open(jack_nframes_t nframes,
                         jack_nframes_t samplerate,
                         int capturing,
                         int playing,
                         int inchannels,
                         int outchannels,
                         bool monitor,
                         const char* capture_driver_uid,
                         const char* playback_driver_uid,
                         jack_nframes_t capture_latency,
                         jack_nframes_t playback_latency)
{
    return 0;

error:
    return -1;
}

int JackASIODriver::Close()
{
    return 0;
}

int JackASIODriver::Start()
{
    jack_log("JackASIODriver::Start");
    return 0;
}

int JackASIODriver::Stop()
{
    jack_log("JackASIODriver::Stop");
    return 0;
}

int JackASIODriver::SetBufferSize(jack_nframes_t nframes)
{
    return 0;
}

void JackASIODriver::PrintState()
{
    int i;
    std::cout << "JackASIODriver state" << std::endl;

    jack_port_id_t port_index;

    std::cout << "Input ports" << std::endl;

    /*
       for (i = 0; i < fPlaybackChannels; i++) {
           port_index = fCapturePortList[i];
           JackPort* port = fGraphManager->GetPort(port_index);
           std::cout << port->GetName() << std::endl;
           if (fGraphManager->GetConnectionsNum(port_index)) {}
       }

       std::cout << "Output ports" << std::endl;

       for (i = 0; i < fCaptureChannels; i++) {
           port_index = fPlaybackPortList[i];
           JackPort* port = fGraphManager->GetPort(port_index);
           std::cout << port->GetName() << std::endl;
           if (fGraphManager->GetConnectionsNum(port_index)) {}
       }
    */
}

} // end of namespace

#ifdef __cplusplus
extern "C"
{
#endif

#include "JackExports.h"

    EXPORT jack_driver_desc_t* driver_get_descriptor() {
        jack_driver_desc_t *desc;
        unsigned int i;
        desc = (jack_driver_desc_t*)calloc(1, sizeof(jack_driver_desc_t));

        strcpy(desc->name, "ASIO");

        desc->nparams = 13;
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
        strcpy(desc->params[i].value.str, "will take default PortAudio input device");
        strcpy(desc->params[i].short_desc, "Provide capture ports. Optionally set PortAudio device name");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "playback");
        desc->params[i].character = 'P';
        desc->params[i].type = JackDriverParamString;
        strcpy(desc->params[i].value.str, "will take default PortAudio output device");
        strcpy(desc->params[i].short_desc, "Provide playback ports. Optionally set PortAudio device name");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy (desc->params[i].name, "monitor");
        desc->params[i].character = 'm';
        desc->params[i].type = JackDriverParamBool;
        desc->params[i].value.i = 0;
        strcpy(desc->params[i].short_desc, "Provide monitor ports for the output");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "duplex");
        desc->params[i].character = 'D';
        desc->params[i].type = JackDriverParamBool;
        desc->params[i].value.i = TRUE;
        strcpy(desc->params[i].short_desc, "Provide both capture and playback ports");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "rate");
        desc->params[i].character = 'r';
        desc->params[i].type = JackDriverParamUInt;
        desc->params[i].value.ui = 44100U;
        strcpy(desc->params[i].short_desc, "Sample rate");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "period");
        desc->params[i].character = 'p';
        desc->params[i].type = JackDriverParamUInt;
        desc->params[i].value.ui = 128U;
        strcpy(desc->params[i].short_desc, "Frames per period");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "device");
        desc->params[i].character = 'd';
        desc->params[i].type = JackDriverParamString;
        desc->params[i].value.ui = 128U;
        strcpy(desc->params[i].value.str, "will take default CoreAudio device name");
        strcpy(desc->params[i].short_desc, "CoreAudio device name");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "input-latency");
        desc->params[i].character = 'I';
        desc->params[i].type = JackDriverParamUInt;
        desc->params[i].value.i = 0;
        strcpy(desc->params[i].short_desc, "Extra input latency");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "output-latency");
        desc->params[i].character = 'O';
        desc->params[i].type = JackDriverParamUInt;
        desc->params[i].value.i = 0;
        strcpy(desc->params[i].short_desc, "Extra output latency");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "list-devices");
        desc->params[i].character  = 'l';
        desc->params[i].type = JackDriverParamBool;
        desc->params[i].value.i  = TRUE;
        strcpy(desc->params[i].short_desc, "Display available CoreAudio devices");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        return desc;
    }

    EXPORT Jack::JackDriverClientInterface* driver_initialize(Jack::JackEngine* engine, Jack::JackSynchro** table, const JSList* params) {
        jack_nframes_t srate = 44100;
        jack_nframes_t frames_per_interrupt = 512;
        int capture = FALSE;
        int playback = FALSE;
        int chan_in = 0;
        int chan_out = 0;
        bool monitor = false;
        char* capture_pcm_name = "";
        char* playback_pcm_name = "";
        const JSList *node;
        const jack_driver_param_t *param;
        jack_nframes_t systemic_input_latency = 0;
        jack_nframes_t systemic_output_latency = 0;

        for (node = params; node; node = jack_slist_next(node)) {
            param = (const jack_driver_param_t *) node->data;

            switch (param->character) {

                case 'd':
                    capture_pcm_name = strdup(param->value.str);
                    playback_pcm_name = strdup(param->value.str);
                    break;

                case 'D':
                    capture = TRUE;
                    playback = TRUE;
                    break;

                case 'c':
                    chan_in = chan_out = (int) param->value.ui;
                    break;

                case 'i':
                    chan_in = (int) param->value.ui;
                    break;

                case 'o':
                    chan_out = (int) param->value.ui;
                    break;

                case 'C':
                    capture = TRUE;
                    if (strcmp(param->value.str, "none") != 0) {
                        capture_pcm_name = strdup(param->value.str);
                    }
                    break;

                case 'P':
                    playback = TRUE;
                    if (strcmp(param->value.str, "none") != 0) {
                        playback_pcm_name = strdup(param->value.str);
                    }
                    break;

                case 'm':
                    monitor = param->value.i;
                    break;

                case 'r':
                    srate = param->value.ui;
                    break;

                case 'p':
                    frames_per_interrupt = (unsigned int) param->value.ui;
                    break;

                case 'I':
                    systemic_input_latency = param->value.ui;
                    break;

                case 'O':
                    systemic_output_latency = param->value.ui;
                    break;

                case 'l':
                    Jack::DisplayDeviceNames();
                    break;
            }
        }

        // duplex is the default
        if (!capture && !playback) {
            capture = TRUE;
            playback = TRUE;
        }

        Jack::JackDriverClientInterface* driver = new Jack::JackASIODriver("ASIO", engine, table);
        if (driver->Open(frames_per_interrupt, srate, capture, playback, chan_in, chan_out, monitor, capture_pcm_name, playback_pcm_name, systemic_input_latency, systemic_output_latency) == 0) {
            return driver;
        } else {
            delete driver;
            return NULL;
        }
    }

#ifdef __cplusplus
}
#endif
