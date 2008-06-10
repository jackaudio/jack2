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

#ifndef __JackNetDriver__
#define __JackNetDriver__

#include "JackAudioDriver.h"
#include "JackNetTool.h"

namespace Jack
{

	class JackNetDriver : public JackAudioDriver
	{
		private:
			session_params_t fParams;
			char* fMulticastIP;
			unsigned int fPort;
			int fSockfd;
			struct sockaddr_in fMasterAddr;
			unsigned int fNSubProcess;

			jack_port_id_t* fMidiCapturePortList;
			jack_port_id_t* fMidiPlaybackPortList;

			packet_header_t fTxHeader;
			packet_header_t fRxHeader;

			char* fTxBuffer;
			char* fRxBuffer;
			char* fTxData;
			char* fRxData;

			NetMidiBuffer* fNetMidiCaptureBuffer;
			NetMidiBuffer* fNetMidiPlaybackBuffer;
			NetAudioBuffer* fNetAudioCaptureBuffer;
			NetAudioBuffer* fNetAudioPlaybackBuffer;

			int fAudioRxLen;
			int fAudioTxLen;

			bool Init();
			net_status_t GetNetMaster();
			net_status_t SendMasterStartSync();
			void Restart();
			int SetParams();
			int AllocPorts();
			int FreePorts();

			JackMidiBuffer* GetMidiInputBuffer ( int port_index );
			JackMidiBuffer* GetMidiOutputBuffer ( int port_index );

			int Recv ( unsigned int size, int flags );
			int Send ( unsigned int size, int flags );
			
		public:
			JackNetDriver ( const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table,
				const char* ip, unsigned int port, int midi_input_ports, int midi_output_ports, const char* master_name );
			~JackNetDriver();

			int Open ( jack_nframes_t frames_per_cycle, jack_nframes_t rate, bool capturing, bool playing,
			           int inchannels, int outchannels, bool monitor, const char* capture_driver_name,
			           const char* playback_driver_name, jack_nframes_t capture_latency, jack_nframes_t playback_latency );
            
            int Attach();
            int Detach();
            
            int Read();
            int Write();
	};
}

#endif
