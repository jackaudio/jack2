/*
    Copyright (C) 2002 Anthony Van Groningen
    
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
#include <getopt.h>
#include <string.h>

#include "jack.h"

typedef jack_default_audio_sample_t sample_t;

const double PI = 3.14;

jack_client_t *client;
jack_port_t *input_port;
jack_port_t *output_port;
unsigned long sr;
int freq = 880;
int bpm;
jack_nframes_t tone_length, wave_length;
sample_t *wave;
long offset = 0;
int transport_aware = 0;
jack_transport_state_t transport_state;

void
usage ()
{
	fprintf (stderr, "\n"
	"usage: jack_metro \n"
	"              [ --frequency OR -f frequency (in Hz) ]\n"
	"              [ --amplitude OR -A maximum amplitude (between 0 and 1) ]\n"
	"              [ --duration OR -D duration (in ms) ]\n"
	"              [ --attack OR -a attack (in percent of duration) ]\n"
	"              [ --decay OR -d decay (in percent of duration) ]\n"
	"              [ --name OR -n jack name for metronome client ]\n"
	"              [ --transport OR -t transport aware ]\n"
	"              --bpm OR -b beats per minute\n"
	);
}

void
process_silence (jack_nframes_t nframes) 
{
	sample_t *buffer = (sample_t *) jack_port_get_buffer (output_port, nframes);
	memset (buffer, 0, sizeof (jack_default_audio_sample_t) * nframes);
}

int
process_audio (jack_nframes_t nframes, void *arg) 
{
	sample_t *buffer = (sample_t *) jack_port_get_buffer (output_port, nframes);
	jack_nframes_t frames_left = nframes;
		
	while (wave_length - offset < frames_left) {
		memcpy (buffer + (nframes - frames_left), wave + offset, sizeof (sample_t) * (wave_length - offset));
		frames_left -= wave_length - offset;
		offset = 0;
	}
	if (frames_left > 0) {
		memcpy (buffer + (nframes - frames_left), wave + offset, sizeof (sample_t) * frames_left);
		offset += frames_left;
	}
	
	return 0;
}

/*
int
process (jack_nframes_t nframes, void *arg)
{
	if (transport_aware) {
		jack_position_t pos;

		if (jack_transport_query (client, &pos)
		    != JackTransportRolling) {

			process_silence (nframes);
			return 0;
		}
		offset = pos.frame % wave_length;
	}
	process_audio (nframes);
	return 0;
}
*/
int
sample_rate_change () {
	printf("Sample rate has changed! Exiting...\n");
	exit(-1);
}

