#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <jack/jack.h>

#define TRUE 1
#define FALSE 0

int
main (int argc, char *argv[])

{
	jack_client_t *client;
	char *my_name = strrchr(argv[0], '/');

	if (my_name == 0) {
		my_name = argv[0];
	} else {
		my_name ++;
	}

	if (argc != 2) {
		fprintf (stderr, "Usage: %s client\n", my_name);
		return 1;
	}

	if ((client = jack_client_open ("input monitoring", JackNullOption, NULL)) == 0) {
		fprintf (stderr, "jack server not running?\n");
		return 1;
	}

	if (jack_port_request_monitor_by_name (client, argv[1], TRUE)) {
		fprintf (stderr, "could not enable monitoring for %s\n", argv[1]);
		jack_client_close (client);
		return 1;
	}
	sleep (30);
	if (jack_port_request_monitor_by_name (client, argv[1], FALSE)) {
		fprintf (stderr, "could not disable monitoring for %s\n", argv[1]);
	}
	jack_client_close (client);
	exit (0);
}

