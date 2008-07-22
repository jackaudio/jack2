/*
Copyright (C) 2001 Paul Davis
Copyright (C) 2004-2008 Grame

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

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <iostream>
#include <assert.h>
#include <signal.h>

#include <sys/types.h>
#include <getopt.h>
#include <string.h>
#include "types.h"
#include "jack.h"
#include "JackConstants.h"
#include "JackDriverLoader.h"

/*
This is a simple port of the old jackdmp.cpp file to use the new Jack 2.0 control API. Available options for the server 
are "hard-coded" in the source. A much better approach would be to use the control API to:
- dynamically retrieve available server parameters and then prepare to parse them
- get available drivers and their possible parameters, then prepare to parse them.
*/

#ifdef __APPLE_
#include <CoreFoundation/CFNotificationCenter.h>

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

static void silent_jack_error_callback(const char *desc)
{}

static void copyright(FILE* file)
{
    fprintf(file, "jackdmp " VERSION "\n"
            "Copyright 2001-2005 Paul Davis and others.\n"
            "Copyright 2004-2008 Grame.\n"
            "jackdmp comes with ABSOLUTELY NO WARRANTY\n"
            "This is free software, and you are welcome to redistribute it\n"
            "under certain conditions; see the file COPYING for details\n");
}

static void usage(FILE* file)
{
    fprintf(file, "\n"
            "usage: jackdmp [ --realtime OR -R [ --realtime-priority OR -P priority ] ]\n"
            "               [ --name OR -n server-name ]\n"
            "               [ --timeout OR -t client-timeout-in-msecs ]\n"
            "               [ --loopback OR -L loopback-port-number ]\n"
            "               [ --verbose OR -v ]\n"
            "               [ --replace-registry OR -r ]\n"
            "               [ --silent OR -s ]\n"
            "               [ --sync OR -S ]\n"
            "               [ --temporary OR -T ]\n"
            "               [ --version OR -V ]\n"
            "         -d driver [ ... driver args ... ]\n"
            "             where driver can be `alsa', `coreaudio', 'portaudio' or `dummy'\n"
            "       jackdmp -d driver --help\n"
            "             to display options for each driver\n\n");
}

// To put in the control.h interface??
static jackctl_driver_t *
jackctl_server_get_driver(
    jackctl_server_t *server,
    const char *driver_name)
{
    const JSList * node_ptr;

    node_ptr = jackctl_server_get_drivers_list(server);

    while (node_ptr)
    {
        if (strcmp(jackctl_driver_get_name((jackctl_driver_t *)node_ptr->data), driver_name) == 0)
        {
            return (jackctl_driver_t *)node_ptr->data;
        }

        node_ptr = jack_slist_next(node_ptr);
    }

    return NULL;
}

static jackctl_parameter_t *
jackctl_get_parameter(
    const JSList * parameters_list,
    const char * parameter_name)
{
    while (parameters_list)
    {
        if (strcmp(jackctl_parameter_get_name((jackctl_parameter_t *)parameters_list->data), parameter_name) == 0)
        {
            return (jackctl_parameter_t *)parameters_list->data;
        }

        parameters_list = jack_slist_next(parameters_list);
    }

    return NULL;
}

int main(int argc, char* argv[])
{
    jackctl_server_t * server_ctl;
    const JSList * server_parameters;
    const char* server_name = "default";
    jackctl_driver_t * driver_ctl;
    const char *options = "-ad:P:uvrshVRL:STFl:t:mn:p:";
    struct option long_options[] = {
                                       { "driver", 1, 0, 'd' },
                                       { "verbose", 0, 0, 'v' },
                                       { "help", 0, 0, 'h' },
                                       { "port-max", 1, 0, 'p' },
                                       { "no-mlock", 0, 0, 'm' },
                                       { "name", 0, 0, 'n' },
                                       { "unlock", 0, 0, 'u' },
                                       { "realtime", 0, 0, 'R' },
                                       { "replace-registry", 0, 0, 'r' },
                                       { "loopback", 0, 0, 'L' },
                                       { "realtime-priority", 1, 0, 'P' },
                                       { "timeout", 1, 0, 't' },
                                       { "temporary", 0, 0, 'T' },
                                       { "version", 0, 0, 'V' },
                                       { "silent", 0, 0, 's' },
                                       { "sync", 0, 0, 'S' },
                                       { 0, 0, 0, 0 }
                                   };
    int i,opt = 0;
    int option_index = 0;
    bool seen_driver = false;
    char *driver_name = NULL;
    char **driver_args = NULL;
    int driver_nargs = 1;
    int port_max;
    bool show_version = false;
    sigset_t signals;
    jackctl_parameter_t* param;
    union jackctl_parameter_value value;

	copyright(stdout);
    
    server_ctl = jackctl_server_create();
    if (server_ctl == NULL) {
        fprintf(stderr, "Failed to create server object\n");
        return -1;
    }
    
    server_parameters = jackctl_server_get_parameters(server_ctl);

    opterr = 0;
    while (!seen_driver &&
            (opt = getopt_long(argc, argv, options,
                               long_options, &option_index)) != EOF) {
        switch (opt) {

            case 'd':
                seen_driver = true;
                driver_name = optarg;
                break;
                
            case 'p':
                port_max = (unsigned int)atol(optarg);
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

            case 'P':
                param = jackctl_get_parameter(server_parameters, "realtime-priority");
                if (param != NULL) {
                    value.i = atoi(optarg);
                    jackctl_parameter_set_value(param, &value);
                }
                break;

            case 'r':
                param = jackctl_get_parameter(server_parameters, "replace-registry");
                if (param != NULL) {
                    value.b = true;
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

            case 'L':
                param = jackctl_get_parameter(server_parameters, "loopback-ports");
                if (param != NULL) {
                    value.ui = atoi(optarg);
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

            case 'V':
                show_version = true;
                break;

            default:
                fprintf(stderr, "unknown option character %c\n", optopt);
                /*fallthru*/
            case 'h':
                usage(stdout);
                goto fail_free;
        }
    }

    if (show_version) {
    	printf("jackdmp version" VERSION 
               "\n");
    	return -1;
    }
 
    if (!seen_driver) {
        usage(stderr);
        goto fail_free;
    }

    driver_ctl = jackctl_server_get_driver(server_ctl, driver_name);
    if (driver_ctl == NULL) {
	fprintf(stderr, "Unkown driver \"%s\"\n", driver_name);
        goto fail_free;
    }

    if (optind < argc) {
        driver_nargs = 1 + argc - optind;
    } else {
        driver_nargs = 1;
    }

    if (driver_nargs == 0) {
        fprintf(stderr, "No driver specified ... hmm. JACK won't do"
                " anything when run like this.\n");
        goto fail_free;
    }

    driver_args = (char **) malloc(sizeof(char *) * driver_nargs);
    driver_args[0] = driver_name;

    for (i = 1; i < driver_nargs; i++) {
        driver_args[i] = argv[optind++];
    }

    if (jackctl_parse_driver_params(driver_ctl, driver_nargs, driver_args)) {
        goto fail_free;
    }
    
    if (!jackctl_server_start(server_ctl, driver_ctl)) {
        fprintf(stderr, "Failed to start server\n");
        goto fail_free;
    }
    
    notify_server_start(server_name);

    // Waits for signal
    signals = jackctl_setup_signals(0);
    jackctl_wait_signals(signals);
       
    if (!jackctl_server_stop(server_ctl))
        fprintf(stderr, "Cannot stop server...\n");
        
fail_free:
        
    jackctl_server_destroy(server_ctl);
    notify_server_stop(server_name);
    return 1;
}
