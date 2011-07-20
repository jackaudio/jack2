/* -*- Mode: C ; c-basic-offset: 4 -*- */
/*
    Copyright (C) 2011 Nedko Arnaudov

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
#include "controller_internal.h"
#include "jack/session.h"

#define controller_ptr ((struct jack_controller *)call->context)

static
void
jack_controller_dbus_session_notify(
    struct jack_dbus_method_call * call)
{
    const char * target;
    dbus_uint32_t u32;
    const char * path;
    jack_session_event_type_t type;
    jack_session_command_t * commands;
    const jack_session_command_t * cmd_ptr;
    DBusMessageIter top_iter, array_iter, struct_iter;

    if (!jack_dbus_get_method_args(
            call,
            DBUS_TYPE_STRING,
            &target,
            DBUS_TYPE_UINT32,
            &u32,
            DBUS_TYPE_STRING,
            &path,
            DBUS_TYPE_INVALID))
    {
        /* The method call had invalid arguments meaning that jack_dbus_get_method_args() has constructed an error for us. */
        goto exit;
    }

    if (*target == 0)
    {
        target = NULL;
    }

    type = (jack_session_event_type_t)u32;

    if (type != JackSessionSave &&
        type != JackSessionSaveAndQuit &&
        type != JackSessionSaveTemplate)
    {
        jack_dbus_error(call, JACK_DBUS_ERROR_INVALID_ARGS, "Invalid session event type %" PRIu32, u32);
        goto exit;
    }

    commands = jack_session_notify(controller_ptr->client, target, type, path);
    if (commands == NULL)
    {
        jack_dbus_error(call, JACK_DBUS_ERROR_GENERIC, "jack_session_notify() failed");
        goto exit;
    }

    call->reply = dbus_message_new_method_return(call->message);
    if (call->reply == NULL)
    {
        goto oom;
    }

    dbus_message_iter_init_append(call->reply, &top_iter);

    if (!dbus_message_iter_open_container(&top_iter, DBUS_TYPE_ARRAY, "(sssu)", &array_iter))
    {
        goto unref;
    }

    for (cmd_ptr = commands; cmd_ptr->uuid != NULL; cmd_ptr++)
    {
        if (!dbus_message_iter_open_container(&array_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter))
        {
            goto close_array;
        }

        if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &cmd_ptr->uuid))
        {
            goto close_struct;
        }

        if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &cmd_ptr->client_name))
        {
            goto close_struct;
        }

        if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &cmd_ptr->command))
        {
            goto close_struct;
        }

        u32 = cmd_ptr->flags;
        if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_UINT32, &u32))
        {
            goto close_struct;
        }

        if (!dbus_message_iter_close_container(&array_iter, &struct_iter))
        {
            goto close_array;
        }
    }

    if (!dbus_message_iter_close_container(&top_iter, &array_iter))
    {
        goto unref;
    }

    goto free;

close_struct:
    dbus_message_iter_close_container(&array_iter, &struct_iter);
close_array:
    dbus_message_iter_close_container(&top_iter, &array_iter);
unref:
    dbus_message_unref(call->reply);
    call->reply = NULL;
oom:
    jack_error("Ran out of memory trying to construct method return");
free:
    jack_session_commands_free(commands);
exit:
    return;
}

static
void
jack_controller_dbus_get_uuid_for_client_name(
    struct jack_dbus_method_call * call)
{
    const char * client_name;
    char * client_uuid;

    if (!jack_dbus_get_method_args(
            call,
            DBUS_TYPE_STRING,
            &client_name,
            DBUS_TYPE_INVALID))
    {
        /* The method call had invalid arguments meaning that jack_dbus_get_method_args() has constructed an error for us. */
        return;
    }

    client_uuid = jack_get_uuid_for_client_name(controller_ptr->client, client_name);
    if (client_uuid == NULL)
    {
        jack_dbus_error(call, JACK_DBUS_ERROR_GENERIC, "jack_get_uuid_for_client_name(\"%s\") failed", client_name);
        return;
    }

    jack_dbus_construct_method_return_single(call, DBUS_TYPE_STRING, (message_arg_t)(const char *)client_uuid);
    free(client_uuid);
}

static
void
jack_controller_dbus_get_client_name_by_uuid(
    struct jack_dbus_method_call * call)
{
    const char * client_uuid;
    char * client_name;

    if (!jack_dbus_get_method_args(
            call,
            DBUS_TYPE_STRING,
            &client_uuid,
            DBUS_TYPE_INVALID))
    {
        /* The method call had invalid arguments meaning that jack_dbus_get_method_args() has constructed an error for us. */
        return;
    }

    client_name = jack_get_client_name_by_uuid(controller_ptr->client, client_uuid);
    if (client_name == NULL)
    {
        jack_dbus_error(call, JACK_DBUS_ERROR_GENERIC, "jack_get_client_name_by_uuid(\"%s\") failed", client_uuid);
        return;
    }

    jack_dbus_construct_method_return_single(call, DBUS_TYPE_STRING, (message_arg_t)(const char *)client_name);
    free(client_name);
}

