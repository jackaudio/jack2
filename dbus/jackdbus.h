/* -*- Mode: C ; c-basic-offset: 4 -*- */
/*
    Copyright (C) 2007,2008 Nedko Arnaudov
    Copyright (C) 2007-2008 Juuso Alasuutari
    Copyright (C) 2008 Marc-Olivier Barre
    
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

#ifndef DBUS_H__3DB2458F_44B2_43EA_882A_9F888DF71A88__INCLUDED
#define DBUS_H__3DB2458F_44B2_43EA_882A_9F888DF71A88__INCLUDED

#include <stdbool.h>

#define JACK_DBUS_DEBUG

//#define DISABLE_SIGNAL_MAGIC

#define DEFAULT_XDG_CONFIG "/.config"
#define DEFAULT_XDG_LOG "/.log"
#define JACKDBUS_DIR "/jack"
#define JACKDBUS_LOG "/jackdbus.log"
#define JACKDBUS_CONF "/conf.xml"

extern char *g_jackdbus_config_dir;
extern size_t g_jackdbus_config_dir_len; /* without terminating '\0' char */
extern int g_exit_command;

bool
jack_controller_settings_init();

void
jack_controller_settings_uninit();

#define JACK_DBUS_ERROR_UNKNOWN_METHOD              "org.jackaudio.Error.UnknownMethod"
#define JACK_DBUS_ERROR_SERVER_NOT_RUNNING          "org.jackaudio.Error.ServerNotRunning"
#define JACK_DBUS_ERROR_SERVER_RUNNING              "org.jackaudio.Error.ServerRunning"
#define JACK_DBUS_ERROR_UNKNOWN_DRIVER              "org.jackaudio.Error.UnknownDriver"
#define JACK_DBUS_ERROR_UNKNOWN_INTERNAL            "org.jackaudio.Error.UnknownInternal"
#define JACK_DBUS_ERROR_UNKNOWN_PARAMETER           "org.jackaudio.Error.UnknownParameter"
#define JACK_DBUS_ERROR_INVALID_ARGS                "org.jackaudio.Error.InvalidArgs"
#define JACK_DBUS_ERROR_GENERIC                     "org.jackaudio.Error.Generic"
#define JACK_DBUS_ERROR_FATAL                       "org.jackaudio.Error.Fatal"

struct jack_dbus_method_call
{
    void *context;
    DBusConnection *connection;
    const char *method_name;
    DBusMessage *message;
    DBusMessage *reply;
};

struct jack_dbus_interface_method_argument_descriptor
{
    const char * name;
    const char * type;
    bool direction_out;     /* true - out, false - in */
};

struct jack_dbus_interface_method_descriptor
{
    const char * name;
    const struct jack_dbus_interface_method_argument_descriptor * arguments;
    void (* handler)(struct jack_dbus_method_call * call);
};

struct jack_dbus_interface_signal_argument_descriptor
{
    const char * name;
    const char * type;
};

struct jack_dbus_interface_signal_descriptor
{
    const char * name;
    const struct jack_dbus_interface_signal_argument_descriptor * arguments;
};

struct jack_dbus_interface_descriptor
{
    const char * name;

    bool
    (* handler)(
        struct jack_dbus_method_call * call,
        const struct jack_dbus_interface_method_descriptor * methods);

    const struct jack_dbus_interface_method_descriptor * methods;
    const struct jack_dbus_interface_signal_descriptor * signals;
};

struct jack_dbus_object_descriptor
{
    struct jack_dbus_interface_descriptor ** interfaces;
    void * context;
};

typedef union
{
    unsigned char byte;
    dbus_bool_t boolean;
    dbus_int16_t int16;
    dbus_uint16_t uint16;
    dbus_int32_t int32;
    dbus_uint32_t uint32;
    dbus_int64_t int64;
    dbus_uint64_t uint64;
    double doubl;
    const char *string;
} message_arg_t;

