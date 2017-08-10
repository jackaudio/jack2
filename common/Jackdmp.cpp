/*
Copyright (C) 2001 Paul Davis
Copyright (C) 2004-2013 Grame

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <iostream>
#include <assert.h>
#include <cassert>
#include <csignal>
#include <sys/types.h>
#include <getopt.h>
#include <cstring>
#include <cstdio>
#include <list>

#include "types.h"
#include "jack.h"
#include "control.h"
#include "JackConstants.h"
#include "JackPlatformPlug.h"
#ifdef __ANDROID__
#include "JackControlAPIAndroid.h"
#endif

#if defined(JACK_DBUS) && defined(__linux__)
#include <cstdlib>
#include <dbus/dbus.h>
#include "audio_reserve.h"
#endif

/*
This is a simple port of the old jackdmp.cpp file to use the new jack2 control API. Available options for the server
are "hard-coded" in the source. A much better approach would be to use the control API to:
- dynamically retrieve available server parameters and then prepare to parse them
- get available drivers and their possible parameters, then prepare to parse them.
*/

#ifdef __APPLE__
#include <CoreFoundation/CFNotificationCenter.h>
#include <CoreFoundation/CoreFoundation.h>

static void notify_server_start(const char* server_name)
{
    // Send notification to be used in the JackRouter plugin
    CFStringRef ref = CFStringCreateWithCString(NULL, server_name, kCFStringEncodingMacRoman);
    CFNotificationCenterPostNotificationWithOptions(CFNotificationCenterGetDistributedCenter(),
            CFSTR("com.grame.jackserver.start"),
            ref,
            NULL,
            kCFNotificationDeliverImmediately | kCFNotificationPostToAllSessions);
    CFRelease(ref);
}

static void notify_server_stop(const char* server_name)
{
    // Send notification to be used in the JackRouter plugin
    CFStringRef ref1 = CFStringCreateWithCString(NULL, server_name, kCFStringEncodingMacRoman);
    CFNotificationCenterPostNotificationWithOptions(CFNotificationCenterGetDistributedCenter(),
            CFSTR("com.grame.jackserver.stop"),
            ref1,
            NULL,
            kCFNotificationDeliverImmediately | kCFNotificationPostToAllSessions);
    CFRelease(ref1);
}

#else

static void notify_server_start(const char* server_name)
{}
static void notify_server_stop(const char* server_name)
{}

#endif

static void copyright(FILE* file)
{
    fprintf(file, "jackdmp " VERSION "\n"
            "Copyright 2001-2005 Paul Davis and others.\n"
            "Copyright 2004-2016 Grame.\n"
            "Copyright 2016-2017 Filipe Coelho.\n"
            "jackdmp comes with ABSOLUTELY NO WARRANTY\n"
            "This is free software, and you are welcome to redistribute it\n"
            "under certain conditions; see the file COPYING for details\n");
}

static jackctl_driver_t * jackctl_server_get_driver(jackctl_server_t *server, const char *driver_name)
{
    const JSList * node_ptr = jackctl_server_get_drivers_list(server);

    while (node_ptr) {
        if (strcmp(jackctl_driver_get_name((jackctl_driver_t *)node_ptr->data), driver_name) == 0) {
            return (jackctl_driver_t *)node_ptr->data;
        }
        node_ptr = jack_slist_next(node_ptr);
    }

    return NULL;
}

static jackctl_internal_t * jackctl_server_get_internal(jackctl_server_t *server, const char *internal_name)
{
    const JSList * node_ptr = jackctl_server_get_internals_list(server);

    while (node_ptr) {
        if (strcmp(jackctl_internal_get_name((jackctl_internal_t *)node_ptr->data), internal_name) == 0) {
            return (jackctl_internal_t *)node_ptr->data;
        }
        node_ptr = jack_slist_next(node_ptr);
    }

    return NULL;
}

static jackctl_parameter_t * jackctl_get_parameter(const JSList * parameters_list, const char * parameter_name)
{
    while (parameters_list) {
        if (strcmp(jackctl_parameter_get_name((jackctl_parameter_t *)parameters_list->data), parameter_name) == 0) {
            return (jackctl_parameter_t *)parameters_list->data;
        }
        parameters_list = jack_slist_next(parameters_list);
    }

    return NULL;
}

