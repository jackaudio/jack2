/*
Copyright (C) 2001 Paul Davis
Copyright (C) 2004 Grame
Copyright (C) 2007 Pieter Palmers
Copyright (C) 2009 Devin Anderson
Copyright (C) 2012 Jonathan Woithe, Adrian Knoth

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

#include <iostream>
#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <regex.h>
#include <string.h>

#include "JackFFADODriver.h"
#include "JackFFADOMidiInputPort.h"
#include "JackFFADOMidiOutputPort.h"
#include "JackEngineControl.h"
#include "JackClientControl.h"
#include "JackPort.h"
#include "JackGraphManager.h"
#include "JackCompilerDeps.h"
#include "JackLockedEngine.h"

// FFADO_API_VERSION was first defined with API_VERSION 9, so all previous
// headers do not provide this define.
#ifndef FFADO_API_VERSION
extern "C" int ffado_streaming_set_period_size(ffado_device_t *dev,
		unsigned int period) __attribute__((__weak__));
#endif

namespace Jack
{

// Basic functionality requires API version 8.  If version 9 or later
// is present the buffers can be resized at runtime.
#define FIREWIRE_REQUIRED_FFADO_API_VERSION 8
#define FIREWIRE_REQUIRED_FFADO_API_VERSION_FOR_SETBUFSIZE 9

#define jack_get_microseconds GetMicroSeconds

int
JackFFADODriver::ffado_driver_read (ffado_driver_t * driver, jack_nframes_t nframes)
{
    channel_t chn;
    jack_default_audio_sample_t* buf = NULL;

    printEnter();
    for (chn = 0; chn < driver->capture_nchannels; chn++) {
        // if nothing connected, don't process
        if (fGraphManager->GetConnectionsNum(fCapturePortList[chn]) == 0) {
            buf = (jack_default_audio_sample_t*)driver->scratchbuffer;
            // we always have to specify a valid buffer
            ffado_streaming_set_capture_stream_buffer(driver->dev, chn, (char *)(buf));
            // notify the streaming system that it can (but doesn't have to) skip
            // this channel
            ffado_streaming_capture_stream_onoff(driver->dev, chn, 0);
        } else {
            if (driver->capture_channels[chn].stream_type == ffado_stream_type_audio) {
                buf = (jack_default_audio_sample_t*)fGraphManager->GetBuffer(fCapturePortList[chn],  nframes);

                /* if the returned buffer is invalid, use the dummy buffer */
                if (!buf) buf = (jack_default_audio_sample_t*)driver->scratchbuffer;

                ffado_streaming_set_capture_stream_buffer(driver->dev, chn, (char *)(buf));
                ffado_streaming_capture_stream_onoff(driver->dev, chn, 1);
            } else if (driver->capture_channels[chn].stream_type == ffado_stream_type_midi) {
                ffado_streaming_set_capture_stream_buffer(driver->dev, chn,
                        (char *)(driver->capture_channels[chn].midi_buffer));
                ffado_streaming_capture_stream_onoff(driver->dev, chn, 1);
            } else { // always have a valid buffer
                ffado_streaming_set_capture_stream_buffer(driver->dev, chn, (char *)(driver->scratchbuffer));
                // don't process what we don't use
                ffado_streaming_capture_stream_onoff(driver->dev, chn, 0);
            }
        }
    }

    /* now transfer the buffers */
    ffado_streaming_transfer_capture_buffers(driver->dev);

    /* process the midi data */
    for (chn = 0; chn < driver->capture_nchannels; chn++) {
        if (driver->capture_channels[chn].stream_type == ffado_stream_type_midi) {
            JackFFADOMidiInputPort *midi_input = (JackFFADOMidiInputPort *) driver->capture_channels[chn].midi_input;
            JackMidiBuffer *buffer = (JackMidiBuffer *) fGraphManager->GetBuffer(fCapturePortList[chn], nframes);
            midi_input->Process(buffer, driver->capture_channels[chn].midi_buffer, nframes);
        }
    }

    printExit();
    return 0;
}

