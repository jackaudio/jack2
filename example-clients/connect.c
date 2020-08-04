/*
    Copyright (C) 2002 Jeremy Hall

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
#ifndef WIN32
#include <unistd.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#include <jack/jack.h>
#include <jack/metadata.h>
#include <jack/session.h>
#include <jack/types.h>

#define TRUE 1
#define FALSE 0

volatile int done = 0;

void port_connect_callback(jack_port_id_t a, jack_port_id_t b, int connect, void* arg)
{
	done = 1;
}

void
show_version (char *my_name)
{
	//fprintf (stderr, "%s: JACK Audio Connection Kit version " VERSION "\n", my_name);
}

void
show_usage (char *my_name)
{
	show_version (my_name);
	fprintf (stderr, "\nusage: %s [options] port1 port2\n", my_name);
	fprintf (stderr, "Connects two JACK ports together.\n\n");
	fprintf (stderr, "        -s, --server <name>   Connect to the jack server named <name>\n");
	fprintf (stderr, "        -u, --uuid            Reference ports by their UUID instead of their name\n");
	fprintf (stderr, "        -f, --force           Force connect even if the ports may not be compatible\n");
	fprintf (stderr, "        -v, --version         Output version information and exit\n");
	fprintf (stderr, "        -h, --help            Display this help message\n\n");
	fprintf (stderr, "For more information see http://jackaudio.org/\n");
}

int
main (int argc, char *argv[])
{
	jack_client_t *client;
	jack_status_t status;
	char *server_name = NULL;
	int c;
	int option_index;
	jack_options_t options = JackNoStartServer;
	char *my_name = strrchr(argv[0], '/');
	jack_port_t *src_port = 0;
	jack_port_t *dst_port = 0;
	jack_port_t *port1 = 0;
	jack_port_t *port2 = 0;
	char portA[300];
	char portB[300];
	int use_uuid=0;
	int force_connect=0;
	int connecting, disconnecting;
	int port1_flags, port2_flags;
	int rc = 1;

	struct option long_options[] = {
		{ "server", 1, 0, 's' },
		{ "help", 0, 0, 'h' },
		{ "version", 0, 0, 'v' },
		{ "uuid", 0, 0, 'u' },
		{ "force", 0, 0, 'f' },
		{ 0, 0, 0, 0 }
	};

	while ((c = getopt_long (argc, argv, "s:fhvu", long_options, &option_index)) >= 0) {
		switch (c) {
		case 's':
			server_name = (char *) malloc (sizeof (char) * (strlen(optarg) + 1));
			strcpy (server_name, optarg);
			options |= JackServerName;
			break;
		case 'u':
			use_uuid = 1;
			break;
		case 'f':
			force_connect = 1;
			break;
		case 'h':
			show_usage (my_name);
			return 1;
			break;
		case 'v':
			show_version (my_name);
			return 1;
			break;
		default:
			show_usage (my_name);
			return 1;
			break;
		}
	}

	connecting = disconnecting = FALSE;
	if (my_name == 0) {
		my_name = argv[0];
	} else {
		my_name ++;
	}

	if (strstr(my_name, "disconnect")) {
		disconnecting = 1;
	} else if (strstr(my_name, "connect")) {
		connecting = 1;
	} else {
		fprintf(stderr, "ERROR! client should be called jack_connect or jack_disconnect. client is called %s\n", my_name);
		return 1;
	}

	if (argc < 3) {
		show_usage(my_name);
		return 1;
	}

	/* try to become a client of the JACK server */

	if ((client = jack_client_open (my_name, options, &status, server_name)) == 0) {
		fprintf (stderr, "jack server not running?\n");
		return 1;
	}

	jack_set_port_connect_callback(client, port_connect_callback, NULL);

	/* find the two ports */

	if( use_uuid ) {
		char *tmpname;
		char *clientname;
		char *portname;
		tmpname = strdup( argv[argc-1] );
		portname = strchr( tmpname, ':' );
		portname[0] = '\0';
		portname+=1;
		clientname = jack_get_client_name_by_uuid( client, tmpname );
		if( clientname ) {

			snprintf( portA, sizeof(portA), "%s:%s", clientname, portname );
			jack_free( clientname );
		} else {
			snprintf( portA, sizeof(portA), "%s", argv[argc-1] );
		}
		free( tmpname );

		tmpname = strdup( argv[argc-2] );
		portname = strchr( tmpname, ':' );
		portname[0] = '\0';
		portname+=1;
		clientname = jack_get_client_name_by_uuid( client, tmpname );
		if( clientname ) {
			snprintf( portB, sizeof(portB), "%s:%s", clientname, portname );
			jack_free( clientname );
		} else {
			snprintf( portB, sizeof(portB), "%s", argv[argc-2] );
		}

		free( tmpname );

	} else {
		snprintf( portA, sizeof(portA), "%s", argv[argc-1] );
		snprintf( portB, sizeof(portB), "%s", argv[argc-2] );
	}
	if ((port1 = jack_port_by_name(client, portA)) == 0) {
		fprintf (stderr, "ERROR %s not a valid port\n", portA);
		goto exit;
	}
	if ((port2 = jack_port_by_name(client, portB)) == 0) {
		fprintf (stderr, "ERROR %s not a valid port\n", portB);
		goto exit;
	}

	port1_flags = jack_port_flags (port1);
	port2_flags = jack_port_flags (port2);

	if (port1_flags & JackPortIsInput) {
		if (port2_flags & JackPortIsOutput) {
			src_port = port2;
			dst_port = port1;
		}
	} else {
		if (port2_flags & JackPortIsInput) {
			src_port = port1;
			dst_port = port2;
		}
	}

	if (!src_port || !dst_port) {
		fprintf (stderr, "arguments must include 1 input port and 1 output port\n");
		goto exit;
	}

	/* tell the JACK server that we are ready to roll */
	if (jack_activate (client)) {
		fprintf (stderr, "cannot activate client");
		goto exit;
	}

	/* connect the ports. Note: you can't do this before
	   the client is activated (this may change in the future).
	*/

	if (connecting) {
		if (!force_connect) {
			int fail = 0;
			
			/* Get the port type to know which proprty to query. */
			const char* src_port_type = jack_port_type(src_port);
			const char* dst_port_type = jack_port_type(dst_port);
			
			/* Get src's port content */
			char* src_type;
			if (strcmp(src_port_type, JACK_DEFAULT_AUDIO_TYPE) == 0)
				jack_get_property(jack_port_uuid(src_port), JACK_METADATA_SIGNAL_TYPE, &src_type, NULL);
			else if (strcmp(src_port_type, JACK_DEFAULT_MESSAGE_TYPE) == 0)
				jack_get_property(jack_port_uuid(src_port), JACK_METADATA_EVENT_TYPES, &src_type, NULL);
			else
				fail = 1;
			
			/* Get src's port content */
			char* dst_type;
			if (strcmp(dst_port_type, JACK_DEFAULT_AUDIO_TYPE) == 0)
				jack_get_property(jack_port_uuid(dst_port), JACK_METADATA_SIGNAL_TYPE, &dst_type, NULL);
			else if (strcmp(dst_port_type, JACK_DEFAULT_MESSAGE_TYPE) == 0)
				jack_get_property(jack_port_uuid(dst_port), JACK_METADATA_EVENT_TYPES, &dst_type, NULL);
			else
				fail = 1;
			
			/* Finally compare the ports */
			if (!fail)
				// TODO this does not handle if multiple content types are supported.
				fail = strcmp(src_type, dst_type);
			
			jack_free(src_type);
			jack_free(dst_type);
			
			if (fail) {
				fprintf (stderr, "cannot connect ports, since they may have different kinds of data. Run with --force to connect them anyways.\n");
				goto exit;
			}
		}
		
		if (jack_connect(client, jack_port_name(src_port), jack_port_name(dst_port))) {
			fprintf (stderr, "cannot connect client, already connected?\n");
			goto exit;
		}
	}
	if (disconnecting) {
		if (jack_disconnect(client, jack_port_name(src_port), jack_port_name(dst_port))) {
			fprintf (stderr, "cannot disconnect client, already disconnected?\n");
			goto exit;
		}
	}

	// Wait for connection/disconnection to be effective
	while(!done) {
#ifdef WIN32
		Sleep(10);
#else
		usleep(10000);
#endif
	}

	/* everything was ok, so setting exitcode to 0 */
	rc = 0;

exit:
	jack_client_close (client);
	exit (rc);
}
