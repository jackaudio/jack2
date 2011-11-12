/* -*- Mode: C ; c-basic-offset: 4 -*- */
/*
    Copyright (C) 2007,2008,2010 Nedko Arnaudov
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

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <signal.h>
#include <dbus/dbus.h>
#include <pthread.h>
#include <unistd.h>

#include "config.h"

#include "jackdbus.h"
#include "controller.h"
#include "jack/jack.h"
#include "jack/jslist.h"
#include "jack/control.h"
#include "sigsegv.h"
#include "svnversion.h"

static char * g_log_filename;
static ino_t g_log_file_ino;
FILE *g_logfile;
char *g_jackdbus_config_dir;
size_t g_jackdbus_config_dir_len; /* without terminating '\0' char */
char *g_jackdbus_log_dir;
size_t g_jackdbus_log_dir_len; /* without terminating '\0' char */
int g_exit_command;
DBusConnection *g_connection;

void
jack_dbus_send_signal(
    const char *sender_object_path,
    const char *iface,
    const char *signal_name,
    int first_arg_type,
    ...)
{
    DBusMessage *message_ptr;
    va_list ap;

    va_start(ap, first_arg_type);

    message_ptr = dbus_message_new_signal(sender_object_path, iface, signal_name);
    if (message_ptr == NULL)
    {
        jack_error("dbus_message_new_signal() failed.");
        goto exit;
    }

    if (!dbus_message_append_args_valist(message_ptr, first_arg_type, ap))
    {
        jack_error("dbus_message_append_args_valist() failed.");
        goto unref;
    }

    /* Add message to outgoing message queue */
    if (!dbus_connection_send(g_connection, message_ptr, NULL))
    {
        jack_error("dbus_connection_send() failed.");
        goto unref;
    }

unref:
    dbus_message_unref(message_ptr);

exit:
    va_end(ap);
}

/*
 * Send a method return.
 *
 * If call->reply is NULL (i.e. a message construct method failed
 * due to lack of memory) attempt to send a void method return.
 */
static
void
jack_dbus_send_method_return(
    struct jack_dbus_method_call * call)
{
    if (call->message == NULL)
    {
        /* async call */
        return;
    }

    if (call->reply)
    {
    retry_send:
        if (!dbus_connection_send (call->connection, call->reply, NULL))
        {
            jack_error ("Ran out of memory trying to queue method return");
        }

        dbus_connection_flush (call->connection);
        dbus_message_unref (call->reply);
        call->reply = NULL;
    }
    else
    {
        jack_error ("send_method_return() called with a NULL message,"
                    " trying to construct a void return...");

        if ((call->reply = dbus_message_new_method_return (call->message)))
        {
            goto retry_send;
        }
        else
        {
            jack_error ("Failed to construct method return!");
        }
    }
}

#define object_ptr ((struct jack_dbus_object_descriptor *)data)

/*
 * The D-Bus message handler for object path /org/jackaudio/Controller.
 */