#ifdef __ANDROID__
static void jackctl_server_switch_master_dummy(jackctl_server_t * server_ctl, char * master_driver_name)
{
    static bool is_dummy_driver = false;
    if(!strcmp(master_driver_name, "dummy")) {
        return;
    }
    jackctl_driver_t * driver_ctr;
    if(is_dummy_driver) {
        is_dummy_driver = false;
        driver_ctr = jackctl_server_get_driver(server_ctl, master_driver_name);
    } else {
        is_dummy_driver = true;
        driver_ctr = jackctl_server_get_driver(server_ctl, "dummy");
    }
    jackctl_server_switch_master(server_ctl, driver_ctr);
}
#endif

static void print_server_drivers(jackctl_server_t *server, FILE* file)
{
    const JSList * node_ptr = jackctl_server_get_drivers_list(server);

    fprintf(file, "Available backends:\n");

    while (node_ptr) {
        jackctl_driver_t* driver = (jackctl_driver_t *)node_ptr->data;
        fprintf(file, "      %s (%s)\n", jackctl_driver_get_name(driver), (jackctl_driver_get_type(driver) == JackMaster) ? "master" : "slave");
        node_ptr = jack_slist_next(node_ptr);
    }
    fprintf(file, "\n");
}

static void print_server_internals(jackctl_server_t *server, FILE* file)
{
    const JSList * node_ptr = jackctl_server_get_internals_list(server);

    fprintf(file, "Available internals:\n");

    while (node_ptr) {
        jackctl_internal_t* internal = (jackctl_internal_t *)node_ptr->data;
        fprintf(file, "      %s\n", jackctl_internal_get_name(internal));
        node_ptr = jack_slist_next(node_ptr);
    }
    fprintf(file, "\n");
}

static void usage(FILE* file, jackctl_server_t *server, bool full = true)
{
    jackctl_parameter_t * param;
    const JSList * server_parameters;
    uint32_t i;
    union jackctl_parameter_value value;

    fprintf(file, "\n"
            "Usage: jackdmp [ --no-realtime OR -r ]\n"
            "               [ --realtime OR -R [ --realtime-priority OR -P priority ] ]\n"
            "      (the two previous arguments are mutually exclusive. The default is --realtime)\n"
            "               [ --name OR -n server-name ]\n"
            "               [ --timeout OR -t client-timeout-in-msecs ]\n"
            "               [ --loopback OR -L loopback-port-number ]\n"
            "               [ --port-max OR -p maximum-number-of-ports]\n"
            "               [ --slave-backend OR -X slave-backend-name ]\n"
            "               [ --internal-client OR -I internal-client-name ]\n"
            "               [ --internal-session-file OR -C internal-session-file ]\n"
            "               [ --verbose OR -v ]\n"
#ifdef __linux__
            "               [ --clocksource OR -c [ h(pet) | s(ystem) ]\n"
#endif
            "               [ --autoconnect OR -a <modechar>]\n");

    server_parameters = jackctl_server_get_parameters(server);
    param = jackctl_get_parameter(server_parameters, "self-connect-mode");
    fprintf(file,
            "                 where <modechar> is one of:\n");
    for (i = 0; i < jackctl_parameter_get_enum_constraints_count(param); i++)
    {
        value = jackctl_parameter_get_enum_constraint_value(param, i);
        fprintf(file, "                   '%c' - %s", value.c, jackctl_parameter_get_enum_constraint_description(param, i));
        if (value.c == JACK_DEFAULT_SELF_CONNECT_MODE)
        {
            fprintf(file, " (default)");
        }
        fprintf(file, "\n");
    }

    fprintf(file,
            "               [ --replace-registry ]\n"
            "               [ --silent OR -s ]\n"
            "               [ --sync OR -S ]\n"
            "               [ --temporary OR -T ]\n"
            "               [ --version OR -V ]\n"
            "         -d master-backend-name [ ... master-backend args ... ]\n"
            "       jackdmp -d master-backend-name --help\n"
            "             to display options for each master backend\n\n");

    if (full) {
        print_server_drivers(server, file);
        print_server_internals(server, file);
    }
}

// Prototype to be found in libjackserver
extern "C" void silent_jack_error_callback(const char *desc);

void print_version()
{
    printf( "jackdmp version " VERSION " tmpdir "
            jack_server_dir " protocol %d" "\n",
            JACK_PROTOCOL_VERSION);
    exit(-1);

}

