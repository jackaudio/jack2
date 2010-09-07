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

#include "external_metro.h"
#include <stdio.h>

typedef jack_default_audio_sample_t sample_t;

const double PI = 3.14;

static void JackSleep(int sec)
{
	#ifdef WIN32
		Sleep(sec * 1000);
	#else
		sleep(sec);
	#endif
}

int ExternalMetro::process_audio (jack_nframes_t nframes, void* arg)
{
    ExternalMetro* metro = (ExternalMetro*)arg;
    sample_t *buffer = (sample_t *) jack_port_get_buffer (metro->output_port, nframes);
    jack_nframes_t frames_left = nframes;

    while (metro->wave_length - metro->offset < frames_left) {
        memcpy (buffer + (nframes - frames_left), metro->wave + metro->offset, sizeof (sample_t) * (metro->wave_length - metro->offset));
        frames_left -= metro->wave_length - metro->offset;
        metro->offset = 0;
    }
    if (frames_left > 0) {
        memcpy (buffer + (nframes - frames_left), metro->wave + metro->offset, sizeof (sample_t) * frames_left);
        metro->offset += frames_left;
    }

    return 0;
}

void ExternalMetro::shutdown (void* arg)
{
    printf("shutdown called..\n");
}

ExternalMetro::ExternalMetro(int freq, double max_amp, int dur_arg, int bpm, const char* client_name)
{
    sample_t scale;
    int i, attack_length, decay_length;
    int attack_percent = 1, decay_percent = 10;
    const char *bpm_string = "bpm";
    jack_options_t options = JackNullOption;
    jack_status_t status;
    offset = 0;

    /* Initial Jack setup, get sample rate */
    if ((client = jack_client_open (client_name, options, &status)) == 0) {
        fprintf (stderr, "jack server not running?\n");
        throw -1;
    }

    jack_set_process_callback (client, process_audio, this);
    jack_on_shutdown (client, shutdown, this);
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
        /*
        fprintf (stderr, "invalid duration (tone length = %" PRIu32
        	 ", wave length = %" PRIu32 "\n", tone_length,
        	 wave_length);
        */
        return ;
    }
    if (attack_length + decay_length > (int)tone_length) {
        fprintf (stderr, "invalid attack/decay\n");
        return ;
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
    }
}

ExternalMetro::~ExternalMetro()
{
    jack_deactivate(client);
    jack_port_unregister(client, input_port);
    jack_port_unregister(client, output_port);
    jack_client_close(client);
    free(amp);
    free(wave);
}

int main (int argc, char *argv[])
{
    ExternalMetro* client1 = NULL;
    ExternalMetro* client2 = NULL;
    ExternalMetro* client3 = NULL;
    ExternalMetro* client4 = NULL;
    ExternalMetro* client5 = NULL;

    printf("Testing multiple Jack clients in one process: open 5 metro clients...\n");
    client1 = new ExternalMetro(1200, 0.4, 20, 80, "t1");
    client2 = new ExternalMetro(600, 0.4, 20, 150, "t2");
    client3 = new ExternalMetro(1000, 0.4, 20, 110, "t3");
	client4 = new ExternalMetro(400, 0.4, 20, 200, "t4");
    client5 = new ExternalMetro(1500, 0.4, 20, 150, "t5");

    printf("Type 'c' to close all clients and go to next test...\n");
    while ((getchar() != 'c')) {
        JackSleep(1);
    };

    delete client1;
    delete client2;
    delete client3;
    delete client4;
    delete client5;

    printf("Testing quitting the server while a client is running...\n");
    client1 = new ExternalMetro(1200, 0.4, 20, 80, "t1");

    printf("Now quit the server, shutdown callback should be called...\n");
    printf("Type 'c' to move on...\n");
    while ((getchar() != 'c')) {
        JackSleep(1);
    };

    printf("Closing client...\n");
    delete client1;

    printf("Now start the server again...\n");
    printf("Type 'c' to move on...\n");
    while ((getchar() != 'c')) {
        JackSleep(1);
    };

    printf("Opening a new client....\n");
    client1 = new ExternalMetro(1200, 0.4, 20, 80, "t1");
    
    printf("Now quit the server, shutdown callback should be called...\n");
    printf("Type 'c' to move on...\n");
    while ((getchar() != 'c')) {
        JackSleep(1);
    };

    printf("Simulating client not correctly closed...\n");
    
    printf("Opening a new client....\n"); 
    try {
        client1 = new ExternalMetro(1200, 0.4, 20, 80, "t1");
    } catch (int num) {
        printf("Cannot open a new client since old one was not closed correctly... OK\n"); 
    }

    printf("Type 'q' to quit...\n");
    while ((getchar() != 'q')) {
        JackSleep(1);
    };
    delete client1;
    return 0;
}
