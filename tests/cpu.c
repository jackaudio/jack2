/*
    Copyright (C) 2005 Samuel TRACOL
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

/** @file jack_cpu.c
 *
 * @brief This client test the capacity for jackd to kick out a to heavy cpu client.
 *
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <jack/jack.h>

jack_port_t *input_port;
jack_port_t *output_port;
jack_client_t *client;
unsigned long sr;
jack_nframes_t idle_time = 0;
int percent_cpu = 0;
int time_to_run = 0;
int time_before_run = 0;
int time_before_exit = 1;
jack_nframes_t cur_buffer_size;

/* a simple state machine for this client */
volatile enum {
	Init,
	Run,
	Exit
} client_state = Init;

void usage()
{
	fprintf (stderr, "\n"
					"usage: jack_cpu \n"
					"              [ --name OR -n client_name ]\n"
					"              [ --time OR -t time_to_run (in seconds) ]\n"
					"              [ --delay OR -d delay_before_cpuload__is_applied (in seconds) ]\n"
					"                --cpu OR -c percent_cpu_load (1-99)\n"
	);
}

int update_buffer_size(jack_nframes_t nframes, void *arg)
{
	cur_buffer_size = nframes;
	printf("Buffer size = %d \n", cur_buffer_size);
	idle_time = (jack_nframes_t) (cur_buffer_size * percent_cpu / 100);
	printf("CPU load applies as %d sample delay.\n", idle_time);
	return 0;
}

/**
 * The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 *
 */

int process(jack_nframes_t nframes, void *arg)
{
	jack_default_audio_sample_t *in, *out;
	jack_nframes_t start_frame = jack_frame_time(client);

	in = (jack_default_audio_sample_t *) jack_port_get_buffer (input_port, nframes);
	out = (jack_default_audio_sample_t *) jack_port_get_buffer (output_port, nframes);
	memset(out, 0, sizeof (jack_default_audio_sample_t) * nframes);

	while ((client_state == Run) && (jack_frame_time(client) < (start_frame + idle_time))) {}
 	return 0;
}

/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */

void jack_shutdown(void *arg)
{
	fprintf(stderr, "JACK shut down, exiting ...\n");
	exit (1);
}

int main(int argc, char *argv[])
{
	const char **ports;
	const char *client_name;
	int got_time = 0;
    jack_status_t status;

	int option_index;
	int opt;
	const char *options = "t:t:d:c:";
	struct option long_options[] =
	{
		{"time", 1, 0, 't'},
		{"name", 1, 0, 'n'},
		{"delay", 1, 0, 'd'},
		{"cpu", 1, 0, 'c'},
		{0, 0, 0, 0}
	};

	client_name = "jack-cpu";
	while ((opt = getopt_long (argc, argv, options, long_options, &option_index)) != EOF) {
		switch (opt) {
			case 'n':
				client_name = optarg;
				break;
			case 't':
				time_to_run = atoi(optarg);
				break;
			case 'd':
				time_before_run = atoi(optarg);
				break;
			case 'c':
				percent_cpu = atoi(optarg);
				got_time = 1;
				break;
			default:
				fprintf(stderr, "unknown option %c\n", opt);
				usage();
		}
	}

	if (!got_time) {
		fprintf(stderr, "CPU load not specified ! See usage as following :\n");
		usage();
		return -1;
	}

	if (time_to_run != 0)
		printf("Running jack-cpu for %d seconds...\n", time_to_run);

	/* open a client connection to the JACK server */

	client = jack_client_open (client_name, JackNoStartServer, &status);

	if (client == NULL) {
		fprintf(stderr, "jack_client_open() failed : is jack server running ?\n");
		exit(1);
	}

    cur_buffer_size = jack_get_buffer_size(client);
	printf("engine buffer size = %d \n", cur_buffer_size);
    printf("engine sample rate: %d Hz\n", jack_get_sample_rate(client));
	idle_time = (jack_nframes_t) (cur_buffer_size * percent_cpu / 100);
	printf("CPU load applies as %d sample delay.\n", idle_time);

	/* tell the JACK server to call `process()' whenever
	   there is work to be done.
	*/

	jack_set_process_callback(client, process, 0);

	/* tell the JACK server to call `jack_shutdown()' if
	   it ever shuts down, either entirely, or if it
	   just decides to stop calling us.
	*/

	jack_on_shutdown(client, jack_shutdown, 0);

	/* create two ports */

	input_port = jack_port_register (client, "input", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
	output_port = jack_port_register (client, "output", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

	printf("registering ports...\n");
	if ((input_port == NULL) || (output_port == NULL)) {
		fprintf(stderr, "no more JACK ports available\n");
		exit(1);
	}

	if (jack_set_buffer_size_callback(client, update_buffer_size, 0) != 0) {
		printf("Error when calling buffer_size_callback !");
		return -1;
	}

	/* Tell the JACK server that we are ready to roll.  Our
	 * process() callback will start running now. */

	printf("Activating as jackd client...\n");
	if (jack_activate(client)) {
		fprintf(stderr, "cannot activate client");
		exit(1);
	}

	/* Connect the ports.  You can't do this before the client is
	 * activated, because we can't make connections to clients
	 * that aren't running.  Note the confusing (but necessary)
	 * orientation of the driver backend ports: playback ports are
	 * "input" to the backend, and capture ports are "output" from
	 * it.
	 */

	ports = jack_get_ports (client, NULL, NULL, JackPortIsPhysical|JackPortIsOutput);
	if (ports == NULL) {
		fprintf(stderr, "no physical capture ports\n");
		exit (1);
	}

	if (jack_connect(client, ports[0], jack_port_name(input_port))) {
		fprintf (stderr, "cannot connect input ports\n");
	}
	jack_free(ports);

	ports = jack_get_ports (client, NULL, NULL, JackPortIsPhysical|JackPortIsInput);
	if (ports == NULL) {
		fprintf(stderr, "no physical playback ports\n");
		exit(1);
	}

	if (jack_connect(client, jack_port_name (output_port), ports[0])) {
		fprintf(stderr, "cannot connect output ports\n");
	}
	jack_free(ports);

	if (time_before_run == 0) {
		client_state = Run;
		printf("Activating cpu load...\n");
	}

	if (time_to_run !=0)
		time_before_exit = time_to_run + time_before_run;

	while (client_state != Exit) {
		if ((time_before_run > 0) && (client_state == Init))
			time_before_run--;
		sleep(1);
		if ((time_before_run < 1) && (client_state != Run)) {
			client_state = Run;
			printf("Activating cpu load...\n");
		}
		if (time_to_run != 0)
			time_before_exit--;
		if (time_before_exit < 1)
			client_state = Exit;
	}
	jack_client_close(client);
	printf("Exiting after a %d seconds run.\n", time_to_run);
	exit(0);
}