DBusHandlerResult
jack_dbus_message_handler(
    DBusConnection *connection,
    DBusMessage *message,
    void *data)
{
    struct jack_dbus_method_call call;
    const char *interface_name;
    struct jack_dbus_interface_descriptor ** interface_ptr_ptr;

    /* Check if the message is a method call. If not, ignore it. */
    if (dbus_message_get_type (message) != DBUS_MESSAGE_TYPE_METHOD_CALL)
    {
        goto handled;
    }

    /* Get the invoked method's name and make sure it's non-NULL. */
    if (!(call.method_name = dbus_message_get_member (message)))
    {
        jack_dbus_error(
            &call,
            JACK_DBUS_ERROR_UNKNOWN_METHOD,
            "Received method call with empty method name");
        goto send_return;
    }

    /* Initialize our data. */
    call.context = object_ptr->context;
    call.connection = connection;
    call.message = message;
    call.reply = NULL;

    /* Check if there's an interface specified for this method call. */
    interface_name = dbus_message_get_interface (message);
    if (interface_name != NULL)
    {
        /* Check if we can match the interface and method.
         * The inteface handler functions only return false if the
         * method name was unknown, otherwise they run the specified
         * method and return TRUE.
         */

        interface_ptr_ptr = object_ptr->interfaces;

        while (*interface_ptr_ptr != NULL)
        {
            if (strcmp(interface_name, (*interface_ptr_ptr)->name) == 0)
            {
                if (!(*interface_ptr_ptr)->handler(&call, (*interface_ptr_ptr)->methods))
                {
                    break;
                }

                goto send_return;
            }

            interface_ptr_ptr++;
        }
    }
    else
    {
        /* No interface was specified so we have to try them all. This is
         * dictated by the D-Bus specification which states that method calls
         * omitting the interface must never be rejected.
         */

        interface_ptr_ptr = object_ptr->interfaces;

        while (*interface_ptr_ptr != NULL)
        {
            if ((*interface_ptr_ptr)->handler(&call, (*interface_ptr_ptr)->methods))
            {
                goto send_return;
            }

            interface_ptr_ptr++;
        }
    }

    jack_dbus_error(
        &call,
        JACK_DBUS_ERROR_UNKNOWN_METHOD,
        "Method \"%s\" with signature \"%s\" on interface \"%s\" doesn't exist",
        call.method_name,
        dbus_message_get_signature(message),
        interface_name);

send_return:
    jack_dbus_send_method_return(&call);

handled:
    return DBUS_HANDLER_RESULT_HANDLED;
}

void
jack_dbus_message_handler_unregister(
    DBusConnection *connection,
    void *data)
{
    jack_info ("Message handler was unregistered");
}

#undef object_ptr

/*
 * Check if the supplied method name exists in org.jackaudio.JackConfigure,
 * if it does execute it and return TRUE. Otherwise return FALSE.
 */
bool
jack_dbus_run_method(
    struct jack_dbus_method_call *call,
    const struct jack_dbus_interface_method_descriptor * methods)
{
    const struct jack_dbus_interface_method_descriptor * method_ptr;

    method_ptr = methods;

    while (method_ptr->name != NULL)
    {
        if (strcmp(call->method_name, method_ptr->name) == 0)
        {
            method_ptr->handler(call);
            return TRUE;
        }

        method_ptr++;
    }

    return FALSE;
}

/*
 * Read arguments from a method call.
 * If the operation fails construct an error and return false,
 * otherwise return true.
 */
bool
jack_dbus_get_method_args(
    struct jack_dbus_method_call *call,
    int type,
    ...)
{
    va_list args;
    DBusError error;
    bool retval = true;

    va_start (args, type);
    dbus_error_init (&error);

    if (!dbus_message_get_args_valist (call->message, &error, type, args))
    {
        jack_dbus_error (call, JACK_DBUS_ERROR_INVALID_ARGS,
                         "Invalid arguments to method \"%s\"",
                         call->method_name);
        retval = false;
    }

    dbus_error_free (&error);
    va_end (args);

    return retval;
}

/*
 * Read a string and a variant argument from a method call.
 * If the operation fails construct an error and return false,
 * otherwise return true.
 */
bool
jack_dbus_get_method_args_string_and_variant(
    struct jack_dbus_method_call *call,
    const char **arg1,
    message_arg_t *arg2,
    int *type_ptr)
{
    DBusMessageIter iter, sub_iter;

    /* First we want a string... */
    if (dbus_message_iter_init (call->message, &iter)
        && dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_STRING)
    {
        dbus_message_iter_get_basic (&iter, arg1);
        dbus_message_iter_next (&iter);

        /* ...and then a variant. */
        if (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_VARIANT)
        {
            dbus_message_iter_recurse (&iter, &sub_iter);
            dbus_message_iter_get_basic (&sub_iter, arg2);
            *type_ptr = dbus_message_iter_get_arg_type (&sub_iter);

            /* Got what we wanted. */
            return true;
        }
    }

    jack_dbus_error (call, JACK_DBUS_ERROR_INVALID_ARGS,
                     "Invalid arguments to method \"%s\"",
                     call->method_name);

    return false;
}

/*
 * Read two strings and a variant argument from a method call.
 * If the operation fails construct an error and return false,
 * otherwise return true.
 */
