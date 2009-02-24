/*
    Copyright (C) 2009 Grame
    
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

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "reserve.h"
#include "audio_reserve.h"
#include "JackError.h"

static DBusConnection* connection = NULL;

SERVER_EXPORT int audio_reservation_init()
{
    DBusError error;
   
    dbus_error_init(&error);

    if (!(connection = dbus_bus_get(DBUS_BUS_SESSION, &error))) {
        jack_error("Failed to connect to session bus for device reservation %s\n", error.message);
        return -1;
    }

    return 0;
}

SERVER_EXPORT int audio_reservation_finish()
{
    if (connection) 
        dbus_connection_unref(connection);
}

SERVER_EXPORT void* audio_acquire(int num)
{
    DBusError error;
    rd_device* device;
    char audio_name[32];
    int e;

    snprintf(audio_name, sizeof(audio_name) - 1, "Audio%d", num);
    if ((e = rd_acquire(
                 &device,
                 connection,
                 audio_name,
                 "Jack audio server",
                 INT32_MAX,
                 NULL,
                 &error)) < 0) {

        jack_error ("Failed to acquire device: %s\n", error.message ? error.message : strerror(-e));
        return NULL;
    }

    jack_info("Acquire audio card %s", audio_name);
    return (void*)device;
}

SERVER_EXPORT void audio_reserve_loop()
{
    if (connection) {
       while (dbus_connection_read_write_dispatch (connection, -1))
         ; // empty loop body
    }
}

SERVER_EXPORT void audio_release(void* dev)
{
   rd_device* device = (rd_device*)dev;
   if (device) {
        jack_info("Release audio card");
        rd_release(device);
   }
}