int main(int argc, char** argv)
{
    jackctl_server_t * server_ctl;
    const JSList * server_parameters;
    const char* server_name = JACK_DEFAULT_SERVER_NAME;
    jackctl_driver_t * master_driver_ctl;
    jackctl_driver_t * loopback_driver_ctl = NULL;
    int replace_registry = 0;

    for(int a = 1; a < argc; ++a) {
        if( !strcmp(argv[a], "--version") || !strcmp(argv[a], "-V") ) {
            print_version();
        }
    }
    const char *options = "-d:X:I:P:uvshrRL:STFl:t:mn:p:C:"
        "a:"
#ifdef __linux__
        "c:"
#endif
        ;

    struct option long_options[] = {
#ifdef __linux__
                                       { "clock-source", 1, 0, 'c' },
#endif
                                       { "internal-session-file", 1, 0, 'C' },
                                       { "loopback-driver", 1, 0, 'L' },
                                       { "audio-driver", 1, 0, 'd' },
                                       { "midi-driver", 1, 0, 'X' },
                                       { "internal-client", 1, 0, 'I' },
                                       { "verbose", 0, 0, 'v' },
                                       { "help", 0, 0, 'h' },
                                       { "port-max", 1, 0, 'p' },
                                       { "no-mlock", 0, 0, 'm' },
                                       { "name", 1, 0, 'n' },
                                       { "unlock", 0, 0, 'u' },
                                       { "realtime", 0, 0, 'R' },
                                       { "no-realtime", 0, 0, 'r' },
                                       { "replace-registry", 0, &replace_registry, 0 },
                                       { "loopback", 0, 0, 'L' },
                                       { "realtime-priority", 1, 0, 'P' },
                                       { "timeout", 1, 0, 't' },
                                       { "temporary", 0, 0, 'T' },
                                       { "silent", 0, 0, 's' },
                                       { "sync", 0, 0, 'S' },
                                       { "autoconnect", 1, 0, 'a' },
                                       { 0, 0, 0, 0 }
                                   };

    int i,opt = 0;
    int option_index = 0;
    char* internal_session_file = NULL;
    char* master_driver_name = NULL;
    char** master_driver_args = NULL;
    int master_driver_nargs = 1;
    int loopback = 0;
    jackctl_sigmask_t * sigmask;
    jackctl_parameter_t* param;
    union jackctl_parameter_value value;

    std::list<char*> internals_list;
    std::list<char*> slaves_list;
    std::list<char*>::iterator it;

    // Assume that we fail.
    int return_value = -1;
    bool notify_sent = false;

    copyright(stdout);
#if defined(JACK_DBUS) && defined(__linux__)
    if (getenv("JACK_NO_AUDIO_RESERVATION"))
        server_ctl = jackctl_server_create(NULL, NULL);
    else
        server_ctl = jackctl_server_create(audio_acquire, audio_release);
#else
    server_ctl = jackctl_server_create(NULL, NULL);
#endif
    if (server_ctl == NULL) {
        fprintf(stderr, "Failed to create server object\n");
        return -1;
    }

    server_parameters = jackctl_server_get_parameters(server_ctl);

    opterr = 0;
    while (!master_driver_name &&
            (opt = getopt_long(argc, argv, options,
                               long_options, &option_index)) != EOF) {
        switch (opt) {

        #ifdef __linux__
            case 'c':
                param = jackctl_get_parameter(server_parameters, "clock-source");
                if (param != NULL) {
                    if (tolower (optarg[0]) == 'h') {
                        value.ui = JACK_TIMER_HPET;
                        jackctl_parameter_set_value(param, &value);
                    } else if (tolower (optarg[0]) == 'c') {
                        /* For backwards compatibility with scripts, allow
                         * the user to request the cycle clock on the
                         * command line, but use the system clock instead
                         */
                        value.ui = JACK_TIMER_SYSTEM_CLOCK;
                        jackctl_parameter_set_value(param, &value);
                    } else if (tolower (optarg[0]) == 's') {
                        value.ui = JACK_TIMER_SYSTEM_CLOCK;
                        jackctl_parameter_set_value(param, &value);
                    } else {
                        usage(stdout, server_ctl);
                        goto destroy_server;
                    }
                }
                break;
        #endif

            case 'a':
                param = jackctl_get_parameter(server_parameters, "self-connect-mode");
                if (param != NULL) {
                    bool value_valid = false;
                    for (uint32_t k=0; k<jackctl_parameter_get_enum_constraints_count( param ); k++ ) {
                        value = jackctl_parameter_get_enum_constraint_value( param, k );
                        if( value.c == optarg[0] )
                            value_valid = true;
                    }

                    if( value_valid ) {
                        value.c = optarg[0];
                        jackctl_parameter_set_value(param, &value);
                    } else {
                        usage(stdout, server_ctl);
                        goto destroy_server;
                    }
                }
                break;

            case 'd':
                master_driver_name = optarg;
                break;

            case 'L':
                loopback = atoi(optarg);
                break;

            case 'X':
                slaves_list.push_back(optarg);
                break;

            case 'I':
                internals_list.push_back(optarg);
                break;

            case 'p':
                param = jackctl_get_parameter(server_parameters, "port-max");
                if (param != NULL) {
                    value.ui = atoi(optarg);
                    jackctl_parameter_set_value(param, &value);
                }
                break;

            case 'm':
                break;

            case 'u':
                break;

            case 'v':
                param = jackctl_get_parameter(server_parameters, "verbose");
                if (param != NULL) {
                    value.b = true;
                    jackctl_parameter_set_value(param, &value);
                }
                break;

            case 's':
                jack_set_error_function(silent_jack_error_callback);
                break;

            case 'S':
                param = jackctl_get_parameter(server_parameters, "sync");
                if (param != NULL) {
                    value.b = true;
                    jackctl_parameter_set_value(param, &value);
                }
                break;

            case 'n':
                server_name = optarg;
                param = jackctl_get_parameter(server_parameters, "name");
                if (param != NULL) {
                    strncpy(value.str, optarg, JACK_PARAM_STRING_MAX);
                    jackctl_parameter_set_value(param, &value);
                }
                break;

            case 'C':
                internal_session_file = optarg;
                break;

            case 'P':
                param = jackctl_get_parameter(server_parameters, "realtime-priority");
                if (param != NULL) {
                    value.i = atoi(optarg);
                    jackctl_parameter_set_value(param, &value);
                }
                break;

            case 'r':
                param = jackctl_get_parameter(server_parameters, "realtime");
                if (param != NULL) {
                    value.b = false;
                    jackctl_parameter_set_value(param, &value);
                }
                break;

            case 'R':
                param = jackctl_get_parameter(server_parameters, "realtime");
                if (param != NULL) {
                    value.b = true;
                    jackctl_parameter_set_value(param, &value);
                }
                break;

            case 'T':
                param = jackctl_get_parameter(server_parameters, "temporary");
                if (param != NULL) {
                    value.b = true;
                    jackctl_parameter_set_value(param, &value);
                }
                break;

            case 't':
                param = jackctl_get_parameter(server_parameters, "client-timeout");
                if (param != NULL) {
                    value.i = atoi(optarg);
                    jackctl_parameter_set_value(param, &value);
                }
                break;

            default:
                fprintf(stderr, "unknown option character %c\n", optopt);
                /*fallthru*/

            case 'h':
                usage(stdout, server_ctl);
                goto destroy_server;
        }
    }

    // Long option with no letter so treated separately
    param = jackctl_get_parameter(server_parameters, "replace-registry");
    if (param != NULL) {
        value.b = replace_registry;
        jackctl_parameter_set_value(param, &value);
    }

    if (!master_driver_name) {
        usage(stderr, server_ctl, false);
        goto destroy_server;
    }

    // Master driver
    master_driver_ctl = jackctl_server_get_driver(server_ctl, master_driver_name);
    if (master_driver_ctl == NULL) {
        fprintf(stderr, "Unknown driver \"%s\"\n", master_driver_name);
        goto destroy_server;
    }

    if (jackctl_driver_get_type(master_driver_ctl) != JackMaster) {
        fprintf(stderr, "Driver \"%s\" is not a master \n", master_driver_name);
        goto destroy_server;
    }

    if (optind < argc) {
        master_driver_nargs = 1 + argc - optind;
    } else {
        master_driver_nargs = 1;
    }

    if (master_driver_nargs == 0) {
        fprintf(stderr, "No driver specified ... hmm. JACK won't do"
                " anything when run like this.\n");
        goto destroy_server;
    }

    master_driver_args = (char **) malloc(sizeof(char *) * master_driver_nargs);
    master_driver_args[0] = master_driver_name;

    for (i = 1; i < master_driver_nargs; i++) {
        master_driver_args[i] = argv[optind++];
    }

    if (jackctl_driver_params_parse(master_driver_ctl, master_driver_nargs, master_driver_args)) {
        goto destroy_server;
    }

    // Setup signals
    sigmask = jackctl_setup_signals(0);

    // Open server
    if (! jackctl_server_open(server_ctl, master_driver_ctl)) {
        fprintf(stderr, "Failed to open server\n");
        goto destroy_server;
    }

    // Slave drivers
    for (it = slaves_list.begin(); it != slaves_list.end(); it++) {
        jackctl_driver_t * slave_driver_ctl = jackctl_server_get_driver(server_ctl, *it);
        if (slave_driver_ctl == NULL) {
            fprintf(stderr, "Unknown driver \"%s\"\n", *it);
            goto close_server;
        }
        if (jackctl_driver_get_type(slave_driver_ctl) != JackSlave) {
            fprintf(stderr, "Driver \"%s\" is not a slave \n", *it);
            goto close_server;
        }
        if (!jackctl_server_add_slave(server_ctl, slave_driver_ctl)) {
            fprintf(stderr, "Driver \"%s\" cannot be loaded\n", *it);
            goto close_server;
        }
    }

    // Loopback driver
    if (loopback > 0) {
        loopback_driver_ctl = jackctl_server_get_driver(server_ctl, "loopback");

        if (loopback_driver_ctl != NULL) {
            const JSList * loopback_parameters = jackctl_driver_get_parameters(loopback_driver_ctl);
            param = jackctl_get_parameter(loopback_parameters, "channels");
            if (param != NULL) {
                value.ui = loopback;
                jackctl_parameter_set_value(param, &value);
            }
            if (!jackctl_server_add_slave(server_ctl, loopback_driver_ctl)) {
                fprintf(stderr, "Driver \"loopback\" cannot be loaded\n");
                goto close_server;
            }
        } else {
            fprintf(stderr, "Driver \"loopback\" not found\n");
            goto close_server;
        }
    }

    // Start the server
    if (!jackctl_server_start(server_ctl)) {
        fprintf(stderr, "Failed to start server\n");
        goto close_server;
    }

    // Internal clients
    for (it = internals_list.begin(); it != internals_list.end(); it++) {
        jackctl_internal_t * internal_driver_ctl = jackctl_server_get_internal(server_ctl, *it);
        if (internal_driver_ctl == NULL) {
            fprintf(stderr, "Unknown internal \"%s\"\n", *it);
            goto stop_server;
        }
        if (!jackctl_server_load_internal(server_ctl, internal_driver_ctl)) {
            fprintf(stderr, "Internal client \"%s\" cannot be loaded\n", *it);
            goto stop_server;
        }
    }

    if (internal_session_file != NULL) {
        if (!jackctl_server_load_session_file(server_ctl, internal_session_file)) {
            fprintf(stderr, "Internal session file %s cannot be loaded!\n", internal_session_file);
            goto stop_server;
        }
    }

    notify_server_start(server_name);
    notify_sent = true;
    return_value = 0;

    // Waits for signal
#ifdef __ANDROID__
    //reserve SIGUSR2 signal for switching master driver
    while(1) {
        int signal = jackctl_wait_signals_and_return(sigmask);
        if (signal == SIGUSR2) {
            jackctl_server_switch_master_dummy(server_ctl, master_driver_name);
        } else {
            break;
        }
    }
#else
    jackctl_wait_signals(sigmask);
#endif

 stop_server:
    if (!jackctl_server_stop(server_ctl)) {
        fprintf(stderr, "Cannot stop server...\n");
    }

 close_server:
    if (loopback > 0 && loopback_driver_ctl) {
        jackctl_server_remove_slave(server_ctl, loopback_driver_ctl);
    }
    // Slave drivers
    for (it = slaves_list.begin(); it != slaves_list.end(); it++) {
        jackctl_driver_t * slave_driver_ctl = jackctl_server_get_driver(server_ctl, *it);
        if (slave_driver_ctl) {
            jackctl_server_remove_slave(server_ctl, slave_driver_ctl);
        }
    }

    // Internal clients
    for (it = internals_list.begin(); it != internals_list.end(); it++) {
        jackctl_internal_t * internal_driver_ctl = jackctl_server_get_internal(server_ctl, *it);
        if (internal_driver_ctl) {
            jackctl_server_unload_internal(server_ctl, internal_driver_ctl);
        }
    }
    jackctl_server_close(server_ctl);

 destroy_server:
    jackctl_server_destroy(server_ctl);
    if (notify_sent) {
        notify_server_stop(server_name);
    }
    return return_value;
}
