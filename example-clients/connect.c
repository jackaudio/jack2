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

    $Id: connect.c,v 1.1.2.1 2006/06/05 16:54:55 letz Exp $
*/

#include <stdio.h>
#include <errno.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <jack/jack.h>

jack_port_t *input_port;
jack_port_t *output_port;
int connecting, disconnecting;
#define TRUE 1
#define FALSE 0

int
main (int argc, char *argv[])
{
	jack_client_t* client = NULL;
	char *my_name = strrchr(argv[0], '/');
	connecting = disconnecting = FALSE;
	if (my_name == 0) {
		my_name = argv[0];
	} else {
		my_name ++;
	}
    
    printf("name %s\n", my_name);

	if (strstr(my_name, "jack_disconnect")) {
		disconnecting = TRUE;
	} else
	if (strstr(my_name, "jack_connect")) {
		connecting = TRUE;
	} else {
		fprintf(stderr, "ERROR! client should be called jack_connect or jack_disconnect. client is called %s\n", my_name);
		return 1;
	}
	
	printf("connecting %ld\n",connecting);

	if (argc != 3) {
		fprintf (stderr, "usage: %s <src_port> <dst_port>\n", my_name);
		fprintf(stderr, "The source port must be an output port of the source client.\n");
		fprintf (stderr, "The destination port must be an input port of the destination client.\n");
		return 1;
	}

	/* try to become a client of the JACK server */

	if ((client = jack_client_new (my_name)) == 0) {
		fprintf (stderr, "jack server not running?\n");
		return 1;
	}

	/* display the current sample rate. once the client is activated 
	   (see below), you should rely on your own sample rate
	   callback (see above) for this value.
	*/

	/* find the two ports */

	if ((input_port = jack_port_by_name(client, argv[1])) == 0) {
		fprintf (stderr, "ERROR %s not a valid port\n", argv[1]);
		goto error;
	}
	if ((output_port = jack_port_by_name(client, argv[2])) == 0) {
		fprintf (stderr, "ERROR %s not a valid port\n", argv[2]);
		goto error;
	}
	
	/* tell the JACK server that we are ready to roll */

	if (jack_activate (client)) {
		fprintf (stderr, "cannot activate client");
		goto error;
	}

	/* connect the ports. Note: you can't do this before
	   the client is activated (this may change in the future).
	*/

 	if (connecting) {
		if (jack_connect(client, jack_port_name(input_port), jack_port_name(output_port))) {
			fprintf (stderr, "cannot connect ports\n");
			goto error;
        }
	}
	if (disconnecting) {
     	if (jack_disconnect(client, jack_port_name(input_port), jack_port_name(output_port))) {
			fprintf (stderr, "cannot disconnect ports\n");
            goto error;
        }
	}
    jack_deactivate (client);
	jack_client_close (client);
	return 0;
    
error:
    if (client) 
		jack_client_close (client);
    return 1;
}

