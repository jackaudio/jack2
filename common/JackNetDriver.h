/*
Copyright (C) 2001 Paul Davis
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
        JackNetSocket fSocket;
        uint fNSubProcess;
        net_transport_data_t fTransportData;

        //jack ports
        jack_port_id_t* fMidiCapturePortList;
        jack_port_id_t* fMidiPlaybackPortList;

        //headers
        packet_header_t fTxHeader;
        packet_header_t fRxHeader;

        //network buffers
        char* fTxBuffer;
        char* fRxBuffer;
        char* fTxData;
        char* fRxData;

        //jack buffers
        NetMidiBuffer* fNetMidiCaptureBuffer;
        NetMidiBuffer* fNetMidiPlaybackBuffer;
        NetAudioBuffer* fNetAudioCaptureBuffer;
        NetAudioBuffer* fNetAudioPlaybackBuffer;

        //sizes
        int fAudioRxLen;
        int fAudioTxLen;
        int fPayloadSize;

        //monitoring
#ifdef JACK_MONITOR
        static uint fMeasureCnt;
        static uint fMeasurePoints;
        static uint fMonitorPlotOptionsCnt;
        static std::string fMonitorPlotOptions[];
        static std::string fMonitorFieldNames[];
        jack_time_t* fMeasure;
        NetMonitor<jack_time_t>* fMonitor;
        jack_time_t fUsecCycleStart;
#endif

        bool Init();
        net_status_t GetNetMaster();
        net_status_t SendMasterStartSync();
        void Restart();
        int SetParams();
        int AllocPorts();
        int FreePorts();

        JackMidiBuffer* GetMidiInputBuffer ( int port_index );
        JackMidiBuffer* GetMidiOutputBuffer ( int port_index );

        int Recv ( size_t size, int flags );
        int Send ( size_t size, int flags );

        int SetSyncPacket();
        int TransportSync();

    public:
        JackNetDriver ( const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table,
                        const char* ip, int port, int mtu, int midi_input_ports, int midi_output_ports, const char* master_name, uint transport_sync );
        ~JackNetDriver();

        int Open ( jack_nframes_t frames_per_cycle, jack_nframes_t rate, bool capturing, bool playing,
                   int inchannels, int outchannels, bool monitor, const char* capture_driver_name,
                   const char* playback_driver_name, jack_nframes_t capture_latency, jack_nframes_t playback_latency );

#ifdef JACK_MONITOR
		int Close();
#endif

        int Attach();
        int Detach();

        int Read();
        int Write();

        // BufferSize can be changed
        bool IsFixedBufferSize()
        {
            return true;
        }

        int SetBufferSize(jack_nframes_t buffer_size)
        {
            return -1;
        }

        int SetSampleRate(jack_nframes_t sample_rate)
        {
            return -1;
        }

    };
}

#endif
