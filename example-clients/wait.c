#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#include <time.h>

#include <jack/jack.h>

char * my_name;

void
show_usage(void)
{
	fprintf(stderr, "\nUsage: %s [options]\n", my_name);
	fprintf(stderr, "Check for jack existence, or wait, until it either quits, or gets started\n");
	fprintf(stderr, "options:\n");
	fprintf(stderr, "        -s, --server <name>   Connect to the jack server named <name>\n");
	fprintf(stderr, "        -w, --wait            Wait for server to become available\n");
	fprintf(stderr, "        -q, --quit            Wait until server is quit\n");
	fprintf(stderr, "        -c, --check           Check wether server is running\n");
	fprintf(stderr, "        -t, --timeout         Wait timeout in seconds\n");
	fprintf(stderr, "        -h, --help            Display this help message\n");
	fprintf(stderr, "For more information see http://jackaudio.org/\n");
}

int
main(int argc, char *argv[])
{
	jack_client_t *client;
	jack_status_t status;
	jack_options_t options = JackNoStartServer;
	int c;
	int option_index;
	char *server_name = NULL;
	int wait_for_start = 0;
	int wait_for_quit = 0;
	int just_check = 0;
	int wait_timeout = 0;
	time_t start_timestamp;


	struct option long_options[] = {
		{ "server", 1, 0, 's' },
		{ "wait", 0, 0, 'w' },
		{ "quit", 0, 0, 'q' },
		{ "check", 0, 0, 'c' },
		{ "timeout", 1, 0, 't' },
		{ "help", 0, 0, 'h' },
		{ 0, 0, 0, 0 }
	};

	my_name = strrchr(argv[0], '/');
	if (my_name == 0) {
		my_name = argv[0];
	} else {
		my_name ++;
	}

	while ((c = getopt_long (argc, argv, "s:wqct:hv", long_options, &option_index)) >= 0) {
		switch (c) {
		case 's':
			server_name = (char *) malloc (sizeof (char) * strlen(optarg));
			strcpy (server_name, optarg);
			options |= JackServerName;
			break;
		case 'w':
			wait_for_start = 1;
			break;
		case 'q':
			wait_for_quit = 1;
			break;
		case 'c':
			just_check = 1;
			break;
		case 't':
			wait_timeout = atoi(optarg);
			break;
		case 'h':
			show_usage();
			return 1;
			break;
		default:
			show_usage();
			return 1;
			break;
		}
	}

	/* try to open server in a loop. breaking under certein conditions */

	start_timestamp = time(NULL);

	while (1) {
		client = jack_client_open ("wait", options, &status, server_name);
		/* check for some real error and bail out */
		if ((client == NULL) && !(status & JackServerFailed)) {
			fprintf (stderr, "jack_client_open() failed, "
					"status = 0x%2.0x\n", status);
			return 1;
		}

		if (client == NULL) {
			if (wait_for_quit) {
				fprintf(stdout, "server is gone\n");
				break;
			}
			if (just_check) {
				fprintf(stdout, "not running\n");
				break;
			}
		} else {
			jack_client_close(client);
			if (wait_for_start) {
				fprintf(stdout, "server is available\n");
				break;
			}
			if (just_check) {
				fprintf(stdout, "running\n");
				break;
			}
		}
		if (wait_timeout) {
		       if ((time(NULL) - start_timestamp) > wait_timeout) {
			       fprintf(stdout, "timeout\n");
			       break;
		       }
		}

		// Wait a second, and repeat
		sleep(1);
	}

	exit(0);
}