static
void
jack_controller_dbus_reserve_client_name(
    struct jack_dbus_method_call * call)
{
    int ret;
    const char * client_name;
    const char * client_uuid;

    if (!jack_dbus_get_method_args(
            call,
            DBUS_TYPE_STRING,
            &client_name,
            DBUS_TYPE_STRING,
            &client_uuid,
            DBUS_TYPE_INVALID))
    {
        /* The method call had invalid arguments meaning that jack_dbus_get_method_args() has constructed an error for us. */
        return;
    }

    ret = jack_reserve_client_name(controller_ptr->client, client_name, client_uuid);
    if (ret < 0)
    {
        jack_dbus_error(call, JACK_DBUS_ERROR_GENERIC, "jack_reserve_client_name(name=\"%s\", uuid=\"%s\") failed (%d)", client_name, client_uuid, ret);
        return;
    }

    jack_dbus_construct_method_return_empty(call);
}

static
void
jack_controller_dbus_has_session_callback(
    struct jack_dbus_method_call * call)
{
    int ret;
    const char * client_name;
    message_arg_t retval;

    if (!jack_dbus_get_method_args(
            call,
            DBUS_TYPE_STRING,
            &client_name,
            DBUS_TYPE_INVALID))
    {
        /* The method call had invalid arguments meaning that jack_dbus_get_method_args() has constructed an error for us. */
        return;
    }

    ret = jack_client_has_session_callback(controller_ptr->client, client_name);
    if (ret < 0)
    {
        jack_dbus_error(call, JACK_DBUS_ERROR_GENERIC, "jack_client_has_session_callback(\"%s\") failed (%d)", client_name, ret);
        return;
    }

    retval.boolean = ret;
    jack_dbus_construct_method_return_single(call, DBUS_TYPE_BOOLEAN, retval);
}

#undef controller_ptr

JACK_DBUS_METHOD_ARGUMENTS_BEGIN(Notify)
    JACK_DBUS_METHOD_ARGUMENT("target", DBUS_TYPE_STRING_AS_STRING, false)
    JACK_DBUS_METHOD_ARGUMENT("type", DBUS_TYPE_UINT32_AS_STRING, false)
    JACK_DBUS_METHOD_ARGUMENT("path", DBUS_TYPE_STRING_AS_STRING, false)
    JACK_DBUS_METHOD_ARGUMENT("result", "a(sssu)", true)
JACK_DBUS_METHOD_ARGUMENTS_END

JACK_DBUS_METHOD_ARGUMENTS_BEGIN(GetUuidForClientName)
    JACK_DBUS_METHOD_ARGUMENT("name", DBUS_TYPE_STRING_AS_STRING, false)
    JACK_DBUS_METHOD_ARGUMENT("uuid", DBUS_TYPE_STRING_AS_STRING, true)
JACK_DBUS_METHOD_ARGUMENTS_END

JACK_DBUS_METHOD_ARGUMENTS_BEGIN(GetClientNameByUuid)
    JACK_DBUS_METHOD_ARGUMENT("uuid", DBUS_TYPE_STRING_AS_STRING, false)
    JACK_DBUS_METHOD_ARGUMENT("name", DBUS_TYPE_STRING_AS_STRING, true)
JACK_DBUS_METHOD_ARGUMENTS_END

JACK_DBUS_METHOD_ARGUMENTS_BEGIN(ReserveClientName)
    JACK_DBUS_METHOD_ARGUMENT("name", DBUS_TYPE_STRING_AS_STRING, false)
    JACK_DBUS_METHOD_ARGUMENT("uuid", DBUS_TYPE_STRING_AS_STRING, false)
JACK_DBUS_METHOD_ARGUMENTS_END

JACK_DBUS_METHOD_ARGUMENTS_BEGIN(HasSessionCallback)
    JACK_DBUS_METHOD_ARGUMENT("client_name", DBUS_TYPE_STRING_AS_STRING, false)
    JACK_DBUS_METHOD_ARGUMENT("has_session_callback", DBUS_TYPE_BOOLEAN_AS_STRING, true)
JACK_DBUS_METHOD_ARGUMENTS_END

JACK_DBUS_METHODS_BEGIN
    JACK_DBUS_METHOD_DESCRIBE(Notify, jack_controller_dbus_session_notify)
    JACK_DBUS_METHOD_DESCRIBE(GetUuidForClientName, jack_controller_dbus_get_uuid_for_client_name)
    JACK_DBUS_METHOD_DESCRIBE(GetClientNameByUuid, jack_controller_dbus_get_client_name_by_uuid)
    JACK_DBUS_METHOD_DESCRIBE(ReserveClientName, jack_controller_dbus_reserve_client_name)
    JACK_DBUS_METHOD_DESCRIBE(HasSessionCallback, jack_controller_dbus_has_session_callback)
JACK_DBUS_METHODS_END

JACK_DBUS_IFACE_BEGIN(g_jack_controller_iface_session_manager, "org.jackaudio.SessionManager")
    JACK_DBUS_IFACE_EXPOSE_METHODS
JACK_DBUS_IFACE_END
