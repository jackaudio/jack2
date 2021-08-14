/*
Copyright (C) 2021 Peter Bridgman

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

#ifndef __JackMachUtils__
#define __JackMachUtils__

#define jack_mach_error_uncurried(type_name, kern_return, message) \
        jack_error(type_name "::%s: " message " - %i:%s", \
                    __FUNCTION__, \
                    kern_return, \
                    mach_error_string(kern_return))

#define jack_mach_bootstrap_err_uncurried(type_name, kern_return, message, service_name) \
        jack_error(type_name "::%s: " message " [%s] - %i:%s", \
                    __FUNCTION__, \
                    service_name, \
                    kern_return, \
                    bootstrap_strerror(kern_return))

#endif
