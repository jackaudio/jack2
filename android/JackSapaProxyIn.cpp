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

#ifdef __cplusplus
extern "C"
{
#endif

#include "driver_interface.h"

    using namespace Jack;

    static Jack::JackSapaProxy* sapaproxy = NULL;

    SERVER_EXPORT jack_driver_desc_t* jack_get_descriptor()
    {
        jack_driver_desc_t * desc;
        jack_driver_desc_filler_t filler;
        jack_driver_param_value_t value;

        desc = jack_driver_descriptor_construct("in", JackDriverNone, "sapaproxy client", &filler);

        value.ui = 0U;
        jack_driver_descriptor_add_parameter(desc, &filler, "capture", 'C', JackDriverParamUInt, &value, NULL, "Number of capture ports", NULL);
        jack_driver_descriptor_add_parameter(desc, &filler, "playback", 'P', JackDriverParamUInt, &value, NULL, "Number of playback ports", NULL);

        return desc;
    }

    SERVER_EXPORT int jack_internal_initialize(jack_client_t* jack_client, const JSList* params)
    {
        if (sapaproxy) {
            jack_info("sapaproxy already loaded");
            return 1;
        }

        jack_log("Loading sapaproxy");
        sapaproxy = new Jack::JackSapaProxy(jack_client, params);
        if (!params) {
            sapaproxy->fCapturePorts  = 2U;
            sapaproxy->fPlaybackPorts = 0U;
        }
        sapaproxy->Setup(jack_client);
        assert(sapaproxy);
        return 0;
    }

    SERVER_EXPORT int jack_initialize(jack_client_t* jack_client, const char* load_init)
    {
        JSList* params = NULL;
        bool parse_params = true;
        int res = 1;
        jack_driver_desc_t* desc = jack_get_descriptor();

        Jack::JackArgParser parser(load_init);
        if (parser.GetArgc() > 0)
            parse_params = parser.ParseParams(desc, &params);

        if (parse_params) {
            res = jack_internal_initialize(jack_client, params);
            parser.FreeParams(params);
        }
        return res;
    }

    SERVER_EXPORT void jack_finish(void* arg)
    {
        Jack::JackSapaProxy* sapaproxy = static_cast<Jack::JackSapaProxy*>(arg);

        if (sapaproxy) {
            jack_log("Unloading sapaproxy");
            delete sapaproxy;
        }
    }

#ifdef __cplusplus
}
#endif
