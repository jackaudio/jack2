/* -*- Mode: C ; c-basic-offset: 4 -*- */
/*
    Copyright (C) 2007,2008 Nedko Arnaudov
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdbool.h>
#include <dbus/dbus.h>

#include <jack/driver.h>
#include <jack/engine.h>
#include "dbus.h"
#include "controller_internal.h"

bool
jack_controller_settings_init()
{
    return true;
}

void
jack_controller_settings_uninit()
{
}

bool
jack_controller_settings_save(
    struct jack_controller * controller_ptr,
    void *dbus_call_context_ptr)
{
    jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "jackdbus compiled without settings persistence");
    return true;
}

void
jack_controller_settings_load(
    struct jack_controller * controller_ptr)
{
}

void
jack_controller_settings_save_auto(
    struct jack_controller * controller_ptr)
{
}
