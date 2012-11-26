/* -*- Mode: C ; c-basic-offset: 4 -*- */
/*
    Copyright (C) 2007,2008,2010,2011 Nedko Arnaudov
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
#include <dbus/dbus.h>
#include <assert.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <errno.h>

#include "controller.h"
#include "controller_internal.h"
#include "xml.h"
#include "reserve.h"

struct jack_dbus_interface_descriptor * g_jackcontroller_interfaces[] =
{
    &g_jack_controller_iface_introspectable,
    &g_jack_controller_iface_control,
    &g_jack_controller_iface_configure,
    &g_jack_controller_iface_patchbay,
    &g_jack_controller_iface_session_manager,
    &g_jack_controller_iface_transport,
    NULL
};

static
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

static bool jack_controller_check_slave_driver(struct jack_controller * controller_ptr, const char * name)
{
    struct list_head * node_ptr;
    struct jack_controller_slave_driver * driver_ptr;

    list_for_each(node_ptr, &controller_ptr->slave_drivers)
    {
        driver_ptr = list_entry(node_ptr, struct jack_controller_slave_driver, siblings);
        if (strcmp(name, driver_ptr->name) == 0)
        {
            return true;
        }
    }

    return false;
}

static bool jack_controller_load_slave_drivers(struct jack_controller * controller_ptr)
{
    struct list_head * node_ptr;
    struct jack_controller_slave_driver * driver_ptr;

    list_for_each(node_ptr, &controller_ptr->slave_drivers)
    {
        driver_ptr = list_entry(node_ptr, struct jack_controller_slave_driver, siblings);

        assert(driver_ptr->handle != NULL);
        assert(!driver_ptr->loaded);

        if (!jackctl_server_add_slave(controller_ptr->server, driver_ptr->handle))
        {
            jack_error("Driver \"%s\" cannot be loaded", driver_ptr->name);
            return false;
        }

        driver_ptr->loaded = true;
    }

    return true;
}

static void jack_controller_unload_slave_drivers(struct jack_controller * controller_ptr)
{
    struct list_head * node_ptr;
    struct jack_controller_slave_driver * driver_ptr;

    list_for_each(node_ptr, &controller_ptr->slave_drivers)
    {
        driver_ptr = list_entry(node_ptr, struct jack_controller_slave_driver, siblings);
        if (driver_ptr->loaded)
        {
            jackctl_server_remove_slave(controller_ptr->server, driver_ptr->handle);
            driver_ptr->loaded = false;
        }
    }
}

static void jack_controller_remove_slave_drivers(struct jack_controller * controller_ptr)
{
    struct jack_controller_slave_driver * driver_ptr;

    while (!list_empty(&controller_ptr->slave_drivers))
    {
        driver_ptr = list_entry(controller_ptr->slave_drivers.next, struct jack_controller_slave_driver, siblings);
        assert(!driver_ptr->loaded);
        list_del(&driver_ptr->siblings);
        free(driver_ptr->name);
        free(driver_ptr);
    }

    controller_ptr->slave_drivers_vparam_value.str[0] = 0;
}

static
jackctl_internal_t *
jack_controller_find_internal(
    jackctl_server_t *server,
    const char *internal_name)
{
    const JSList * node_ptr;

    node_ptr = jackctl_server_get_internals_list(server);

    while (node_ptr)
    {
        if (strcmp(jackctl_internal_get_name((jackctl_internal_t *)node_ptr->data), internal_name) == 0)
        {
            return node_ptr->data;
        }

        node_ptr = jack_slist_next(node_ptr);
    }

    return NULL;
}

bool
jack_controller_select_driver(
    struct jack_controller * controller_ptr,
    const char * driver_name)
{
    if (!jack_params_set_driver(controller_ptr->params, driver_name))
    {
        return false;
    }

    jack_info("driver \"%s\" selected", driver_name);

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

    assert(!controller_ptr->started); /* should be ensured by caller */

    controller_ptr->xruns = 0;

    if (!jackctl_server_open(
            controller_ptr->server,
            jack_params_get_driver(controller_ptr->params)))
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "Failed to open server");
        goto fail;
    }

    jack_controller_load_slave_drivers(controller_ptr);

    if (!jackctl_server_start(
            controller_ptr->server))
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "Failed to start server");
        goto fail_close_server;
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
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "Failed to initialize patchbay district");
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

