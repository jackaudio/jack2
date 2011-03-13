/*
Copyright (C) 2001-2005 Paul Davis
Copyright (C) 2004-2008 Grame

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

#ifndef __JackDriverLoader__
#define __JackDriverLoader__

#include "driver_interface.h"
#include "JackControlAPI.h"
#include "JackPlatformPlug.h"
#include "JackDriver.h"
#include "JackSystemDeps.h"

typedef jack_driver_desc_t * (*JackDriverDescFunction) ();
typedef Jack::JackDriverClientInterface* (*driverInitialize) (Jack::JackLockedEngine*, Jack::JackSynchro*, const JSList*);

class JackDriverInfo
{

    private:

        driverInitialize fInitialize;
        DRIVER_HANDLE fHandle;
        Jack::JackDriverClientInterface* fBackend;

    public:

        JackDriverInfo():fInitialize(NULL),fHandle(NULL),fBackend(NULL)
        {}
        ~JackDriverInfo();

        Jack::JackDriverClientInterface* Open(jack_driver_desc_t* driver_desc, Jack::JackLockedEngine*, Jack::JackSynchro*, const JSList*);

        Jack::JackDriverClientInterface* GetBackend()
        {
            return fBackend;
        }

};

jack_driver_desc_t * jack_find_driver_descriptor(JSList * drivers, const char * name);

JSList * jack_drivers_load(JSList * drivers);
JSList * jack_internals_load(JSList * internals);

EXPORT int jackctl_parse_driver_params (jackctl_driver * driver_ptr, int argc, char* argv[]);
EXPORT void jack_free_driver_params(JSList * param_ptr);
EXPORT void jack_print_driver_options(jack_driver_desc_t* desc, FILE* file);

#endif

