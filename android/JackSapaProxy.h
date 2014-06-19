/*
Copyright (C) 2014 Samsung Electronics

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

#ifndef __JackSapaProxy__
#define __JackSapaProxy__

#include "JackConstants.h"
#include "JackPlatformPlug.h"
#include "jack.h"
#include "jslist.h"
#include <map>
#include <string>

#define SAPAPROXY_PORT_NUM_FOR_CLIENT  16

namespace Jack
{

class JackSapaProxy
{

    private:

        jack_client_t* fClient;
        jack_port_t *fInputPorts[SAPAPROXY_PORT_NUM_FOR_CLIENT];
        jack_port_t *fOutputPorts[SAPAPROXY_PORT_NUM_FOR_CLIENT];

    public:

        unsigned int fCapturePorts;
        unsigned int fPlaybackPorts;

        JackSapaProxy(jack_client_t* jack_client, const JSList* params);
        ~JackSapaProxy();

        int Setup(jack_client_t* jack_client);
        static int Process(jack_nframes_t nframes, void* arg);

};

}

#endif