int
JackFFADODriver::ffado_driver_write (ffado_driver_t * driver, jack_nframes_t nframes)
{
    channel_t chn;
    jack_default_audio_sample_t* buf;
    printEnter();

    driver->process_count++;

    for (chn = 0; chn < driver->playback_nchannels; chn++) {
        if (fGraphManager->GetConnectionsNum(fPlaybackPortList[chn]) == 0) {
            buf = (jack_default_audio_sample_t*)driver->nullbuffer;
            // we always have to specify a valid buffer
            ffado_streaming_set_playback_stream_buffer(driver->dev, chn, (char *)(buf));
            // notify the streaming system that it can (but doesn't have to) skip
            // this channel
            ffado_streaming_playback_stream_onoff(driver->dev, chn, 0);
        } else {
            if (driver->playback_channels[chn].stream_type == ffado_stream_type_audio) {
                buf = (jack_default_audio_sample_t*)fGraphManager->GetBuffer(fPlaybackPortList[chn], nframes);
                /* use the silent buffer if there is no valid jack buffer */
                if (!buf) buf = (jack_default_audio_sample_t*)driver->nullbuffer;
                ffado_streaming_set_playback_stream_buffer(driver->dev, chn, (char *)(buf));
                ffado_streaming_playback_stream_onoff(driver->dev, chn, 1);
            } else if (driver->playback_channels[chn].stream_type == ffado_stream_type_midi) {
                uint32_t *midi_buffer = driver->playback_channels[chn].midi_buffer;
                memset(midi_buffer, 0, nframes * sizeof(uint32_t));
                buf = (jack_default_audio_sample_t *) fGraphManager->GetBuffer(fPlaybackPortList[chn], nframes);
                ffado_streaming_set_playback_stream_buffer(driver->dev, chn, (char *)(midi_buffer));
                ffado_streaming_playback_stream_onoff(driver->dev, chn, buf ? 1 : 0);
                JackFFADOMidiOutputPort *midi_output = (JackFFADOMidiOutputPort *) driver->playback_channels[chn].midi_output;
                midi_output->Process((JackMidiBuffer *) buf, midi_buffer, nframes);

            } else { // always have a valid buffer
                ffado_streaming_set_playback_stream_buffer(driver->dev, chn, (char *)(driver->nullbuffer));
                ffado_streaming_playback_stream_onoff(driver->dev, chn, 0);
            }
        }
    }
    ffado_streaming_transfer_playback_buffers(driver->dev);
    printExit();
    return 0;
}

jack_nframes_t
JackFFADODriver::ffado_driver_wait (ffado_driver_t *driver, int extra_fd, int *status,
                                    float *delayed_usecs)
{
    jack_time_t wait_enter;
    jack_time_t wait_ret;
    ffado_wait_response response;

    printEnter();

    wait_enter = jack_get_microseconds ();
    if (wait_enter > driver->wait_next) {
        /*
                * This processing cycle was delayed past the
                * next due interrupt!  Do not account this as
                * a wakeup delay:
                */
        driver->wait_next = 0;
        driver->wait_late++;
    }
// *status = -2; interrupt
// *status = -3; timeout
// *status = -4; extra FD

    response = ffado_streaming_wait(driver->dev);

    wait_ret = jack_get_microseconds ();

    if (driver->wait_next && wait_ret > driver->wait_next) {
        *delayed_usecs = wait_ret - driver->wait_next;
    }
    driver->wait_last = wait_ret;
    driver->wait_next = wait_ret + driver->period_usecs;
//         driver->engine->transport_cycle_start (driver->engine, wait_ret);

    if(response == ffado_wait_ok) {
       // all good
       *status = 0;
    } else if (response == ffado_wait_xrun) {
        // xrun happened, but it's handled
        *status = 0;
        return 0;
    } else if (response == ffado_wait_error) {
        // an error happened (unhandled xrun)
        // this should be fatal
        jack_error("JackFFADODriver::ffado_driver_wait - unhandled xrun");
        *status = -1;
        return 0;
    } else if (response == ffado_wait_shutdown) {
        // ffado requested shutdown (e.g. device unplugged)
        // this should be fatal
        jack_error("JackFFADODriver::ffado_driver_wait - shutdown requested "
                   "(device unplugged?)");
        *status = -1;
        return 0;
    } else {
        // unknown response code. should be fatal
        // this should be fatal
        jack_error("JackFFADODriver::ffado_driver_wait - unexpected error "
                   "code '%d' returned from 'ffado_streaming_wait'", response);
        *status = -1;
        return 0;
    }

    fBeginDateUst = wait_ret;

    printExit();
    return driver->period_size;
}

int
JackFFADODriver::ffado_driver_start (ffado_driver_t *driver)
{
    int retval = 0;

    if ((retval = ffado_streaming_start(driver->dev))) {
        printError("Could not start streaming threads");

        return retval;
    }
    return 0;
}

