/*
Copyright (C) 2009 Grame

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

#ifndef __JackProfiler__
#define __JackProfiler__

#include "JackConstants.h"
#include "JackPlatformPlug.h"
#include "jack.h"
#include "jslist.h"
#include <map>
#include <string>

#ifdef JACK_MONITOR
#include "JackEngineProfiling.h"
#endif

namespace Jack
{

struct JackProfilerClient {

    int fRefNum;
    jack_client_t* fClient;
    jack_port_t* fSchedulingPort;
    jack_port_t* fDurationPort;
    
    JackProfilerClient(jack_client_t* client, const char* name);
    ~JackProfilerClient();
    
};

/*!
\brief Server real-time monitoring
*/

class JackProfiler 
{

    private:
    
        jack_client_t* fClient;
        jack_port_t* fCPULoadPort;
        jack_port_t* fDriverPeriodPort;
        jack_port_t* fDriverEndPort;
    #ifdef JACK_MONITOR
        JackTimingMeasure* fLastMeasure;
        std::map<std::string, JackProfilerClient*> fClientTable;
        JackMutex fMutex;
    #endif
 
    public:

        JackProfiler(jack_client_t* jack_client, const JSList* params);
        ~JackProfiler();

        static int Process(jack_nframes_t nframes, void* arg);
        static void ClientRegistration(const char* name, int val, void *arg);

};

}

#endif
