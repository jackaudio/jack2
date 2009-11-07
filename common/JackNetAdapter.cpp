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

#include "JackNetAdapter.h"
#include "JackException.h"
#include "JackServerGlobals.h"
#include "JackEngineControl.h"
#include "JackArgParser.h"
#include <assert.h>

namespace Jack
{
    JackNetAdapter::JackNetAdapter ( jack_client_t* jack_client, jack_nframes_t buffer_size, jack_nframes_t sample_rate, const JSList* params )
            : JackAudioAdapterInterface ( buffer_size, sample_rate ), JackNetSlaveInterface(), fThread ( this )
    {
        jack_log ( "JackNetAdapter::JackNetAdapter" );

        //global parametering
        //we can't call JackNetSlaveInterface constructor with some parameters before
        //because we don't have full parametering right now
        //parameters will be parsed from the param list, and then JackNetSlaveInterface will be filled with proper values
        strcpy ( fMulticastIP, DEFAULT_MULTICAST_IP );
        uint port = DEFAULT_PORT;
        GetHostName ( fParams.fName, JACK_CLIENT_NAME_SIZE );
        fSocket.GetName ( fParams.fSlaveNetName );
        fParams.fMtu = DEFAULT_MTU;
        fParams.fTransportSync = 0;
        fParams.fSendAudioChannels = 2;
        fParams.fReturnAudioChannels = 2;
        fParams.fSendMidiChannels = 0;
        fParams.fReturnMidiChannels = 0;
        fParams.fSampleRate = sample_rate;
        fParams.fPeriodSize = buffer_size;
        fParams.fSlaveSyncMode = 1;
        fParams.fNetworkMode = 's';
        fJackClient = jack_client;

        //options parsing
        const JSList* node;
        const jack_driver_param_t* param;
        for ( node = params; node; node = jack_slist_next ( node ) )
        {
            param = ( const jack_driver_param_t* ) node->data;
            switch ( param->character )
            {
                case 'a' :
                    if (strlen (param->value.str) < 32)
                        strcpy(fMulticastIP, param->value.str);
                    else
                        jack_error("Can't use multicast address %s, using default %s", param->value.ui, DEFAULT_MULTICAST_IP);
                    break;
                case 'p' :
                    fSocket.SetPort ( param->value.ui );
                    break;
                case 'M' :
                    fParams.fMtu = param->value.i;
                    break;
                case 'C' :
                    fParams.fSendAudioChannels = param->value.i;
                    break;
                case 'P' :
                    fParams.fReturnAudioChannels = param->value.i;
                    break;
                case 'n' :
                    strncpy ( fParams.fName, param->value.str, JACK_CLIENT_NAME_SIZE );
                    break;
                case 't' :
                    //fParams.fTransportSync = param->value.ui;
                    break;
                case 'm' :
                    if ( strcmp ( param->value.str, "normal" ) == 0 )
                        fParams.fNetworkMode = 'n';
                    else if ( strcmp ( param->value.str, "slow" ) == 0 )
                        fParams.fNetworkMode = 's';
                    else if ( strcmp ( param->value.str, "fast" ) == 0 )
                        fParams.fNetworkMode = 'f';
                    else
                        jack_error ( "Unknown network mode, using 'normal' mode." );
                    break;
                case 'q':
                    fQuality = param->value.ui;
                    break;
                case 'g':
                    fRingbufferCurSize = param->value.ui;
                    fAdaptative = false;
                    break;
             }
        }

        //set the socket parameters
        fSocket.SetPort ( port );
        fSocket.SetAddress ( fMulticastIP, port );

        //set the audio adapter interface channel values
        SetInputs ( fParams.fSendAudioChannels );
        SetOutputs ( fParams.fReturnAudioChannels );

        //soft buffers will be allocated later (once network initialization done)
        fSoftCaptureBuffer = NULL;
        fSoftPlaybackBuffer = NULL;
    }

    JackNetAdapter::~JackNetAdapter()
    {
        jack_log ( "JackNetAdapter::~JackNetAdapter" );

        int port_index;
        if ( fSoftCaptureBuffer )
        {
            for ( port_index = 0; port_index < fCaptureChannels; port_index++ )
                delete[] fSoftCaptureBuffer[port_index];
            delete[] fSoftCaptureBuffer;
        }
        if ( fSoftPlaybackBuffer )
        {
            for ( port_index = 0; port_index < fPlaybackChannels; port_index++ )
                delete[] fSoftPlaybackBuffer[port_index];
            delete[] fSoftPlaybackBuffer;
        }
    }

//open/close--------------------------------------------------------------------------
    int JackNetAdapter::Open()
    {
        jack_log ( "JackNetAdapter::Open" );

        jack_info ( "NetAdapter started in %s mode %s Master's transport sync.",
                    ( fParams.fSlaveSyncMode ) ? "sync" : "async", ( fParams.fTransportSync ) ? "with" : "without" );

        if ( fThread.StartSync() < 0 )
        {
            jack_error ( "Cannot start netadapter thread" );
            return -1;
        }

        return 0;
    }

