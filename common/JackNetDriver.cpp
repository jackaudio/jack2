/*
Copyright (C) 2001 Paul Davis
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

#include "JackNetDriver.h"
#include "JackEngineControl.h"
#include "JackClientControl.h"
#include "JackGraphManager.h"
#include "driver_interface.h"
#include "JackDriverLoader.h"
#include "JackThreadedDriver.h"
#include "JackWaitThreadedDriver.h"
#include "JackException.h"
#include "JackExports.h"

#define DEFAULT_MULTICAST_IP "225.3.19.154"
#define DEFAULT_PORT 19000

namespace Jack
{
	JackNetDriver::JackNetDriver ( const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table,
	                               const char* ip, size_t port, int midi_input_ports, int midi_output_ports, const char* net_name )
			: JackAudioDriver ( name, alias, engine, table )
	{
		fMulticastIP = new char[strlen ( ip ) + 1];
		strcpy ( fMulticastIP, ip );
		fPort = port;
		fParams.fSendMidiChannels = midi_input_ports;
		fParams.fReturnMidiChannels = midi_output_ports;
		strcpy ( fParams.fName, net_name );
		fSockfd = 0;
	}

	JackNetDriver::~JackNetDriver()
	{
		if ( fSockfd )
			close ( fSockfd );
		delete fNetAudioCaptureBuffer;
		delete fNetAudioPlaybackBuffer;
		delete fNetMidiCaptureBuffer;
		delete fNetMidiPlaybackBuffer;
		delete[] fTxBuffer;
		delete[] fRxBuffer;
		delete[] fMulticastIP;
		delete[] fMidiCapturePortList;
		delete[] fMidiPlaybackPortList;
	}

//*************************************initialization***********************************************************************

	int JackNetDriver::Open ( jack_nframes_t nframes, jack_nframes_t samplerate, bool capturing, bool playing,
	                          int inchannels, int outchannels, bool monitor,
	                          const char* capture_driver_name, const char* playback_driver_name,
	                          jack_nframes_t capture_latency, jack_nframes_t playback_latency )
	{
		int res = JackAudioDriver::Open ( nframes, samplerate, capturing, playing, inchannels, outchannels, monitor,
		                                  capture_driver_name, playback_driver_name, capture_latency, playback_latency );
		fEngineControl->fPeriod = 0;
		fEngineControl->fComputation = 500 * 1000;
		fEngineControl->fConstraint = 500 * 1000;
		return res;
	}

	int JackNetDriver::Attach()
	{
		return 0;
	}

	int JackNetDriver::Detach()
	{
		return 0;
	}
    
    int JackNetDriver::ProcessNull()
    {
        static unsigned int wait_time = (unsigned int)((float(fEngineControl->fBufferSize) / (float(fEngineControl->fSampleRate))) * 1000000.0f);
        usleep(wait_time);
        return JackAudioDriver::ProcessNull();
    }

	bool JackNetDriver::Init()
	{
		jack_log ( "JackNetDriver::Init()" );
		if ( fSockfd )
			Restart();

		//set the parameters to send
		strcpy ( fParams.fPacketType, "params" );
		fParams.fProtocolVersion = 'a';
		SetPacketType ( &fParams, SLAVE_AVAILABLE );
		gethostname ( fParams.fSlaveNetName, 255 );
		fParams.fSendAudioChannels = fCaptureChannels;
		fParams.fReturnAudioChannels = fPlaybackChannels;

		//init loop : get a master and start, do it until connection is ok
		net_status_t status;
		do
		{
			//first, get a master, do it until a valid connection is running
			jack_info ( "Initializing Net Driver..." );
			do
			{
				status = GetNetMaster();
				if ( status == SOCKET_ERROR )
					return false;
			}
			while ( status != CONNECTED );

			//then tell the master we are ready
			jack_info ( "Initializing connection with %s...", fParams.fMasterNetName );
			status = SendMasterStartSync();
			if ( status == NET_ERROR )
				return false;
		}
		while ( status != ROLLING );

		//driver parametering
		if ( SetParams() )
		{
			jack_error ( "Fatal error : can't alloc net driver ports." );
			return false;
		}

		//init done, display parameters
		SessionParamsDisplay ( &fParams );

		return true;
	}

	net_status_t JackNetDriver::GetNetMaster()
	{
		jack_log ( "JackNetDriver::GetNetMaster()" );
		//utility
		session_params_t params;
		struct sockaddr_in mcast_addr, listen_addr;
		struct timeval rcv_timeout;
		rcv_timeout.tv_sec = 2;
		rcv_timeout.tv_usec = 0;
		socklen_t addr_len = sizeof ( socket_address_t );
		int rx_bytes = 0;

		//set the multicast address
		mcast_addr.sin_family = AF_INET;
		mcast_addr.sin_port = htons ( fPort );
		inet_aton ( fMulticastIP, &mcast_addr.sin_addr );
		memset ( &mcast_addr.sin_zero, 0, 8 );

		//set the listening address
		listen_addr.sin_family = AF_INET;
		listen_addr.sin_port = htons ( fPort );
		listen_addr.sin_addr.s_addr = htonl ( INADDR_ANY );
		memset ( &listen_addr.sin_zero, 0, 8 );

		//set the master address family
		fMasterAddr.sin_family = AF_INET;

		//socket
		if ( fSockfd )
			close ( fSockfd );
		if ( ( fSockfd = socket ( AF_INET, SOCK_DGRAM, 0 ) ) < 0 )
		{
			jack_error ( "Fatal error : network unreachable - %s", strerror ( errno ) );
			return SOCKET_ERROR;
		}

		//bind the socket
		if ( bind ( fSockfd, reinterpret_cast<socket_address_t*> ( &listen_addr ), addr_len )  < 0 )
			jack_error ( "Can't bind the socket : %s", strerror ( errno ) );

		//timeout on receive
		setsockopt ( fSockfd, SOL_SOCKET, SO_RCVTIMEO, &rcv_timeout, sizeof ( rcv_timeout ) );

		//send 'AVAILABLE' until 'SLAVE_SETUP' received
		jack_info ( "Waiting for a master..." );
		do
		{
			//send 'available'
			if ( sendto ( fSockfd, &fParams, sizeof ( session_params_t ), MSG_DONTWAIT,
			              reinterpret_cast<socket_address_t*> ( &mcast_addr ), addr_len ) < 0 )
				jack_error ( "Error in data send : %s", strerror ( errno ) );
			//filter incoming packets : don't exit while receiving wrong packets
			do
			{
				rx_bytes = recvfrom ( fSockfd, &params, sizeof ( session_params_t ), 0,
				                      reinterpret_cast<socket_address_t*> ( &fMasterAddr ), &addr_len );
				if ( ( rx_bytes < 0 ) && ( errno != EAGAIN ) )
				{
					jack_error ( "Can't receive : %s", strerror ( errno ) );
					return RECV_ERROR;
				}
			}
			while ( ( rx_bytes > 0 )  && strcmp ( params.fPacketType, fParams.fPacketType ) );
		}
		while ( ( GetPacketType ( &params ) != SLAVE_SETUP ) );

		//connect the socket
		if ( connect ( fSockfd, reinterpret_cast<socket_address_t*> ( &fMasterAddr ), sizeof ( socket_address_t ) )  < 0 )
		{
			jack_error ( "Error in connect : %s", strerror ( errno ) );
			return CONNECT_ERROR;
		}

		//everything is OK, copy parameters and return
		fParams = params;

		return CONNECTED;
	}

	net_status_t JackNetDriver::SendMasterStartSync()
	{
		jack_log ( "JackNetDriver::GetNetMasterStartSync()" );
		//tell the master to start
		SetPacketType ( &fParams, START_MASTER );
		if ( send ( fSockfd, &fParams, sizeof ( session_params_t ), MSG_DONTWAIT ) < 0 )
		{
			jack_error ( "Error in send : %s", strerror ( errno ) );
			return ( ( errno == ECONNABORTED ) || ( errno == ECONNREFUSED ) || ( errno == ECONNRESET ) ) ? NET_ERROR : SEND_ERROR;
		}
		return ROLLING;
	}

	void JackNetDriver::Restart()
	{
		jack_info ( "Restarting driver..." );
		close ( fSockfd );
		delete[] fTxBuffer;
		delete[] fRxBuffer;
		delete fNetAudioCaptureBuffer;
		delete fNetAudioPlaybackBuffer;
		delete fNetMidiCaptureBuffer;
		delete fNetMidiPlaybackBuffer;
		FreePorts();
		delete[] fMidiCapturePortList;
		delete[] fMidiPlaybackPortList;
		fTxBuffer = NULL;
		fRxBuffer = NULL;
		fNetAudioCaptureBuffer = NULL;
		fNetAudioPlaybackBuffer = NULL;
		fNetMidiCaptureBuffer = NULL;
		fNetMidiPlaybackBuffer = NULL;
		fMidiCapturePortList = NULL;
		fMidiPlaybackPortList = NULL;
	}

	int JackNetDriver::SetParams()
	{
		fNSubProcess = fParams.fPeriodSize / fParams.fFramesPerPacket;
		SetBufferSize ( fParams.fPeriodSize );
		SetSampleRate ( fParams.fSampleRate );

		//allocate midi ports lists
		fMidiCapturePortList = new jack_port_id_t [fParams.fSendMidiChannels];
		fMidiPlaybackPortList = new jack_port_id_t [fParams.fReturnMidiChannels];

		//register jack ports
		if ( AllocPorts() != 0 )
		{
			jack_error ( "Can't allocate ports." );
			return -1;
		}

		//TX header init
		strcpy ( fTxHeader.fPacketType, "header" );
		fTxHeader.fDataStream = 'r';
		fTxHeader.fID = fParams.fID;
		fTxHeader.fCycle = 0;
		fTxHeader.fSubCycle = 0;
		fTxHeader.fMidiDataSize = 0;
		fTxHeader.fBitdepth = fParams.fBitdepth;

		//RX header init
		strcpy ( fRxHeader.fPacketType, "header" );
		fRxHeader.fDataStream = 's';
		fRxHeader.fID = fParams.fID;
		fRxHeader.fCycle = 0;
		fRxHeader.fSubCycle = 0;
		fRxHeader.fMidiDataSize = 0;
		fRxHeader.fBitdepth = fParams.fBitdepth;

		//network buffers
		fTxBuffer = new char[fParams.fMtu];
		fRxBuffer = new char[fParams.fMtu];

		//net audio/midi buffers
		fTxData = fTxBuffer + sizeof ( packet_header_t );
		fRxData = fRxBuffer + sizeof ( packet_header_t );

		//midi net buffers
		fNetMidiCaptureBuffer = new NetMidiBuffer ( &fParams, fParams.fSendMidiChannels, fRxData );
		fNetMidiPlaybackBuffer = new NetMidiBuffer ( &fParams, fParams.fReturnMidiChannels, fTxData );

		//audio net buffers
		fNetAudioCaptureBuffer = new NetAudioBuffer ( &fParams, fParams.fSendAudioChannels, fRxData );
		fNetAudioPlaybackBuffer = new NetAudioBuffer ( &fParams, fParams.fReturnAudioChannels, fTxData );

		//audio netbuffer length
		fAudioTxLen = sizeof ( packet_header_t ) + fNetAudioPlaybackBuffer->GetSize();
		fAudioRxLen = sizeof ( packet_header_t ) + fNetAudioCaptureBuffer->GetSize();

		return 0;
	}

	int JackNetDriver::AllocPorts()
	{
		jack_log ( "JackNetDriver::AllocPorts fBufferSize = %ld fSampleRate = %ld", fEngineControl->fBufferSize, fEngineControl->fSampleRate );
		JackPort* port;
		jack_port_id_t port_id;
		char name[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
		char alias[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
		unsigned long port_flags;

		//audio
		port_flags = JackPortIsOutput | JackPortIsPhysical | JackPortIsTerminal;
		for ( int port_index = 0; port_index < fCaptureChannels; port_index++ )
		{
			snprintf ( alias, sizeof ( alias ) - 1, "%s:%s:out%d", fAliasName, fCaptureDriverName, port_index + 1 );
			snprintf ( name, sizeof ( name ) - 1, "%s:capture_%d", fClientControl->fName, port_index + 1 );
			if ( ( port_id = fGraphManager->AllocatePort ( fClientControl->fRefNum, name, JACK_DEFAULT_AUDIO_TYPE,
			                 static_cast<JackPortFlags> ( port_flags ), fEngineControl->fBufferSize ) ) == NO_PORT )
			{
				jack_error ( "driver: cannot register port for %s", name );
				return -1;
			}
			port = fGraphManager->GetPort ( port_id );
			port->SetAlias ( alias );
			port->SetLatency ( fEngineControl->fBufferSize + fCaptureLatency );
			fCapturePortList[port_index] = port_id;
			jack_log ( "JackNetDriver::AllocPorts() fCapturePortList[%d] port_index = %ld", port_index, port_id );
		}
		port_flags = JackPortIsInput | JackPortIsPhysical | JackPortIsTerminal;
		for ( int port_index = 0; port_index < fPlaybackChannels; port_index++ )
		{
			snprintf ( alias, sizeof ( alias ) - 1, "%s:%s:in%d", fAliasName, fPlaybackDriverName, port_index + 1 );
			snprintf ( name, sizeof ( name ) - 1, "%s:playback_%d",fClientControl->fName, port_index + 1 );
			if ( ( port_id = fGraphManager->AllocatePort ( fClientControl->fRefNum, name, JACK_DEFAULT_AUDIO_TYPE,
			                 static_cast<JackPortFlags> ( port_flags ), fEngineControl->fBufferSize ) ) == NO_PORT )
			{
				jack_error ( "driver: cannot register port for %s", name );
				return -1;
			}
			port = fGraphManager->GetPort ( port_id );
			port->SetAlias ( alias );
			port->SetLatency ( fEngineControl->fBufferSize + ( ( fEngineControl->fSyncMode ) ? 0 : fEngineControl->fBufferSize ) + fPlaybackLatency );
			fPlaybackPortList[port_index] = port_id;
			jack_log ( "JackNetDriver::AllocPorts() fPlaybackPortList[%d] port_index = %ld", port_index, port_id );
		}
		//midi
		port_flags = JackPortIsOutput | JackPortIsPhysical | JackPortIsTerminal;
		for ( int port_index = 0; port_index < fParams.fSendMidiChannels; port_index++ )
		{
			snprintf ( alias, sizeof ( alias ) - 1, "%s:%s:out%d", fAliasName, fCaptureDriverName, port_index + 1 );
			snprintf ( name, sizeof ( name ) - 1, "%s:midi_capture_%d", fClientControl->fName, port_index + 1 );
			if ( ( port_id = fGraphManager->AllocatePort ( fClientControl->fRefNum, name, JACK_DEFAULT_MIDI_TYPE,
			                 static_cast<JackPortFlags> ( port_flags ), fEngineControl->fBufferSize ) ) == NO_PORT )
			{
				jack_error ( "driver: cannot register port for %s", name );
				return -1;
			}
			fMidiCapturePortList[port_index] = port_id;
			jack_log ( "JackNetDriver::AllocPorts() fMidiCapturePortList[%d] port_index = %ld", port_index, port_id );
		}

		port_flags = JackPortIsInput | JackPortIsPhysical | JackPortIsTerminal;
		for ( int port_index = 0; port_index < fParams.fReturnMidiChannels; port_index++ )
		{
			snprintf ( alias, sizeof ( alias ) - 1, "%s:%s:in%d", fAliasName, fPlaybackDriverName, port_index + 1 );
			snprintf ( name, sizeof ( name ) - 1, "%s:midi_playback_%d", fClientControl->fName, port_index + 1 );
			if ( ( port_id = fGraphManager->AllocatePort ( fClientControl->fRefNum, name, JACK_DEFAULT_MIDI_TYPE,
			                 static_cast<JackPortFlags> ( port_flags ), fEngineControl->fBufferSize ) ) == NO_PORT )
			{
				jack_error ( "driver: cannot register port for %s", name );
				return -1;
			}
			fMidiPlaybackPortList[port_index] = port_id;
			jack_log ( "JackNetDriver::AllocPorts() fMidiPlaybackPortList[%d] port_index = %ld", port_index, port_id );
		}

		return 0;
	}

	int JackNetDriver::FreePorts()
	{
		jack_log ( "JackNetDriver::FreePorts" );
		for ( int port_index = 0; port_index < fCaptureChannels; port_index++ )
			fGraphManager->ReleasePort ( fClientControl->fRefNum, fCapturePortList[port_index] );
		for ( int port_index = 0; port_index < fPlaybackChannels; port_index++ )
			fGraphManager->ReleasePort ( fClientControl->fRefNum, fPlaybackPortList[port_index] );
		for ( int port_index = 0; port_index < fParams.fSendMidiChannels; port_index++ )
			fGraphManager->ReleasePort ( fClientControl->fRefNum, fMidiCapturePortList[port_index] );
		for ( int port_index = 0; port_index < fParams.fReturnMidiChannels; port_index++ )
			fGraphManager->ReleasePort ( fClientControl->fRefNum, fMidiPlaybackPortList[port_index] );
		return 0;
	}

	JackMidiBuffer* JackNetDriver::GetMidiInputBuffer ( int port_index )
	{
		return static_cast<JackMidiBuffer*> ( fGraphManager->GetBuffer ( fMidiCapturePortList[port_index], fEngineControl->fBufferSize ) );
	}

	JackMidiBuffer* JackNetDriver::GetMidiOutputBuffer ( int port_index )
	{
		return static_cast<JackMidiBuffer*> ( fGraphManager->GetBuffer ( fMidiPlaybackPortList[port_index], fEngineControl->fBufferSize ) );
	}

	int JackNetDriver::Recv ( size_t size, int flags )
	{
		int rx_bytes;
		if ( ( rx_bytes = recv ( fSockfd, fRxBuffer, size, flags ) ) < 0 )
		{
			if ( errno == EAGAIN )
			{
				jack_error ( "No incoming data, is the master still running ?" );
				return 0;
			}
			else if ( ( errno == ECONNABORTED ) || ( errno == ECONNREFUSED ) || ( errno == ECONNRESET ) )
			{
				jack_error ( "Fatal error : %s.", strerror ( errno ) );
				throw JackDriverException();
			}
			else
			{
				jack_error ( "Error in receive : %s", strerror ( errno ) );
				return 0;
			}
		}
		return rx_bytes;
	}

	int JackNetDriver::Send ( size_t size, int flags )
	{
		int tx_bytes;
		if ( ( tx_bytes = send ( fSockfd, fTxBuffer, size, flags ) ) < 0 )
		{
			if ( ( errno == ECONNABORTED ) || ( errno == ECONNREFUSED ) || ( errno == ECONNRESET ) )
			{
				jack_error ( "Fatal error : %s.", strerror ( errno ) );
				throw JackDriverException();
			}
			else
				jack_error ( "Error in send : %s", strerror ( errno ) );
		}
		return tx_bytes;
	}

//*************************************process************************************************************************

	int JackNetDriver::Read()
	{
		int rx_bytes;
		size_t recvd_midi_pckt = 0;
		packet_header_t* rx_head = reinterpret_cast<packet_header_t*> ( fRxBuffer );
		fRxHeader.fIsLastPckt = 'n';

		//buffers
		for ( int port_index = 0; port_index < fParams.fSendMidiChannels; port_index++ )
			fNetMidiCaptureBuffer->fPortBuffer[port_index] = GetMidiInputBuffer ( port_index );
		for ( int port_index = 0; port_index < fCaptureChannels; port_index++ )
			fNetAudioCaptureBuffer->fPortBuffer[port_index] = GetInputBuffer ( port_index );

		//receive sync (launch the cycle)
		do
		{
			if ( ( rx_bytes = Recv ( sizeof ( packet_header_t ), 0 ) ) < 1 )
				return rx_bytes;
		}
		while ( !rx_bytes && ( rx_head->fDataType != 's' ) );

		JackDriver::CycleTakeTime();

		//audio, midi or sync if driver is late
		if ( fParams.fSendMidiChannels || fParams.fSendAudioChannels )
		{
			do
			{
				rx_bytes = Recv ( fParams.fMtu, MSG_PEEK );
				if ( rx_bytes < 1 )
					return rx_bytes;
				if ( rx_bytes && ( rx_head->fDataStream == 's' ) && ( rx_head->fID == fParams.fID ) )
				{
					switch ( rx_head->fDataType )
					{
						case 'm':	//midi
							rx_bytes = Recv ( rx_bytes, MSG_DONTWAIT );
							fRxHeader.fIsLastPckt = rx_head->fIsLastPckt;
							fNetMidiCaptureBuffer->RenderFromNetwork ( rx_head->fSubCycle, rx_bytes - sizeof ( packet_header_t ) );
							if ( ++recvd_midi_pckt == rx_head->fNMidiPckt )
								fNetMidiCaptureBuffer->RenderToJackPorts();
							break;
						case 'a':	//audio
							rx_bytes = Recv ( fAudioRxLen, MSG_DONTWAIT );
							if ( !IsNextPacket ( &fRxHeader, rx_head, fNSubProcess ) )
								jack_error ( "Packet(s) missing..." );
							fRxHeader.fCycle = rx_head->fCycle;
							fRxHeader.fSubCycle = rx_head->fSubCycle;
							fRxHeader.fIsLastPckt = rx_head->fIsLastPckt;
							fNetAudioCaptureBuffer->RenderToJackPorts ( rx_head->fSubCycle );
							break;
						case 's':	//sync
							jack_info ( "NetDriver : driver overloaded, skipping receive." );
							fRxHeader.fCycle = rx_head->fCycle;
							return 0;
					}
				}
			}
			while ( fRxHeader.fIsLastPckt != 'y' );
		}
		fRxHeader.fCycle = rx_head->fCycle;
		return 0;
	}

	int JackNetDriver::Write()
	{
		int tx_bytes, copy_size;
		fTxHeader.fCycle = fRxHeader.fCycle;
		fTxHeader.fSubCycle = 0;
		fTxHeader.fIsLastPckt = 'n';

		//buffers
		for ( int port_index = 0; port_index < fParams.fReturnMidiChannels; port_index++ )
			fNetMidiPlaybackBuffer->fPortBuffer[port_index] = GetMidiOutputBuffer ( port_index );
		for ( int port_index = 0; port_index < fPlaybackChannels; port_index++ )
			fNetAudioPlaybackBuffer->fPortBuffer[port_index] = GetOutputBuffer ( port_index );

		//midi
		if ( fParams.fReturnMidiChannels )
		{
			fTxHeader.fDataType = 'm';
			fTxHeader.fMidiDataSize = fNetMidiPlaybackBuffer->RenderFromJackPorts();
			fTxHeader.fNMidiPckt = GetNMidiPckt ( &fParams, fTxHeader.fMidiDataSize );
			for ( size_t subproc = 0; subproc < fTxHeader.fNMidiPckt; subproc++ )
			{
				fTxHeader.fSubCycle = subproc;
				if ( ( subproc == ( fTxHeader.fNMidiPckt - 1 ) ) && !fParams.fReturnAudioChannels )
					fTxHeader.fIsLastPckt = 'y';
				memcpy ( fTxBuffer, &fTxHeader, sizeof ( packet_header_t ) );
				copy_size = fNetMidiPlaybackBuffer->RenderToNetwork ( subproc, fTxHeader.fMidiDataSize );
				tx_bytes = Send ( sizeof ( packet_header_t ) + copy_size, MSG_DONTWAIT );
			}
		}

		//audio
		if ( fParams.fReturnAudioChannels )
		{
			fTxHeader.fDataType = 'a';
			for ( size_t subproc = 0; subproc < fNSubProcess; subproc++ )
			{
				fTxHeader.fSubCycle = subproc;
				if ( subproc == ( fNSubProcess - 1 ) )
					fTxHeader.fIsLastPckt = 'y';
				fNetAudioPlaybackBuffer->RenderFromJackPorts ( subproc );
				memcpy ( fTxBuffer, &fTxHeader, sizeof ( packet_header_t ) );
				tx_bytes = Send ( fAudioTxLen, MSG_DONTWAIT );
			}
		}
		return 0;
	}

//*************************************loader*******************************************************

#ifdef __cplusplus
	extern "C"
	{
#endif
		EXPORT jack_driver_desc_t* driver_get_descriptor ()
		{
			jack_driver_desc_t* desc = ( jack_driver_desc_t* ) calloc ( 1, sizeof ( jack_driver_desc_t ) );
			strcpy ( desc->name, "net" );
			desc->nparams = 7;
			desc->params = ( jack_driver_param_desc_t* ) calloc ( desc->nparams, sizeof ( jack_driver_param_desc_t ) );

			size_t i = 0;
			strcpy ( desc->params[i].name, "multicast_ip" );
			desc->params[i].character = 'a';
			desc->params[i].type = JackDriverParamString;
			strcpy ( desc->params[i].value.str, DEFAULT_MULTICAST_IP );
			strcpy ( desc->params[i].short_desc, "Multicast Address" );
			strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

			i++;
			strcpy ( desc->params[i].name, "udp_net_port" );
			desc->params[i].character = 'p';
			desc->params[i].type = JackDriverParamUInt;
			desc->params[i].value.ui = 19000U;
			strcpy ( desc->params[i].short_desc, "UDP port" );
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
			desc->params[i].type = JackDriverParamUInt;
			desc->params[i].value.i = 2;
			strcpy ( desc->params[i].short_desc, "Number of audio output ports" );
			strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

			i++;
			strcpy ( desc->params[i].name, "midi_in_ports" );
			desc->params[i].character = 'i';
			desc->params[i].type = JackDriverParamInt;
			desc->params[i].value.i = 0;
			strcpy ( desc->params[i].short_desc, "Number of midi input ports" );
			strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

			i++;
			strcpy ( desc->params[i].name, "midi_out_ports" );
			desc->params[i].character = 'o';
			desc->params[i].type = JackDriverParamUInt;
			desc->params[i].value.i = 0;
			strcpy ( desc->params[i].short_desc, "Number of midi output ports" );
			strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

			i++;
			strcpy ( desc->params[i].name, "client_name" );
			desc->params[i].character = 'n';
			desc->params[i].type = JackDriverParamString;
			strcpy ( desc->params[i].value.str, "'hostname'" );
			strcpy ( desc->params[i].short_desc, "Name of the jack client" );
			strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

			return desc;
		}

		EXPORT Jack::JackDriverClientInterface* driver_initialize ( Jack::JackLockedEngine* engine, Jack::JackSynchro* table, const JSList* params )
		{
			const char* multicast_ip = DEFAULT_MULTICAST_IP;
			char name[JACK_CLIENT_NAME_SIZE];
			gethostname ( name, JACK_CLIENT_NAME_SIZE );
			jack_nframes_t udp_port = DEFAULT_PORT;
			jack_nframes_t period_size = 128;
			jack_nframes_t sample_rate = 48000;
			int audio_capture_ports = 2;
			int audio_playback_ports = 2;
			int midi_input_ports = 0;
			int midi_output_ports = 0;
			bool monitor = false;

			const JSList* node;
			const jack_driver_param_t* param;
			for ( node = params; node; node = jack_slist_next ( node ) )
			{
				param = ( const jack_driver_param_t* ) node->data;
				switch ( param->character )
				{
					case 'a' :
						multicast_ip = strdup ( param->value.str );
						break;
					case 'p':
						udp_port = param->value.ui;
						break;
					case 'C':
						audio_capture_ports = param->value.i;
						break;
					case 'P':
						audio_playback_ports = param->value.i;
						break;
					case 'i':
						midi_input_ports = param->value.i;
						break;
					case 'o':
						midi_output_ports = param->value.i;
						break;
					case 'n' :
						strncpy ( name, param->value.str, JACK_CLIENT_NAME_SIZE );
				}
			}
       
            Jack::JackDriverClientInterface* driver = new Jack::JackWaitThreadedDriver(
			    new Jack::JackNetDriver("system", "net_pcm", engine, table, multicast_ip, udp_port, midi_input_ports, midi_output_ports, name));
            if (driver->Open (period_size, sample_rate, 1, 1, audio_capture_ports, audio_playback_ports,
			                    monitor, "from_master_", "to_master_", 0, 0 ) == 0)
				return driver;

			delete driver;
			return NULL;
		}

#ifdef __cplusplus
	}
#endif
}
