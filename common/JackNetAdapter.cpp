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

#include "JackNetAdapter.h"
#include "JackTools.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

namespace Jack
{

JackNetAdapter::JackNetAdapter ( jack_nframes_t buffer_size, jack_nframes_t sample_rate, const JSList* params )
     :JackAudioAdapterInterface(buffer_size, sample_rate),fThread(this)
{
    fCaptureChannels = 2;
    fPlaybackChannels = 2;
}

JackNetAdapter::~JackNetAdapter()
{}

int JackNetAdapter::Open()
{
    return 0;
}

int JackNetAdapter::Close()
{
    return 0;
}

int JackNetAdapter::SetBufferSize ( jack_nframes_t buffer_size )
{
    return 0;
}

bool JackNetAdapter::Init()
{
    return true;
}

bool JackNetAdapter::Execute()
{
    return true;
}

} // namespace Jack

#ifdef __cplusplus
extern "C"
{
#endif

#include "driver_interface.h"
#include "JackAudioAdapter.h"

    using namespace Jack;

    EXPORT jack_driver_desc_t* jack_get_descriptor()
    {
        jack_driver_desc_t *desc;
        jack_driver_param_desc_t * params;
        //unsigned int i;

        desc = ( jack_driver_desc_t* ) calloc ( 1, sizeof ( jack_driver_desc_t ) );
        strcpy ( desc->name, "net-adapter" );
        desc->nparams = 2;
        params = ( jack_driver_param_desc_t* ) calloc ( desc->nparams, sizeof ( jack_driver_param_desc_t ) );

        // TODO

        desc->params = params;
        return desc;
    }

    EXPORT int jack_internal_initialize ( jack_client_t* jack_client, const JSList* params )
    {
        jack_log ( "Loading netadapter" );

        Jack::JackAudioAdapter* adapter;
        jack_nframes_t buffer_size = jack_get_buffer_size ( jack_client );
        jack_nframes_t sample_rate = jack_get_sample_rate ( jack_client );

        adapter = new Jack::JackAudioAdapter ( jack_client, new Jack::JackNetAdapter ( buffer_size, sample_rate, params ) );
        assert ( adapter );

        if ( adapter->Open() == 0 )
        {
            return 0;
        }
        else
        {
            delete adapter;
            return 1;
        }
    }

    EXPORT int jack_initialize ( jack_client_t* jack_client, const char* load_init )
    {
        JSList* params = NULL;
        jack_driver_desc_t *desc = jack_get_descriptor();

        JackArgParser parser ( load_init );

        if ( parser.GetArgc() > 0 )
        {
            if ( parser.ParseParams ( desc, &params ) != 0 )
                jack_error ( "Internal client : JackArgParser::ParseParams error." );
        }

        return jack_internal_initialize ( jack_client, params );
    }

    EXPORT void jack_finish ( void* arg )
    {
        Jack::JackAudioAdapter* adapter = static_cast<Jack::JackAudioAdapter*> ( arg );

        if ( adapter )
        {
            jack_log ( "Unloading netadapter" );
            adapter->Close();
            delete adapter;
        }
    }

#ifdef __cplusplus
}
#endif
