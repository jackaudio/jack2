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
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dbus/dbus.h>
#include <time.h>

#include "controller_internal.h"
#include "jackdbus.h"

bool
jack_controller_settings_write_string(int fd, const char * string, void *dbus_call_context_ptr)
{
    size_t len;

    len = strlen(string);

    if (write(fd, string, len) != len)
    {
        jack_dbus_error(dbus_call_context_ptr, JACK_DBUS_ERROR_GENERIC, "write() failed to write config file.");
        return false;
    }

    return true;
}

struct save_context
{
    void * call;
    int fd;
    const char * indent;
    jack_params_handle params;
    const char * address[PARAM_ADDRESS_SIZE];
    const char * str;
};

#define ctx_ptr ((struct save_context *)context)
#define fd (ctx_ptr->fd)

static bool jack_controller_serialize_parameter(void * context, const struct jack_parameter * param_ptr)
{
    char value[JACK_PARAM_STRING_MAX + 1];

    if (!param_ptr->vtable.is_set(param_ptr->obj))
    {
        return true;
    }

    jack_controller_serialize_parameter_value(param_ptr, value);

    return
        jack_controller_settings_write_string(fd, ctx_ptr->indent, ctx_ptr->call) &&
        jack_controller_settings_write_string(fd, "<option name=\"", ctx_ptr->call) &&
        jack_controller_settings_write_string(fd, param_ptr->name, ctx_ptr->call) &&
        jack_controller_settings_write_string(fd, "\">", ctx_ptr->call) &&
        jack_controller_settings_write_string(fd, value, ctx_ptr->call) &&
        jack_controller_settings_write_string(fd, "</option>\n", ctx_ptr->call);
}

bool serialize_modules(void * context, const char * name)
{
    ctx_ptr->indent = "   ";
    ctx_ptr->address[1] = name;
    ctx_ptr->address[2] = NULL;

    return
        jack_controller_settings_write_string(fd, "  <", ctx_ptr->call) &&
        jack_controller_settings_write_string(fd, ctx_ptr->str, ctx_ptr->call) &&
        jack_controller_settings_write_string(fd, " name=\"", ctx_ptr->call) &&
        jack_controller_settings_write_string(fd, name, ctx_ptr->call) &&
        jack_controller_settings_write_string(fd, "\">\n", ctx_ptr->call) &&
        jack_params_iterate_params(ctx_ptr->params, ctx_ptr->address, jack_controller_serialize_parameter, ctx_ptr) &&
        jack_controller_settings_write_string(fd, "  </", ctx_ptr->call) &&
        jack_controller_settings_write_string(fd, ctx_ptr->str, ctx_ptr->call) &&
        jack_controller_settings_write_string(fd, ">\n", ctx_ptr->call);
}

#undef fd
#undef ctx_ptr

bool
jack_controller_settings_save(
    struct jack_controller * controller_ptr,
    void *dbus_call_context_ptr)
{
    char *filename;
    size_t conf_len;
    int fd;
    bool ret;
    time_t timestamp;
    char timestamp_str[26];
    struct save_context context;
    const char * modules[] = {"driver", "internal", NULL};
    char buffer[100];
    unsigned int i;

    time(&timestamp);
    ctime_r(&timestamp, timestamp_str);
    timestamp_str[24] = 0;

    ret = false;

    conf_len = strlen(JACKDBUS_CONF);

    filename = malloc(g_jackdbus_config_dir_len + conf_len + 1);
    if (filename == NULL)
    {
        jack_error("Out of memory.");
        goto exit;
    }

    memcpy(filename, g_jackdbus_config_dir, g_jackdbus_config_dir_len);
    memcpy(filename + g_jackdbus_config_dir_len, JACKDBUS_CONF, conf_len);
    filename[g_jackdbus_config_dir_len + conf_len] = 0;

    jack_info("Saving settings to \"%s\" ...", filename);

    fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1)
    {
        jack_error("open() failed to open conf filename. error is %d (%s)", errno, strerror(errno));
        goto exit_free_filename;
    }

    context.fd = fd;
    context.call = dbus_call_context_ptr;

    if (!jack_controller_settings_write_string(fd, "<?xml version=\"1.0\"?>\n", dbus_call_context_ptr))
    {
        goto exit_close;
    }

    if (!jack_controller_settings_write_string(fd, "<!--\n", dbus_call_context_ptr))
    {
        goto exit_close;
    }

    if (!jack_controller_settings_write_string(fd, JACK_CONF_HEADER_TEXT, dbus_call_context_ptr))
    {
        goto exit_close;
    }

    if (!jack_controller_settings_write_string(fd, "-->\n", dbus_call_context_ptr))
    {
        goto exit_close;
    }

    if (!jack_controller_settings_write_string(fd, "<!-- ", dbus_call_context_ptr))
    {
        goto exit_close;
    }

    if (!jack_controller_settings_write_string(fd, timestamp_str, dbus_call_context_ptr))
    {
        goto exit_close;
    }

    if (!jack_controller_settings_write_string(fd, " -->\n", dbus_call_context_ptr))
    {
        goto exit_close;
    }

    if (!jack_controller_settings_write_string(fd, "<jack>\n", dbus_call_context_ptr))
    {
        goto exit_close;
    }
    
    /* engine */

    if (!jack_controller_settings_write_string(fd, " <engine>\n", dbus_call_context_ptr))
    {
        goto exit_close;
    }

    context.indent = "  ";
    context.address[0] = PTNODE_ENGINE;
    context.address[1] = NULL;
    if (!jack_params_iterate_params(controller_ptr->params, context.address, jack_controller_serialize_parameter, &context))
    {
        goto exit_close;
    }

    if (!jack_controller_settings_write_string(fd, " </engine>\n", dbus_call_context_ptr))
    {
        goto exit_close;
    }

    for (i = 0; modules[i] != NULL; i++)
    {
        if (!jack_controller_settings_write_string(fd, " <", dbus_call_context_ptr))
        {
            goto exit_close;
        }

        if (!jack_controller_settings_write_string(fd, modules[i], dbus_call_context_ptr))
        {
            goto exit_close;
        }

        if (!jack_controller_settings_write_string(fd, "s>\n", dbus_call_context_ptr))
        {
            goto exit_close;
        }

        context.indent = "  ";
        context.params = controller_ptr->params;
        context.str = modules[i];
        strcpy(buffer, modules[i]);
        strcat(buffer, "s");
        context.address[0] = buffer;
        context.address[1] = NULL;

        if (!jack_params_iterate_container(controller_ptr->params, context.address, serialize_modules, &context))
        {
            goto exit_close;
        }

        if (!jack_controller_settings_write_string(fd, " </", dbus_call_context_ptr))
        {
            goto exit_close;
        }

        if (!jack_controller_settings_write_string(fd, modules[i], dbus_call_context_ptr))
        {
            goto exit_close;
        }

        if (!jack_controller_settings_write_string(fd, "s>\n", dbus_call_context_ptr))
        {
            goto exit_close;
        }
    }

    if (!jack_controller_settings_write_string(fd, "</jack>\n", dbus_call_context_ptr))
    {
        goto exit_close;
    }

    ret = true;

exit_close:
    close(fd);

exit_free_filename:
    free(filename);

exit:
    return ret;
}

void
jack_controller_settings_save_auto(
    struct jack_controller * controller_ptr)
{
    jack_controller_settings_save(controller_ptr, NULL);
}