fail_close_server:
    jack_controller_unload_slave_drivers(controller_ptr);

    if (!jackctl_server_close(controller_ptr->server))
    {
        jack_error("failed to close jack server");
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

    pthread_mutex_lock(&controller_ptr->lock);
    if (!list_empty(&controller_ptr->session_pending_commands))
    {
        pthread_mutex_unlock(&controller_ptr->lock);
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "Refusing to stop JACK server because of pending session commands");
        return false;
    }
    pthread_mutex_unlock(&controller_ptr->lock);

    jack_info("Stopping jack server...");

    assert(controller_ptr->started); /* should be ensured by caller */

    ret = jack_deactivate(controller_ptr->client);
    if (ret != 0)
    {
        jack_error("failed to deactivate dbusapi jack client. error is %d", ret);
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
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "Failed to stop server");
        return FALSE;
    }

    jack_controller_unload_slave_drivers(controller_ptr);

    if (!jackctl_server_close(controller_ptr->server))
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "Failed to close server");
        return FALSE;
    }

    controller_ptr->started = false;

    return TRUE;
}

bool
jack_controller_switch_master(
    struct jack_controller * controller_ptr,
    void *dbus_call_context_ptr)
{
    assert(controller_ptr->started); /* should be ensured by caller */

    if (!jackctl_server_switch_master(
            controller_ptr->server,
            jack_params_get_driver(controller_ptr->params)))
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "Failed to switch master");
        controller_ptr->started = false;
        return FALSE;
    }

    return TRUE;
}

#define DEVICE_MAX 2

typedef struct reserved_audio_device {

     char device_name[64];
     rd_device * reserved_device;

} reserved_audio_device;


int g_device_count = 0;
static reserved_audio_device g_reserved_device[DEVICE_MAX];

static
bool
on_device_acquire(const char * device_name)
{
    int ret;
    DBusError error;

    dbus_error_init(&error);

    ret = rd_acquire(
        &g_reserved_device[g_device_count].reserved_device,
        g_connection,
        device_name,
        "Jack audio server",
        INT32_MAX,
        NULL,
        &error);
    if (ret  < 0)
    {
        jack_error("Failed to acquire device name : %s error : %s", device_name, (error.message ? error.message : strerror(-ret)));
        dbus_error_free(&error);
        return false;
    }

    strcpy(g_reserved_device[g_device_count].device_name, device_name);
    g_device_count++;
    jack_info("Acquired audio card %s", device_name);
    return true;
}

static
void
on_device_release(const char * device_name)
{
    int i;

    // Look for corresponding reserved device
    for (i = 0; i < DEVICE_MAX; i++) {
 	if (strcmp(g_reserved_device[i].device_name, device_name) == 0)
	    break;
    }

    if (i < DEVICE_MAX) {
	jack_info("Released audio card %s", device_name);
        rd_release(g_reserved_device[i].reserved_device);
    } else {
	jack_error("Audio card %s not found!!", device_name);
    }

    g_device_count--;
}

#define controller_ptr ((struct jack_controller *)obj)

static bool slave_drivers_parameter_is_set(void * obj)
{
    return controller_ptr->slave_drivers_set;
}

static bool slave_drivers_parameter_reset(void * obj)
{
    if (controller_ptr->started)
    {
        jack_error("Cannot modify slave-drivers when server is started");
        return false;
    }

    jack_controller_remove_slave_drivers(controller_ptr);
    controller_ptr->slave_drivers_set = false;
    return true;
}

static union jackctl_parameter_value slave_drivers_parameter_get_value(void * obj)
{
    return controller_ptr->slave_drivers_vparam_value;
}

static bool slave_drivers_parameter_set_value(void * obj, const union jackctl_parameter_value * value_ptr)
{
    char * buffer;
    char * save;
    const char * token;
    struct list_head old_list;
    struct list_head new_list;
    union jackctl_parameter_value old_value;
    union jackctl_parameter_value new_value;
    bool old_set;

    if (controller_ptr->started)
    {
        jack_error("Cannot modify slave-drivers when server is started");
        return false;
    }

    old_set = controller_ptr->slave_drivers_set;
    old_value = controller_ptr->slave_drivers_vparam_value;
    controller_ptr->slave_drivers_vparam_value.str[0] = 0;
    old_list = controller_ptr->slave_drivers;
    INIT_LIST_HEAD(&controller_ptr->slave_drivers);

    buffer = strdup(value_ptr->str);
    if (buffer == NULL)
    {
        jack_error("strdup() failed.");
        return false;
    }

    token = strtok_r(buffer, ",", &save);
    while (token)
    {
        //jack_info("slave driver '%s'", token);
        if (!jack_controller_add_slave_driver(controller_ptr, token))
        {
            jack_controller_remove_slave_drivers(controller_ptr);
            controller_ptr->slave_drivers = old_list;
            controller_ptr->slave_drivers_vparam_value = old_value;
            controller_ptr->slave_drivers_set = old_set;

            free(buffer);

            return false;
        }

        token = strtok_r(NULL, ",", &save);
    }

    new_value = controller_ptr->slave_drivers_vparam_value;
    new_list = controller_ptr->slave_drivers;
    controller_ptr->slave_drivers = old_list;
    jack_controller_remove_slave_drivers(controller_ptr);
    controller_ptr->slave_drivers_vparam_value = new_value;
    controller_ptr->slave_drivers = new_list;
    controller_ptr->slave_drivers_set = true;

    free(buffer);

    return true;
}

