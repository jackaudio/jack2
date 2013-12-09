/*
Copyright (C) 2013 Matt Flax <flatmax@>

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

#include "JackIIODriver.H"
#include "driver_interface.h"

#define IIO_DEFAULT_SAMPLERATE 1e6; ///< The default sample rate for the default chip
#define IIO_DEFAULT_PERIODSIZE 1024; ///< The defaul period size

namespace Jack {

int JackIIODriver::Open(jack_nframes_t buffer_size,
                          jack_nframes_t samplerate,
                          bool capturing,
                          bool playing,
                          int inchannels,
                          int outchannels,
                          bool monitor,
                          const char* capture_driver_name,
                          const char* playback_driver_name,
                          jack_nframes_t capture_latency,
                          jack_nframes_t playback_latency)
{
    return JackAudioDriver::Open(buffer_size, samplerate, capturing, playing, inchannels, outchannels,
        monitor, capture_driver_name, playback_driver_name, capture_latency, playback_latency);
}

} // end namespace Jack


#ifdef __cplusplus
extern "C"
{
#endif

    SERVER_EXPORT const jack_driver_desc_t *
    driver_get_descriptor () {
        jack_driver_desc_t * desc;
        jack_driver_desc_filler_t filler;
        jack_driver_param_value_t value;

        desc = jack_driver_descriptor_construct("iio", JackDriverMaster, "Linux Industrial IO backend", &filler);

        strcpy(value.str, "AD7476");
        jack_driver_descriptor_add_parameter(
            desc,
            &filler,
            "device",
            'd',
            JackDriverParamString,
            &value,
            NULL,
            "The IIO chip to use.",
            "The IIO chip to use. Specifies which chip name to scan for on all devices present in /sys/bus/iio.");

        value.ui = IIO_DEFAULT_PERIODSIZE;
        jack_driver_descriptor_add_parameter(desc, &filler, "period", 'p', JackDriverParamUInt, &value, NULL, "Frames per period", NULL);

        value.ui = 2;
        jack_driver_descriptor_add_parameter(desc, &filler, "nperiods", 'n', JackDriverParamUInt, &value, NULL, "Number of periods of playback latency", NULL);

        value.ui = (IIO_DEFAULT_SAMPLERATE)U;
        jack_driver_descriptor_add_parameter(desc, &filler, "rate", 'r', JackDriverParamUInt, &value, NULL, "Sample rate", NULL);

        value.i = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "capture", 'C', JackDriverParamBool, &value, NULL, "Provide capture ports.", NULL);
        jack_driver_descriptor_add_parameter(desc, &filler, "playback", 'P', JackDriverParamBool, &value, NULL, "Provide playback ports.", NULL);

        value.i = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "duplex", 'D', JackDriverParamBool, &value, NULL, "Provide both capture and playback ports.", NULL);

        value.ui = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "input-latency", 'I', JackDriverParamUInt, &value, NULL, "Extra input latency (frames)", NULL);
        jack_driver_descriptor_add_parameter(desc, &filler, "output-latency", 'O', JackDriverParamUInt, &value, NULL, "Extra output latency (frames)", NULL);

        value.ui = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "inchannels", 'i', JackDriverParamUInt, &value, NULL, "Number of input channels to provide (note: currently ignored)", NULL);
        jack_driver_descriptor_add_parameter(desc, &filler, "outchannels", 'o', JackDriverParamUInt, &value, NULL, "Number of output channels to provide (note: currently ignored)", NULL);

        value.ui = 3;
        jack_driver_descriptor_add_parameter(desc, &filler, "verbose", 'v', JackDriverParamUInt, &value, NULL, "gtkIOStream verbose level", NULL);

//        value.i = 0;
//        jack_driver_descriptor_add_parameter(desc, &filler, "snoop", 'X', JackDriverParamBool, &value, NULL, "Snoop firewire traffic", NULL);

        return desc;
    }

    SERVER_EXPORT Jack::JackDriverClientInterface* driver_initialize(Jack::JackLockedEngine* engine, Jack::JackSynchro* table, const JSList* params) {
        const JSList * node;
        const jack_driver_param_t * param;

        char *device_name=(char*)"AD7476";

        for (node = params; node; node = jack_slist_next (node)) {
            param = (jack_driver_param_t *) node->data;

            switch (param->character) {
                case 'd':
                    device_name = const_cast<char*>(param->value.str);
                    break;
                case 'p':
                    cmlparams.period_size = param->value.ui;
                    cmlparams.period_size_set = 1;
                    break;
                case 'n':
                    cmlparams.buffer_size = param->value.ui;
                    cmlparams.buffer_size_set = 1;
                    break;
                case 'r':
                    cmlparams.sample_rate = param->value.ui;
                    cmlparams.sample_rate_set = 1;
                    break;
                case 'i':
                    cmlparams.capture_ports = param->value.ui;
                    break;
                case 'o':
                    cmlparams.playback_ports = param->value.ui;
                    break;
                case 'I':
                    cmlparams.capture_frame_latency = param->value.ui;
                    break;
                case 'O':
                    cmlparams.playback_frame_latency = param->value.ui;
                    break;
                case 'x':
                    cmlparams.slave_mode = param->value.ui;
                    break;
//                case 'X':
//                    cmlparams.snoop_mode = param->value.i;
//                    break;
                case 'v':
                    cmlparams.verbose_level = param->value.ui;
            }
        }

        /* duplex is the default */
        if (!cmlparams.playback_ports && !cmlparams.capture_ports) {
            cmlparams.playback_ports = 1;
            cmlparams.capture_ports = 1;
        }

        iio.findDevicesByChipName(chipName);

        // Special open for FFADO driver...
        if (ffado_driver->Open(&cmlparams) == 0) {
            return threaded_driver;
        } else {
            delete threaded_driver; // Delete the decorated driver
            return NULL;
        }
    }

#ifdef __cplusplus
}
#endif
