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

#include "JackNetAdapter.h"

#define DEFAULT_MULTICAST_IP "225.3.19.154"
#define DEFAULT_PORT 19000

namespace Jack
{

    JackNetAdapter::JackNetAdapter ( jack_nframes_t buffer_size, jack_nframes_t sample_rate, const JSList* params )
            :JackAudioAdapterInterface(buffer_size, sample_rate), fThread(this)
    {
        jack_log ( "JackNetAdapter::JackNetAdapter" );

        if ( SocketAPIInit() < 0 )
            jack_error ( "Can't init Socket API, exiting..." );

		//global parametering
        fMulticastIP = new char[16];
        strcpy ( fMulticastIP, DEFAULT_MULTICAST_IP );
        fSocket.SetPort ( DEFAULT_PORT );
        GetHostName ( fParams.fName, JACK_CLIENT_NAME_SIZE );
        fSocket.GetName ( fParams.fSlaveNetName );
        fParams.fMtu = 1500;
        fParams.fTransportSync = 1;
        fParams.fSendAudioChannels = 2;
        fParams.fReturnAudioChannels = 2;
        fParams.fSendMidiChannels = 0;
        fParams.fReturnMidiChannels = 0;
        fParams.fSampleRate = 48000;
        fParams.fPeriodSize = 128;
        fParams.fSlaveSyncMode = 0;
        fParams.fNetworkMode = 'n';

		//options parsing
        const JSList* node;
        const jack_driver_param_t* param;
        for ( node = params; node; node = jack_slist_next ( node ) )
        {
            param = ( const jack_driver_param_t* ) node->data;
            switch ( param->character )
            {
            case 'a' :
                fMulticastIP = strdup ( param->value.str );
                break;
            case 'p':
                fSocket.SetPort ( param->value.ui );
                break;
            case 'M':
                fParams.fMtu = param->value.i;
                break;
            case 'C':
                fParams.fSendAudioChannels = param->value.i;
                break;
            case 'P':
                fParams.fReturnAudioChannels = param->value.i;
                break;
            case 'n' :
                strncpy ( fParams.fName, param->value.str, JACK_CLIENT_NAME_SIZE );
                break;
            case 't' :
                fParams.fTransportSync = param->value.ui;
                break;
            case 'f' :
                fParams.fNetworkMode = 'f';
                break;
			case 'S' :
				fParams.fSlaveSyncMode = 1;
            }
        }

        fCaptureChannels = fParams.fSendAudioChannels;
        fPlaybackChannels = fParams.fReturnAudioChannels;
        fSoftCaptureBuffer = NULL;
        fSoftPlaybackBuffer = NULL;
    }

    JackNetAdapter::~JackNetAdapter()
    {
    	int port_index;
    	for ( port_index = 0; port_index < fCaptureChannels; port_index++ )
			delete[] fSoftCaptureBuffer[port_index];
		delete[] fSoftCaptureBuffer;
    	for ( port_index = 0; port_index < fPlaybackChannels; port_index++ )
			delete[] fSoftPlaybackBuffer[port_index];
		delete[] fSoftPlaybackBuffer;
	}

    int JackNetAdapter::Open()
    {
		jack_log ( "JackNetAdapter::Open()" );

		int port_index;

		//display some additional infos
		jack_info ( "NetAdapter started in %s mode %s Master's transport sync.",
					( fParams.fSlaveSyncMode ) ? "sync" : "async", ( fParams.fTransportSync ) ? "with" : "without" );

		//init network connection
		if ( !JackNetSlaveInterface::Init() )
			return -1;

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

		fThread.AcquireRealTime(85);
		return fThread.StartSync();
    }

    int JackNetAdapter::Close()
    {
        fSocket.Close();
        return 0;
    }

    int JackNetAdapter::SetBufferSize ( jack_nframes_t buffer_size )
    {
    	fParams.fPeriodSize = buffer_size;
        return 0;
    }

    bool JackNetAdapter::Init()
    {
    	jack_info ( "Starting NetAdapter." );
        return true;
    }

    bool JackNetAdapter::Execute()
    {
		switch ( fParams.fSlaveSyncMode )
		{
			case true :
				return ProcessSync();
			case false :
				return ProcessAsync();
		}
		return true;
    }

    bool JackNetAdapter::ProcessSync()
    {
    	bool failure = false;
    	int port_index;

		//receive
    	if ( SyncRecv() == SOCKET_ERROR )
			return true;

		if ( DataRecv() == SOCKET_ERROR )
			return false;

		//resample
		jack_nframes_t time1, time2;
		ResampleFactor ( time1, time2 );

		for ( port_index = 0; port_index < fCaptureChannels; port_index++ )
		{
			fCaptureRingBuffer[port_index]->SetRatio ( time1, time2 );
			if ( fCaptureRingBuffer[port_index]->WriteResample ( fSoftCaptureBuffer[port_index], fBufferSize ) < fBufferSize )
				failure = true;
		}

		for ( port_index = 0; port_index < fPlaybackChannels; port_index++ )
		{
			fPlaybackRingBuffer[port_index]->SetRatio ( time2, time1 );
			if ( fPlaybackRingBuffer[port_index]->ReadResample ( fSoftPlaybackBuffer[port_index], fBufferSize ) < fBufferSize )
				failure = true;
		}

		//send
		if ( SyncSend() == SOCKET_ERROR )
			return false;

		if ( failure )
		{
			jack_error ( "JackNetAdapter::Execute ringbuffer failure...reset." );
			ResetRingBuffers();
		}

		return true;
    }

