/* -*- Mode: C ; c-basic-offset: 4 -*- */
/*
    Copyright (C) 2007-2008 Nedko Arnaudov
    Copyright (C) 2007-2008 Juuso Alasuutari

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

static char g_xml_data[102400];

static
void
jack_controller_dbus_introspect(
    struct jack_dbus_method_call * call)
{
    jack_dbus_construct_method_return_single(
        call,
        DBUS_TYPE_STRING,
        (message_arg_t)(const char *)g_xml_data);
}

JACK_DBUS_METHOD_ARGUMENTS_BEGIN(Introspect)
    JACK_DBUS_METHOD_ARGUMENT("xml_data", "s", true)
JACK_DBUS_METHOD_ARGUMENTS_END

JACK_DBUS_METHODS_BEGIN
    JACK_DBUS_METHOD_DESCRIBE(Introspect, jack_controller_dbus_introspect)
JACK_DBUS_METHODS_END

JACK_DBUS_IFACE_BEGIN(g_jack_controller_iface_introspectable, "org.freedesktop.DBus.Introspectable")
    JACK_DBUS_IFACE_EXPOSE_METHODS
JACK_DBUS_IFACE_END

static char * g_buffer_ptr;

static
void
write_line_format(const char * format, ...)
{
    va_list ap;

    va_start(ap, format);
    g_buffer_ptr += vsprintf(g_buffer_ptr, format, ap);
    va_end(ap);
}

static
void
write_line(const char * line)
{
    write_line_format("%s\n", line);
}

void jack_controller_introspect_init() __attribute__((constructor));

void
jack_controller_introspect_init()
{
    struct jack_dbus_interface_descriptor ** interface_ptr_ptr;
    const struct jack_dbus_interface_method_descriptor * method_ptr;
    const struct jack_dbus_interface_method_argument_descriptor * method_argument_ptr;
    const struct jack_dbus_interface_signal_descriptor * signal_ptr;
    const struct jack_dbus_interface_signal_argument_descriptor * signal_argument_ptr;

    g_buffer_ptr = g_xml_data;

    write_line("<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"");
    write_line("\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">");

    write_line("<node name=\"" JACK_CONTROLLER_OBJECT_PATH "\">");

    interface_ptr_ptr = g_jackcontroller_interfaces;

    while (*interface_ptr_ptr != NULL)
    {
        write_line_format("  <interface name=\"%s\">\n", (*interface_ptr_ptr)->name);

        if ((*interface_ptr_ptr)->methods != NULL)
        {
            method_ptr = (*interface_ptr_ptr)->methods;
            while (method_ptr->name != NULL)
            {
                write_line_format("    <method name=\"%s\">\n", method_ptr->name);

                method_argument_ptr = method_ptr->arguments;

                while (method_argument_ptr->name != NULL)
                {
                    write_line_format(
                        "      <arg name=\"%s\" type=\"%s\" direction=\"%s\" />\n",
                        method_argument_ptr->name,
                        method_argument_ptr->type,
                        method_argument_ptr->direction_out ? "out" : "in");
                    method_argument_ptr++;
                }

                write_line("    </method>");
                method_ptr++;
            }
        }

        if ((*interface_ptr_ptr)->signals != NULL)
        {
            signal_ptr = (*interface_ptr_ptr)->signals;
            while (signal_ptr->name != NULL)
            {
                write_line_format("    <signal name=\"%s\">\n", signal_ptr->name);

                signal_argument_ptr = signal_ptr->arguments;

                while (signal_argument_ptr->name != NULL)
                {
                    write_line_format(
                        "      <arg name=\"%s\" type=\"%s\" />\n",
                        signal_argument_ptr->name,
                        signal_argument_ptr->type);
                    signal_argument_ptr++;
                }

                write_line("    </signal>");
                signal_ptr++;
            }
        }

        write_line("  </interface>");
        interface_ptr_ptr++;
    }

    write_line("</node>");

    *g_buffer_ptr = 0;
}
