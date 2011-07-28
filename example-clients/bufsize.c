/*
 *  bufsize.c -- change JACK buffer size.
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
jack_nframes_t nframes;
int just_print_bufsize=0;

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
		just_print_bufsize = 1;
		return;
	}
	if (argc < 2) {
		fprintf(stderr, "usage: %s <bufsize>\n", package);
		exit(9);
	}

    if (strspn (argv[1], "0123456789") != strlen (argv[1])) {
		fprintf(stderr, "usage: %s <bufsize>\n", package);
		exit(8);
    }

	nframes = strtoul(argv[1], NULL, 0);
	if (errno == ERANGE) {
		fprintf(stderr, "%s: invalid buffer size: %s (range is 1-8192)\n",
			package, argv[1]);
		exit(2);
	}

    if (nframes < 1 || nframes > 8182) {
		fprintf(stderr, "%s: invalid buffer size: %s (range is 1-8192)\n",
			package, argv[1]);
		exit(3);
    }
}

int main(int argc, char *argv[])
{
	int rc;

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

	if (just_print_bufsize) {
		fprintf(stdout, "buffer size = %d  sample rate = %d\n", jack_get_buffer_size(client), jack_get_sample_rate(client));
		rc=0;
	}
	else
	{
		rc = jack_set_buffer_size(client, nframes);
		if (rc)
			fprintf(stderr, "jack_set_buffer_size(): %s\n", strerror(rc));
	}
	jack_client_close(client);

	return rc;
}