bool
jack_dbus_get_method_args_two_strings_and_variant(
    struct jack_dbus_method_call *call,
    const char **arg1,
    const char **arg2,
    message_arg_t *arg3,
    int *type_ptr)
{
    DBusMessageIter iter, sub_iter;

    /* First we want a string... */
    if (dbus_message_iter_init (call->message, &iter)
        && dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_STRING)
    {
        dbus_message_iter_get_basic (&iter, arg1);
        dbus_message_iter_next (&iter);
        
        /* ...and then a second string. */
        if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRING)
        {
            return false;
        }
        
        /* Got what we wanted. */
        dbus_message_iter_get_basic (&iter, arg2);
        dbus_message_iter_next (&iter);

        /* ...and then a variant. */
        if (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_VARIANT)
        {
            dbus_message_iter_recurse (&iter, &sub_iter);
            dbus_message_iter_get_basic (&sub_iter, arg3);
            *type_ptr = dbus_message_iter_get_arg_type (&sub_iter);

            /* Got what we wanted. */
            return true;
        }
    }

    jack_dbus_error (call, JACK_DBUS_ERROR_INVALID_ARGS,
                     "Invalid arguments to method \"%s\"",
                     call->method_name);

    return false;
}

/*
 * Append a variant type to a D-Bus message.
 * Return false if something fails, true otherwise.
 */
bool
jack_dbus_message_append_variant(
    DBusMessageIter *iter,
    int type,
    const char *signature,
    message_arg_t *arg)
{
    DBusMessageIter sub_iter;

    /* Open a variant container. */
    if (!dbus_message_iter_open_container (iter, DBUS_TYPE_VARIANT, signature, &sub_iter))
    {
        goto fail;
    }

    /* Append the supplied value. */
    if (!dbus_message_iter_append_basic (&sub_iter, type, (const void *) arg))
    {
        dbus_message_iter_close_container (iter, &sub_iter);
        goto fail;
    }

    /* Close the container. */
    if (!dbus_message_iter_close_container (iter, &sub_iter))
    {
        goto fail;
    }

    return true;

fail:
    return false;
}

/*
 * Construct an empty method return message.
 *
 * The operation can only fail due to lack of memory, in which case
 * there's no sense in trying to construct an error return. Instead,
 * call->reply will be set to NULL and handled in send_method_return().
 */
void
jack_dbus_construct_method_return_empty(
    struct jack_dbus_method_call * call)
{
    call->reply = dbus_message_new_method_return (call->message);

    if (call->reply == NULL)
    {
        jack_error ("Ran out of memory trying to construct method return");
    }
}

/*
 * Construct a method return which holds a single argument or, if
 * the type parameter is DBUS_TYPE_INVALID, no arguments at all
 * (a void message).
 *
 * The operation can only fail due to lack of memory, in which case
 * there's no sense in trying to construct an error return. Instead,
 * call->reply will be set to NULL and handled in send_method_return().
 */
void
jack_dbus_construct_method_return_single(
    struct jack_dbus_method_call *call,
    int type,
    message_arg_t arg)
{
    DBusMessageIter iter;
    call->reply = dbus_message_new_method_return (call->message);

    if (call->reply == NULL)
    {
        goto fail_no_mem;
    }

    /* Void method return requested by caller. */
    if (type == DBUS_TYPE_INVALID)
    {
        return;
    }

    /* Prevent crash on NULL input string. */
    else if (type == DBUS_TYPE_STRING && arg.string == NULL)
    {
        arg.string = "";
    }

    dbus_message_iter_init_append (call->reply, &iter);

    if (!dbus_message_iter_append_basic (&iter, type, (const void *) &arg))
    {
        dbus_message_unref (call->reply);
        call->reply = NULL;
        goto fail_no_mem;
    }

    return;

fail_no_mem:
    jack_error ("Ran out of memory trying to construct method return");
}

/*
 * Construct a method return which holds an array of strings.
 *
 * The operation can only fail due to lack of memory, in which case
 * there's no sense in trying to construct an error return. Instead,
 * call->reply will be set to NULL and handled in send_method_return().
 */