    int JackNetAdapter::Close()
    {
        jack_log ( "JackNetAdapter::Close" );

#ifdef JACK_MONITOR
        fTable.Save(fHostBufferSize, fHostSampleRate, fAdaptedSampleRate, fAdaptedBufferSize);
#endif

        switch ( fThread.GetStatus() )
        {
                // Kill the thread in Init phase
            case JackThread::kStarting:
            case JackThread::kIniting:
                if ( fThread.Kill() < 0 )
                {
                    jack_error ( "Cannot kill thread" );
                    return -1;
                }
                break;
                // Stop when the thread cycle is finished
                
            case JackThread::kRunning:
                if ( fThread.Stop() < 0 )
                {
                    jack_error ( "Cannot stop thread" );
                    return -1;
                }
                break;
                
            default:
                break;
        }
        fSocket.Close();
        return 0;
    }

    int JackNetAdapter::SetBufferSize ( jack_nframes_t buffer_size )
    {
        JackAudioAdapterInterface::SetHostBufferSize ( buffer_size );
        return 0;
    }

//thread------------------------------------------------------------------------------
    bool JackNetAdapter::Init()
    {
        jack_log ( "JackNetAdapter::Init" );

        int port_index;

        //init network connection
        if ( !JackNetSlaveInterface::Init() )
            return false;

        //then set global parameters
        SetParams();

        //set buffers
        fSoftCaptureBuffer = new sample_t*[fCaptureChannels];
        for ( port_index = 0; port_index < fCaptureChannels; port_index++ )
        {
            fSoftCaptureBuffer[port_index] = new sample_t[fParams.fPeriodSize];
            fNetAudioCaptureBuffer->SetBuffer ( port_index, fSoftCaptureBuffer[port_index] );
        }
        fSoftPlaybackBuffer = new sample_t*[fPlaybackChannels];
        for ( port_index = 0; port_index < fCaptureChannels; port_index++ )
        {
            fSoftPlaybackBuffer[port_index] = new sample_t[fParams.fPeriodSize];
            fNetAudioPlaybackBuffer->SetBuffer ( port_index, fSoftPlaybackBuffer[port_index] );
        }

        //set audio adapter parameters
        SetAdaptedBufferSize ( fParams.fPeriodSize );
        SetAdaptedSampleRate ( fParams.fSampleRate );
        
        // Will do "something" on OSX only...
        fThread.SetParams(GetEngineControl()->fPeriod, GetEngineControl()->fComputation, GetEngineControl()->fConstraint);
        
        if (fThread.AcquireSelfRealTime(GetEngineControl()->fClientPriority) < 0) {
            jack_error("AcquireSelfRealTime error");
        } else {
            set_threaded_log_function();
        }
  
        //init done, display parameters
        SessionParamsDisplay ( &fParams );
        return true;
    }

    bool JackNetAdapter::Execute()
    {
        try {
            // Keep running even in case of error
            while (fThread.GetStatus() == JackThread::kRunning)
                if (Process() == SOCKET_ERROR)
                    return false;
            return false;
        } catch (JackNetException& e) {
            e.PrintMessage();
            jack_info("NetAdapter is restarted.");
            Reset();
            fThread.DropSelfRealTime();
            fThread.SetStatus(JackThread::kIniting);
            if (Init()) {
                fThread.SetStatus(JackThread::kRunning);
                return true;
            } else {
                return false;
            }
        }
    }

//transport---------------------------------------------------------------------------
    void JackNetAdapter::DecodeTransportData()
    {
        //TODO : we need here to get the actual timebase master to eventually release it from its duty (see JackNetDriver)

        //is there a new transport state ?
        if ( fSendTransportData.fNewState && ( fSendTransportData.fState != jack_transport_query ( fJackClient, NULL ) ) )
        {
            switch ( fSendTransportData.fState )
            {
                case JackTransportStopped :
                    jack_transport_stop ( fJackClient );
                    jack_info ( "NetMaster : transport stops." );
                    break;
                    
                case JackTransportStarting :
                    jack_transport_reposition ( fJackClient, &fSendTransportData.fPosition );
                    jack_transport_start ( fJackClient );
                    jack_info ( "NetMaster : transport starts." );
                    break;
                    
                case JackTransportRolling :
                    //TODO , we need to :
                    // - find a way to call TransportEngine->SetNetworkSync()
                    // - turn the transport state to JackTransportRolling
                    jack_info ( "NetMaster : transport rolls." );
                    break;
            }
        }
    }

