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
#include <stdint.h>

#include "reserve.h"
#include "audio_reserve.h"
#include "jack/control.h"

#define DEVICE_MAX 2

typedef struct reserved_audio_device {

     char device_name[64];
     rd_device * reserved_device;

} reserved_audio_device;

static DBusConnection* gConnection = NULL;
static reserved_audio_device gReservedDevice[DEVICE_MAX];
static int gReserveCount = 0;

SERVER_EXPORT int audio_reservation_init()
{
    DBusError error;
    dbus_error_init(&error);

    if (!(gConnection = dbus_bus_get(DBUS_BUS_SESSION, &error))) {
        jack_error("Failed to connect to session bus for device reservation %s\n", error.message);
        return -1;
    }

    jack_info("audio_reservation_init");
    return 0;
}

SERVER_EXPORT int audio_reservation_finish()
{
    if (gConnection) {
        dbus_connection_unref(gConnection);
        gConnection = NULL;
        jack_info("audio_reservation_finish");
    }
    return 0;
}

SERVER_EXPORT bool audio_acquire(const char * device_name)
{
    DBusError error;   
    int ret;

    // Open DBus connection first time
    if (gReserveCount == 0)
       audio_reservation_init();

    assert(gReserveCount < DEVICE_MAX);

    dbus_error_init(&error);

    if ((ret= rd_acquire(
                 &gReservedDevice[gReserveCount].reserved_device,
                 gConnection,
                 device_name,
                 "Jack audio server",
                 INT32_MAX,
                 NULL,
                 &error)) < 0) {

        jack_error("Failed to acquire device name : %s error : %s", device_name, (error.message ? error.message : strerror(-ret)));
        dbus_error_free(&error);
        return false;
    }

    strcpy(gReservedDevice[gReserveCount].device_name, device_name);
    gReserveCount++;
    jack_info("Acquire audio card %s", device_name);
    return true;
}

SERVER_EXPORT void audio_release(const char * device_name)
{
    int i;

    // Look for corresponding reserved device
    for (i = 0; i < DEVICE_MAX; i++) {
 	if (strcmp(gReservedDevice[i].device_name, device_name) == 0)  
	    break;
    }
   
    if (i < DEVICE_MAX) {
        jack_info("Released audio card %s", device_name);
        rd_release(gReservedDevice[i].reserved_device);
    } else {
        jack_error("Audio card %s not found!!", device_name);
    }

    // Close DBus connection last time
    gReserveCount--;
    if (gReserveCount == 0)
        audio_reservation_finish();
}

SERVER_EXPORT void audio_reserve_loop()
{
    if (gConnection != NULL) {
       while (dbus_connection_read_write_dispatch (gConnection, -1))
         ; // empty loop body
    }
}

