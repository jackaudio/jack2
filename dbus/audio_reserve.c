/***
  Copyright 2009 Grame

  Permission is hereby granted, free of charge, to any person
  obtaining a copy of this software and associated documentation files
  (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge,
  publish, distribute, sublicense, and/or sell copies of the Software,
  and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
***/

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
