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
#include "avb_1722avtp.h"


namespace Jack
{
/**
\Brief This class describes the Net Backend
*/

class JackAVBPDriver : public JackWaiterDriver
{
    private:

        ieee1722_avtp_driver_state_t ieee1722mc;
        int num_packets_even_odd;

    public:

        JackAVBPDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table,
                           char* stream_id, char* destination_mac, char* eth_dev,
                           int sample_rate, int period_size, int num_periods);
        virtual ~JackAVBPDriver();

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

#endif