int
JackFFADODriver::ffado_driver_stop (ffado_driver_t *driver)
{
    int retval = 0;

    if ((retval = ffado_streaming_stop(driver->dev))) {
        printError("Could not stop streaming threads");
        return retval;
    }

    return 0;
}

int
JackFFADODriver::ffado_driver_restart (ffado_driver_t *driver)
{
    if (Stop())
        return -1;
    return Start();
}

void
JackFFADODriver::UpdateLatencies(void)
{
    jack_latency_range_t range;
    ffado_driver_t* driver = (ffado_driver_t*)fDriver;

    for (int i = 0; i < fCaptureChannels; i++) {
        range.min = range.max = driver->period_size + driver->capture_frame_latency;
        fGraphManager->GetPort(fCapturePortList[i])->SetLatencyRange(JackCaptureLatency, &range);
    }

    for (int i = 0; i < fPlaybackChannels; i++) {
        // Add one buffer more latency if "async" mode is used...
        range.min = range.max = (driver->period_size *
			(driver->device_options.nb_buffers - 1)) +
                         ((fEngineControl->fSyncMode) ? 0 : fEngineControl->fBufferSize) + driver->playback_frame_latency;
        fGraphManager->GetPort(fPlaybackPortList[i])->SetLatencyRange(JackPlaybackLatency, &range);
        // Monitor port
        if (fWithMonitorPorts) {
            range.min = range.max =driver->period_size;
            fGraphManager->GetPort(fMonitorPortList[i])->SetLatencyRange(JackCaptureLatency, &range);
        }
    }
}

int
JackFFADODriver::SetBufferSize (jack_nframes_t nframes)
{
    ffado_driver_t* driver = (ffado_driver_t*)fDriver;
    signed int chn;

    // The speed of this function isn't critical; we can afford the
    // time to check the FFADO API version.
    if (ffado_get_api_version() < FIREWIRE_REQUIRED_FFADO_API_VERSION_FOR_SETBUFSIZE ||
		    ffado_streaming_set_period_size == NULL) {
	    printError("unsupported on current version of FFADO; please upgrade FFADO");
	    return -1;
    }

    driver->period_size = nframes;
    driver->period_usecs =
            (jack_time_t) floor ((((float) nframes) / driver->sample_rate)
                                 * 1000000.0f);


    // Reallocate the null and scratch buffers.
    driver->nullbuffer = (ffado_sample_t*) calloc(driver->period_size, sizeof(ffado_sample_t));
    if(driver->nullbuffer == NULL) {
	    printError("could not allocate memory for null buffer");
	    return -1;
    }
    driver->scratchbuffer = (ffado_sample_t*) calloc(driver->period_size, sizeof(ffado_sample_t));
    if(driver->scratchbuffer == NULL) {
	    printError("could not allocate memory for scratch buffer");
	    return -1;
    }

    // MIDI buffers need reallocating
    for (chn = 0; chn < driver->capture_nchannels; chn++) {
	    if(driver->capture_channels[chn].stream_type == ffado_stream_type_midi) {
		    // setup the midi buffer
		    if (driver->capture_channels[chn].midi_buffer != NULL)
			    free(driver->capture_channels[chn].midi_buffer);
		    driver->capture_channels[chn].midi_buffer = (ffado_sample_t*) calloc(driver->period_size, sizeof(uint32_t));
	    }
    }
    for (chn = 0; chn < driver->playback_nchannels; chn++) {
	    if(driver->playback_channels[chn].stream_type == ffado_stream_type_midi) {
		    if (driver->playback_channels[chn].midi_buffer != NULL)
			    free(driver->playback_channels[chn].midi_buffer);
		    driver->playback_channels[chn].midi_buffer = (ffado_sample_t*) calloc(driver->period_size, sizeof(uint32_t));
	    }
    }

    // Notify FFADO of the period size change
    if (ffado_streaming_set_period_size(driver->dev, nframes) != 0) {
	    printError("could not alter FFADO device period size");
	    return -1;
    }

    // This is needed to give the shadow variables a chance to
    // properly update to the changes.
    sleep(1);

    /* tell the engine to change its buffer size */
    JackAudioDriver::SetBufferSize(nframes);  // Generic change, never fails

    UpdateLatencies();

    return 0;
}

typedef void (*JackDriverFinishFunction) (jack_driver_t *);

