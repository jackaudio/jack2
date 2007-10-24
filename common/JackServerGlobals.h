/*
Copyright (C) 2005 Grame  

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

#ifndef __JackServerGlobals__
#define __JackServerGlobals__

#include "driver_interface.h"
#include "driver_parse.h"
#include "JackDriverLoader.h"
#include "JackServer.h"

#include <assert.h>

namespace Jack
{

class JackClient;

/*!
\brief Global server static structure: singleton kind of pattern.
*/

struct JackServerGlobals
{
    static long fClientCount;
    static JackServer* fServer;

    JackServerGlobals();
    virtual ~JackServerGlobals();

    static bool Init();
    static void Destroy();
    static int JackStart(const char* server_name, 
						jack_driver_desc_t* driver_desc, 
						JSList* driver_params, 
						int sync, 
						int temporary, 
						int time_out_ms, 
						int rt, 
						int priority, 
						int loopback, 
						int verbose);
    static int JackStop();
    static int JackDelete();
};

} // end of namespace

#endif

