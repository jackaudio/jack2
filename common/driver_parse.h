/*
Copyright (C) 2003 Bob Ham <rah@bash.sh
  
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

#ifndef __jack_driver_parse_h__
#define __jack_driver_parse_h__

#include "jslist.h"
#include "driver_interface.h"
#include "JackExports.h"

EXPORT int
jack_parse_driver_params (jack_driver_desc_t * desc, int argc, char* argv[], JSList ** param_ptr);


#endif /* __jack_driver_parse_h__ */


