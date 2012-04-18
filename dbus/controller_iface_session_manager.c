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
#include "jack/control.h"

#define JACK_DBUS_IFACE_NAME "org.jackaudio.SessionManager"

static
void
jack_controller_control_send_signal_session_state_changed(
    jack_session_event_type_t type,
    const char * target)
{
    dbus_uint32_t u32;

    u32 = type;
    if (target == NULL)
    {
        target = "";
    }

    jack_dbus_send_signal(
        JACK_CONTROLLER_OBJECT_PATH,
        JACK_DBUS_IFACE_NAME,
        "StateChanged",
        DBUS_TYPE_UINT32,
        &u32,
        DBUS_TYPE_STRING,
        &target,
        DBUS_TYPE_INVALID);
}

static bool start_detached_thread(void * (* start_routine)(void *), void * arg)
{
    int ret;
    static pthread_attr_t attr;
    pthread_t tid;

    ret = pthread_attr_init(&attr);
    if (ret != 0)
    {
        jack_error("pthread_attr_init() failed with %d", ret);
        goto exit;
    }

    ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (ret != 0)
    {
        jack_error("pthread_attr_setdetachstate() failed with %d", ret);
        goto destroy_attr;
    }

    ret = pthread_create(&tid, &attr, start_routine, arg);
    if (ret != 0)
    {
        jack_error("pthread_create() failed with %d", ret);
        goto destroy_attr;
    }

    jack_log("Detached thread %d created", (int)tid);

destroy_attr:
    pthread_attr_destroy(&attr);
exit:
    return ret == 0;
}

static void send_session_notify_reply(struct jack_session_pending_command * pending_cmd_ptr, jack_session_command_t * commands)
{
    struct jack_dbus_method_call call;
    const jack_session_command_t * cmd_ptr;
    DBusMessageIter top_iter, array_iter, struct_iter;
    dbus_uint32_t u32;

    /* jack_dbus_error() wants call struct */
    call.message = pending_cmd_ptr->message;
    call.connection = pending_cmd_ptr->connection;

    if (commands == NULL)
    {
        jack_dbus_error(&call, JACK_DBUS_ERROR_GENERIC, "jack_session_notify() failed");
        goto send_reply;
    }

    jack_info("Session notify complete, commands follow:");

    call.reply = dbus_message_new_method_return(pending_cmd_ptr->message);
    if (call.reply == NULL)
    {
        goto oom;
    }

    dbus_message_iter_init_append(call.reply, &top_iter);

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

        jack_info("uuid='%s', client='%s', command='%s', flags=0x%"PRIX32, cmd_ptr->uuid, cmd_ptr->client_name, cmd_ptr->command, u32);

        if (!dbus_message_iter_close_container(&array_iter, &struct_iter))
        {
            goto close_array;
        }
    }

    jack_info("End of session commands.");

    if (!dbus_message_iter_close_container(&top_iter, &array_iter))
    {
        goto unref;
    }

    goto send_reply;

close_struct:
    dbus_message_iter_close_container(&array_iter, &struct_iter);
close_array:
    dbus_message_iter_close_container(&top_iter, &array_iter);
unref:
    dbus_message_unref(call.reply);
    goto oom;

send_reply:
    if (call.reply != NULL)
    {
        if (!dbus_connection_send(pending_cmd_ptr->connection, call.reply, NULL))
        {
            jack_error("Ran out of memory trying to queue method return");
        }

        dbus_connection_flush(pending_cmd_ptr->connection);
        dbus_message_unref(call.reply);
    }
    else
    {
oom:
        jack_error("Ran out of memory trying to construct method return");
    }
}

