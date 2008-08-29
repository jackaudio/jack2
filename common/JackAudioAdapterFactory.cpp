/*
Copyright (C) 2008 Grame

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
#include "JackTools.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include "driver_interface.h"

#ifdef __linux__
#include "JackAlsaAdapter.h"
#endif

#ifdef __APPLE__
#include "JackCoreAudioAdapter.h"
#endif

#ifdef WIN32
#include "JackPortAudioAdapter.h"
#endif

    using namespace Jack;

    EXPORT int jack_internal_initialize(jack_client_t* jack_client, const JSList* params)
    {
        jack_log("Loading audioadapter");

        Jack::JackAudioAdapter* adapter;
        jack_nframes_t buffer_size = jack_get_buffer_size(jack_client);
        jack_nframes_t sample_rate = jack_get_sample_rate(jack_client);
        
        try {

    #ifdef __linux__
            adapter = new Jack::JackAudioAdapter(jack_client, new Jack::JackAlsaAdapter(buffer_size, sample_rate, params));
    #endif

    #ifdef WIN32
            adapter = new Jack::JackAudioAdapter(jack_client, new Jack::JackPortAudioAdapter(buffer_size, sample_rate, params));
    #endif

    #ifdef __APPLE__
            adapter = new Jack::JackAudioAdapter(jack_client, new Jack::JackCoreAudioAdapter(buffer_size, sample_rate, params));
    #endif
           assert(adapter);
           
            if (adapter->Open() == 0)
                return 0;
            else
            {
                delete adapter;
                return 1;
            }
         
        } catch (...) {
            return 1;
        }
    }

    EXPORT int jack_initialize(jack_client_t* jack_client, const char* load_init)
    {
        JSList* params = NULL;
        jack_driver_desc_t *desc = jack_get_descriptor();

        JackArgParser parser(load_init);
        if (parser.GetArgc() > 0) 
            parser.ParseParams(desc, &params);
    
        int res = jack_internal_initialize(jack_client, params);
        parser.FreeParams(params);
        return res;
    }

    EXPORT void jack_finish(void* arg)
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
