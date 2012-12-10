/*
Copyright (C) 2001 Paul Davis
Copyright (C) 2004-2008 Grame

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#ifndef __JackPort__
#define __JackPort__

#include "types.h"
#include "JackConstants.h"
#include "JackCompilerDeps.h"

namespace Jack
{

#define ALL_PORTS	0xFFFF
#define NO_PORT		0xFFFE

/*!
\brief Base class for port.
*/

PRE_PACKED_STRUCTURE
class SERVER_EXPORT JackPort
{

        friend class JackGraphManager;

    private:

        int fTypeId;
        enum JackPortFlags fFlags;
        char fName[REAL_JACK_PORT_NAME_SIZE];
        char fAlias1[REAL_JACK_PORT_NAME_SIZE];
        char fAlias2[REAL_JACK_PORT_NAME_SIZE];
        int fRefNum;

        jack_nframes_t fLatency;
        jack_nframes_t fTotalLatency;
        jack_latency_range_t  fPlaybackLatency;
        jack_latency_range_t  fCaptureLatency;
        uint8_t fMonitorRequests;

        bool fInUse;
        jack_port_id_t fTied;   // Locally tied source port
        jack_default_audio_sample_t fBuffer[BUFFER_SIZE_MAX + 8];

        bool IsUsed() const
        {
            return fInUse;
        }

        // RT
        void ClearBuffer(jack_nframes_t frames);
        void MixBuffers(void** src_buffers, int src_count, jack_nframes_t frames);

    public:

        JackPort();

        bool Allocate(int refnum, const char* port_name, const char* port_type, JackPortFlags flags);
        void Release();
        const char* GetName() const;
        const char* GetShortName() const;
        void SetName(const char* name);

        int GetAliases(char* const aliases[2]);
        int SetAlias(const char* alias);
        int UnsetAlias(const char* alias);
        bool NameEquals(const char* target);

        int	GetFlags() const;
        const char* GetType() const;

        int Tie(jack_port_id_t port_index);
        int UnTie();

        jack_nframes_t GetLatency() const;
        void SetLatency(jack_nframes_t latency);

        void SetLatencyRange(jack_latency_callback_mode_t mode, jack_latency_range_t* range);
        void GetLatencyRange(jack_latency_callback_mode_t mode, jack_latency_range_t* range) const;

        jack_nframes_t GetTotalLatency() const;

        int RequestMonitor(bool onoff);
        int EnsureMonitor(bool onoff);
        bool MonitoringInput()
        {
            return (fMonitorRequests > 0);
        }

        // Since we are in shared memory, the resulting pointer cannot be cached, so align it here...
        jack_default_audio_sample_t* GetBuffer()
        {
            return (jack_default_audio_sample_t*)((uintptr_t)fBuffer & ~31L) + 8;
        }

        int GetRefNum() const;

} POST_PACKED_STRUCTURE;

} // end of namespace


#endif