ffado_driver_t *
JackFFADODriver::ffado_driver_new (const char *name,
                                   ffado_jack_settings_t *params)
{
    ffado_driver_t *driver;

    assert(params);

    if (ffado_get_api_version() < FIREWIRE_REQUIRED_FFADO_API_VERSION) {
        printError("Incompatible libffado version! (%s)", ffado_get_version());
        return NULL;
    }

    printMessage("Starting FFADO backend (%s)", ffado_get_version());

    driver = (ffado_driver_t*)calloc (1, sizeof (ffado_driver_t));

    /* Setup the jack interfaces */
    jack_driver_nt_init ((jack_driver_nt_t *) driver);

    /*        driver->nt_attach    = (JackDriverNTAttachFunction)   ffado_driver_attach;
            driver->nt_detach    = (JackDriverNTDetachFunction)   ffado_driver_detach;
            driver->nt_start     = (JackDriverNTStartFunction)    ffado_driver_start;
            driver->nt_stop      = (JackDriverNTStopFunction)     ffado_driver_stop;
            driver->nt_run_cycle = (JackDriverNTRunCycleFunction) ffado_driver_run_cycle;
            driver->null_cycle   = (JackDriverNullCycleFunction)  ffado_driver_null_cycle;
            driver->write        = (JackDriverReadFunction)       ffado_driver_write;
            driver->read         = (JackDriverReadFunction)       ffado_driver_read;
            driver->nt_bufsize   = (JackDriverNTBufSizeFunction)  ffado_driver_bufsize;
            */

    /* copy command line parameter contents to the driver structure */
    memcpy(&driver->settings, params, sizeof(ffado_jack_settings_t));

    /* prepare all parameters */
    driver->sample_rate = params->sample_rate;
    driver->period_size = params->period_size;
    fBeginDateUst = 0;

    driver->period_usecs =
        (jack_time_t) floor ((((float) driver->period_size) * 1000000.0f) / driver->sample_rate);

//         driver->client = client;
    driver->engine = NULL;

    //from jack1 ffado_driver.c: put arg -dxxx to ffado device_info_t struct
    driver->device_info.nb_device_spec_strings=1;
    driver->device_info.device_spec_strings=(char**)calloc(1, sizeof(char *));
    driver->device_info.device_spec_strings[0]=strdup(params->device_info);

    memset(&driver->device_options, 0, sizeof(driver->device_options));
    driver->device_options.sample_rate = params->sample_rate;
    driver->device_options.period_size = params->period_size;
    driver->device_options.nb_buffers = params->buffer_size;
    driver->device_options.verbose = params->verbose_level;
    driver->capture_frame_latency = params->capture_frame_latency;
    driver->playback_frame_latency = params->playback_frame_latency;
    driver->device_options.snoop_mode = params->snoop_mode;

    debugPrint(DEBUG_LEVEL_STARTUP, " Driver compiled on %s %s", __DATE__, __TIME__);
    debugPrint(DEBUG_LEVEL_STARTUP, " Created driver %s", name);
    debugPrint(DEBUG_LEVEL_STARTUP, "            period_size:   %d", driver->device_options.period_size);
    debugPrint(DEBUG_LEVEL_STARTUP, "            period_usecs:  %d", driver->period_usecs);
    debugPrint(DEBUG_LEVEL_STARTUP, "            sample rate:   %d", driver->device_options.sample_rate);
    debugPrint(DEBUG_LEVEL_STARTUP, "            verbose level: %d", driver->device_options.verbose);

    return (ffado_driver_t *) driver;
}

void
JackFFADODriver::ffado_driver_delete (ffado_driver_t *driver)
{
    free (driver);
}

