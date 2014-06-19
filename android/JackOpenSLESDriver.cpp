/*
Copyright (C) 2001 Paul Davis
Copyright (C) 2004-2008 Grame
Copyright (C) 2013 Samsung Electronics

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

#include "JackOpenSLESDriver.h"
#include "JackDriverLoader.h"
#include "JackThreadedDriver.h"
#include "JackEngineControl.h"
#include "JackGraphManager.h"
#include "JackCompilerDeps.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

#include <android/log.h>
#include "opensl_io.h"

#define JACK_OPENSLES_DEFAULT_SAMPLERATE   48000
#define JACK_OPENSLES_DEFAULT_BUFFER_SIZE  960

namespace Jack
{

static OPENSL_STREAM  *pOpenSL_stream;

int JackOpenSLESDriver::Open(jack_nframes_t buffer_size,
                              jack_nframes_t samplerate,
                              bool capturing,
                              bool playing,
                              int inchannels,
                              int outchannels,
                              bool monitor,
                              const char* capture_driver_uid,
                              const char* playback_driver_uid,
                              jack_nframes_t capture_latency,
                              jack_nframes_t playback_latency) {
    jack_log("JackOpenSLESDriver::Open");

    // Generic JackAudioDriver Open
    if (JackAudioDriver::Open(buffer_size, samplerate, capturing, playing, inchannels, outchannels, monitor,
        capture_driver_uid, playback_driver_uid, capture_latency, playback_latency) != 0) {
        return -1;
    }

    if (capturing) {
        inbuffer = (float *) malloc(sizeof(float) * buffer_size);  //mono input
        memset(inbuffer, 0, sizeof(float) * buffer_size);
    }
    if (playing) {
        outbuffer = (float *) malloc(sizeof(float) * buffer_size * 2);  //stereo output
        memset(outbuffer, 0, sizeof(float) * buffer_size * 2);
    }
    pOpenSL_stream = android_OpenAudioDevice(samplerate, capturing ? 1 : 0, playing ? 2 : 0, buffer_size);
    if (pOpenSL_stream == NULL)  return -1;

    return 0;
}

int JackOpenSLESDriver::Close() {
    jack_log("JackOpenSLESDriver::Close");

    // Generic audio driver close
    int res = JackAudioDriver::Close();

    android_CloseAudioDevice(pOpenSL_stream);

    if (inbuffer) {
        free(inbuffer);
        inbuffer = NULL;
    }
    if (outbuffer) {
        free(outbuffer);
        outbuffer = NULL;
    }
    return res;
}

int JackOpenSLESDriver::Read() {
    //jack_log("JackOpenSLESDriver::Read");
    jack_default_audio_sample_t* inputBuffer_1 = GetInputBuffer(0);
    jack_default_audio_sample_t* inputBuffer_2 = GetInputBuffer(1);

    if (inbuffer) {
        int samps = android_AudioIn(pOpenSL_stream,inbuffer,fEngineControl->fBufferSize);
        for (int i = 0; i < samps; i++) {
            *(inputBuffer_1 + i) = *(inbuffer + i);
            *(inputBuffer_2 + i) = *(inbuffer + i);
        }
    } else {
        for (int i = 0; i < fCaptureChannels; i++) {
            memset(GetInputBuffer(i), 0, sizeof(jack_default_audio_sample_t) * fEngineControl->fBufferSize);  //silence
        }
    }

    return 0;
}

int JackOpenSLESDriver::Write() {
    //jack_log("JackOpenSLESDriver::Write");
    jack_default_audio_sample_t* outputBuffer_1 = GetOutputBuffer(0);
    jack_default_audio_sample_t* outputBuffer_2 = GetOutputBuffer(1);

    if (outbuffer) {
        android_AudioOut(pOpenSL_stream, outbuffer, fEngineControl->fBufferSize * 2);  //stereo output
        for (unsigned int i = 0, j = 0; i < fEngineControl->fBufferSize; i++) {
            *(outbuffer + j) = *(outputBuffer_1 + i);  j++;
            *(outbuffer + j) = *(outputBuffer_2 + i);  j++;
        }
    }

    return 0;
}

int JackOpenSLESDriver::SetBufferSize(jack_nframes_t buffer_size) {
    jack_log("JackOpenSLESDriver::SetBufferSize");
    JackAudioDriver::SetBufferSize(buffer_size);
    return 0;
}

} // end of namespace


#ifdef __cplusplus
extern "C"
{
#endif

    SERVER_EXPORT jack_driver_desc_t * driver_get_descriptor () {
        jack_driver_desc_t * desc;
        jack_driver_desc_filler_t filler;
        jack_driver_param_value_t value;

        desc = jack_driver_descriptor_construct("opensles", JackDriverMaster, "Timer based backend", &filler);

        value.ui = 2U;
        jack_driver_descriptor_add_parameter(desc, &filler, "capture", 'C', JackDriverParamUInt, &value, NULL, "Number of capture ports", NULL);
        jack_driver_descriptor_add_parameter(desc, &filler, "playback", 'P', JackDriverParamUInt, &value, NULL, "Number of playback ports", NULL);

        value.ui = 48000U;
        jack_driver_descriptor_add_parameter(desc, &filler, "rate", 'r', JackDriverParamUInt, &value, NULL, "Sample rate", NULL);

        value.i = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "monitor", 'm', JackDriverParamBool, &value, NULL, "Provide monitor ports for the output", NULL);

        value.ui = JACK_OPENSLES_DEFAULT_BUFFER_SIZE;
        jack_driver_descriptor_add_parameter(desc, &filler, "period", 'p', JackDriverParamUInt, &value, NULL, "Frames per period", NULL);

        value.ui = 21333U;
        jack_driver_descriptor_add_parameter(desc, &filler, "wait", 'w', JackDriverParamUInt, &value, NULL, "Number of usecs to wait between engine processes", NULL);

        return desc;
    }

    SERVER_EXPORT Jack::JackDriverClientInterface* driver_initialize(Jack::JackLockedEngine* engine, Jack::JackSynchro* table, const JSList* params) {
        jack_nframes_t sample_rate = JACK_OPENSLES_DEFAULT_SAMPLERATE;
        jack_nframes_t buffer_size = JACK_OPENSLES_DEFAULT_BUFFER_SIZE;
        unsigned int capture_ports = 0;
        unsigned int playback_ports = 2;
        int wait_time = 0;
        const JSList * node;
        const jack_driver_param_t * param;
        bool monitor = false;

        for (node = params; node; node = jack_slist_next (node)) {
            param = (const jack_driver_param_t *) node->data;

            switch (param->character) {

                case 'C':
                    capture_ports = param->value.ui;
                    break;

                case 'P':
                    playback_ports = param->value.ui;
                    break;

                case 'r':
                    sample_rate = param->value.ui;
                    break;

                case 'p':
                    buffer_size = param->value.ui;
                    break;

                case 'w':
                    wait_time = param->value.ui;
                    break;

                case 'm':
                    monitor = param->value.i;
                    break;
            }
        }

        if (wait_time > 0) {
            buffer_size = lroundf((wait_time * sample_rate) / 1000000.0f);
            if (buffer_size > BUFFER_SIZE_MAX) {
                buffer_size = BUFFER_SIZE_MAX;
                jack_error("Buffer size set to %d", BUFFER_SIZE_MAX);
            }
        }

        Jack::JackDriverClientInterface* driver = new Jack::JackThreadedDriver(new Jack::JackOpenSLESDriver("system", "opensles_pcm", engine, table));
        if (driver->Open(buffer_size, sample_rate, capture_ports? 1 : 0, playback_ports? 1 : 0, capture_ports, playback_ports, monitor, "opensles", "opensles", 0, 0) == 0) {
            return driver;
        } else {
            delete driver;
            return NULL;
        }
    }

#ifdef __cplusplus
}
#endif