    void JackNetAdapter::EncodeTransportData()
    {
        //is there a timebase master change ?
        int refnum = -1;
        bool conditional = 0;
        //TODO : get the actual timebase master
        if ( refnum != fLastTimebaseMaster )
        {
            //timebase master has released its function
            if ( refnum == -1 )
            {
                fReturnTransportData.fTimebaseMaster = RELEASE_TIMEBASEMASTER;
                jack_info ( "Sending a timebase master release request." );
            }
            //there is a new timebase master
            else
            {
                fReturnTransportData.fTimebaseMaster = ( conditional ) ? CONDITIONAL_TIMEBASEMASTER : TIMEBASEMASTER;
                jack_info ( "Sending a %s timebase master request.", ( conditional ) ? "conditional" : "non-conditional" );
            }
            fLastTimebaseMaster = refnum;
        }
        else
            fReturnTransportData.fTimebaseMaster = NO_CHANGE;

        //update transport state and position
        fReturnTransportData.fState = jack_transport_query ( fJackClient, &fReturnTransportData.fPosition );

        //is it a new state (that the master need to know...) ?
        fReturnTransportData.fNewState = ( ( fReturnTransportData.fState != fLastTransportState ) &&
                                            ( fReturnTransportData.fState != fSendTransportData.fState ) );
        if ( fReturnTransportData.fNewState )
            jack_info ( "Sending transport state '%s'.", GetTransportState ( fReturnTransportData.fState ) );
        fLastTransportState = fReturnTransportData.fState;
    }

//read/write operations---------------------------------------------------------------
    int JackNetAdapter::Read()
    {
        //don't return -1 in case of sync recv failure
        //we need the process to continue for network error detection
        if ( SyncRecv() == SOCKET_ERROR )
            return 0;

        DecodeSyncPacket();
        return DataRecv();
    }

    int JackNetAdapter::Write()
    {
        EncodeSyncPacket();
     
        if ( SyncSend() == SOCKET_ERROR )
            return SOCKET_ERROR;

        return DataSend();
    }

//process-----------------------------------------------------------------------------
    int JackNetAdapter::Process()
    {
        //read data from the network
        //in case of fatal network error, stop the process
        if (Read() == SOCKET_ERROR)
            return SOCKET_ERROR;
            
        PushAndPull(fSoftCaptureBuffer, fSoftPlaybackBuffer, fAdaptedBufferSize);

        //then write data to network
        //in case of failure, stop process
        if (Write() == SOCKET_ERROR)
            return SOCKET_ERROR;

        return 0;
    }
    
} // namespace Jack

