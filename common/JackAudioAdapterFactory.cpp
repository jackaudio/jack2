/*
Copyright (C) 2008-2012 Grame

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

#include "JackAudioAdapter.h"
#include "JackPlatformPlug.h"
#include "JackArgParser.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifdef __APPLE__
#include "JackCoreAudioAdapter.h"
#define JackPlatformAdapter JackCoreAudioAdapter
#endif

#ifdef __linux__
#include "JackAlsaAdapter.h"
#define JackPlatformAdapter JackAlsaAdapter
#endif

#if defined(__sun__) || defined(sun)
#include "JackOSSAdapter.h"
#define JackPlatformAdapter JackOSSAdapter
#endif

#ifdef WIN32
#include "JackPortAudioAdapter.h"
#define JackPlatformAdapter JackPortAudioAdapter
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    using namespace Jack;

    SERVER_EXPORT int jack_internal_initialize(jack_client_t* jack_client, const JSList* params)
    {
        jack_log("Loading audioadapter");

        Jack::JackAudioAdapter* adapter;
        jack_nframes_t buffer_size = jack_get_buffer_size(jack_client);
        jack_nframes_t sample_rate = jack_get_sample_rate(jack_client);

        try {

            adapter = new Jack::JackAudioAdapter(jack_client, new Jack::JackPlatformAdapter(buffer_size, sample_rate, params));
            assert(adapter);

            if (adapter->Open() == 0) {
                return 0;
            } else {
                delete adapter;
                return 1;
            }

        } catch (...) {
            jack_info("audioadapter allocation error");
            return 1;
        }
    }

    SERVER_EXPORT int jack_initialize(jack_client_t* jack_client, const char* load_init)
    {
        JSList* params = NULL;
        bool parse_params = true;
        int res = 1;
        jack_driver_desc_t* desc = jack_get_descriptor();

        Jack::JackArgParser parser(load_init);
        if (parser.GetArgc() > 0) {
            parse_params = parser.ParseParams(desc, &params);
        }

        if (parse_params) {
            res = jack_internal_initialize(jack_client, params);
            parser.FreeParams(params);
        }
        return res;
    }

    SERVER_EXPORT void jack_finish(void* arg)
    {
        Jack::JackAudioAdapter* adapter = static_cast<Jack::JackAudioAdapter*>(arg);

        if (adapter) {
            jack_log("Unloading audioadapter");
            adapter->Close();
            delete adapter;
        }
    }

#ifdef __cplusplus
}
#endif
