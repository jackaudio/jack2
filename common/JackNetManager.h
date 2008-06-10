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

#ifndef __JACKNETMASTER_H__
#define __JACKNETMASTER_H__

#include "JackNetTool.h"
#include <list>
#include <algorithm>
#include "thread.h"
#include <unistd.h>
#include "jack.h"

namespace Jack
{
	class JackNetMasterManager;

	class JackNetMaster
	{
			friend class JackNetMasterManager;
		private:
			static int SetProcess ( jack_nframes_t nframes, void* arg );

			JackNetMasterManager* fMasterManager;
			session_params_t fParams;
			struct sockaddr_in fAddr;
			struct sockaddr_in fMcastAddr;
			int fSockfd;
			unsigned int fNSubProcess;
			unsigned int fNetJumpCnt;
			bool fRunning;

			jack_client_t* fJackClient;
			const char* fClientName;

			jack_port_t** fAudioCapturePorts;
			jack_port_t** fAudioPlaybackPorts;
			jack_port_t** fMidiCapturePorts;
			jack_port_t** fMidiPlaybackPorts;

			packet_header_t fTxHeader;
			packet_header_t fRxHeader;

			char* fTxBuffer;
			char* fRxBuffer;
			char* fTxData;
			char* fRxData;

			NetAudioBuffer* fNetAudioCaptureBuffer;
			NetAudioBuffer* fNetAudioPlaybackBuffer;
			NetMidiBuffer* fNetMidiCaptureBuffer;
			NetMidiBuffer* fNetMidiPlaybackBuffer;

			int fAudioTxLen;
			int fAudioRxLen;

			bool Init();
			void FreePorts();
			void Exit();

			int Send ( char* buffer, unsigned int size, int flags );
			int Recv ( unsigned int size, int flags );
			int Process();
		public:
			JackNetMaster ( JackNetMasterManager* manager, session_params_t& params, struct sockaddr_in& address, struct sockaddr_in& mcast_addr );
			~JackNetMaster ();
	};

	typedef std::list<JackNetMaster*> master_list_t;
	typedef master_list_t::iterator master_list_it_t;

	class JackNetMasterManager
	{
		private:
			static void* NetManagerThread ( void* arg );
			static int SetProcess ( jack_nframes_t nframes, void* arg );

			jack_client_t* fManagerClient;
			const char* fManagerName;
			const char* fMCastIP;
			pthread_t fManagerThread;
			master_list_t fMasterList;
			unsigned int fGlobalID;
			bool fRunning;
			unsigned int fPort;

			void Run();
			JackNetMaster* MasterInit ( session_params_t& params, struct sockaddr_in& address, struct sockaddr_in& mcast_addr );
			master_list_it_t FindMaster ( unsigned int client_id );
			void KillMaster ( session_params_t* params );
			void SetSlaveName ( session_params_t& params );
			int Process();
		public:
			JackNetMasterManager ( jack_client_t* jack_client );
			~JackNetMasterManager();

			void Exit();
	};
}

#endif
