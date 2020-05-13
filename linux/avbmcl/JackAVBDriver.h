/*
Copyright (C) 2016-2019 Christoph Kuhr

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

#ifndef _JACK_AVB_DRIVER_H_
#define _JACK_AVB_DRIVER_H_

#include "JackTimedDriver.h"
#include "avb.h"

namespace Jack
{

// Brief This class describes the AVB Backend
class JackAVBDriver : public JackWaiterDriver
{
    private:
        avb_driver_state_t 	avb_ctx;
        int 				num_packets_even_odd;

    public:
        JackAVBDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table,
                           char* stream_id, char* destination_mac, char* eth_dev,
                           int sample_rate, int period_size, int num_periods,
                           int adjust, int capture_ports, int playback_ports);
        virtual ~JackAVBDriver();

        int Close();
        int Attach(){return 0;}
        int Detach(){return 0;}

        int Read();
        int Write();

        bool Initialize();
        int AllocPorts();
        void FreePorts();

        // BufferSize can't be changed
        bool IsFixedBufferSize(){return true;}
        int SetBufferSize(jack_nframes_t buffer_size){return -1;}
        int SetSampleRate(jack_nframes_t sample_rate){return -1;}
};

}

#endif //_JACK_AVB_DRIVER_H_
