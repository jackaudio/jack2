/*
 *  smaplerate.c -- get current samplerate
 *
 *  Copyright (C) 2003 Jack O'Quin.
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
#include <jack/transport.h>

char *package;				/* program name */
jack_client_t *client;

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

	if (argc==1) {
		return;
	}
	fprintf(stderr, "usage: %s\n", package);
	exit(9);
}

int main(int argc, char *argv[])
{
	parse_arguments(argc, argv);

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

	fprintf(stdout, "%d\n", jack_get_sample_rate( client ) );

	jack_client_close(client);

	return 0;
}
