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

#ifndef __JackPort__
#define __JackPort__

#include "types.h"
#include "JackConstants.h"

namespace Jack
{

#define ALL_PORTS	0xFFFF
#define NO_PORT		0xFFFE

/*!
\brief Base class for port. 
*/

class JackPort
{

        friend class JackGraphManager;

    private:

        int fTypeId;
        enum JackPortFlags fFlags;
        char fName[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
		char fAlias1[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
		char fAlias2[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
        int fRefNum;

        jack_nframes_t fLatency;
		jack_nframes_t fTotalLatency;
        uint8_t fMonitorRequests;

        bool fInUse;
        jack_port_id_t fTied;   // Locally tied source port

	#ifdef WIN32
        //__declspec(align(16)) float fBuffer[BUFFER_SIZE_MAX];
		float fBuffer[BUFFER_SIZE_MAX];
	#elif __GNUC__
		float fBuffer[BUFFER_SIZE_MAX] __attribute__((aligned(64)));  // 16 bytes alignment for vector code, 64 bytes better for cache loads/stores
	#else
		#warning Buffer will not be aligned on 16 bytes boundaries : vector based code (Altivec of SSE) will fail 
		float fBuffer[BUFFER_SIZE_MAX];
	#endif

        bool IsUsed() const;
		
		// RT
        void ClearBuffer(jack_nframes_t frames);
        void MixBuffers(void** src_buffers, int src_count, jack_nframes_t frames);

    public:

        JackPort();
        virtual ~JackPort();

        bool Allocate(int refnum, const char* port_name, const char* port_type, JackPortFlags flags);
        void Release();
        const char* GetName() const;
        const char* GetShortName() const;
        int	SetName(const char* name);
		
		int GetAliases(char* const aliases[2]);
		int SetAlias(const char* alias);
		int UnsetAlias(const char* alias);
		bool NameEquals(const char* target);

        int	GetFlags() const;
        const char* GetType() const;

		int Tie(jack_port_id_t port_index);
        int UnTie();

        jack_nframes_t GetLatency() const;
		jack_nframes_t GetTotalLatency() const;
        void SetLatency(jack_nframes_t latency);

        int RequestMonitor(bool onoff);
        int EnsureMonitor(bool onoff);
        bool MonitoringInput();

        float* GetBuffer();
        int GetRefNum() const;
        
};


} // end of namespace


#endif

