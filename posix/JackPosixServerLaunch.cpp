/*
Copyright (C) 2001-2003 Paul Davis
Copyright (C) 2004-2008 Grame

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#include "JackConstants.h"
#include "JackChannel.h"
#include "JackLibGlobals.h"
#include "JackServerLaunch.h"
#include "JackPlatformPlug.h"

using namespace Jack;

#if defined(USE_LIBDBUS_AUTOLAUNCH)

#include <dbus/dbus.h>

static int start_server_dbus(const char* server_name)
{
    DBusError err;
    DBusConnection *conn;
    DBusMessage *msg;

    // initialise the errors
    dbus_error_init(&err);

    // connect to the bus
    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Connection Error (%s)\n", err.message);
        dbus_error_free(&err);
    }
    if (NULL == conn) {
        return 1;
    }

    msg = dbus_message_new_method_call(
        "org.jackaudio.service",     // target for the method call
        "/org/jackaudio/Controller", // object to call on
        "org.jackaudio.JackControl", // interface to call on
        "StartServer");              // method name
    if (NULL == msg) {
        fprintf(stderr, "Message Null\n");
        return 1;
    }

    // send message and get a handle for a reply
    if (!dbus_connection_send(conn, msg, NULL))
    {
        fprintf(stderr, "Out Of Memory!\n");
        return 1;
    }

    dbus_message_unref(msg);
    dbus_connection_flush(conn);
    dbus_error_free(&err);

    return 0;
}

#elif defined(USE_CLASSIC_AUTOLAUNCH)

/* Exec the JACK server in this process.  Does not return. */
static void start_server_classic_aux(const char* server_name)
{
    FILE* fp = 0;
    char filename[255];
    char arguments[255];
    char buffer[255];
    char* command = 0;
    size_t pos = 0;
    size_t result = 0;
    char** argv = 0;
    int i = 0;
    int good = 0;
    int ret;

    snprintf(filename, 255, "%s/.jackdrc", getenv("HOME"));
    fp = fopen(filename, "r");

    if (!fp) {
        fp = fopen("/etc/jackdrc", "r");
    }
    /* if still not found, check old config name for backwards compatability */
    if (!fp) {
        fp = fopen("/etc/jackd.conf", "r");
    }

    if (fp) {
        arguments[0] = '\0';
        ret = fscanf(fp, "%s", buffer);
        while (ret != 0 && ret != EOF) {
            strcat(arguments, buffer);
            strcat(arguments, " ");
            ret = fscanf(fp, "%s", buffer);
        }
        if (strlen(arguments) > 0) {
            good = 1;
        }
        fclose(fp);
    }

    if (!good) {
        command = (char*)(JACK_LOCATION "/jackd");
        strncpy(arguments, JACK_LOCATION "/jackd -T -d "JACK_DEFAULT_DRIVER, 255);
    } else {
        result = strcspn(arguments, " ");
        command = (char*)malloc(result + 1);
        strncpy(command, arguments, result);
        command[result] = '\0';
    }

    argv = (char**)malloc(255);

    while (1) {
        /* insert -T and -nserver_name in front of arguments */
        if (i == 1) {
            argv[i] = (char*)malloc(strlen ("-T") + 1);
            strcpy (argv[i++], "-T");
            if (server_name) {
                size_t optlen = strlen("-n");
                char* buf = (char*)malloc(optlen + strlen(server_name) + 1);
                strcpy(buf, "-n");
                strcpy(buf + optlen, server_name);
                argv[i++] = buf;
            }
        }

        result = strcspn(arguments + pos, " ");
        if (result == 0) {
            break;
        }
        argv[i] = (char*)malloc(result + 1);
        strncpy(argv[i], arguments + pos, result);
        argv[i][result] = '\0';
        pos += result + 1;
        ++i;
    }
    argv[i] = 0;
    execv(command, argv);

    /* If execv() succeeds, it does not return. There's no point
     * in calling jack_error() here in the child process. */
    fprintf(stderr, "exec of JACK server (command = \"%s\") failed: %s\n", command, strerror(errno));
}

static int start_server_classic(const char* server_name)
{
    /* The double fork() forces the server to become a child of
     * init, which will always clean up zombie process state on
     * termination. This even works in cases where the server
     * terminates but this client does not.
     *
     * Since fork() is usually implemented using copy-on-write
     * virtual memory tricks, the overhead of the second fork() is
     * probably relatively small.
     */
    switch (fork()) {
        case 0:					/* child process */
            switch (fork()) {
                case 0:			/* grandchild process */
                    start_server_classic_aux(server_name);
                    _exit(99);	/* exec failed */
                case - 1:
                    _exit(98);
                default:
                    _exit(0);
            }
        case - 1:			/* fork() error */
            return 1;		/* failed to start server */
    }

    /* only the original parent process goes here */
    return 0;			/* (probably) successful */
}

#endif

static int start_server(const char* server_name, jack_options_t options)
{
    if ((options & JackNoStartServer) || getenv("JACK_NO_START_SERVER")) {
        return 1;
    }

#if defined(USE_LIBDBUS_AUTOLAUNCH)
    return start_server_dbus(server_name);
#elif defined(USE_CLASSIC_AUTOLAUNCH)
    return start_server_classic(server_name);
#else
    fprintf(stderr, "Automatic start of JACK server is disabled at configure time\n");
    return 1;
#endif
}

static int server_connect(char* server_name)
{
    JackClientChannel channel;
    int res = channel.ServerCheck(server_name);
    channel.Close();
    JackSleep(2000); // Added by JE - 02-01-2009 (gives
                     // the channel some time to close)
    return res;
}

int try_start_server(jack_varargs_t* va, jack_options_t options, jack_status_t* status)
{
    if (server_connect(va->server_name) < 0) {
        int trys;
        if (start_server(va->server_name, options)) {
            int my_status1 = *status | JackFailure | JackServerFailed;
            *status = (jack_status_t)my_status1;
            return -1;
        }
        trys = 5;
        do {
            sleep(1);
            if (--trys < 0) {
                int my_status1 = *status | JackFailure | JackServerFailed;
                *status = (jack_status_t)my_status1;
                return -1;
            }
        } while (server_connect(va->server_name) < 0);
        int my_status1 = *status | JackServerStarted;
        *status = (jack_status_t)my_status1;
    }

    return 0;
}

