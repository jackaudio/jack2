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
#include <assert.h>

#include <jack/net.h>

jack_net_master_t* net;

#define BUFFER_SIZE 512
#define SAMPLE_RATE 44100

static void signal_handler(int sig)
{
	jack_net_master_close(net);
	fprintf(stderr, "signal received, exiting ...\n");
	exit(0);
}

static void
usage ()
{
	fprintf (stderr, "\n"
    "usage: jack_net_master \n"
    "              [ -b buffer size (default = %d) ]\n"
    "              [ -r sample rate (default = %d) ]\n"
    "              [ -a hostname (default = %s) ]\n"
    "              [ -p port (default = %d) ]\n", BUFFER_SIZE, SAMPLE_RATE, DEFAULT_MULTICAST_IP, DEFAULT_PORT);
}

int
main (int argc, char *argv[])
{
    int buffer_size = BUFFER_SIZE;
    int sample_rate = SAMPLE_RATE;
    int udp_port = DEFAULT_PORT;
    const char* multicast_ip = DEFAULT_MULTICAST_IP;
 	const char *options = "b:r:a:p:h";
    int option_index;
	int opt;

    struct option long_options[] =
	{
		{"buffer size", 1, 0, 'b'},
		{"sample rate", 1, 0, 'r'},
		{"hostname", 1, 0, 'a'},
		{"port", 1, 0, 'p'},
		{0, 0, 0, 0}
	};

	while ((opt = getopt_long (argc, argv, options, long_options, &option_index)) != -1) {

		switch (opt) {

		case 'b':
			buffer_size = atoi(optarg);
			break;

		case 'r':
			sample_rate = atoi(optarg);
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

    int i;
    jack_master_t request = { 4, 4, -1, -1, buffer_size, sample_rate, "master" };
    //jack_master_t request = { -1, -1, -1, -1, buffer_size, sample_rate, "master" };
    jack_slave_t result;
    float** audio_input_buffer;
    float** audio_output_buffer;
    int wait_usec = (int) ((((float)buffer_size) * 1000000) / ((float)sample_rate));

    printf("Waiting for a slave...\n");

    if ((net = jack_net_master_open(multicast_ip, udp_port, "net_master", &request, &result))  == 0) {
        fprintf(stderr, "NetJack master can not be opened\n");
		return 1;
	}

    printf("Slave is running...\n");

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

    // Allocate buffers
    
    audio_input_buffer = (float**)calloc(result.audio_input, sizeof(float*));
    for (i = 0; i < result.audio_input; i++) {
        audio_input_buffer[i] = (float*)calloc(buffer_size, sizeof(float));
    }

    audio_output_buffer = (float**)calloc(result.audio_output, sizeof(float*));
    for (i = 0; i < result.audio_output; i++) {
        audio_output_buffer[i] = (float*)calloc(buffer_size, sizeof(float));
    }

    /*
    Run until interrupted.

    WARNING !! : this code is given for demonstration purpose. For proper timing bevahiour
    it has to be called in a real-time context (which is *not* the case here...)
    */

  	while (1) {

        // Copy input to output
        assert(result.audio_input == result.audio_output);
        for (i = 0; i < result.audio_input; i++) {
            memcpy(audio_output_buffer[i], audio_input_buffer[i], buffer_size * sizeof(float));
        }

        if (jack_net_master_send(net, result.audio_output, audio_output_buffer, 0, NULL) < 0) {
            printf("jack_net_master_send failure, exiting\n");
            break;
        }

        if (jack_net_master_recv(net, result.audio_input, audio_input_buffer, 0, NULL) < 0) {
            printf("jack_net_master_recv failure, exiting\n");
            break;
        }

        usleep(wait_usec);
	};

    // Wait for application end
    jack_net_master_close(net);

    for (i = 0; i < result.audio_input; i++) {
        free(audio_input_buffer[i]);
    }
    free(audio_input_buffer);

    for (i = 0; i < result.audio_output; i++) {
          free(audio_output_buffer[i]);
    }
    free(audio_output_buffer);

    exit (0);
}
