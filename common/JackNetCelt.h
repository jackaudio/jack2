/*
Copyright (C) 2008-2011 Romain Moret at Grame

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

#ifndef __JackNetCelt__
#define __JackNetCelt__

#include "JackNetTool.h"

#include <celt/celt.h>

namespace Jack
{
    class SERVER_EXPORT NetCeltAudioBuffer : public NetAudioBuffer
    {
        private:

            CELTMode** fCeltMode;
            CELTEncoder** fCeltEncoder;
            CELTDecoder** fCeltDecoder;

            int fCompressedSizeByte;
            unsigned char** fCompressedBuffer;

            size_t fLastSubPeriodBytesSize;

            void FreeCelt();

        public:

            NetCeltAudioBuffer(session_params_t* params, uint32_t nports, char* net_buffer, int kbps);
            virtual ~NetCeltAudioBuffer();

            // needed size in bytes for an entire cycle
            size_t GetCycleSize();

             // cycle duration in sec
            float GetCycleDuration();
            int GetNumPackets(int active_ports);

            //jack<->buffer
            int RenderFromJackPorts(int nframes);
            void RenderToJackPorts(int nframes);

            //network<->buffer
            int RenderFromNetwork(int cycle, int sub_cycle, uint32_t port_num);
            int RenderToNetwork(int sub_cycle, uint32_t port_num);
    };
}

#endif
