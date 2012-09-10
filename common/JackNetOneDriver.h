/*
Copyright (C) 2008-2011 Torben Horn

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

#include "JackTimedDriver.h"
#include "netjack.h"
#include "netjack_packet.h"

namespace Jack
{
/**
\Brief This class describes the Net Backend
*/

class JackNetOneDriver : public JackWaiterDriver
{
    private:

        netjack_driver_state_t netj;

        void
        render_payload_to_jack_ports_float(void *packet_payload, jack_nframes_t net_period_down, JSList *capture_ports, JSList *capture_srcs, jack_nframes_t nframes, int dont_htonl_floats);
        void
        render_jack_ports_to_payload_float(JSList *playback_ports, JSList *playback_srcs, jack_nframes_t nframes, void *packet_payload, jack_nframes_t net_period_up, int dont_htonl_floats );
#if HAVE_CELT
        void
        render_payload_to_jack_ports_celt(void *packet_payload, jack_nframes_t net_period_down, JSList *capture_ports, JSList *capture_srcs, jack_nframes_t nframes);
        void
        render_jack_ports_to_payload_celt(JSList *playback_ports, JSList *playback_srcs, jack_nframes_t nframes, void *packet_payload, jack_nframes_t net_period_up);
#endif
#if HAVE_OPUS
        void
        render_payload_to_jack_ports_opus(void *packet_payload, jack_nframes_t net_period_down, JSList *capture_ports, JSList *capture_srcs, jack_nframes_t nframes);
        void
        render_jack_ports_to_payload_opus(JSList *playback_ports, JSList *playback_srcs, jack_nframes_t nframes, void *packet_payload, jack_nframes_t net_period_up);
#endif
        void
        render_payload_to_jack_ports(int bitdepth, void *packet_payload, jack_nframes_t net_period_down, JSList *capture_ports, JSList *capture_srcs, jack_nframes_t nframes, int dont_htonl_floats);
        void
        render_jack_ports_to_payload(int bitdepth, JSList *playback_ports, JSList *playback_srcs, jack_nframes_t nframes, void *packet_payload, jack_nframes_t net_period_up, int dont_htonl_floats);

    public:

        JackNetOneDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table,
                           int port, int mtu, int capture_ports, int playback_ports, int midi_input_ports, int midi_output_ports,
                           int sample_rate, int period_size, int resample_factor,
                           const char* net_name, uint transport_sync, int bitdepth, int use_autoconfig,
                           int latency, int redundancy, int dont_htonl_floats, int always_deadline, int jitter_val);
        virtual ~JackNetOneDriver();

        int Close();
        int Attach();
        int Detach();

        int Read();
        int Write();

        bool Initialize();
        int AllocPorts();
        void FreePorts();

        // BufferSize can't be changed
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
