/*
 *  session_notify.c -- ultra minimal session manager
 *
 *  Copyright (C) 2010 Torben Hohn.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <jack/jack.h>
#include <jack/jslist.h>
#include <jack/transport.h>
#include <jack/session.h>

char *package;				/* program name */
jack_client_t *client;

jack_session_event_type_t notify_type;
char *save_path = NULL;

void jack_shutdown(void *arg)
{
	fprintf(stderr, "JACK shut down, exiting ...\n");
	exit(1);
}

void signal_handler(int sig)
{
	jack_client_close(client);
	fprintf(stderr, "signal received, exiting ...\n");
	exit(0);
}

void parse_arguments(int argc, char *argv[])
{

	/* basename $0 */
	package = strrchr(argv[0], '/');
	if (package == 0)
		package = argv[0];
	else
		package++;

	if (argc==2) {
		if( !strcmp( argv[1], "quit" ) ) {
			notify_type = JackSessionSaveAndQuit;
			return;
		}
	}
	if (argc==3) {
		if( !strcmp( argv[1], "save" ) ) {
			notify_type = JackSessionSave;
			save_path = argv[2];
			return;
		}

	}
	fprintf(stderr, "usage: %s quit|save [path]\n", package);
	exit(9);
}

typedef struct {
	char name[32];
	char uuid[16];
} uuid_map_t;

JSList *uuid_map = NULL;

void add_uuid_mapping( const char *uuid ) {
	char *clientname = jack_get_client_name_by_uuid( client, uuid );
	if( !clientname ) {
		printf( "error... cant find client for uuid" );
		return;
	}

	uuid_map_t *mapping = malloc( sizeof(uuid_map_t) );
	snprintf( mapping->uuid, sizeof(mapping->uuid), "%s", uuid );
	snprintf( mapping->name, sizeof(mapping->name), "%s", clientname );
	uuid_map = jack_slist_append( uuid_map, mapping );
}

char *map_port_name_to_uuid_port( const char *port_name )
{
	JSList *node;
	char retval[300];
	char *port_component = strchr( port_name,':' );
	char *client_component = strdup( port_name );
	strchr( client_component, ':' )[0] = '\0';

	sprintf( retval, "%s", port_name );

	for( node=uuid_map; node; node=jack_slist_next(node) ) {
		uuid_map_t *mapping = node->data;
		if( !strcmp( mapping->name, client_component ) ) {
			sprintf( retval, "%s%s", mapping->uuid, port_component );
			break;
		}
	}

	return strdup(retval);
}

int main(int argc, char *argv[])
{
	parse_arguments(argc, argv);
	jack_session_command_t *retval;
	int k,i,j;


	/* become a JACK client */
	if ((client = jack_client_open(package, JackNullOption, NULL)) == 0) {
		fprintf(stderr, "JACK server not running?\n");
		exit(1);
	}

	signal(SIGQUIT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGHUP, signal_handler);
	signal(SIGINT, signal_handler);

	jack_on_shutdown(client, jack_shutdown, 0);

	jack_activate(client);


	retval = jack_session_notify( client, NULL, notify_type, save_path );
	for (i = 0; retval[i].uuid; i++) {
		printf( "export SESSION_DIR=\"%s%s/\"\n", save_path, retval[i].client_name );
		printf( "%s &\n", retval[i].command );
		add_uuid_mapping(retval[i].uuid);
	}

	printf( "sleep 10\n" );

	for (k = 0; retval[k].uuid; k++) {

		char* port_regexp = alloca( jack_client_name_size()+3 );
		char* client_name = jack_get_client_name_by_uuid( client, retval[k].uuid );
		snprintf( port_regexp, jack_client_name_size()+3, "%s:.*", client_name );
		jack_free(client_name);
		const char **ports = jack_get_ports( client, port_regexp, NULL, 0 );
		if( !ports ) {
			continue;
		}
		for (i = 0; ports[i]; ++i) {
			const char **connections;
			if ((connections = jack_port_get_all_connections (client, jack_port_by_name(client, ports[i]))) != 0) {
				for (j = 0; connections[j]; j++) {
					char *src = map_port_name_to_uuid_port( ports[i] );
					char *dst = map_port_name_to_uuid_port( connections[j] );
					printf( "jack_connect -u \"%s\" \"%s\"\n", src, dst );
				}
				jack_free (connections);
			}
		}
		jack_free(ports);

	}
	jack_session_commands_free(retval);

	jack_client_close(client);

	return 0;
}
