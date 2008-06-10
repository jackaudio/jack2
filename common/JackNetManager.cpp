/***************************************************************************
*   Copyright (C) 2008 by Romain Moret   *
*   moret@grame.fr   *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#include "JackNetManager.h"
#include "JackError.h"
#include "JackExports.h"

#define DEFAULT_MULTICAST_IP "225.3.19.154"
#define DEFAULT_PORT 19000

using namespace std;

namespace Jack
{
//JackNetMaster******************************************************************************************************

	JackNetMaster::JackNetMaster ( JackNetMasterManager* manager, session_params_t& params, struct sockaddr_in& address, struct sockaddr_in& mcast_addr )
	{
		jack_log ( "JackNetMaster::JackNetMaster" );
		//settings
		fMasterManager = manager;
		fParams = params;
		fAddr = address;
		fMcastAddr = mcast_addr;
		fNSubProcess = fParams.fPeriodSize / fParams.fFramesPerPacket;
		fClientName = const_cast<char*> ( fParams.fName );
		fNetJumpCnt = 0;
		fJackClient = NULL;
		fSockfd = 0;
		fRunning = false;

		//jack audio ports
		fAudioCapturePorts = new jack_port_t* [fParams.fSendAudioChannels];
		for ( int port_index = 0; port_index < fParams.fSendAudioChannels; port_index++ )
			fAudioCapturePorts[port_index] = NULL;
		fAudioPlaybackPorts = new jack_port_t* [fParams.fReturnAudioChannels];
		for ( int port_index = 0; port_index < fParams.fReturnAudioChannels; port_index++ )
			fAudioPlaybackPorts[port_index] = NULL;
		//jack midi ports
		fMidiCapturePorts = new jack_port_t* [fParams.fSendMidiChannels];
		for ( int port_index = 0; port_index < fParams.fSendMidiChannels; port_index++ )
			fMidiCapturePorts[port_index] = NULL;
		fMidiPlaybackPorts = new jack_port_t* [fParams.fReturnMidiChannels];
		for ( int port_index = 0; port_index < fParams.fReturnMidiChannels; port_index++ )
			fMidiPlaybackPorts[port_index] = NULL;

		//TX header init
		strcpy ( fTxHeader.fPacketType, "header" );
		fTxHeader.fDataStream = 's';
		fTxHeader.fID = fParams.fID;
		fTxHeader.fCycle = 0;
		fTxHeader.fSubCycle = 0;
		fTxHeader.fMidiDataSize = 0;
		fTxHeader.fBitdepth = fParams.fBitdepth;

		//RX header init
		strcpy ( fRxHeader.fPacketType, "header" );
		fRxHeader.fDataStream = 'r';
		fRxHeader.fID = fParams.fID;
		fRxHeader.fCycle = 0;
		fRxHeader.fSubCycle = 0;
		fRxHeader.fMidiDataSize = 0;
		fRxHeader.fBitdepth = fParams.fBitdepth;

		//network buffers
		fTxBuffer = new char [fParams.fMtu];
		fRxBuffer = new char [fParams.fMtu];

		//net audio buffers
		fTxData = fTxBuffer + sizeof ( packet_header_t );
		fRxData = fRxBuffer + sizeof ( packet_header_t );

		//midi net buffers
		fNetMidiCaptureBuffer = new NetMidiBuffer ( &fParams, fParams.fSendMidiChannels, fTxData );
		fNetMidiPlaybackBuffer = new NetMidiBuffer ( &fParams, fParams.fReturnMidiChannels, fRxData );

		//audio net buffers
		fNetAudioCaptureBuffer = new NetAudioBuffer ( &fParams, fParams.fSendAudioChannels, fTxData );
		fNetAudioPlaybackBuffer = new NetAudioBuffer ( &fParams, fParams.fReturnAudioChannels, fRxData );

		//audio netbuffer length
		fAudioTxLen = sizeof ( packet_header_t ) + fNetAudioCaptureBuffer->GetSize();
		fAudioRxLen = sizeof ( packet_header_t ) + fNetAudioPlaybackBuffer->GetSize();
	}

	JackNetMaster::~JackNetMaster()
	{
		jack_log ( "JackNetMaster::~JackNetMaster, ID %u.", fParams.fID );
		if ( fJackClient )
		{
			jack_deactivate ( fJackClient );
			FreePorts();
			jack_client_close ( fJackClient );
		}
		if ( fSockfd )
			close ( fSockfd );
		delete fNetAudioCaptureBuffer;
		delete fNetAudioPlaybackBuffer;
		delete fNetMidiCaptureBuffer;
		delete fNetMidiPlaybackBuffer;
		delete[] fAudioCapturePorts;
		delete[] fAudioPlaybackPorts;
		delete[] fMidiCapturePorts;
		delete[] fMidiPlaybackPorts;
		delete[] fTxBuffer;
		delete[] fRxBuffer;
	}

	bool JackNetMaster::Init()
	{
		jack_log ( "JackNetMaster::Init, ID %u.", fParams.fID );
		session_params_t params;
		struct timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		size_t attempt = 0;
		int rx_bytes = 0;

		//socket
		if ( ( fSockfd = socket ( AF_INET, SOCK_DGRAM, 0 ) ) < 0 )
		{
			jack_error ( "Can't create socket : %s", strerror ( errno ) );
			return false;
		}

		//timeout on receive (for init)
		if ( setsockopt ( fSockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof ( timeout ) ) < 0 )
			jack_error ( "Can't set timeout : %s", strerror ( errno ) );

		//connect
		if ( connect ( fSockfd, reinterpret_cast<sockaddr*> ( &fAddr ), sizeof ( struct sockaddr ) ) < 0 )
		{
			jack_error ( "Can't connect : %s", strerror ( errno ) );
			return false;
		}

		//send 'SLAVE_SETUP' until 'START_MASTER' received
		jack_info ( "Sending parameters to %s ...", fParams.fSlaveNetName );
		do
		{
			SetPacketType ( &fParams, SLAVE_SETUP );
			if ( send ( fSockfd, &fParams, sizeof ( session_params_t ), 0 ) < 0 )
				jack_error ( "Error in send : ", strerror ( errno ) );
			if ( ( ( rx_bytes = recv ( fSockfd, &params, sizeof ( session_params_t ), 0 ) ) < 0 ) && ( errno != EAGAIN ) )
			{
				jack_error ( "Problem with network." );
				return false;
			}
		}
		while ( ( GetPacketType ( &params ) != START_MASTER ) && ( ++attempt < 5 ) );
		if ( attempt == 5 )
		{
			jack_error ( "Slave doesn't respond, exiting." );
			return false;
		}

		//set the new timeout for the socket
		if ( SetRxTimeout ( &fSockfd, &fParams ) < 0 )
		{
			jack_error ( "Can't set rx timeout : %s", strerror ( errno ) );
			return false;
		}

		//jack client and process
		jack_status_t status;
		jack_options_t options = JackNullOption;
		if ( ( fJackClient = jack_client_open ( fClientName, options, &status, NULL ) ) == NULL )
		{
			jack_error ( "Can't open a new jack client." );
			return false;
		}

		jack_set_process_callback ( fJackClient, SetProcess, this );

		//port registering
		int i;
		char name[24];
		//audio
		for ( i = 0; i < fParams.fSendAudioChannels; i++ )
		{
			sprintf ( name, "to_slave_%d", i+1 );
			if ( ( fAudioCapturePorts[i] = jack_port_register ( fJackClient, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0 ) ) == NULL )
				goto fail;
		}
		for ( i = 0; i < fParams.fReturnAudioChannels; i++ )
		{
			sprintf ( name, "from_slave_%d", i+1 );
			if ( ( fAudioPlaybackPorts[i] = jack_port_register ( fJackClient, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 ) ) == NULL )
				goto fail;
		}
		//midi
		for ( i = 0; i < fParams.fSendMidiChannels; i++ )
		{
			sprintf ( name, "midi_to_slave_%d", i+1 );
			if ( ( fMidiCapturePorts[i] = jack_port_register ( fJackClient, name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0 ) ) == NULL )
				goto fail;
		}
		for ( i = 0; i < fParams.fReturnMidiChannels; i++ )
		{
			sprintf ( name, "midi_from_slave_%d", i+1 );
			if ( ( fMidiPlaybackPorts[i] = jack_port_register ( fJackClient, name, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0 ) ) == NULL )
				goto fail;
		}

		fRunning = true;

		//finally activate jack client
		if ( jack_activate ( fJackClient ) != 0 )
		{
			jack_error ( "Can't activate jack client." );
			goto fail;
		}

		jack_info ( "NetJack new master started." );

		return true;

	fail:
		FreePorts();
		jack_client_close ( fJackClient );
		fJackClient = NULL;
		return false;
	}

	void JackNetMaster::FreePorts()
	{
		jack_log ( "JackNetMaster::FreePorts, ID %u", fParams.fID );
		for ( int port_index = 0; port_index < fParams.fSendAudioChannels; port_index++ )
			if ( fAudioCapturePorts[port_index] )
				jack_port_unregister ( fJackClient, fAudioCapturePorts[port_index] );
		for ( int port_index = 0; port_index < fParams.fReturnAudioChannels; port_index++ )
			if ( fAudioPlaybackPorts[port_index] )
				jack_port_unregister ( fJackClient, fAudioPlaybackPorts[port_index] );
		for ( int port_index = 0; port_index < fParams.fSendMidiChannels; port_index++ )
			if ( fMidiCapturePorts[port_index] )
				jack_port_unregister ( fJackClient, fMidiCapturePorts[port_index] );
		for ( int port_index = 0; port_index < fParams.fReturnMidiChannels; port_index++ )
			if ( fMidiPlaybackPorts[port_index] )
				jack_port_unregister ( fJackClient, fMidiPlaybackPorts[port_index] );
	}

	void JackNetMaster::Exit()
	{
		jack_log ( "JackNetMaster::Exit, ID %u", fParams.fID );
		//stop process
		fRunning = false;
		//send a 'multicast euthanasia request' - new socket is required on macosx
		jack_info ( "Exiting '%s'", fParams.fName );
		SetPacketType ( &fParams, KILL_MASTER );
		int mcast_sockfd = socket ( AF_INET, SOCK_DGRAM, 0 );
		if ( mcast_sockfd < 0 )
			jack_error ( "Can't create socket : %s", strerror ( errno ) );
		if ( sendto ( mcast_sockfd, &fParams, sizeof ( session_params_t ), 0,
		              reinterpret_cast<socket_address_t*> ( &fMcastAddr ), sizeof ( socket_address_t ) ) < 0 )
			jack_error ( "Can't send suicide request : %s", strerror ( errno ) );
		close ( mcast_sockfd );
	}

	int JackNetMaster::Send ( char* buffer, size_t size, int flags )
	{
		int tx_bytes;
		if ( ( tx_bytes = send ( fSockfd, buffer, size, flags ) ) < 0 )
		{
			if ( ( errno == ECONNABORTED ) || ( errno == ECONNREFUSED ) || ( errno == ECONNRESET ) )
			{
				//fatal connection issue, exit
				jack_error ( "'%s' : %s, please check network connection with '%s'.",
				             fParams.fName, strerror ( errno ), fParams.fSlaveNetName );
				Exit();
				return 0;
			}
			else
				jack_error ( "Error in send : %s", strerror ( errno ) );
		}
		return tx_bytes;
	}

	int JackNetMaster::Recv ( size_t size, int flags )
	{
		int rx_bytes;
		if ( ( rx_bytes = recv ( fSockfd, fRxBuffer, size, flags ) ) < 0 )
		{
			if ( errno == EAGAIN )
			{
				//too much receive failure, react...
				if ( ++fNetJumpCnt == 100 )
				{
					jack_error ( "Connection lost, is %s still running ?", fParams.fName );
					fNetJumpCnt = 0;
				}
				return 0;
			}
			else if ( ( errno == ECONNABORTED ) || ( errno == ECONNREFUSED ) || ( errno == ECONNRESET ) )
			{
				//fatal connection issue, exit
				jack_error ( "'%s' : %s, please check network connection with '%s'.",
				             fParams.fName, strerror ( errno ), fParams.fSlaveNetName );
				Exit();
				return 0;
			}
			else if ( errno != EAGAIN )
				jack_error ( "Error in receive : %s", strerror ( errno ) );
		}
		return rx_bytes;
	}

	int JackNetMaster::SetProcess ( jack_nframes_t nframes, void* arg )
	{
		JackNetMaster* master = static_cast<JackNetMaster*> ( arg );	;
		return master->Process();
	}

	int JackNetMaster::Process()
	{
		if ( !fRunning )
			return 0;

		int tx_bytes, rx_bytes, copy_size;
		size_t midi_recvd_pckt = 0;
		fTxHeader.fCycle++;
		fTxHeader.fSubCycle = 0;
		fTxHeader.fIsLastPckt = 'n';
		packet_header_t* rx_head = reinterpret_cast<packet_header_t*> ( fRxBuffer );

		//buffers
		for ( int port_index = 0; port_index < fParams.fSendMidiChannels; port_index++ )
			fNetMidiCaptureBuffer->fPortBuffer[port_index] =
			    static_cast<JackMidiBuffer*> ( jack_port_get_buffer ( fMidiCapturePorts[port_index], fParams.fPeriodSize ) );
		for ( int port_index = 0; port_index < fParams.fSendAudioChannels; port_index++ )
			fNetAudioCaptureBuffer->fPortBuffer[port_index] =
			    static_cast<sample_t*> ( jack_port_get_buffer ( fAudioCapturePorts[port_index], fParams.fPeriodSize ) );
		for ( int port_index = 0; port_index < fParams.fReturnMidiChannels; port_index++ )
			fNetMidiPlaybackBuffer->fPortBuffer[port_index] =
			    static_cast<JackMidiBuffer*> ( jack_port_get_buffer ( fMidiPlaybackPorts[port_index], fParams.fPeriodSize ) );
		for ( int port_index = 0; port_index < fParams.fReturnAudioChannels; port_index++ )
			fNetAudioPlaybackBuffer->fPortBuffer[port_index] =
			    static_cast<sample_t*> ( jack_port_get_buffer ( fAudioPlaybackPorts[port_index], fParams.fPeriodSize ) );

		//send ------------------------------------------------------------------------------------------------------------------
		//sync
		fTxHeader.fDataType = 's';
		if ( !fParams.fSendMidiChannels && !fParams.fSendAudioChannels )
			fTxHeader.fIsLastPckt = 'y';
		tx_bytes = Send ( reinterpret_cast<char*> ( &fTxHeader ), sizeof ( packet_header_t ), 0 );
		if ( tx_bytes < 1 )
			return tx_bytes;

		//midi
		if ( fParams.fSendMidiChannels )
		{
			fTxHeader.fDataType = 'm';
			fTxHeader.fMidiDataSize = fNetMidiCaptureBuffer->RenderFromJackPorts();
			fTxHeader.fNMidiPckt = GetNMidiPckt ( &fParams, fTxHeader.fMidiDataSize );
			for ( size_t subproc = 0; subproc < fTxHeader.fNMidiPckt; subproc++ )
			{
				fTxHeader.fSubCycle = subproc;
				if ( ( subproc == ( fTxHeader.fNMidiPckt - 1 ) ) && !fParams.fSendAudioChannels )
					fTxHeader.fIsLastPckt = 'y';
				memcpy ( fTxBuffer, &fTxHeader, sizeof ( packet_header_t ) );
				copy_size = fNetMidiCaptureBuffer->RenderToNetwork ( subproc, fTxHeader.fMidiDataSize );
				tx_bytes = Send ( fTxBuffer, sizeof ( packet_header_t ) + copy_size, 0 );
				if ( tx_bytes < 1 )
					return tx_bytes;
			}
		}

		//audio
		if ( fParams.fSendAudioChannels )
		{
			fTxHeader.fDataType = 'a';
			for ( size_t subproc = 0; subproc < fNSubProcess; subproc++ )
			{
				fTxHeader.fSubCycle = subproc;
				if ( subproc == ( fNSubProcess - 1 ) )
					fTxHeader.fIsLastPckt = 'y';
				memcpy ( fTxBuffer, &fTxHeader, sizeof ( packet_header_t ) );
				fNetAudioCaptureBuffer->RenderFromJackPorts ( subproc );
				tx_bytes = Send ( fTxBuffer, fAudioTxLen, 0 );
				if ( tx_bytes < 1 )
					return tx_bytes;
			}
		}

		//receive ( if there is stg to receive...)-------------------------------------------------------------------------------------
		if ( fParams.fReturnMidiChannels || fParams.fReturnAudioChannels )
		{
			do
			{
				rx_bytes = Recv ( fParams.fMtu, MSG_PEEK );
				if ( rx_bytes < 1 )
					return rx_bytes;
				if ( rx_bytes && ( rx_head->fDataStream == 'r' ) && ( rx_head->fID == fParams.fID ) )
				{
					switch ( rx_head->fDataType )
					{
						case 'm':	//midi
							rx_bytes = Recv ( rx_bytes, MSG_DONTWAIT );
							fRxHeader.fIsLastPckt = rx_head->fIsLastPckt;
							fNetMidiPlaybackBuffer->RenderFromNetwork ( rx_head->fSubCycle, rx_bytes - sizeof ( packet_header_t ) );
							if ( ++midi_recvd_pckt == rx_head->fNMidiPckt )
								fNetMidiPlaybackBuffer->RenderToJackPorts();
							fNetJumpCnt = 0;
							break;
						case 'a':	//audio
							rx_bytes = Recv ( fAudioRxLen, MSG_DONTWAIT );
							if ( !IsNextPacket ( &fRxHeader, rx_head, fNSubProcess ) )
								jack_error ( "Packet(s) missing from '%s'...", fParams.fName );
							fRxHeader.fCycle = rx_head->fCycle;
							fRxHeader.fSubCycle = rx_head->fSubCycle;
							fRxHeader.fIsLastPckt = rx_head->fIsLastPckt;
							fNetAudioPlaybackBuffer->RenderToJackPorts ( rx_head->fSubCycle );
							fNetJumpCnt = 0;
							break;
					}
				}
			}
			while ( fRxHeader.fIsLastPckt != 'y' );
		}
		return 0;
	}

//JackNetMasterManager***********************************************************************************************

	JackNetMasterManager::JackNetMasterManager ( jack_client_t* client )
	{
		jack_log ( "JackNetMasterManager::JackNetMasterManager" );
		fManagerClient = client;
		fManagerName = jack_get_client_name ( fManagerClient );
		fMCastIP = DEFAULT_MULTICAST_IP;
		fPort = DEFAULT_PORT;
		fGlobalID = 0;
		fRunning = true;

		//launch the manager thread
		if ( jack_client_create_thread ( fManagerClient, &fManagerThread, 0, 0, NetManagerThread, this ) )
			jack_error ( "Can't create the network manager control thread." );
	}

	JackNetMasterManager::~JackNetMasterManager()
	{
		jack_log ( "JackNetMasterManager::~JackNetMasterManager" );
		Exit();
		master_list_t::iterator it;
		for ( it = fMasterList.begin(); it != fMasterList.end(); it++ )
			delete ( *it );
	}

	void* JackNetMasterManager::NetManagerThread ( void* arg )
	{
		jack_info ( "Starting Jack Network Manager." );
		JackNetMasterManager* master_manager = static_cast<JackNetMasterManager*> ( arg );
		master_manager->Run();
		return NULL;
	}

	void JackNetMasterManager::Run()
	{
		jack_log ( "JackNetMasterManager::Run" );
		//utility variables
		socklen_t addr_len = sizeof ( socket_address_t );
		char disable = 0;
		struct timeval timeout;
		timeout.tv_sec = 2;
		timeout.tv_usec = 0;
		size_t attempt = 0;

		//network
		int mcast_sockfd;
		struct ip_mreq multicast_req;
		struct sockaddr_in mcast_addr;
		struct sockaddr_in response_addr;

		//data
		session_params_t params;
		int rx_bytes = 0;
		JackNetMaster* net_master;

		//socket
		if ( ( mcast_sockfd = socket ( AF_INET, SOCK_DGRAM, 0 ) ) < 0 )
		{
			jack_error ( "Can't create the network management input socket : %s", strerror ( errno ) );
			return;
		}

		//set the multicast address
		mcast_addr.sin_family = AF_INET;
		mcast_addr.sin_port = htons ( fPort );
		if ( inet_aton ( fMCastIP, &mcast_addr.sin_addr ) < 0 )
		{
			jack_error ( "Cant set multicast address : %s", strerror ( errno ) );
			close ( mcast_sockfd );
			return;
		}
		memset ( &mcast_addr.sin_zero, 0, 8 );

		//bind the socket to the multicast address
		if ( bind ( mcast_sockfd, reinterpret_cast<socket_address_t *> ( &mcast_addr ), addr_len ) < 0 )
		{
			jack_error ( "Can't bind the network manager socket : %s", strerror ( errno ) );
			close ( mcast_sockfd );
			return;
		}

		//join multicast group
		inet_aton ( fMCastIP, &multicast_req.imr_multiaddr );
		multicast_req.imr_interface.s_addr = htonl ( INADDR_ANY );
		if ( setsockopt ( mcast_sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &multicast_req, sizeof ( multicast_req ) ) < 0 )
		{
			jack_error ( "Can't join multicast group : %s", strerror ( errno ) );
			close ( mcast_sockfd );
			return;
		}

		//disable local loop
		if ( setsockopt ( mcast_sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &disable, sizeof ( disable ) ) < 0 )
			jack_error ( "Can't set multicast loop option : %s", strerror ( errno ) );

		//set a timeout on the multicast receive (the thread can now be cancelled)
		if ( setsockopt ( mcast_sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof ( timeout ) ) < 0 )
			jack_error ( "Can't set timeout : %s", strerror ( errno ) );

		jack_info ( "Waiting for a slave..." );

		//main loop, wait for data, deal with it and wait again
		do
		{
			rx_bytes = recvfrom ( mcast_sockfd, &params, sizeof ( session_params_t ), 0,
			                    reinterpret_cast<socket_address_t*> ( &response_addr ), &addr_len );
			if ( ( rx_bytes < 0 ) && ( errno != EAGAIN ) )
			{
				jack_error ( "Error in receive : %s", strerror ( errno ) );
				if ( ++attempt == 10 )
				{
					jack_error ( "Can't receive on the socket, exiting net manager." );
					return;
				}
			}
			if ( rx_bytes == sizeof ( session_params_t ) )
			{
				switch ( GetPacketType ( &params ) )
				{
					case SLAVE_AVAILABLE:
						if ( ( net_master = MasterInit ( params, response_addr, mcast_addr ) ) )
							SessionParamsDisplay ( &net_master->fParams );
						else
							jack_error ( "Can't init new net master..." );
						jack_info ( "Waiting for a slave..." );
						break;
					case KILL_MASTER:
						KillMaster ( &params );
						jack_info ( "Waiting for a slave..." );
						break;
					default:
						break;
				}
			}
		}
		while ( fRunning );
		close ( mcast_sockfd );
	}

	void JackNetMasterManager::Exit()
	{
		jack_log ( "JackNetMasterManager::Exit" );
		fRunning = false;
		pthread_join ( fManagerThread, NULL );
		jack_info ( "Exiting net manager..." );
	}

	JackNetMaster* JackNetMasterManager::MasterInit ( session_params_t& params, struct sockaddr_in& address, struct sockaddr_in& mcast_addr )
	{
		jack_log ( "JackNetMasterManager::MasterInit, Slave : %s", params.fName );
		//settings
		gethostname ( params.fMasterNetName, 255 );
		params.fMtu = 1500;
		params.fID = ++fGlobalID;
		params.fSampleRate = jack_get_sample_rate ( fManagerClient );
		params.fPeriodSize = jack_get_buffer_size ( fManagerClient );
		params.fBitdepth = 0;
		SetFramesPerPacket ( &params );
		SetSlaveName ( params );

		//create a new master and add it to the list
		JackNetMaster* master = new JackNetMaster ( this, params, address, mcast_addr );
		if ( master->Init() )
		{
			fMasterList.push_back ( master );
			return master;
		}
		delete master;
		return NULL;
	}

	void JackNetMasterManager::SetSlaveName ( session_params_t& params )
	{
		jack_log ( "JackNetMasterManager::SetSlaveName" );
		master_list_it_t it;
		for ( it = fMasterList.begin(); it != fMasterList.end(); it++ )
			if ( strcmp ( ( *it )->fParams.fName, params.fName ) == 0 )
				sprintf ( params.fName, "%s-%u", params.fName, params.fID );
	}

	master_list_it_t JackNetMasterManager::FindMaster ( size_t id )
	{
		jack_log ( "JackNetMasterManager::FindMaster, ID %u.", id );
		master_list_it_t it;
		for ( it = fMasterList.begin(); it != fMasterList.end(); it++ )
			if ( ( *it )->fParams.fID == id )
				return it;
		return it;
	}

	void JackNetMasterManager::KillMaster ( session_params_t* params )
	{
		jack_log ( "JackNetMasterManager::KillMaster, ID %u.", params->fID );
		master_list_it_t master = FindMaster ( params->fID );
		if ( master != fMasterList.end() )
		{
			fMasterList.erase ( master );
			delete *master;
		}
	}
}//namespace

static Jack::JackNetMasterManager* master_manager = NULL;

#ifdef __cplusplus
extern "C"
{
#endif
	EXPORT int jack_initialize ( jack_client_t* jack_client, const char* load_init )
	{
		if ( master_manager )
		{
			jack_error ( "Master Manager already loaded" );
			return 1;
		}
		else
		{
			jack_log ( "Loading Master Manager" );
			master_manager = new Jack::JackNetMasterManager ( jack_client );
			return ( master_manager ) ? 0 : 1;
		}
	}

	EXPORT void jack_finish ( void* arg )
	{
		if ( master_manager )
		{
			jack_log ( "Unloading Master Manager" );
			delete master_manager;
			master_manager = NULL;
		}
	}
#ifdef __cplusplus
}
#endif
