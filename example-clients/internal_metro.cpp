/*
 Copyright (C) 2002 Anthony Van Groningen
 Copyright (C) 2005 Grame
 
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

#include "internal_metro.h"

typedef jack_default_audio_sample_t sample_t;

const double PI = 3.14;

static int process_audio (jack_nframes_t nframes, void* arg)
{
    InternalMetro* metro = (InternalMetro*)arg;
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

InternalMetro::InternalMetro(int freq, double max_amp, int dur_arg, int bpm, char* client_name)
{
    sample_t scale;
    int i, attack_length, decay_length;
    int attack_percent = 1, decay_percent = 10;
    const char *bpm_string = "bpm";

    offset = 0;

    /* Initial Jack setup, get sample rate */
    if (!client_name) {
        client_name = (char *) malloc (9 * sizeof (char));
        strcpy (client_name, "metro");
    }
    if ((client = jack_client_open (client_name, JackNullOption, NULL)) == 0) {
        fprintf (stderr, "jack server not running?\n");
        return;
    }

    jack_set_process_callback (client, process_audio, this);
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
        return;
    }
    if (attack_length + decay_length > (int)tone_length) {
        fprintf (stderr, "invalid attack/decay\n");
        return;
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
        fprintf(stderr, "cannot activate client");
    }
}

InternalMetro::~InternalMetro()
{
    jack_deactivate(client);
    jack_port_unregister(client, input_port);
    jack_port_unregister(client, output_port);
    jack_client_close(client);
    free(amp);
    free(wave);
}