void
jack_dbus_construct_method_return_array_of_strings(
    struct jack_dbus_method_call *call,
    unsigned int num_members,
    const char **array)
{
    DBusMessageIter iter, sub_iter;
    unsigned int i;

    call->reply = dbus_message_new_method_return (call->message);
    if (!call->reply)
    {
        goto fail;
    }

    dbus_message_iter_init_append (call->reply, &iter);

    if (!dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, "s", &sub_iter))
    {
        goto fail_unref;
    }

    for (i = 0; i < num_members; ++i)
    {
        if (!dbus_message_iter_append_basic (&sub_iter, DBUS_TYPE_STRING, (const void *) &array[i]))
        {
            dbus_message_iter_close_container (&iter, &sub_iter);
            goto fail_unref;
        }
    }

    if (!dbus_message_iter_close_container (&iter, &sub_iter))
    {
        goto fail_unref;
    }

    return;

fail_unref:
    dbus_message_unref (call->reply);
    call->reply = NULL;

fail:
    jack_error ("Ran out of memory trying to construct method return");
}

static bool jack_dbus_log_open(void)
{
    struct stat st;
    int ret;
    int retry;

    if (g_logfile != NULL)
    {
        ret = stat(g_log_filename, &st);
        if (ret != 0 || g_log_file_ino != st.st_ino)
        {
            fclose(g_logfile);
        }
        else
        {
            return true;
        }
    }

    for (retry = 0; retry < 10; retry++)
    {
        g_logfile = fopen(g_log_filename, "a");
        if (g_logfile == NULL)
        {
            fprintf(stderr, "Cannot open jackdbus log file \"%s\": %d (%s)\n", g_log_filename, errno, strerror(errno));
            return false;
        }

        ret = stat(g_log_filename, &st);
        if (ret == 0)
        {
            g_log_file_ino = st.st_ino;
            return true;
        }

        fclose(g_logfile);
        g_logfile = NULL;
    }

    fprintf(stderr, "Cannot stat just opened jackdbus log file \"%s\": %d (%s). %d retries\n", g_log_filename, errno, strerror(errno), retry);
    return false;
}

void 
jack_dbus_info_callback(const char *msg)
{
    time_t timestamp;
    char timestamp_str[26];

    time(&timestamp);
    ctime_r(&timestamp, timestamp_str);
    timestamp_str[24] = 0;

    if (jack_dbus_log_open())
    {
        fprintf(g_logfile, "%s: %s\n", timestamp_str, msg);
        fflush(g_logfile);
    }
}

#define ANSI_BOLD_ON    "\033[1m"
#define ANSI_BOLD_OFF   "\033[22m"
#define ANSI_COLOR_RED  "\033[31m"
#define ANSI_RESET      "\033[0m"

void 
jack_dbus_error_callback(const char *msg)
{
    time_t timestamp;
    char timestamp_str[26];

    time(&timestamp);
    ctime_r(&timestamp, timestamp_str);
    timestamp_str[24] = 0;

    if (jack_dbus_log_open())
    {
        fprintf(g_logfile, "%s: " ANSI_BOLD_ON ANSI_COLOR_RED "ERROR: %s" ANSI_RESET "\n", timestamp_str, msg);
        fflush(g_logfile);
    }
}

bool
ensure_dir_exist(const char *dirname, int mode)
{
    struct stat st;
    if (stat(dirname, &st) != 0)
    {
        if (errno == ENOENT)
        {
            printf("Directory \"%s\" does not exist. Creating...\n", dirname);
            if (mkdir(dirname, mode) != 0)
            {
                fprintf(stderr, "Failed to create \"%s\" directory: %d (%s)\n", dirname, errno, strerror(errno));
                return false;
            }
        }
        else
        {
            fprintf(stderr, "Failed to stat \"%s\": %d (%s)\n", dirname, errno, strerror(errno));
            return false;
        }
    }
    else
    {
        if (!S_ISDIR(st.st_mode))
        {
            fprintf(stderr, "\"%s\" exists but is not directory.\n", dirname);
            return false;
        }
    }
    return true;
}

