/*
Copyright (C) 2008-2011 Romain Moret at Grame

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

#ifndef __JackNetSocket__
#define __JackNetSocket__

#include "JackCompilerDeps.h"

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <errno.h>

namespace Jack
{
    //get host name*********************************
    SERVER_EXPORT int GetHostName(char * name, int size);

    //net errors ***********************************
    enum _net_error
    {
        NET_CONN_ERROR = 10000,
        NET_OP_ERROR,
        NET_NO_DATA,
        NET_NO_NETWORK,
        NET_NO_ERROR
    };

    typedef enum _net_error net_error_t;
}

#endif