#define JACK_DBUS_METHOD_ARGUMENTS_BEGIN(method_name)                                   \
static const                                                                            \
struct jack_dbus_interface_method_argument_descriptor method_name ## _arguments[] =     \
{

#define JACK_DBUS_METHOD_ARGUMENTS_BEGIN_EX(method_name, descr)                         \
static const                                                                            \
struct jack_dbus_interface_method_argument_descriptor method_name ## _arguments[] =     \
{

#define JACK_DBUS_METHOD_ARGUMENT(argument_name, argument_type, argument_direction_out) \
        {                                                                               \
                .name = argument_name,                                                  \
                .type = argument_type,                                                  \
                .direction_out = argument_direction_out                                 \
        },

#define JACK_DBUS_METHOD_ARGUMENT_IN(argument_name, argument_type, descr)               \
        {                                                                               \
                .name = argument_name,                                                  \
                .type = argument_type,                                                  \
                .direction_out = false                                                  \
        },

#define JACK_DBUS_METHOD_ARGUMENT_OUT(argument_name, argument_type, descr)              \
        {                                                                               \
                .name = argument_name,                                                  \
                .type = argument_type,                                                  \
                .direction_out = true                                                  \
        },

#define JACK_DBUS_METHOD_ARGUMENT(argument_name, argument_type, argument_direction_out) \
        {                                                                               \
                .name = argument_name,                                                  \
                .type = argument_type,                                                  \
                .direction_out = argument_direction_out                                 \
        },

#define JACK_DBUS_METHOD_ARGUMENTS_END                                                  \
    JACK_DBUS_METHOD_ARGUMENT(NULL, NULL, false)                                        \
};

#define JACK_DBUS_METHODS_BEGIN                                                         \
static const                                                                            \
struct jack_dbus_interface_method_descriptor methods_dtor[] =                           \
{

#define JACK_DBUS_METHOD_DESCRIBE(method_name, handler_name)                            \
        {                                                                               \
            .name = # method_name,                                                      \
            .arguments = method_name ## _arguments,                                     \
            .handler = handler_name                                                     \
        },

#define JACK_DBUS_METHODS_END                                                           \
        {                                                                               \
            .name = NULL,                                                               \
            .arguments = NULL,                                                          \
            .handler = NULL                                                             \
        }                                                                               \
};

#define JACK_DBUS_SIGNAL_ARGUMENTS_BEGIN(signal_name)                                   \
static const                                                                            \
struct jack_dbus_interface_signal_argument_descriptor signal_name ## _arguments[] =     \
{

#define JACK_DBUS_SIGNAL_ARGUMENT(argument_name, argument_type)                         \
        {                                                                               \
                .name = argument_name,                                                  \
                .type = argument_type                                                   \
        },

#define JACK_DBUS_SIGNAL_ARGUMENTS_END                                                  \
        JACK_DBUS_SIGNAL_ARGUMENT(NULL, NULL)                                           \
};

#define JACK_DBUS_SIGNALS_BEGIN                                                         \
static const                                                                            \
struct jack_dbus_interface_signal_descriptor signals_dtor[] =                           \
{

#define JACK_DBUS_SIGNAL_DESCRIBE(signal_name)                                          \
        {                                                                               \
                .name = # signal_name,                                                  \
                .arguments = signal_name ## _arguments                                  \
        },

#define JACK_DBUS_SIGNALS_END                                                           \
        {                                                                               \
                .name = NULL,                                                           \
                .arguments = NULL,                                                      \
        }                                                                               \
};

#define JACK_DBUS_IFACE_BEGIN(iface_var, iface_name)                                    \
struct jack_dbus_interface_descriptor iface_var =                                       \
{                                                                                       \
        .name = iface_name,                                                             \
        .handler = jack_dbus_run_method,

#define JACK_DBUS_IFACE_HANDLER(handler_func)                                           \
        .handler = handler_func,

#define JACK_DBUS_IFACE_EXPOSE_METHODS                                                  \
        .methods = methods_dtor,

#define JACK_DBUS_IFACE_EXPOSE_SIGNALS                                                  \
        .signals = signals_dtor,

#define JACK_DBUS_IFACE_END                                                             \
};

DBusHandlerResult
jack_dbus_message_handler(
    DBusConnection *connection,
    DBusMessage *message,
    void *data);

void
jack_dbus_message_handler_unregister(
    DBusConnection *connection,
    void *data);

bool
jack_dbus_run_method(
    struct jack_dbus_method_call * call,
    const struct jack_dbus_interface_method_descriptor * methods);

void
jack_dbus_error(
    void *dbus_call_context_ptr,
    const char *error_name,
    const char *format,
    ...);

void
jack_dbus_only_error(
    void *dbus_call_context_ptr,
    const char *error_name,
    const char *format,
    ...);

bool
jack_dbus_get_method_args(
    struct jack_dbus_method_call *call,
    int type,
    ...);

bool
jack_dbus_get_method_args_string_and_variant(
    struct jack_dbus_method_call *call,
    const char **arg1,
    message_arg_t *arg2,
    int *type_ptr);

bool
jack_dbus_get_method_args_two_strings_and_variant(
    struct jack_dbus_method_call *call,
    const char **arg1,
    const char **arg2,
    message_arg_t *arg3,
    int *type_ptr);
    
bool
jack_dbus_message_append_variant(
    DBusMessageIter *iter,
    int type,
    const char *signature,
    message_arg_t *arg);

void
jack_dbus_construct_method_return_empty(
    struct jack_dbus_method_call * call);

void
jack_dbus_construct_method_return_single(
    struct jack_dbus_method_call *call,
    int type,
    message_arg_t arg);

void
jack_dbus_construct_method_return_array_of_strings(
    struct jack_dbus_method_call *call,
    unsigned int num_members,
    const char **array);

void
jack_dbus_send_signal(
    const char *sender_object_path,
    const char *iface,
    const char *signal_name,
    int first_arg_type,
    ...);

#define JACK_CONTROLLER_OBJECT_PATH "/org/jackaudio/Controller"

extern struct jack_dbus_interface_descriptor * g_jackcontroller_interfaces[];
extern DBusConnection * g_connection;

#endif /* #ifndef DBUS_H__3DB2458F_44B2_43EA_882A_9F888DF71A88__INCLUDED */
