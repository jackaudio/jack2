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

    $Id: zombie.c,v 1.1 2005/08/18 11:42:08 letz Exp $
*/

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <jack/jack.h>

int running = 1;
int count = 0;
jack_port_t* output_port;

static int
process(jack_nframes_t nframes, void* arg)
{
	if (count++ == 1000) {
        printf("process block\n");
        //while (1) {}
        sleep(1);
    }

    return 0;
}

static void
shutdown (void *arg)
{
    printf("shutdown \n");
    running = 0;
}

int
main (int argc, char *argv[])
{
	jack_client_t* client = NULL;
    /* try to become a client of the JACK server */
	if ((client = jack_client_open ("zombie", JackNullOption, NULL)) == 0) {
		fprintf (stderr, "JACK server not running?\n");
		goto error;
	}

    jack_set_process_callback (client, process, NULL);
    jack_on_shutdown(client, shutdown, NULL);
    output_port = jack_port_register (client, "port1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

	/* tell the JACK server that we are ready to roll */
	if (jack_activate (client)) {
		fprintf (stderr, "cannot activate client");
		goto error;
	}

    jack_connect(client, jack_port_name(output_port), "coreaudio:Built-in Audio:in2");

    while (running) {
        sleep(1);
        printf ("run\n");
    }

    jack_deactivate (client);
	jack_client_close (client);
	return 0;

error:
    if (client)
        jack_client_close (client);
    return 1;
}

