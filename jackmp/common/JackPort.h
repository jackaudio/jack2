/*
Copyright (C) 2001 Paul Davis
Copyright (C) 2004-2006 Grame  

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

        enum JackPortFlags fFlags;
        char fName[JACK_PORT_NAME_SIZE + 2];
        int fRefNum;

        jack_nframes_t fLatency;
        uint8_t fMonitorRequests;

        bool fInUse;
        bool fLocked;
        jack_port_id_t fTied;   // Locally tied source port

        float fBuffer[BUFFER_SIZE_MAX];

        bool IsUsed() const;
		
		static void MixBuffer(float* mixbuffer, float* buffer, jack_nframes_t frames);

    public:

        JackPort();
        virtual ~JackPort();

        void Allocate(int refnum, const char* port_name, JackPortFlags flags);
        void Release();
        const char* GetName() const;
        const char* GetShortName() const;
        int	SetName(const char * name);

        int	Flags() const;
        const char* Type() const;

        int Lock();
        int Unlock();

        int Tie(jack_port_id_t port_index);
        int UnTie();

        jack_nframes_t GetLatency() const;
        void SetLatency(jack_nframes_t latency);

        int RequestMonitor(bool onoff);
        int EnsureMonitor(bool onoff);
        bool MonitoringInput();

        float* GetBuffer();
        int GetRefNum() const;
        
};


} // end of namespace


#endif