    bool JackNetAdapter::ProcessAsync()
    {
    	bool failure = false;
    	int port_index;

		//receive
    	if ( SyncRecv() == SOCKET_ERROR )
			return true;

		if ( DataRecv() == SOCKET_ERROR )
			return false;

		//send
		if ( SyncSend() == SOCKET_ERROR )
			return false;

		if ( failure )
		{
			jack_error ( "JackNetAdapter::Execute ringbuffer failure...reset." );
			ResetRingBuffers();
		}

		//resample
		jack_nframes_t time1, time2;
		ResampleFactor ( time1, time2 );

		for ( port_index = 0; port_index < fCaptureChannels; port_index++ )
		{
			fCaptureRingBuffer[port_index]->SetRatio ( time1, time2 );
			if ( fCaptureRingBuffer[port_index]->WriteResample ( fSoftCaptureBuffer[port_index], fBufferSize ) < fBufferSize )
				failure = true;
		}

		for ( port_index = 0; port_index < fPlaybackChannels; port_index++ )
		{
			fPlaybackRingBuffer[port_index]->SetRatio ( time2, time1 );
			if ( fPlaybackRingBuffer[port_index]->ReadResample ( fSoftPlaybackBuffer[port_index], fBufferSize ) < fBufferSize )
				failure = true;
		}

		return true;
    }

} // namespace Jack

#ifdef __cplusplus
extern "C"
{
#endif

#include "driver_interface.h"
#include "JackAudioAdapter.h"

    using namespace Jack;

    EXPORT jack_driver_desc_t* jack_get_descriptor()
    {
        jack_driver_desc_t* desc = ( jack_driver_desc_t* ) calloc ( 1, sizeof ( jack_driver_desc_t ) );
        strcpy ( desc->name, "net" );
        desc->nparams = 9;
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
        desc->params[i].value.i = 19000;
        strcpy ( desc->params[i].short_desc, "UDP port" );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

        i++;
        strcpy ( desc->params[i].name, "mtu" );
        desc->params[i].character = 'M';
        desc->params[i].type = JackDriverParamInt;
        desc->params[i].value.i = 1500;
        strcpy ( desc->params[i].short_desc, "MTU to the master" );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

        i++;
        strcpy ( desc->params[i].name, "input_ports" );
        desc->params[i].character = 'C';
        desc->params[i].type = JackDriverParamInt;
        desc->params[i].value.i = 2;
        strcpy ( desc->params[i].short_desc, "Number of audio input ports" );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

        i++;
        strcpy ( desc->params[i].name, "output_ports" );
        desc->params[i].character = 'P';
        desc->params[i].type = JackDriverParamInt;
        desc->params[i].value.i = 2;
        strcpy ( desc->params[i].short_desc, "Number of audio output ports" );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

        i++;
        strcpy ( desc->params[i].name, "client_name" );
        desc->params[i].character = 'n';
        desc->params[i].type = JackDriverParamString;
        strcpy ( desc->params[i].value.str, "'hostname'" );
        strcpy ( desc->params[i].short_desc, "Name of the jack client" );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

        i++;
        strcpy ( desc->params[i].name, "transport_sync" );
        desc->params[i].character  = 't';
        desc->params[i].type = JackDriverParamUInt;
        desc->params[i].value.ui = 1U;
        strcpy ( desc->params[i].short_desc, "Sync transport with master's" );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

        i++;
        strcpy ( desc->params[i].name, "fast_mode" );
        desc->params[i].character  = 'f';
        desc->params[i].type = JackDriverParamString;
        strcpy ( desc->params[i].value.str, "" );
        strcpy ( desc->params[i].short_desc, "Fast mode allows a zero latency transmission." );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

        i++;
        strcpy ( desc->params[i].name, "sync_mode" );
        desc->params[i].character  = 'S';
        desc->params[i].type = JackDriverParamString;
        strcpy ( desc->params[i].value.str, "" );
        strcpy ( desc->params[i].short_desc, "Sync mode (same as driver's sync mode) ?" );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

        return desc;
    }

    EXPORT int jack_internal_initialize ( jack_client_t* jack_client, const JSList* params )
    {
        jack_log ( "Loading netadapter" );

        Jack::JackAudioAdapter* adapter;
        jack_nframes_t buffer_size = jack_get_buffer_size ( jack_client );
        jack_nframes_t sample_rate = jack_get_sample_rate ( jack_client );

        adapter = new Jack::JackAudioAdapter ( jack_client, new Jack::JackNetAdapter ( buffer_size, sample_rate, params ) );
        assert ( adapter );

        if ( adapter->Open() == 0 )
        {
            return 0;
        }
        else
        {
            delete adapter;
            return 1;
        }
    }

    EXPORT int jack_initialize ( jack_client_t* jack_client, const char* load_init )
    {
        JSList* params = NULL;
        jack_driver_desc_t *desc = jack_get_descriptor();

        JackArgParser parser ( load_init );

        if ( parser.GetArgc() > 0 )
        {
            if ( parser.ParseParams ( desc, &params ) != 0 )
                jack_error ( "Internal client : JackArgParser::ParseParams error." );
        }

        return jack_internal_initialize ( jack_client, params );
    }

    EXPORT void jack_finish ( void* arg )
    {
        Jack::JackAudioAdapter* adapter = static_cast<Jack::JackAudioAdapter*> ( arg );

        if ( adapter )
        {
            jack_log ( "Unloading netadapter" );
            adapter->Close();
            delete adapter;
        }
    }

#ifdef __cplusplus
}
#endif
