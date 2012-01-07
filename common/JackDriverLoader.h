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

jack_driver_desc_t* jack_find_driver_descriptor(JSList* drivers, const char* name);
JSList* jack_drivers_load(JSList* drivers);
JSList* jack_internals_load(JSList* internals);
void jack_free_driver_params(JSList * param_ptr);
void jack_print_driver_options(jack_driver_desc_t* desc, FILE* file);

// External control.h API
extern "C" SERVER_EXPORT int jackctl_driver_params_parse(jackctl_driver * driver, int argc, char* argv[]);

#endif