int JackFFADODriver::Attach()
{
    JackPort* port;
    jack_port_id_t port_index;
    char buf[REAL_JACK_PORT_NAME_SIZE];
    char portname[REAL_JACK_PORT_NAME_SIZE];

    ffado_driver_t* driver = (ffado_driver_t*)fDriver;

    jack_log("JackFFADODriver::Attach fBufferSize %ld fSampleRate %ld", fEngineControl->fBufferSize, fEngineControl->fSampleRate);

    g_verbose = (fEngineControl->fVerbose ? 1 : 0);

    /* preallocate some buffers such that they don't have to be allocated
       in RT context (or from the stack)
     */
    /* the null buffer is a buffer that contains one period of silence */
    driver->nullbuffer = (ffado_sample_t *)calloc(driver->period_size, sizeof(ffado_sample_t));
    if (driver->nullbuffer == NULL) {
        printError("could not allocate memory for null buffer");
        return -1;
    }
    /* calloc should do this, but it can't hurt to be sure */
    memset(driver->nullbuffer, 0, driver->period_size*sizeof(ffado_sample_t));

    /* the scratch buffer is a buffer of one period that can be used as dummy memory */
    driver->scratchbuffer = (ffado_sample_t *)calloc(driver->period_size, sizeof(ffado_sample_t));
    if (driver->scratchbuffer == NULL) {
        printError("could not allocate memory for scratch buffer");
        return -1;
    }

    /* packetizer thread options */
    driver->device_options.realtime = (fEngineControl->fRealTime ? 1 : 0);

    driver->device_options.packetizer_priority = fEngineControl->fServerPriority +
            FFADO_RT_PRIORITY_PACKETIZER_RELATIVE;
    if (driver->device_options.packetizer_priority > 98) {
        driver->device_options.packetizer_priority = 98;
    }

    // initialize the thread
    driver->dev = ffado_streaming_init(driver->device_info, driver->device_options);

    if (!driver->dev) {
        printError("FFADO: Error creating virtual device");
        return -1;
    }

    if (driver->device_options.realtime) {
        printMessage("Streaming thread running with Realtime scheduling, priority %d",
                     driver->device_options.packetizer_priority);
    } else {
        printMessage("Streaming thread running without Realtime scheduling");
    }

    ffado_streaming_set_audio_datatype(driver->dev, ffado_audio_datatype_float);

    /* ports */

    // capture
    driver->capture_nchannels = ffado_streaming_get_nb_capture_streams(driver->dev);
    driver->capture_channels = (ffado_capture_channel_t *)calloc(driver->capture_nchannels, sizeof(ffado_capture_channel_t));
    if (driver->capture_channels == NULL) {
        printError("could not allocate memory for capture channel list");
        return -1;
    }

    fCaptureChannels = 0;
    for (channel_t chn = 0; chn < driver->capture_nchannels; chn++) {
        ffado_streaming_get_capture_stream_name(driver->dev, chn, portname, sizeof(portname));

        driver->capture_channels[chn].stream_type = ffado_streaming_get_capture_stream_type(driver->dev, chn);
        if (driver->capture_channels[chn].stream_type == ffado_stream_type_audio) {
            snprintf(buf, sizeof(buf), "firewire_pcm:%s_in", portname);
            printMessage ("Registering audio capture port %s", buf);
            if (fEngine->PortRegister(fClientControl.fRefNum, buf,
                              JACK_DEFAULT_AUDIO_TYPE,
                              CaptureDriverFlags,
                              fEngineControl->fBufferSize, &port_index) < 0) {
                jack_error("driver: cannot register port for %s", buf);
                return -1;
            }

            // setup port parameters
            if (ffado_streaming_set_capture_stream_buffer(driver->dev, chn, NULL)) {
                printError(" cannot configure initial port buffer for %s", buf);
            }
            ffado_streaming_capture_stream_onoff(driver->dev, chn, 0);

            port = fGraphManager->GetPort(port_index);
            // capture port aliases (jackd1 style port names)
            snprintf(buf, sizeof(buf), "%s:capture_%i", fClientControl.fName, (int) chn + 1);
            port->SetAlias(buf);
            fCapturePortList[chn] = port_index;
            jack_log("JackFFADODriver::Attach fCapturePortList[i] %ld ", port_index);
            fCaptureChannels++;
        } else if (driver->capture_channels[chn].stream_type == ffado_stream_type_midi) {
            snprintf(buf, sizeof(buf), "firewire_pcm:%s_in", portname);
            printMessage ("Registering midi capture port %s", buf);
            if (fEngine->PortRegister(fClientControl.fRefNum, buf,
                              JACK_DEFAULT_MIDI_TYPE,
                              CaptureDriverFlags,
                              fEngineControl->fBufferSize, &port_index) < 0) {
                jack_error("driver: cannot register port for %s", buf);
                return -1;
            }

            // setup port parameters
            if (ffado_streaming_set_capture_stream_buffer(driver->dev, chn, NULL)) {
                printError(" cannot configure initial port buffer for %s", buf);
            }
            if (ffado_streaming_capture_stream_onoff(driver->dev, chn, 0)) {
                printError(" cannot enable port %s", buf);
            }

            driver->capture_channels[chn].midi_input = new JackFFADOMidiInputPort();
            // setup the midi buffer
            driver->capture_channels[chn].midi_buffer = (uint32_t *)calloc(driver->period_size, sizeof(uint32_t));

            fCapturePortList[chn] = port_index;
            jack_log("JackFFADODriver::Attach fCapturePortList[i] %ld ", port_index);
            fCaptureChannels++;
        } else {
            printMessage ("Don't register capture port %s", portname);
        }
    }

    // playback
    driver->playback_nchannels = ffado_streaming_get_nb_playback_streams(driver->dev);
    driver->playback_channels = (ffado_playback_channel_t *)calloc(driver->playback_nchannels, sizeof(ffado_playback_channel_t));
    if (driver->playback_channels == NULL) {
        printError("could not allocate memory for playback channel list");
        return -1;
    }

    fPlaybackChannels = 0;
    for (channel_t chn = 0; chn < driver->playback_nchannels; chn++) {
        ffado_streaming_get_playback_stream_name(driver->dev, chn, portname, sizeof(portname));

        driver->playback_channels[chn].stream_type = ffado_streaming_get_playback_stream_type(driver->dev, chn);

        if (driver->playback_channels[chn].stream_type == ffado_stream_type_audio) {
            snprintf(buf, sizeof(buf), "firewire_pcm:%s_out", portname);
            printMessage ("Registering audio playback port %s", buf);
            if (fEngine->PortRegister(fClientControl.fRefNum, buf,
                              JACK_DEFAULT_AUDIO_TYPE,
                              PlaybackDriverFlags,
                              fEngineControl->fBufferSize, &port_index) < 0) {
                jack_error("driver: cannot register port for %s", buf);
                return -1;
            }

            // setup port parameters
            if (ffado_streaming_set_playback_stream_buffer(driver->dev, chn, NULL)) {
                printError(" cannot configure initial port buffer for %s", buf);
            }
            if (ffado_streaming_playback_stream_onoff(driver->dev, chn, 0)) {
                printError(" cannot enable port %s", buf);
            }

            port = fGraphManager->GetPort(port_index);
            // Add one buffer more latency if "async" mode is used...
            // playback port aliases (jackd1 style port names)
            snprintf(buf, sizeof(buf), "%s:playback_%i", fClientControl.fName, (int) chn + 1);
            port->SetAlias(buf);
            fPlaybackPortList[chn] = port_index;
            jack_log("JackFFADODriver::Attach fPlaybackPortList[i] %ld ", port_index);
            fPlaybackChannels++;
        } else if (driver->playback_channels[chn].stream_type == ffado_stream_type_midi) {
            snprintf(buf, sizeof(buf), "firewire_pcm:%s_out", portname);
            printMessage ("Registering midi playback port %s", buf);

            if (fEngine->PortRegister(fClientControl.fRefNum, buf,
                              JACK_DEFAULT_MIDI_TYPE,
                              PlaybackDriverFlags,
                              fEngineControl->fBufferSize, &port_index) < 0) {
                jack_error("driver: cannot register port for %s", buf);
                return -1;
            }

            // setup port parameters
            if (ffado_streaming_set_playback_stream_buffer(driver->dev, chn, NULL)) {
                printError(" cannot configure initial port buffer for %s", buf);
            }
            if (ffado_streaming_playback_stream_onoff(driver->dev, chn, 0)) {
                printError(" cannot enable port %s", buf);
            }
            // setup the midi buffer

            // This constructor optionally accepts arguments for the
            // non-realtime buffer size and the realtime buffer size.  Ideally,
            // these would become command-line options for the FFADO driver.
            driver->playback_channels[chn].midi_output = new JackFFADOMidiOutputPort();

            driver->playback_channels[chn].midi_buffer = (uint32_t *)calloc(driver->period_size, sizeof(uint32_t));

            fPlaybackPortList[chn] = port_index;
            jack_log("JackFFADODriver::Attach fPlaybackPortList[i] %ld ", port_index);
            fPlaybackChannels++;
        } else {
            printMessage ("Don't register playback port %s", portname);
        }
    }

    UpdateLatencies();

    assert(fCaptureChannels < DRIVER_PORT_NUM);
    assert(fPlaybackChannels < DRIVER_PORT_NUM);

    if (ffado_streaming_prepare(driver->dev)) {
        printError("Could not prepare streaming device!");
        return -1;
    }

    // this makes no sense...
    assert(fCaptureChannels + fPlaybackChannels > 0);
    return 0;
}