char *
pathname_cat(const char *pathname_a, const char *pathname_b)
{
    char *pathname;
    int pathname_a_len, pathname_b_len, pathname_len;
    pathname_a_len = strlen(pathname_a);
    pathname_b_len = strlen(pathname_b);
    pathname = malloc(pathname_a_len + pathname_b_len + 1);
    if (pathname == NULL)
    {
        fprintf(stderr, "Out of memory\n");
        return NULL;
    }
    memcpy(pathname, pathname_a, pathname_a_len);
    memcpy(pathname + pathname_a_len, pathname_b, pathname_b_len);
    pathname_len = pathname_a_len + pathname_b_len;
    pathname[pathname_len] = 0;
    return pathname;
}

bool
paths_init()
{
	const char *home_dir, *xdg_config_home, *xdg_log_home;
    
    home_dir = getenv("HOME");
    if (home_dir == NULL)
    {
        fprintf(stderr, "Environment variable HOME not set\n");
        goto fail;
    }

	xdg_config_home = getenv("XDG_CONFIG_HOME");
	if (xdg_config_home == NULL)
	{
	    if (!(xdg_config_home = pathname_cat(home_dir, DEFAULT_XDG_CONFIG))) goto fail;
	}

    if (!(xdg_log_home = pathname_cat(home_dir, DEFAULT_XDG_LOG))) goto fail;

	if (!(g_jackdbus_config_dir = pathname_cat(xdg_config_home, JACKDBUS_DIR))) goto fail;
	if (!(g_jackdbus_log_dir = pathname_cat(xdg_log_home, JACKDBUS_DIR))) goto fail;

    if (!ensure_dir_exist(xdg_config_home, 0700))
    {
        goto fail;
    }
    
    if (!ensure_dir_exist(xdg_log_home, 0700))
    {
        goto fail;
    }

    if (!ensure_dir_exist(g_jackdbus_config_dir, 0700))
    {
        free(g_jackdbus_config_dir);
        goto fail;
    }
    g_jackdbus_config_dir_len = strlen(g_jackdbus_config_dir);
    
    if (!ensure_dir_exist(g_jackdbus_log_dir, 0700))
    {
        free(g_jackdbus_log_dir);
        goto fail;
    }
    g_jackdbus_log_dir_len = strlen(g_jackdbus_log_dir);

    return true;

fail:
    return false;
}

void
paths_uninit()
{
    free(g_jackdbus_config_dir);
    free(g_jackdbus_log_dir);
}

static bool log_init(void)
{
    size_t log_len;

    log_len = strlen(JACKDBUS_LOG);

    g_log_filename = malloc(g_jackdbus_log_dir_len + log_len + 1);
    if (g_log_filename == NULL)
    {
        fprintf(stderr, "Out of memory\n");
        return false;
    }

    memcpy(g_log_filename, g_jackdbus_log_dir, g_jackdbus_log_dir_len);
    memcpy(g_log_filename + g_jackdbus_log_dir_len, JACKDBUS_LOG, log_len);
    g_log_filename[g_jackdbus_log_dir_len + log_len] = 0;

    if (!jack_dbus_log_open())
    {
        return false;
    }

    return true;
}

static void log_uninit(void)
{
    if (g_logfile != NULL)
    {
        fclose(g_logfile);
    }

    free(g_log_filename);
}

void
jack_dbus_error(
    void *dbus_call_context_ptr,
    const char *error_name,
    const char *format,
    ...)
{
    va_list ap;
    char buffer[300];

    va_start(ap, format);

    vsnprintf(buffer, sizeof(buffer), format, ap);

    jack_error_callback(buffer);
    if (dbus_call_context_ptr != NULL)
    {
        if (((struct jack_dbus_method_call *)dbus_call_context_ptr)->reply != NULL)
        {
            dbus_message_unref(((struct jack_dbus_method_call *)dbus_call_context_ptr)->reply);
            ((struct jack_dbus_method_call *)dbus_call_context_ptr)->reply = NULL;
        }

        ((struct jack_dbus_method_call *)dbus_call_context_ptr)->reply = dbus_message_new_error(
            ((struct jack_dbus_method_call *)dbus_call_context_ptr)->message,
            error_name,
            buffer);
    }

    va_end(ap);
}

