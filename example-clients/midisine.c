/*
    Copyright (C) 2004 Ian Esten

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
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <math.h>

#include <jack/jack.h>
#include <jack/midiport.h>

jack_port_t *input_port;
jack_port_t *output_port;
jack_default_audio_sample_t ramp=0.0;
jack_default_audio_sample_t note_on;
unsigned char note = 0;
jack_default_audio_sample_t note_frqs[128];

jack_client_t *client;

static void signal_handler(int sig)
{
	jack_client_close(client);
	fprintf(stderr, "signal received, exiting ...\n");
	exit(0);
}

static void calc_note_frqs(jack_default_audio_sample_t srate)
{
	int i;
	for(i=0; i<128; i++)
	{
		note_frqs[i] = (2.0 * 440.0 / 32.0) * pow(2, (((jack_default_audio_sample_t)i - 9.0) / 12.0)) / srate;
	}
}

static int process(jack_nframes_t nframes, void *arg)
{
	int i;
	void* port_buf = jack_port_get_buffer(input_port, nframes);
	jack_default_audio_sample_t *out = (jack_default_audio_sample_t *) jack_port_get_buffer (output_port, nframes);
	jack_midi_event_t in_event;
	jack_nframes_t event_index = 0;
	jack_nframes_t event_count = jack_midi_get_event_count(port_buf);
	if(event_count > 1)
	{
		printf(" midisine: have %d events\n", event_count);
		for(i=0; i<event_count; i++)
		{
			jack_midi_event_get(&in_event, port_buf, i);
			printf("    event %d time is %d. 1st byte is 0x%x\n", i, in_event.time, *(in_event.buffer));
		}
/*		printf("1st byte of 1st event addr is %p\n", in_events[0].buffer);*/
	}
	jack_midi_event_get(&in_event, port_buf, 0);
	for(i = 0; i < nframes; i++)
	{
		if ((in_event.time == i) && (event_index < event_count))
		{
			if (((*(in_event.buffer) & 0xf0)) == 0x90)
			{
                /* note on */
                note = *(in_event.buffer + 1);
                if (*(in_event.buffer + 2) == 0) {
                    note_on = 0.0;
                } else {
                    note_on = (float)(*(in_event.buffer + 2)) / 127.f;
                }
			}
			else if (((*(in_event.buffer)) & 0xf0) == 0x80)
			{
				/* note off */
				note = *(in_event.buffer + 1);
				note_on = 0.0;
			}
			event_index++;
			if(event_index < event_count)
				jack_midi_event_get(&in_event, port_buf, event_index);
		}
		ramp += note_frqs[note];
		ramp = (ramp > 1.0) ? ramp - 2.0 : ramp;
		out[i] = note_on*sin(2*M_PI*ramp);
	}
	return 0;
}

static int srate(jack_nframes_t nframes, void *arg)
{
	printf("the sample rate is now %" PRIu32 "/sec\n", nframes);
	calc_note_frqs((jack_default_audio_sample_t)nframes);
	return 0;
}

static void jack_shutdown(void *arg)
{
    fprintf(stderr, "JACK shut down, exiting ...\n");
	exit(1);
}

int main(int narg, char **args)
{
	if ((client = jack_client_open("midisine", JackNullOption, NULL)) == 0)
	{
		fprintf(stderr, "JACK server not running?\n");
		return 1;
	}

	calc_note_frqs(jack_get_sample_rate (client));

	jack_set_process_callback (client, process, 0);

	jack_set_sample_rate_callback (client, srate, 0);

	jack_on_shutdown (client, jack_shutdown, 0);

	input_port = jack_port_register (client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
	output_port = jack_port_register (client, "audio_out", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

	if (jack_activate (client))
	{
		fprintf(stderr, "cannot activate client");
		return 1;
	}

    /* install a signal handler to properly quits jack client */
    signal(SIGQUIT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGHUP, signal_handler);
	signal(SIGINT, signal_handler);

	/* run until interrupted */
	while(1) {
		sleep(1);
	}
	jack_client_close(client);
	exit (0);
}

