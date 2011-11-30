/*
    Copyright (C) 2009 Grame

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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <math.h>
#include <signal.h>
#include <getopt.h>
#include <string.h>

#include <jack/net.h>

jack_net_slave_t* net;

static void signal_handler(int sig)
{
	jack_net_slave_close(net);
	fprintf(stderr, "signal received, exiting ...\n");
	exit(0);
}

static void
usage ()
{
	fprintf (stderr, "\n"
    "usage: jack_net_slave \n"
    "              [ -C capture channels (default = 2)]\n"
    "              [ -P playback channels (default = 2) ]\n"
    "              [ -a hostname (default = %s) ]\n"
    "              [ -p port (default = %d)]\n", DEFAULT_MULTICAST_IP, DEFAULT_PORT);
}

static void net_shutdown(void* data)
{
    printf("Restarting...\n");
}

static int net_process(jack_nframes_t buffer_size,
                        int audio_input,
                        float** audio_input_buffer,
                        int midi_input,
                        void** midi_input_buffer,
                        int audio_output,
                        float** audio_output_buffer,
                        int midi_output,
                        void** midi_output_buffer,
                        void* data)
{
    int i;

    // Copy input to output
    for (i = 0; i < audio_input; i++) {
        memcpy(audio_output_buffer[i], audio_input_buffer[i], buffer_size * sizeof(float));
    }
    return 0;
}

int
main (int argc, char *argv[])
{
    int audio_input = 2;
    int audio_output = 2;
    int udp_port = DEFAULT_PORT;
    const char* multicast_ip = DEFAULT_MULTICAST_IP;
    const char *options = "C:P:a:p:h";
    int option_index;
    int opt;

	struct option long_options[] =
	{
		{"audio input", 1, 0, 'C'},
		{"audio output", 1, 0, 'P'},
		{"hostname", 1, 0, 'a'},
		{"port", 1, 0, 'p'},
		{0, 0, 0, 0}
	};

	while ((opt = getopt_long (argc, argv, options, long_options, &option_index)) != EOF) {

		switch (opt) {

		case 'C':
			audio_input = atoi(optarg);
			break;

		case 'P':
			audio_output = atoi(optarg);
			break;

		case 'a':
            multicast_ip = strdup(optarg);
            break;

		case 'p':
			udp_port = atoi(optarg);
			break;

		case 'h':
			usage();
			return -1;
		}
	}

    jack_slave_t request = { audio_input, audio_output, 0, 0, DEFAULT_MTU, -1, JackFloatEncoder, 0, 2 };
    jack_master_t result;

    printf("Waiting for a master...\n");

    if ((net = jack_net_slave_open(multicast_ip, udp_port, "net_slave", &request, &result)) == 0) {
    	fprintf(stderr, "JACK server not running?\n");
		return 1;
	}

    printf("Master is found and running...\n");

    jack_set_net_slave_process_callback(net, net_process, NULL);
    jack_set_net_slave_shutdown_callback(net, net_shutdown, NULL);

    if (jack_net_slave_activate(net) != 0) {
    	fprintf(stderr, "Cannot activate slave client\n");
		return 1;
	}

    /* install a signal handler to properly quits jack client */
#ifdef WIN32
	signal(SIGINT, signal_handler);
    signal(SIGABRT, signal_handler);
	signal(SIGTERM, signal_handler);
#else
	signal(SIGQUIT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGHUP, signal_handler);
	signal(SIGINT, signal_handler);
#endif

    /* run until interrupted */
	while (1) {
	#ifdef WIN32
		Sleep(1000);
	#else
		sleep(1);
	#endif
	};

    // Wait for application end
    jack_net_slave_deactivate(net);
    jack_net_slave_close(net);
    exit(0);
}