int JackFFADODriver::Detach()
{
    channel_t chn;
    ffado_driver_t* driver = (ffado_driver_t*)fDriver;
    jack_log("JackFFADODriver::Detach");

    // finish the libfreebob streaming
    ffado_streaming_finish(driver->dev);
    driver->dev = NULL;

    // free all internal buffers
    for (chn = 0; chn < driver->capture_nchannels; chn++) {
        if (driver->capture_channels[chn].midi_buffer)
            free(driver->capture_channels[chn].midi_buffer);
        if (driver->capture_channels[chn].midi_input)
            delete ((JackFFADOMidiInputPort *) (driver->capture_channels[chn].midi_input));
    }
    free(driver->capture_channels);

    for (chn = 0; chn < driver->playback_nchannels; chn++) {
        if (driver->playback_channels[chn].midi_buffer)
            free(driver->playback_channels[chn].midi_buffer);
        if (driver->playback_channels[chn].midi_output)
            delete ((JackFFADOMidiOutputPort *) (driver->playback_channels[chn].midi_output));
    }
    free(driver->playback_channels);

    free(driver->nullbuffer);
    free(driver->scratchbuffer);

    return JackAudioDriver::Detach();  // Generic JackAudioDriver Detach
}

int JackFFADODriver::Open(ffado_jack_settings_t *params)
{
    // Generic JackAudioDriver Open
    if (JackAudioDriver::Open(
                params->period_size, params->sample_rate,
                params->playback_ports, params->playback_ports,
                0, 0, 0, "", "",
                params->capture_frame_latency, params->playback_frame_latency) != 0) {
        return -1;
    }

    fDriver = (jack_driver_t *)ffado_driver_new ("ffado_pcm", params);

    if (fDriver) {
        // FFADO driver may have changed the in/out values
        //fCaptureChannels = ((ffado_driver_t *)fDriver)->capture_nchannels_audio;
        //fPlaybackChannels = ((ffado_driver_t *)fDriver)->playback_nchannels_audio;
        return 0;
    } else {
        JackAudioDriver::Close();
        return -1;
    }
}