void
jack_dbus_only_error(
    void *dbus_call_context_ptr,
    const char *error_name,
    const char *format,
    ...)
{
    va_list ap;
    char buffer[300];

    va_start(ap, format);

    vsnprintf(buffer, sizeof(buffer), format, ap);

    if (((struct jack_dbus_method_call *)dbus_call_context_ptr)->reply != NULL)
    {
        dbus_message_unref(((struct jack_dbus_method_call *)dbus_call_context_ptr)->reply);
        ((struct jack_dbus_method_call *)dbus_call_context_ptr)->reply = NULL;
    }

    ((struct jack_dbus_method_call *)dbus_call_context_ptr)->reply = dbus_message_new_error(
        ((struct jack_dbus_method_call *)dbus_call_context_ptr)->message,
        error_name,
        buffer);

    va_end(ap);
}

int
main (int argc, char **argv)
{
    DBusError error;
    int ret;
    void *controller_ptr;
    struct stat st;
    char timestamp_str[26];

    st.st_mtime = 0;
    stat(argv[0], &st);
    ctime_r(&st.st_mtime, timestamp_str);
    timestamp_str[24] = 0;

    if (!jack_controller_settings_init())
    {
        ret = 1;
        goto fail;
    }

    if (argc != 2 || strcmp(argv[1], "auto") != 0)
    {
        ret = 0;
        fprintf(
            stderr,
            "jackdbus should be auto-executed by D-Bus message bus daemon.\n"
            "If you want to run it manually anyway, specify \"auto\" as only parameter\n");
        goto fail_uninit_xml;
    }

    if (!paths_init())
    {
        ret = 1;
        goto fail_uninit_xml;
    }

    if (!log_init())
    {
        ret = 1;
        goto fail_uninit_paths;
    }

#if !defined(DISABLE_SIGNAL_MAGIC)
    jackctl_setup_signals(0);
#endif

    jack_set_error_function(jack_dbus_error_callback);
    jack_set_info_function(jack_dbus_info_callback);

    /* setup our SIGSEGV magic that prints nice stack in our logfile */
    setup_sigsegv();

    jack_info("------------------");
    jack_info("Controller activated. Version %s (%s) built on %s", jack_get_version_string(), SVN_VERSION, timestamp_str);

    if (!dbus_threads_init_default())
    {
        jack_error("dbus_threads_init_default() failed");
        ret = 1;
        goto fail_uninit_log;
    }

    dbus_error_init (&error);
    g_connection = dbus_bus_get (DBUS_BUS_SESSION, &error);
    if (dbus_error_is_set (&error))
    {
        jack_error("Cannot connect to D-Bus session bus: %s", error.message);
        ret = 1;
        goto fail_uninit_log;
    }

    ret = dbus_bus_request_name(
                g_connection,
                "org.jackaudio.service",
                DBUS_NAME_FLAG_DO_NOT_QUEUE,
                &error);
    if (ret == -1)
    {
        jack_error("Cannot request service name: %s", error.message);
                dbus_error_free(&error);
        ret = 1;
        goto fail_unref_connection;
    }
    else if (ret == DBUS_REQUEST_NAME_REPLY_EXISTS)
    {
        jack_error("Requested D-Bus service name already exists");
        ret = 1;
        goto fail_unref_connection;
    }

    controller_ptr = jack_controller_create(g_connection);

    if (controller_ptr == NULL)
    {
        ret = 1;
        goto fail_unref_connection;
    }

    jack_info("Listening for D-Bus messages");

    g_exit_command = FALSE;
    while (!g_exit_command && dbus_connection_read_write_dispatch (g_connection, 200))
    {
        jack_controller_run(controller_ptr);
    }

    jack_controller_destroy(controller_ptr);

    jack_info("Controller deactivated.");

    ret = 0;

fail_unref_connection:
    dbus_connection_unref(g_connection);

fail_uninit_log:
    log_uninit();

fail_uninit_paths:
    paths_uninit();

fail_uninit_xml:
    jack_controller_settings_uninit();

fail:
    return ret;
}