static union jackctl_parameter_value slave_drivers_parameter_get_default_value(void * obj)
{
    union jackctl_parameter_value value;
    value.str[0] = 0;
    return value;
}

#undef controller_ptr

void *
jack_controller_create(
        DBusConnection *connection)
{
    int error;
    struct jack_controller *controller_ptr;
    const char * address[PARAM_ADDRESS_SIZE];
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

    error = pthread_mutex_init(&controller_ptr->lock, NULL);
    if (error != 0)
    {
        jack_error("Failed to initialize mutex. error %d", error);
        goto fail_free;
    }

    INIT_LIST_HEAD(&controller_ptr->session_pending_commands);

    controller_ptr->server = jackctl_server_create(on_device_acquire, on_device_release);
    if (controller_ptr->server == NULL)
    {
        jack_error("Failed to create server object");
        goto fail_uninit_mutex;
    }

    controller_ptr->params = jack_params_create(controller_ptr->server);
    if (controller_ptr->params == NULL)
    {
        jack_error("Failed to initialize parameter tree");
        goto fail_destroy_server;
    }

    controller_ptr->client = NULL;
    controller_ptr->started = false;

    controller_ptr->pending_save = 0;

    INIT_LIST_HEAD(&controller_ptr->slave_drivers);
    controller_ptr->slave_drivers_set = false;
    controller_ptr->slave_drivers_vparam_value.str[0] = 0;

    controller_ptr->slave_drivers_vparam.obj = controller_ptr;

    controller_ptr->slave_drivers_vparam.vtable.is_set = slave_drivers_parameter_is_set;
    controller_ptr->slave_drivers_vparam.vtable.reset = slave_drivers_parameter_reset;
    controller_ptr->slave_drivers_vparam.vtable.get_value = slave_drivers_parameter_get_value;
    controller_ptr->slave_drivers_vparam.vtable.set_value = slave_drivers_parameter_set_value;
    controller_ptr->slave_drivers_vparam.vtable.get_default_value = slave_drivers_parameter_get_default_value;

    controller_ptr->slave_drivers_vparam.type = JackParamString;
    controller_ptr->slave_drivers_vparam.name = "slave-drivers";
    controller_ptr->slave_drivers_vparam.short_decr = "Slave drivers to use";
    controller_ptr->slave_drivers_vparam.long_descr = "A comma separated list of slave drivers";
    controller_ptr->slave_drivers_vparam.constraint_flags = 0;

    address[0] = PTNODE_ENGINE;
    address[1] = NULL;
    jack_params_add_parameter(controller_ptr->params, address, true, &controller_ptr->slave_drivers_vparam);

    controller_ptr->dbus_descriptor.context = controller_ptr;
    controller_ptr->dbus_descriptor.interfaces = g_jackcontroller_interfaces;

    if (!dbus_connection_register_object_path(
            connection,
            JACK_CONTROLLER_OBJECT_PATH,
            &vtable,
            &controller_ptr->dbus_descriptor))
    {
        jack_error("Ran out of memory trying to register D-Bus object path");
        goto fail_destroy_params;
    }

    jack_controller_settings_load(controller_ptr);

    return controller_ptr;

fail_destroy_params:
    jack_params_destroy(controller_ptr->params);

fail_destroy_server:
    jackctl_server_destroy(controller_ptr->server);

fail_uninit_mutex:
    pthread_mutex_destroy(&controller_ptr->lock);

fail_free:
    free(controller_ptr);

fail:
    return NULL;
}

