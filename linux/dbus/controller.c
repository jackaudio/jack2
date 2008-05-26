/* -*- Mode: C ; c-basic-offset: 4 -*- */
/*
    Copyright (C) 2007,2008 Nedko Arnaudov
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

#include <stdint.h>
#include <string.h>
#include <dbus/dbus.h>

#include "controller.h"
#include "controller_internal.h"
#include "xml.h"

struct jack_dbus_interface_descriptor * g_jackcontroller_interfaces[] =
{
    &g_jack_controller_iface_introspectable,
    &g_jack_controller_iface_control,
    &g_jack_controller_iface_configure,
    &g_jack_controller_iface_patchbay,
    &g_jack_controller_iface_transport,
    NULL
};

jackctl_driver_t *
jack_controller_find_driver(
    jackctl_server_t *server,
    const char *driver_name)
{
    const JSList * node_ptr;

    node_ptr = jackctl_server_get_drivers_list(server);

    while (node_ptr)
    {
        if (strcmp(jackctl_driver_get_name((jackctl_driver_t *)node_ptr->data), driver_name) == 0)
        {
            return node_ptr->data;
        }

        node_ptr = jack_slist_next(node_ptr);
    }

    return NULL;
}

jackctl_parameter_t *
jack_controller_find_parameter(
    const JSList * parameters_list,
    const char * parameter_name)
{
    while (parameters_list)
    {
        if (strcmp(jackctl_parameter_get_name((jackctl_parameter_t *)parameters_list->data), parameter_name) == 0)
        {
            return parameters_list->data;
        }

        parameters_list = jack_slist_next(parameters_list);
    }

    return NULL;
}

bool
jack_controller_select_driver(
    struct jack_controller * controller_ptr,
    const char * driver_name)
{
    jackctl_driver_t *driver;

    driver = jack_controller_find_driver(controller_ptr->server, driver_name);
    if (driver == NULL)
    {
        return false;
    }

    jack_info("driver \"%s\" selected", driver_name);

    controller_ptr->driver = driver;

    return true;
}

static
int
jack_controller_xrun(void * arg)
{
	((struct jack_controller *)arg)->xruns++;

	return 0;
}

bool
jack_controller_start_server(
    struct jack_controller * controller_ptr,
    void *dbus_call_context_ptr)
{
    int ret;

    jack_info("Starting jack server...");

    if (controller_ptr->started)
    {
        jack_info("Already started.");
        return TRUE;
    }

    if (controller_ptr->driver == NULL)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "Select driver first!");
        goto fail;
    }

    controller_ptr->xruns = 0;

    if (!jackctl_server_start(
            controller_ptr->server,
            controller_ptr->driver))
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "Failed to start server");
        goto fail;
    }

    controller_ptr->client = jack_client_open(
        "dbusapi",
        JackNoStartServer,
        NULL);
    if (controller_ptr->client == NULL)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "failed to create dbusapi jack client");

        goto fail_stop_server;
    }

    ret = jack_set_xrun_callback(controller_ptr->client, jack_controller_xrun, controller_ptr);
    if (ret != 0)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "failed to set xrun callback. error is %d", ret);

        goto fail_close_client;
    }

    if (!jack_controller_patchbay_init(controller_ptr))
    {
        jack_error("Failed to initialize patchbay district");
        goto fail_close_client;
    }

    ret = jack_activate(controller_ptr->client);
    if (ret != 0)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "failed to activate dbusapi jack client. error is %d", ret);

        goto fail_patchbay_uninit;
    }

    controller_ptr->started = true;

    return TRUE;

fail_patchbay_uninit:
    jack_controller_patchbay_uninit(controller_ptr);

fail_close_client:
    ret = jack_client_close(controller_ptr->client);
    if (ret != 0)
    {
        jack_error("jack_client_close() failed with error %d", ret);
    }

    controller_ptr->client = NULL;

fail_stop_server:
    if (!jackctl_server_stop(controller_ptr->server))
    {
        jack_error("failed to stop jack server");
    }

fail:
    return FALSE;
}

bool
jack_controller_stop_server(
    struct jack_controller * controller_ptr,
    void *dbus_call_context_ptr)
{
    int ret;

    jack_info("Stopping jack server...");

    if (!controller_ptr->started)
    {
        jack_info("Already stopped.");
        return TRUE;
    }

    ret = jack_deactivate(controller_ptr->client);
    if (ret != 0)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "failed to deactivate dbusapi jack client. error is %d", ret);
    }

    jack_controller_patchbay_uninit(controller_ptr);

    ret = jack_client_close(controller_ptr->client);
    if (ret != 0)
    {
        jack_error("jack_client_close() failed with error %d", ret);
    }

    controller_ptr->client = NULL;

    if (!jackctl_server_stop(controller_ptr->server))
    {
        return FALSE;
    }

    controller_ptr->started = false;

    return TRUE;
}

void *
jack_controller_create(
        DBusConnection *connection)
{
    struct jack_controller *controller_ptr;
    const JSList * node_ptr;
    const char ** driver_name_target;
    JSList * drivers;
    DBusObjectPathVTable vtable =
    {
        jack_dbus_message_handler_unregister,
        jack_dbus_message_handler,
        NULL
    };

    controller_ptr = malloc(sizeof(struct jack_controller));
    if (!controller_ptr)
    {
        jack_error("Ran out of memory trying to allocate struct jack_controller");
        goto fail;
    }

    controller_ptr->server = jackctl_server_create();
    if (controller_ptr->server == NULL)
    {
        jack_error("Failed to create server object");
        goto fail_free;
    }

    controller_ptr->client = NULL;
    controller_ptr->started = false;

    drivers = (JSList *)jackctl_server_get_drivers_list(controller_ptr->server);
    controller_ptr->drivers_count = jack_slist_length(drivers);
    controller_ptr->driver_names = malloc(controller_ptr->drivers_count * sizeof(const char *));
    if (controller_ptr->driver_names == NULL)
    {
        jack_error("Ran out of memory trying to allocate driver names array");
        goto fail_destroy_server;
    }

    driver_name_target = controller_ptr->driver_names;
    node_ptr = jackctl_server_get_drivers_list(controller_ptr->server);
    while (node_ptr != NULL)
    {
        *driver_name_target = jackctl_driver_get_name((jackctl_driver_t *)node_ptr->data);
        node_ptr = jack_slist_next(node_ptr);
        driver_name_target++;
    }

    controller_ptr->dbus_descriptor.context = controller_ptr;
    controller_ptr->dbus_descriptor.interfaces = g_jackcontroller_interfaces;

    if (!dbus_connection_register_object_path(
            connection,
            JACK_CONTROLLER_OBJECT_PATH,
            &vtable,
            &controller_ptr->dbus_descriptor))
    {
        jack_error("Ran out of memory trying to register D-Bus object path");
        goto fail_free_driver_names_array;
    }

    jack_controller_settings_load(controller_ptr);

    return controller_ptr;

fail_free_driver_names_array:
    free(controller_ptr->driver_names);

fail_destroy_server:
    jackctl_server_destroy(controller_ptr->server);

fail_free:
    free(controller_ptr);

fail:
    return NULL;
}

#define controller_ptr ((struct jack_controller *)context)

void
jack_controller_destroy(
        void * context)
{
    if (controller_ptr->started)
    {
        jack_controller_stop_server(controller_ptr, NULL);
    }

    free(controller_ptr->driver_names);

    jackctl_server_destroy(controller_ptr->server);

    free(controller_ptr);
}

