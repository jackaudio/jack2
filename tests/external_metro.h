/*
 Copyright (C) 2001 Paul Davis
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation; either version 2.1 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public License
 along with this program; if not, write to the Free Software 
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 $Id: external_metro.h,v 1.3.2.1 2006/06/20 14:44:00 letz Exp $
*/

#ifndef __external_metro__
#define __external_metro__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <getopt.h>
#include <string.h>
#include <jack/jack.h>
#include <jack/transport.h>

    typedef jack_default_audio_sample_t sample_t;

    /*!
    \brief A class to test external clients
    */

    struct ExternalMetro {

        jack_client_t *client;
        jack_port_t *input_port;
        jack_port_t *output_port;

        unsigned long sr;
        int freq;
        int bpm;
        jack_nframes_t tone_length, wave_length;
        sample_t *wave;
        double *amp;
        long offset ;

        ExternalMetro(int freq, double max_amp, int dur_arg, int bpm, const char* client_name = "metro");
        virtual ~ExternalMetro();

        static int process_audio (jack_nframes_t nframes, void* arg);
        static void shutdown (void* arg);
    };

#ifdef __cplusplus
}
#endif

#endif
