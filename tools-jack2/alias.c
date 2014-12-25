/*
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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <jack/jack.h>

char * my_name;

void
show_version (void)
{
	//fprintf (stderr, "%s: JACK Audio Connection Kit version " VERSION "\n", my_name);
}

void
show_usage (void)
{
	show_version ();
	fprintf (stderr, "\nUsage: %s [options] portname alias\n", my_name);
	fprintf (stderr, "List active Jack ports, and optionally display extra information.\n\n");
	fprintf (stderr, "Display options:\n");
	fprintf (stderr, "        -u, --unalias         remove `alias' as an alias for `port'\n");
	fprintf (stderr, "        -h, --help            Display this help message\n");
	fprintf (stderr, "        --version             Output version information and exit\n\n");
	fprintf (stderr, "For more information see http://jackaudio.org/\n");
}

int
main (int argc, char *argv[])
{
	jack_client_t *client;
	jack_status_t status;
	char* portname;
	char* alias;
	int unset = 0;
	int ret;
	int c;
	int option_index;
	extern int optind;
	jack_port_t* port;

	struct option long_options[] = {
		{ "unalias", 0, 0, 'u' },
		{ "help", 0, 0, 'h' },
		{ "version", 0, 0, 'v' },
		{ 0, 0, 0, 0 }
	};

	if (argc < 3) {
		show_usage ();
		return 1;
	}

	my_name = strrchr(argv[0], '/');
	if (my_name == 0) {
		my_name = argv[0];
	} else {
		my_name ++;
	}

	while ((c = getopt_long (argc, argv, "uhv", long_options, &option_index)) >= 0) {
		switch (c) {
		case 'u':
			unset = 1;
			break;
		case 'h':
			show_usage ();
			return 1;
			break;
		case 'v':
			show_version ();
			return 1;
			break;
		default:
			show_usage ();
			return 1;
			break;
		}
	}

	portname = argv[optind++];
	alias = argv[optind];

	/* Open a client connection to the JACK server.  Starting a
	 * new server only to list its ports seems pointless, so we
	 * specify JackNoStartServer. */
	//JOQ: need a new server name option

	client = jack_client_open ("lsp", JackNoStartServer, &status);

	if (client == NULL) {
		if (status & JackServerFailed) {
			fprintf (stderr, "JACK server not running\n");
		} else {
			fprintf (stderr, "jack_client_open() failed, "
				 "status = 0x%2.0x\n", status);
		}
		return 1;
	}

	if ((port = jack_port_by_name (client, portname)) == 0) {
		fprintf (stderr, "No port named \"%s\"\n", portname);
		return 1;
	}

	if (!unset) {
		ret = jack_port_set_alias (port, alias);
	} else {
		ret = jack_port_unset_alias (port, alias);
	}

	jack_client_close (client);

	return ret;

}
