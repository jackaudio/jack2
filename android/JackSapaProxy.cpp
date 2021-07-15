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

#include "JackSapaProxy.h"
#include "JackServerGlobals.h"
#include "JackEngineControl.h"
#include "JackLockedEngine.h"
#include "JackArgParser.h"
#include <assert.h>
#include <string>

// Example)
//                                  sapaproxy
//                                 .----------.
// sapaproxy:__system_capture_1 -->|          |--> sapaproxy:capture_1
// sapaproxy:__system_capture_2 -->|          |--> sapaproxy:capture_2
// ...                             |          |    ...
//                                 |          |
// sapaproxy:playback_1 ---------->|          |--> sapaproxy:__system_playback_1
// sapaproxy:playback_2 ---------->|          |--> sapaproxy:__system_playback_2
// sapaproxy:playback_3 ---------->|          |--> sapaproxy:__system_playback_3
// sapaproxy:playback_4 ---------->|          |--> sapaproxy:__system_playback_4
// ...                             |          |    ...
//                                 '----------'

namespace Jack
{

    JackSapaProxy::JackSapaProxy(jack_client_t* client, const JSList* params)
        :fClient(client)
    {
        jack_log("JackSapaProxy::JackSapaProxy");

        fCapturePorts = fPlaybackPorts = 0;

        const JSList* node;
        const jack_driver_param_t* param;
        for (node = params; node; node = jack_slist_next(node)) {
            param = (const jack_driver_param_t*)node->data;

            switch (param->character) {
                case 'C':
                    fCapturePorts = (param->value.ui > SAPAPROXY_PORT_NUM_FOR_CLIENT)? SAPAPROXY_PORT_NUM_FOR_CLIENT : param->value.ui;
                    break;

                case 'P':
                    fPlaybackPorts = (param->value.ui > SAPAPROXY_PORT_NUM_FOR_CLIENT)? SAPAPROXY_PORT_NUM_FOR_CLIENT : param->value.ui;
                    break;
            }
        }
    }

    JackSapaProxy::~JackSapaProxy()
    {
        jack_log("JackSapaProxy::~JackSapaProxy");
    }

    int JackSapaProxy::Setup(jack_client_t* client)
    {
        jack_log("JackSapaProxy::Setup");

        //refer to system ports and create sapaproxy ports
        unsigned int i = 0, j = 0;
        const char **ports_system_capture;
        const char **ports_system_playback;
        unsigned int ports_system_capture_cnt = 0;
        unsigned int ports_system_playback_cnt = 0;
        char port_name[JACK_PORT_NAME_SIZE] = {0,};
        ports_system_capture = jack_get_ports(client, "system:.*", NULL, JackPortIsPhysical | JackPortIsOutput);
        if (ports_system_capture != NULL) {
            for (i = 0; i < fCapturePorts && ports_system_capture[i]; i++) {
                sprintf(port_name, "__system_capture_%d", i + 1);
                fInputPorts[i] = jack_port_register(client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
                sprintf(port_name, "capture_%d", i + 1);
                fOutputPorts[i] = jack_port_register(client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
                ports_system_capture_cnt++;
            }
            jack_free(ports_system_capture);
        }

        ports_system_playback = jack_get_ports(client, "system:.*", NULL, JackPortIsPhysical | JackPortIsInput);
        if (ports_system_playback != NULL) {
            for (j = 0; j < fPlaybackPorts && ports_system_playback[j]; j++, i++) {
                sprintf(port_name, "playback_%d", j + 1);
                fInputPorts[i] = jack_port_register(client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
                sprintf(port_name, "__system_playback_%d", j + 1);
                fOutputPorts[i] = jack_port_register(client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
                ports_system_playback_cnt++;
            }
            jack_free(ports_system_playback);
        }

        //store actual number of system ports
        fCapturePorts = ports_system_capture_cnt;
        fPlaybackPorts = ports_system_playback_cnt;

        jack_set_process_callback(client, Process, this);
        jack_activate(client);

        //connect between sapaproxy and system ports
        for (unsigned int i = 0; i < ports_system_capture_cnt; i++) {
            sprintf(port_name, "system:capture_%d", i + 1);
            jack_connect(client, port_name, jack_port_name(fInputPorts[i]));
        }

        for (unsigned int i = 0; i < ports_system_playback_cnt; i++) {
            sprintf(port_name, "system:playback_%d", i + 1);
            jack_connect(client, jack_port_name(fOutputPorts[ports_system_capture_cnt + i]), port_name);
        }

        return 0;
    }

    int JackSapaProxy::Process(jack_nframes_t nframes, void* arg)
    {
        JackSapaProxy* sapaproxy = static_cast<JackSapaProxy*>(arg);
        jack_default_audio_sample_t *in, *out;

        //for capture
        for (unsigned int i = 0; i < sapaproxy->fCapturePorts; i++) {
            in  = (jack_default_audio_sample_t*)jack_port_get_buffer(sapaproxy->fInputPorts[i] , nframes);
            out = (jack_default_audio_sample_t*)jack_port_get_buffer(sapaproxy->fOutputPorts[i], nframes);

            // TODO: adjust pcm gain each platform here

            memcpy(out, in, sizeof(jack_default_audio_sample_t) * nframes);
        }

        //for playback
        for (unsigned int i = sapaproxy->fCapturePorts; i < (sapaproxy->fCapturePorts + sapaproxy->fPlaybackPorts); i++) {
            in  = (jack_default_audio_sample_t*)jack_port_get_buffer(sapaproxy->fInputPorts[i] , nframes);
            out = (jack_default_audio_sample_t*)jack_port_get_buffer(sapaproxy->fOutputPorts[i], nframes);

            // TODO: adjust pcm gain each platform here

            memcpy(out, in, sizeof(jack_default_audio_sample_t) * nframes);
        }

        return 0;
    }

} // namespace Jack
