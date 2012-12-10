/*
 	Copyright (C) 2008 Grame

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

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <jack/jack.h>
#include <jack/control.h>

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

#if 0
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
#endif

static void print_value(union jackctl_parameter_value value, jackctl_param_type_t type)
{
    switch (type) {

        case JackParamInt:
            printf("parameter value = %d\n", value.i);
            break;

         case JackParamUInt:
            printf("parameter value = %u\n", value.ui);
            break;

         case JackParamChar:
            printf("parameter value = %c\n", value.c);
            break;

         case JackParamString:
            printf("parameter value = %s\n", value.str);
            break;

         case JackParamBool:
            printf("parameter value = %d\n", value.b);
            break;
     }
}

static void print_parameters(const JSList * node_ptr)
{
    while (node_ptr != NULL) {
        jackctl_parameter_t * parameter = (jackctl_parameter_t *)node_ptr->data;
        printf("\nparameter name = %s\n", jackctl_parameter_get_name(parameter));
        printf("parameter id = %c\n", jackctl_parameter_get_id(parameter));
        printf("parameter short decs = %s\n", jackctl_parameter_get_short_description(parameter));
        printf("parameter long decs = %s\n", jackctl_parameter_get_long_description(parameter));
        print_value(jackctl_parameter_get_default_value(parameter), jackctl_parameter_get_type(parameter));
        node_ptr = jack_slist_next(node_ptr);
    }
}

static void print_driver(jackctl_driver_t * driver)
{
    printf("\n--------------------------\n");
    printf("driver = %s\n", jackctl_driver_get_name(driver));
    printf("-------------------------- \n");
    print_parameters(jackctl_driver_get_parameters(driver));
}

static void print_internal(jackctl_internal_t * internal)
{
    printf("\n-------------------------- \n");
    printf("internal = %s\n", jackctl_internal_get_name(internal));
    printf("-------------------------- \n");
    print_parameters(jackctl_internal_get_parameters(internal));
}

static void usage()
{
	fprintf (stderr, "\n"
					"usage: jack_server_control \n"
					"              [ --driver OR -d driver_name ]\n"
					"              [ --client OR -c client_name ]\n"
	);
}

int main(int argc, char *argv[])
{
    jackctl_server_t * server;
    const JSList * parameters;
    const JSList * drivers;
    const JSList * internals;
    const JSList * node_ptr;
    jackctl_sigmask_t * sigmask;
    int opt, option_index;
    const char* driver_name = "dummy";
    const char* client_name = "audioadapter";

    const char *options = "d:c:";
	struct option long_options[] = {
		{"driver", 1, 0, 'd'},
		{"client", 1, 0, 'c'},
        {0, 0, 0, 0}
	};

 	while ((opt = getopt_long (argc, argv, options, long_options, &option_index)) != -1) {
		switch (opt) {
			case 'd':
				driver_name = optarg;
				break;
			case 'c':
				client_name = optarg;
				break;
            default:
				usage();
                exit(0);
		}
	}

	server = jackctl_server_create(NULL, NULL);
    parameters = jackctl_server_get_parameters(server);

    /*
    jackctl_parameter_t* param;
    union jackctl_parameter_value value;
    param = jackctl_get_parameter(parameters, "verbose");
    if (param != NULL) {
        value.b = true;
        jackctl_parameter_set_value(param, &value);
    }
    */

    printf("\n========================== \n");
    printf("List of server parameters \n");
    printf("========================== \n");

    print_parameters(parameters);

    printf("\n========================== \n");
    printf("List of drivers \n");
    printf("========================== \n");

    drivers = jackctl_server_get_drivers_list(server);
    node_ptr = drivers;
    while (node_ptr != NULL) {
        print_driver((jackctl_driver_t *)node_ptr->data);
        node_ptr = jack_slist_next(node_ptr);
    }

    printf("\n========================== \n");
    printf("List of internal clients \n");
    printf("========================== \n");

    internals = jackctl_server_get_internals_list(server);
    node_ptr = internals;
    while (node_ptr != NULL) {
        print_internal((jackctl_internal_t *)node_ptr->data);
        node_ptr = jack_slist_next(node_ptr);
    }

    // No error checking in this simple example...

    jackctl_server_open(server, jackctl_server_get_driver(server, driver_name));
    jackctl_server_start(server);

    jackctl_server_load_internal(server, jackctl_server_get_internal(server, client_name));

    /*
    // Switch master test

    jackctl_driver_t* master;

    usleep(5000000);
    printf("jackctl_server_load_master\n");
    master = jackctl_server_get_driver(server, "coreaudio");
    jackctl_server_switch_master(server, master);

    usleep(5000000);
    printf("jackctl_server_load_master\n");
    master = jackctl_server_get_driver(server, "dummy");
    jackctl_server_switch_master(server, master);

    */

    sigmask = jackctl_setup_signals(0);
    jackctl_wait_signals(sigmask);
    jackctl_server_stop(server);
    jackctl_server_close(server);
    jackctl_server_destroy(server);
    return 0;
}