bool
jack_controller_add_slave_driver(
    struct jack_controller * controller_ptr,
    const char * driver_name)
{
    jackctl_driver_t * driver;
    struct jack_controller_slave_driver * driver_ptr;
    size_t len_old;
    size_t len_new;

    len_old = strlen(controller_ptr->slave_drivers_vparam_value.str);
    len_new = strlen(driver_name);
    if (len_old + len_new + 2 > sizeof(controller_ptr->slave_drivers_vparam_value.str))
    {
        jack_error("No more space for slave drivers.");
        return false;
    }

    driver = jack_controller_find_driver(controller_ptr->server, driver_name);
    if (driver == NULL)
    {
        jack_error("Unknown driver \"%s\"", driver_name);
        return false;
    }

    if (jack_controller_check_slave_driver(controller_ptr, driver_name))
    {
        jack_info("Driver \"%s\" is already slave", driver_name);
        return true;
    }

    driver_ptr = malloc(sizeof(struct jack_controller_slave_driver));
    if (driver_ptr == NULL)
    {
        jack_error("malloc() failed to allocate jack_controller_slave_driver struct");
        return false;
    }

    driver_ptr->name = strdup(driver_name);
    if (driver_ptr->name == NULL)
    {
        jack_error("strdup() failed for slave driver name \"%s\"", driver_name);
        free(driver_ptr);
        return false;
    }

    driver_ptr->handle = driver;
    driver_ptr->loaded = false;

    jack_info("driver \"%s\" set as slave", driver_name);

    list_add_tail(&driver_ptr->siblings, &controller_ptr->slave_drivers);

    if (len_old != 0)
    {
        controller_ptr->slave_drivers_vparam_value.str[len_old++] = ',';
    }

    memcpy(controller_ptr->slave_drivers_vparam_value.str + len_old, driver_name, len_new + 1);
    controller_ptr->slave_drivers_set = true;

    return true;
}

bool
jack_controller_remove_slave_driver(
    struct jack_controller * controller_ptr,
    const char * driver_name)
{
    struct list_head * node_ptr;
    struct jack_controller_slave_driver * driver_ptr;

    list_for_each(node_ptr, &controller_ptr->slave_drivers)
    {
        driver_ptr = list_entry(node_ptr, struct jack_controller_slave_driver, siblings);
        if (strcmp(driver_ptr->name, driver_name) == 0)
        {
            list_del(&driver_ptr->siblings);
            free(driver_ptr->name);
            free(driver_ptr);

            /* update the slave-drivers param value */
            controller_ptr->slave_drivers_vparam_value.str[0] = 0;
            list_for_each(node_ptr, &controller_ptr->slave_drivers)
            {
                driver_ptr = list_entry(node_ptr, struct jack_controller_slave_driver, siblings);
                if (controller_ptr->slave_drivers_vparam_value.str[0] != 0)
                {
                    strcat(controller_ptr->slave_drivers_vparam_value.str, ",");
                }

                strcat(controller_ptr->slave_drivers_vparam_value.str, driver_ptr->name);
            }

            jack_info("driver \"%s\" is not slave anymore", driver_name);

            return true;
        }
    }

    return false;
}

bool
jack_controller_load_internal(
    struct jack_controller *controller_ptr,
    const char * internal_name)
{
    jackctl_internal_t *internal;

    internal = jack_controller_find_internal(controller_ptr->server, internal_name);
    if (internal == NULL)
    {
        return false;
    }

    jack_info("internal \"%s\" selected", internal_name);

    return jackctl_server_load_internal(controller_ptr->server, internal);
}

bool
jack_controller_unload_internal(
    struct jack_controller *controller_ptr,
    const char * internal_name)
{
    jackctl_internal_t *internal;

    internal = jack_controller_find_internal(controller_ptr->server, internal_name);
    if (internal == NULL)
    {
        return false;
    }

    jack_info("internal \"%s\" selected", internal_name);

    return jackctl_server_unload_internal(controller_ptr->server, internal);
}

#define controller_ptr ((struct jack_controller *)context)

void
jack_controller_destroy(
        void * context)
{
    if (controller_ptr->started)
    {
        while (!jack_controller_stop_server(controller_ptr, NULL))
        {
            jack_info("jack server failed to stop, retrying in 3 seconds...");
            usleep(3000000);
        }
    }

    jack_controller_remove_slave_drivers(controller_ptr);
    jack_params_destroy(controller_ptr->params);
    jackctl_server_destroy(controller_ptr->server);
    pthread_mutex_destroy(&controller_ptr->lock);
    free(controller_ptr);
}

void
jack_controller_run(
    void * context)
{
    struct sysinfo si;

    if (controller_ptr->pending_save == 0)
    {
        return;
    }

    if (sysinfo(&si) != 0)
    {
        jack_error("sysinfo() failed with %d", errno);
    }
    else if (si.uptime < controller_ptr->pending_save + 2) /* delay save by two seconds */
    {
        return;
    }

    controller_ptr->pending_save = 0;
    jack_controller_settings_save_auto(controller_ptr);
}

#undef controller_ptr

void
jack_controller_pending_save(
    struct jack_controller * controller_ptr)
{
    struct sysinfo si;

    if (sysinfo(&si) != 0)
    {
        jack_error("sysinfo() failed with %d.", errno);
        controller_ptr->pending_save = 0;
        jack_controller_settings_save_auto(controller_ptr);
        return;
    }

    controller_ptr->pending_save = si.uptime;
}
