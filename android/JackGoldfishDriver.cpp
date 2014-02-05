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

#include "JackGoldfishDriver.h"
#include "JackDriverLoader.h"
#include "JackThreadedDriver.h"
#include "JackEngineControl.h"
#include "JackGraphManager.h"
#include "JackCompilerDeps.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

#define JACK_GOLDFISH_BUFFER_SIZE  4096

namespace Jack
{

static char const * const kAudioDeviceName = "/dev/eac";

int JackGoldfishDriver::Open(jack_nframes_t buffer_size,
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
    jack_log("JackGoldfishDriver::Open");

    // Generic JackAudioDriver Open
    if (JackAudioDriver::Open(buffer_size, samplerate, capturing, playing, inchannels, outchannels, monitor,
        capture_driver_uid, playback_driver_uid, capture_latency, playback_latency) != 0) {
        return -1;
    }

    mFd = ::open(kAudioDeviceName, O_RDWR);
    jack_log("JackGoldfishDriver::Open(mFd=%d)", mFd);

    if (!mBuffer)
        mBuffer = (short *) malloc(sizeof(short) * JACK_GOLDFISH_BUFFER_SIZE * 2);

	//JackAudioDriver::SetBufferSize(buffer_size);
	//JackAudioDriver::SetSampleRate(samplerate);

    return 0;
}

int JackGoldfishDriver::Close() {
    jack_log("JackGoldfishDriver::Close");

    // Generic audio driver close
    int res = JackAudioDriver::Close();

    if (mFd >= 0) ::close(mFd);

    if (mBuffer) {
        free(mBuffer);
        mBuffer = NULL;
    }

    return res;
}

int JackGoldfishDriver::Read() {
    jack_log("JackGoldfishDriver::Read");
    for (int i = 0; i < fCaptureChannels; i++) {
        //silence
        memset(GetInputBuffer(i), 0, sizeof(jack_default_audio_sample_t) * JACK_GOLDFISH_BUFFER_SIZE /* fEngineControl->fBufferSize */);
    }
    return 0;
}

int JackGoldfishDriver::Write() {
    jack_log("JackGoldfishDriver::Write");
    //write(mFd, GetOutputBuffer(i), sizeof(jack_default_audio_sample_t) * fEngineControl->fBufferSize);

    jack_default_audio_sample_t* outputBuffer_1 = GetOutputBuffer(0);
    jack_default_audio_sample_t* outputBuffer_2 = GetOutputBuffer(1);

    for(int i=0, j=0; i<JACK_GOLDFISH_BUFFER_SIZE /* fEngineControl->fBufferSize */; i++) {
        //convert float to short
        *(mBuffer + j) = (short) (*(outputBuffer_1 + i) * 32640);  j++;
        *(mBuffer + j) = (short) (*(outputBuffer_2 + i) * 32640);  j++;
    }
    write(mFd, mBuffer, sizeof(short) * JACK_GOLDFISH_BUFFER_SIZE * 2);
    return 0;
}

int JackGoldfishDriver::SetBufferSize(jack_nframes_t buffer_size) {
    jack_log("JackGoldfishDriver::SetBufferSize");
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

        desc = jack_driver_descriptor_construct("goldfish", JackDriverMaster, "Timer based backend", &filler);

        value.ui = 2U;
        jack_driver_descriptor_add_parameter(desc, &filler, "capture", 'C', JackDriverParamUInt, &value, NULL, "Number of capture ports", NULL);
        jack_driver_descriptor_add_parameter(desc, &filler, "playback", 'P', JackDriverParamUInt, &value, NULL, "Number of playback ports", NULL);

        value.ui = 44100U;
        jack_driver_descriptor_add_parameter(desc, &filler, "rate", 'r', JackDriverParamUInt, &value, NULL, "Sample rate", NULL);

        value.i = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "monitor", 'm', JackDriverParamBool, &value, NULL, "Provide monitor ports for the output", NULL);

        value.ui = JACK_GOLDFISH_BUFFER_SIZE;
        jack_driver_descriptor_add_parameter(desc, &filler, "period", 'p', JackDriverParamUInt, &value, NULL, "Frames per period", NULL);

        value.ui = 21333U;
        jack_driver_descriptor_add_parameter(desc, &filler, "wait", 'w', JackDriverParamUInt, &value, NULL, "Number of usecs to wait between engine processes", NULL);

        return desc;
    }

    SERVER_EXPORT Jack::JackDriverClientInterface* driver_initialize(Jack::JackLockedEngine* engine, Jack::JackSynchro* table, const JSList* params) {
        jack_nframes_t sample_rate = 44100;
        jack_nframes_t buffer_size = JACK_GOLDFISH_BUFFER_SIZE;
        unsigned int capture_ports = 2;
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

        Jack::JackDriverClientInterface* driver = new Jack::JackThreadedDriver(new Jack::JackGoldfishDriver("system", "goldfish_pcm", engine, table));
        if (driver->Open(buffer_size, sample_rate, 1, 1, capture_ports, playback_ports, monitor, "goldfish", "goldfish", 0, 0) == 0) {
            return driver;
        } else {
            delete driver;
            return NULL;
        }
    }

#ifdef __cplusplus
}
#endif