#define controller_ptr ((struct jack_controller *)context)
void * jack_controller_process_session_command_thread(void * context)
{
    struct jack_session_pending_command * pending_cmd_ptr;
    jack_session_command_t * commands;

    jack_log("jack_controller_process_session_command_thread enter");

    pthread_mutex_lock(&controller_ptr->lock);
loop:
    /* get next command */
    assert(!list_empty(&controller_ptr->session_pending_commands));
    pending_cmd_ptr = list_entry(controller_ptr->session_pending_commands.next, struct jack_session_pending_command, siblings);
    pthread_mutex_unlock(&controller_ptr->lock);

    jack_info("Session notify initiated. target='%s', type=%d, path='%s'", pending_cmd_ptr->target, (int)pending_cmd_ptr->type, pending_cmd_ptr->path);

    jack_controller_control_send_signal_session_state_changed(pending_cmd_ptr->type, pending_cmd_ptr->target);

    commands = jack_session_notify(controller_ptr->client, pending_cmd_ptr->target, pending_cmd_ptr->type, pending_cmd_ptr->path);
    send_session_notify_reply(pending_cmd_ptr, commands);
    if (commands != NULL)
    {
        jack_session_commands_free(commands);
    }

    pthread_mutex_lock(&controller_ptr->lock);

    /* keep state consistent by sending signal after to lock */
    /* otherwise the main thread may receive not-to-be-queued request and fail */
    jack_controller_control_send_signal_session_state_changed(0, NULL);

    /* remove the head of the list (queue) */
    assert(!list_empty(&controller_ptr->session_pending_commands));
    assert(pending_cmd_ptr == list_entry(controller_ptr->session_pending_commands.next, struct jack_session_pending_command, siblings));
    list_del(&pending_cmd_ptr->siblings);

    /* command cleanup */
    dbus_message_unref(pending_cmd_ptr->message);
    dbus_connection_ref(pending_cmd_ptr->connection);
    free(pending_cmd_ptr);

    /* If there are more commands, process them. Otherwise - exit the thread */
    if (!list_empty(&controller_ptr->session_pending_commands))
    {
        goto loop;
    }

    pthread_mutex_unlock(&controller_ptr->lock);

    jack_log("jack_controller_process_session_command_thread exit");
    return NULL;
}

#undef controller_ptr
#define controller_ptr ((struct jack_controller *)call->context)

static
void
jack_controller_dbus_session_notify(
    struct jack_dbus_method_call * call)
{
    dbus_bool_t queue;
    const char * target;
    dbus_uint32_t u32;
    const char * path;
    jack_session_event_type_t type;
    struct jack_session_pending_command * cmd_ptr;

    if (!controller_ptr->started)
    {
        jack_dbus_only_error(call, JACK_DBUS_ERROR_SERVER_NOT_RUNNING, "Can't execute method '%s' with stopped JACK server", call->method_name);
        return;
    }

    if (!jack_dbus_get_method_args(
            call,
            DBUS_TYPE_BOOLEAN,
            &queue,
            DBUS_TYPE_STRING,
            &target,
            DBUS_TYPE_UINT32,
            &u32,
            DBUS_TYPE_STRING,
            &path,
            DBUS_TYPE_INVALID))
    {
        /* The method call had invalid arguments meaning that jack_dbus_get_method_args() has constructed an error for us. */
        return;
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
        return;
    }

    pthread_mutex_lock(&controller_ptr->lock);
    if (list_empty(&controller_ptr->session_pending_commands))
    {
        if (!start_detached_thread(jack_controller_process_session_command_thread, controller_ptr))
        {
            jack_dbus_error(call, JACK_DBUS_ERROR_GENERIC, "Cannot start thread to process the command");
            goto unlock;
        }

        jack_log("Session notify thread started");
    }
    else if (!queue)
    {
        jack_dbus_error(call, JACK_DBUS_ERROR_GENERIC, "Busy");
        goto unlock;
    }

    cmd_ptr = malloc(sizeof(struct jack_session_pending_command));
    if (cmd_ptr == NULL)
    {
        jack_dbus_error(call, JACK_DBUS_ERROR_GENERIC, "malloc() failed for jack_session_pending_command struct");
        goto unlock;
    }

    cmd_ptr->message = dbus_message_ref(call->message);
    call->message = NULL; /* mark that reply will be sent asynchronously */
    cmd_ptr->connection = dbus_connection_ref(call->connection);

