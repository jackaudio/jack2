/* -*- Mode: C ; c-basic-offset: 4 -*- */
/*
    Copyright (C) 2008 Nedko Arnaudov
    Copyright (C) 2008 Juuso Alasuutari

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

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <dbus/dbus.h>

#include "jackdbus.h"

JACK_DBUS_METHODS_BEGIN
JACK_DBUS_METHODS_END

JACK_DBUS_IFACE_BEGIN(g_jack_controller_iface_transport, "org.jackaudio.JackTransport")
    JACK_DBUS_IFACE_EXPOSE_METHODS
JACK_DBUS_IFACE_END