int JackFFADODriver::Close()
{
    // Generic audio driver close
    int res = JackAudioDriver::Close();

    ffado_driver_delete((ffado_driver_t*)fDriver);
    return res;
}

int JackFFADODriver::Start()
{
    int res = JackAudioDriver::Start();
    if (res >= 0) {
        res = ffado_driver_start((ffado_driver_t *)fDriver);
        if (res < 0) {
            JackAudioDriver::Stop();
        }
    }
    return res;
}

int JackFFADODriver::Stop()
{
    int res = ffado_driver_stop((ffado_driver_t *)fDriver);
    if (JackAudioDriver::Stop() < 0) {
        res = -1;
    }
    return res;
}

int JackFFADODriver::Read()
{
    printEnter();

    /* Taken from ffado_driver_run_cycle */
    ffado_driver_t* driver = (ffado_driver_t*)fDriver;
    int wait_status = 0;
    fDelayedUsecs = 0.f;

retry:

    jack_nframes_t nframes = ffado_driver_wait(driver, -1, &wait_status,
                             &fDelayedUsecs);

    if ((wait_status < 0)) {
        printError( "wait status < 0! (= %d)", wait_status);
        return -1;
    }

    if (nframes == 0) {
        /* we detected an xrun and restarted: notify
         * clients about the delay.
         */
        jack_log("FFADO XRun");
        NotifyXRun(fBeginDateUst, fDelayedUsecs);
        goto retry; /* recoverable error*/
    }

    if (nframes != fEngineControl->fBufferSize)
        jack_log("JackFFADODriver::Read warning nframes = %ld", nframes);

    // Has to be done before read
    JackDriver::CycleIncTime();

    printExit();
    return ffado_driver_read((ffado_driver_t *)fDriver, fEngineControl->fBufferSize);
}

int JackFFADODriver::Write()
{
    printEnter();
    int res = ffado_driver_write((ffado_driver_t *)fDriver, fEngineControl->fBufferSize);
    printExit();
    return res;
}

void
JackFFADODriver::jack_driver_init (jack_driver_t *driver)
{
    memset (driver, 0, sizeof (*driver));

    driver->attach = 0;
    driver->detach = 0;
    driver->write = 0;
    driver->read = 0;
    driver->null_cycle = 0;
    driver->bufsize = 0;
    driver->start = 0;
    driver->stop = 0;
}

void
JackFFADODriver::jack_driver_nt_init (jack_driver_nt_t * driver)
{
    memset (driver, 0, sizeof (*driver));

    jack_driver_init ((jack_driver_t *) driver);

    driver->attach = 0;
    driver->detach = 0;
    driver->bufsize = 0;
    driver->stop = 0;
    driver->start = 0;

    driver->nt_bufsize = 0;
    driver->nt_start = 0;
    driver->nt_stop = 0;
    driver->nt_attach = 0;
    driver->nt_detach = 0;
    driver->nt_run_cycle = 0;
}

} // end of namespace