//loader------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C"
{
#endif

#include "driver_interface.h"
#include "JackAudioAdapter.h"

    using namespace Jack;

    SERVER_EXPORT jack_driver_desc_t* jack_get_descriptor()
    {
        jack_driver_desc_t* desc = ( jack_driver_desc_t* ) calloc ( 1, sizeof ( jack_driver_desc_t ) );
        
        strcpy(desc->name, "netadapter");                              // size MUST be less then JACK_DRIVER_NAME_MAX + 1
        strcpy(desc->desc, "netjack net <==> audio backend adapter");  // size MUST be less then JACK_DRIVER_PARAM_DESC + 1
       
        desc->nparams = 11;
        desc->params = ( jack_driver_param_desc_t* ) calloc ( desc->nparams, sizeof ( jack_driver_param_desc_t ) );

        int i = 0;
        strcpy ( desc->params[i].name, "multicast_ip" );
        desc->params[i].character = 'a';
        desc->params[i].type = JackDriverParamString;
        strcpy ( desc->params[i].value.str, DEFAULT_MULTICAST_IP );
        strcpy ( desc->params[i].short_desc, "Multicast Address" );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

        i++;
        strcpy ( desc->params[i].name, "udp_net_port" );
        desc->params[i].character = 'p';
        desc->params[i].type = JackDriverParamInt;
        desc->params[i].value.i = DEFAULT_PORT;
        strcpy ( desc->params[i].short_desc, "UDP port" );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

        i++;
        strcpy ( desc->params[i].name, "mtu" );
        desc->params[i].character = 'M';
        desc->params[i].type = JackDriverParamInt;
        desc->params[i].value.i = DEFAULT_MTU;
        strcpy ( desc->params[i].short_desc, "MTU to the master" );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

        i++;
        strcpy ( desc->params[i].name, "input-ports" );
        desc->params[i].character = 'C';
        desc->params[i].type = JackDriverParamInt;
        desc->params[i].value.i = 2;
        strcpy ( desc->params[i].short_desc, "Number of audio input ports" );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

        i++;
        strcpy ( desc->params[i].name, "output-ports" );
        desc->params[i].character = 'P';
        desc->params[i].type = JackDriverParamInt;
        desc->params[i].value.i = 2;
        strcpy ( desc->params[i].short_desc, "Number of audio output ports" );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

        i++;
        strcpy ( desc->params[i].name, "client-name" );
        desc->params[i].character = 'n';
        desc->params[i].type = JackDriverParamString;
        strcpy ( desc->params[i].value.str, "'hostname'" );
        strcpy ( desc->params[i].short_desc, "Name of the jack client" );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

        i++;
        strcpy ( desc->params[i].name, "transport-sync" );
        desc->params[i].character  = 't';
        desc->params[i].type = JackDriverParamUInt;
        desc->params[i].value.ui = 1U;
        strcpy ( desc->params[i].short_desc, "Sync transport with master's" );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

        i++;
        strcpy ( desc->params[i].name, "mode" );
        desc->params[i].character  = 'm';
        desc->params[i].type = JackDriverParamString;
        strcpy ( desc->params[i].value.str, "slow" );
        strcpy ( desc->params[i].short_desc, "Slow, Normal or Fast mode." );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );
        
        i++;
        strcpy(desc->params[i].name, "quality");
        desc->params[i].character = 'q';
        desc->params[i].type = JackDriverParamInt;
        desc->params[i].value.ui = 0;
        strcpy(desc->params[i].short_desc, "Resample algorithm quality (0 - 4)");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);
        
        i++;
        strcpy(desc->params[i].name, "ring-buffer");
        desc->params[i].character = 'g';
        desc->params[i].type = JackDriverParamInt;
        desc->params[i].value.ui = 32768;
        strcpy(desc->params[i].short_desc, "Fixed ringbuffer size");
        strcpy(desc->params[i].long_desc, "Fixed ringbuffer size (if not set => automatic adaptative)");
        
        i++;
        strcpy ( desc->params[i].name, "auto-connect" );
        desc->params[i].character = 'c';
        desc->params[i].type = JackDriverParamBool;
        desc->params[i].value.i = false;
        strcpy ( desc->params[i].short_desc, "Auto connect netmaster to system ports" );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );
    
        return desc;
    }

    SERVER_EXPORT int jack_internal_initialize ( jack_client_t* jack_client, const JSList* params )
    {
        jack_log ( "Loading netadapter" );

        Jack::JackAudioAdapter* adapter;
        jack_nframes_t buffer_size = jack_get_buffer_size ( jack_client );
        jack_nframes_t sample_rate = jack_get_sample_rate ( jack_client );
        
        try {
        
            adapter = new Jack::JackAudioAdapter(jack_client, new Jack::JackNetAdapter(jack_client, buffer_size, sample_rate, params), params, false);
            assert ( adapter );

            if ( adapter->Open() == 0 )
                return 0;
            else
            {
                delete adapter;
                return 1;
            }
            
        } catch (...) {
            return 1;
        }
    }

    SERVER_EXPORT int jack_initialize ( jack_client_t* jack_client, const char* load_init )
    {
        JSList* params = NULL;
        bool parse_params = true;
        int res = 1;
        jack_driver_desc_t* desc = jack_get_descriptor();

        Jack::JackArgParser parser ( load_init );
        if ( parser.GetArgc() > 0 )
            parse_params = parser.ParseParams ( desc, &params );

        if (parse_params) {
            res = jack_internal_initialize ( jack_client, params );
            parser.FreeParams ( params );
        }
        return res;
    }

    SERVER_EXPORT void jack_finish ( void* arg )
    {
        Jack::JackAudioAdapter* adapter = static_cast<Jack::JackAudioAdapter*> ( arg );

        if (adapter) {
            jack_log ( "Unloading netadapter" );
            adapter->Close();
            delete adapter;
        }
    }

#ifdef __cplusplus
}
#endif