int
main (int argc, char *argv[])
{
	sample_t scale;
	int i, attack_length, decay_length;
	double *amp;
	double max_amp = 0.5;
	int option_index;
	int opt;
	int got_bpm = 0;
	int attack_percent = 1, decay_percent = 10, dur_arg = 100;
	char *client_name = 0;
	char *bpm_string = "bpm";
	int verbose = 0;

	const char *options = "f:A:D:a:d:b:n:thv";
	struct option long_options[] =
	{
		{"frequency", 1, 0, 'f'},
		{"amplitude", 1, 0, 'A'},
		{"duration", 1, 0, 'D'},
		{"attack", 1, 0, 'a'},
		{"decay", 1, 0, 'd'},
		{"bpm", 1, 0, 'b'},
		{"name", 1, 0, 'n'},
		{"transport", 0, 0, 't'},
		{"help", 0, 0, 'h'},
		{"verbose", 0, 0, 'v'},
		{0, 0, 0, 0}
	};
 
	while ((opt = getopt_long (argc, argv, options, long_options, &option_index)) != EOF) {
		switch (opt) {
		case 'f':
			if ((freq = atoi (optarg)) <= 0) {
				fprintf (stderr, "invalid frequency\n");
				return -1;
			}
			break;
		case 'A':
			if (((max_amp = atof (optarg)) <= 0)|| (max_amp > 1)) {
				fprintf (stderr, "invalid amplitude\n");
				return -1;
			}
			break;
		case 'D':
			dur_arg = atoi (optarg);
			fprintf (stderr, "durarg = %u\n", dur_arg);
			break;
		case 'a':
			if (((attack_percent = atoi (optarg)) < 0) || (attack_percent > 100)) {
				fprintf (stderr, "invalid attack percent\n");
				return -1;
			}
			break;
		case 'd':
			if (((decay_percent = atoi (optarg)) < 0) || (decay_percent > 100)) {
				fprintf (stderr, "invalid decay percent\n");
				return -1;
			}
			break;
		case 'b':
			got_bpm = 1;
			if ((bpm = atoi (optarg)) < 0) {
				fprintf (stderr, "invalid bpm\n");
				return -1;
			}
			bpm_string = (char *) malloc ((strlen (optarg) + 4) * sizeof (char));
			strcpy (bpm_string, optarg);
			strcat (bpm_string, "_bpm");
			break;
		case 'n':
			client_name = (char *) malloc (strlen (optarg) * sizeof (char));
			strcpy (client_name, optarg);
			break;
		case 'v':
			verbose = 1;
			break;
		case 't':
			transport_aware = 1;
			break;
		default:
			fprintf (stderr, "unknown option %c\n", opt); 
		case 'h':
			usage ();
			return -1;
		}
	}

	if (!got_bpm) {
		fprintf (stderr, "bpm not specified\n");
		usage ();
		return -1;
	}

	/* Initial Jack setup, get sample rate */
	if (!client_name) {
		client_name = (char *) malloc (9 * sizeof (char));
		strcpy (client_name, "metro");
	}
	if ((client = jack_client_new (client_name)) == 0) {
		fprintf (stderr, "jack server not running?\n");
		return 1;
	}
    
	jack_set_process_callback (client, process_audio, 0);
	output_port = jack_port_register (client, bpm_string, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
	input_port = jack_port_register (client, "metro_in", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
	
    sr = jack_get_sample_rate (client);

	/* setup wave table parameters */
	wave_length = 60 * sr / bpm;
	tone_length = sr * dur_arg / 1000;
	attack_length = tone_length * attack_percent / 100;
	decay_length = tone_length * decay_percent / 100;
	scale = 2 * PI * freq / sr;

	if (tone_length >= wave_length) {
		//fprintf (stderr, "invalid duration (tone length = %" PRIu32
		//	 ", wave length = %" PRIu32 "\n", tone_length,
		//	 wave_length);
		return -1;
	}
	if (attack_length + decay_length > (int)tone_length) {
		fprintf (stderr, "invalid attack/decay\n");
		return -1;
	}

	/* Build the wave table */
	wave = (sample_t *) malloc (wave_length * sizeof(sample_t));
	amp = (double *) malloc (tone_length * sizeof(double));

	for (i = 0; i < attack_length; i++) {
		amp[i] = max_amp * i / ((double) attack_length);
	}
	for (i = attack_length; i < (int) tone_length - decay_length; i++) {
		amp[i] = max_amp;
	}
	for (i = (int)tone_length - decay_length; i < (int)tone_length; i++) {
		amp[i] = - max_amp * (i - (double) tone_length) / ((double) decay_length);
	}
	for (i = 0; i < (int) tone_length; i++) {
		wave[i] = amp[i] * sin (scale * i);
	}
	for (i = tone_length; i < (int) wave_length; i++) {
		wave[i] = 0;
	}
	
    if (jack_activate (client)) {
		fprintf (stderr, "cannot activate client");
		return 1;
	}
    
#ifdef WIN32
    // Connection can only be done after activation
    jack_connect(client,jack_port_name(output_port), "portaudio:winmme:in1");
	jack_connect(client,jack_port_name(output_port), "portaudio:winmme:in2");
	jack_connect(client,"portaudio:winmme:out2", jack_port_name(input_port));
#else
	// Connection can only be done after activation
    jack_connect(client,jack_port_name(output_port), "coreaudio:Built-in Audio:in2");
	jack_connect(client,"coreaudio:Built-in Audio:out2", jack_port_name(input_port));
#endif
	
   while ((getchar() != 'q')) {
		//while (1) {
		//sleep(1);
		//printf("jack_frame_time %ld\n", (long)jack_frame_time(client));
		//usleep(2000);
	};
   
    if (jack_deactivate (client)) {
		fprintf (stderr, "cannot deactivate client");
		return 1;
	}
    
    jack_client_close(client);
    return 0;
}