    /* it is safe to use the retrived pointers because we already made an additional message reference */
    cmd_ptr->type = type;
    cmd_ptr->target = target;
    cmd_ptr->path = path;

    list_add_tail(&cmd_ptr->siblings, &controller_ptr->session_pending_commands);

    jack_log("Session notify scheduled. target='%s', type=%"PRIu32", path='%s'", target, u32, path);

unlock:
    pthread_mutex_unlock(&controller_ptr->lock);
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

static
void
jack_controller_dbus_get_session_state(
    struct jack_dbus_method_call * call)
{
    DBusMessageIter iter;
    struct jack_session_pending_command * cmd_ptr;
    const char * target;
    dbus_uint32_t type;
    bool append_failed;

    call->reply = dbus_message_new_method_return(call->message);
    if (call->reply == NULL)
    {
        goto oom;
    }

    dbus_message_iter_init_append(call->reply, &iter);

    pthread_mutex_lock(&controller_ptr->lock);

    if (list_empty(&controller_ptr->session_pending_commands))
    {
        type = 0;
        target = "";
    }
    else
    {
        cmd_ptr = list_entry(controller_ptr->session_pending_commands.next, struct jack_session_pending_command, siblings);
        type = (dbus_uint32_t)cmd_ptr->type;
        target = cmd_ptr->target;
    }

    append_failed =
        !dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT32, &type) ||
        !dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &target);

    pthread_mutex_unlock(&controller_ptr->lock);

    if (!append_failed)
    {
        return;
    }

    dbus_message_unref(call->reply);
    call->reply = NULL;
oom:
    jack_error("Ran out of memory trying to construct method return");
}

#undef controller_ptr

JACK_DBUS_METHOD_ARGUMENTS_BEGIN(Notify)
    JACK_DBUS_METHOD_ARGUMENT("queue", DBUS_TYPE_BOOLEAN_AS_STRING, false)
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

JACK_DBUS_METHOD_ARGUMENTS_BEGIN(GetState)
    JACK_DBUS_METHOD_ARGUMENT("type", DBUS_TYPE_UINT32_AS_STRING, true)
    JACK_DBUS_METHOD_ARGUMENT("target", DBUS_TYPE_STRING_AS_STRING, true)
JACK_DBUS_METHOD_ARGUMENTS_END

JACK_DBUS_SIGNAL_ARGUMENTS_BEGIN(StateChanged)
    JACK_DBUS_SIGNAL_ARGUMENT("type", DBUS_TYPE_UINT32_AS_STRING)
    JACK_DBUS_SIGNAL_ARGUMENT("target", DBUS_TYPE_STRING_AS_STRING)
JACK_DBUS_SIGNAL_ARGUMENTS_END

JACK_DBUS_METHODS_BEGIN
    JACK_DBUS_METHOD_DESCRIBE(Notify, jack_controller_dbus_session_notify)
    JACK_DBUS_METHOD_DESCRIBE(GetUuidForClientName, jack_controller_dbus_get_uuid_for_client_name)
    JACK_DBUS_METHOD_DESCRIBE(GetClientNameByUuid, jack_controller_dbus_get_client_name_by_uuid)
    JACK_DBUS_METHOD_DESCRIBE(ReserveClientName, jack_controller_dbus_reserve_client_name)
    JACK_DBUS_METHOD_DESCRIBE(HasSessionCallback, jack_controller_dbus_has_session_callback)
    JACK_DBUS_METHOD_DESCRIBE(GetState, jack_controller_dbus_get_session_state)
JACK_DBUS_METHODS_END

JACK_DBUS_SIGNALS_BEGIN
    JACK_DBUS_SIGNAL_DESCRIBE(StateChanged)
JACK_DBUS_SIGNALS_END

JACK_DBUS_IFACE_BEGIN(g_jack_controller_iface_session_manager, JACK_DBUS_IFACE_NAME)
    JACK_DBUS_IFACE_EXPOSE_METHODS
    JACK_DBUS_IFACE_EXPOSE_SIGNALS
JACK_DBUS_IFACE_END