#ifdef __cplusplus
extern "C"
{
#endif

    SERVER_EXPORT const jack_driver_desc_t *
    driver_get_descriptor () {
        jack_driver_desc_t * desc;
        jack_driver_desc_filler_t filler;
        jack_driver_param_value_t value;

        desc = jack_driver_descriptor_construct("firewire", JackDriverMaster, "Linux FFADO API based audio backend", &filler);

        strcpy(value.str, "hw:0");
        jack_driver_descriptor_add_parameter(
            desc,
            &filler,
            "device",
            'd',
            JackDriverParamString,
            &value,
            NULL,
            "The FireWire device to use.",
            "The FireWire device to use. Please consult the FFADO documentation for more info.");

        value.ui = 1024;
        jack_driver_descriptor_add_parameter(desc, &filler, "period", 'p', JackDriverParamUInt, &value, NULL, "Frames per period", NULL);

        value.ui = 3;
        jack_driver_descriptor_add_parameter(desc, &filler, "nperiods", 'n', JackDriverParamUInt, &value, NULL, "Number of periods of playback latency", NULL);

        value.ui = 48000U;
        jack_driver_descriptor_add_parameter(desc, &filler, "rate", 'r', JackDriverParamUInt, &value, NULL, "Sample rate", NULL);

        value.i = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "capture", 'C', JackDriverParamBool, &value, NULL, "Provide capture ports.", NULL);
        jack_driver_descriptor_add_parameter(desc, &filler, "playback", 'P', JackDriverParamBool, &value, NULL, "Provide playback ports.", NULL);

        value.i = 1;
        jack_driver_descriptor_add_parameter(desc, &filler, "duplex", 'D', JackDriverParamBool, &value, NULL, "Provide both capture and playback ports.", NULL);

        value.ui = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "input-latency", 'I', JackDriverParamUInt, &value, NULL, "Extra input latency (frames)", NULL);
        jack_driver_descriptor_add_parameter(desc, &filler, "output-latency", 'O', JackDriverParamUInt, &value, NULL, "Extra output latency (frames)", NULL);

        value.ui = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "inchannels", 'i', JackDriverParamUInt, &value, NULL, "Number of input channels to provide (note: currently ignored)", NULL);
        jack_driver_descriptor_add_parameter(desc, &filler, "outchannels", 'o', JackDriverParamUInt, &value, NULL, "Number of output channels to provide (note: currently ignored)", NULL);

        value.ui = 3;
        jack_driver_descriptor_add_parameter(desc, &filler, "verbose", 'v', JackDriverParamUInt, &value, NULL, "libffado verbose level", NULL);

        value.i = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "snoop", 'X', JackDriverParamBool, &value, NULL, "Snoop firewire traffic", NULL);

        return desc;
    }

    SERVER_EXPORT Jack::JackDriverClientInterface* driver_initialize(Jack::JackLockedEngine* engine, Jack::JackSynchro* table, const JSList* params) {
        const JSList * node;
        const jack_driver_param_t * param;

        ffado_jack_settings_t cmlparams;

        char *device_name=(char*)"hw:0";

        cmlparams.period_size_set = 0;
        cmlparams.sample_rate_set = 0;
        cmlparams.buffer_size_set = 0;

        /* default values */
        cmlparams.period_size = 1024;
        cmlparams.sample_rate = 48000;
        cmlparams.buffer_size = 3;
        cmlparams.playback_ports = 0;
        cmlparams.capture_ports = 0;
        cmlparams.playback_frame_latency = 0;
        cmlparams.capture_frame_latency = 0;

        cmlparams.verbose_level = 0;

        cmlparams.slave_mode = 0;
        cmlparams.snoop_mode = 0;
        cmlparams.device_info = NULL;

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
                case 'X':
                    cmlparams.snoop_mode = param->value.i;
                    break;
                case 'v':
                    cmlparams.verbose_level = param->value.ui;
            }
        }

        /* duplex is the default */
        if (!cmlparams.playback_ports && !cmlparams.capture_ports) {
            cmlparams.playback_ports = 1;
            cmlparams.capture_ports = 1;
        }

        // temporary
        cmlparams.device_info = device_name;

        Jack::JackFFADODriver* ffado_driver = new Jack::JackFFADODriver("system", "firewire_pcm", engine, table);
        Jack::JackDriverClientInterface* threaded_driver = new Jack::JackThreadedDriver(ffado_driver);
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


