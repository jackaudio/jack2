/*
Copyright (C) 2001 Paul Davis
Copyright (C) 2004 Grame
Copyright (C) 2007 Pieter Palmers

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

#include "JackFreebobDriver.h"
#include "JackEngineControl.h"
#include "JackClientControl.h"
#include "JackPort.h"
#include "JackGraphManager.h"
#include "JackLockedEngine.h"

namespace Jack
{

#define jack_get_microseconds GetMicroSeconds

#define SAMPLE_MAX_24BIT  8388608.0f
#define SAMPLE_MAX_16BIT  32768.0f

int
JackFreebobDriver::freebob_driver_read (freebob_driver_t * driver, jack_nframes_t nframes)
{
    jack_default_audio_sample_t* buf = NULL;
    freebob_sample_t nullbuffer[nframes];
    void *addr_of_nullbuffer = (void *)nullbuffer;

    freebob_streaming_stream_type stream_type;

    printEnter();

    // make sure all buffers have a valid buffer if not connected
    for (unsigned int i = 0; i < driver->capture_nchannels; i++) {
        stream_type = freebob_streaming_get_playback_stream_type(driver->dev, i);
        if (stream_type == freebob_stream_type_audio) {
            freebob_streaming_set_playback_stream_buffer(driver->dev, i,
                    (char *)(nullbuffer), freebob_buffer_type_float);
        } else if (stream_type == freebob_stream_type_midi) {
            // these should be read/written with the per-stream functions
        } else {	// empty other buffers without doing something with them
            freebob_streaming_set_playback_stream_buffer(driver->dev, i,
                    (char *)(nullbuffer), freebob_buffer_type_uint24);
        }
    }

    for (int i = 0; i < fCaptureChannels; i++) {
        stream_type = freebob_streaming_get_capture_stream_type(driver->dev, i);
        if (stream_type == freebob_stream_type_audio) {

            if (fGraphManager->GetConnectionsNum(fCapturePortList[i]) > 0) {
                buf = (jack_default_audio_sample_t*)fGraphManager->GetBuffer(fCapturePortList[i],  nframes);

                if (!buf) {
                    buf = (jack_default_audio_sample_t *)addr_of_nullbuffer;
                }

                freebob_streaming_set_capture_stream_buffer(driver->dev, i, (char *)(buf), freebob_buffer_type_float);
            }
        } else if (stream_type == freebob_stream_type_midi) {
            // these should be read/written with the per-stream functions
        } else {	// empty other buffers without doing something with them
            freebob_streaming_set_capture_stream_buffer(driver->dev, i, (char *)(nullbuffer), freebob_buffer_type_uint24);
        }

    }

    // now transfer the buffers
    freebob_streaming_transfer_capture_buffers(driver->dev);
    printExit();
    return 0;
}

int
JackFreebobDriver::freebob_driver_write (freebob_driver_t * driver, jack_nframes_t nframes)
{
    jack_default_audio_sample_t* buf = NULL;

    freebob_streaming_stream_type stream_type;

    freebob_sample_t nullbuffer[nframes];
    void *addr_of_nullbuffer = (void*)nullbuffer;

    memset(&nullbuffer, 0, nframes*sizeof(freebob_sample_t));

    printEnter();
    driver->process_count++;
    assert(driver->dev);

    // make sure all buffers output silence if not connected
    for (unsigned int i = 0; i < driver->playback_nchannels; i++) {
        stream_type = freebob_streaming_get_playback_stream_type(driver->dev, i);
        if (stream_type == freebob_stream_type_audio) {
            freebob_streaming_set_playback_stream_buffer(driver->dev, i,
                    (char *)(nullbuffer), freebob_buffer_type_float);
        } else if (stream_type == freebob_stream_type_midi) {
            // these should be read/written with the per-stream functions
        } else { // empty other buffers without doing something with them
            freebob_streaming_set_playback_stream_buffer(driver->dev, i,
                    (char *)(nullbuffer), freebob_buffer_type_uint24);
        }
    }

    for (int i = 0; i < fPlaybackChannels; i++) {
        stream_type = freebob_streaming_get_playback_stream_type(driver->dev, i);
        if (stream_type == freebob_stream_type_audio) {
            // Ouput ports
            if (fGraphManager->GetConnectionsNum(fPlaybackPortList[i]) > 0) {
                buf = (jack_default_audio_sample_t*)fGraphManager->GetBuffer(fPlaybackPortList[i], nframes);
                if (!buf) {
                    buf = (jack_default_audio_sample_t *)addr_of_nullbuffer;
                }
                freebob_streaming_set_playback_stream_buffer(driver->dev, i, (char *)(buf), freebob_buffer_type_float);
            }
        }
    }

    freebob_streaming_transfer_playback_buffers(driver->dev);
    printExit();
    return 0;
}

jack_nframes_t
JackFreebobDriver::freebob_driver_wait (freebob_driver_t *driver, int extra_fd, int *status,
                                        float *delayed_usecs)
{
    int nframes;
    jack_time_t wait_enter;
    jack_time_t wait_ret;

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

    nframes = freebob_streaming_wait(driver->dev);

    wait_ret = jack_get_microseconds ();

    if (driver->wait_next && wait_ret > driver->wait_next) {
        *delayed_usecs = wait_ret - driver->wait_next;
    }
    driver->wait_last = wait_ret;
    driver->wait_next = wait_ret + driver->period_usecs;
// 	driver->engine->transport_cycle_start (driver->engine, wait_ret);

    if (nframes < 0) {
        *status = 0;
        return 0;
    }

    *status = 0;
    fBeginDateUst = wait_ret;

    // FIXME: this should do something more usefull
    *delayed_usecs = 0;
    printExit();
    return nframes - nframes % driver->period_size;
}

int
JackFreebobDriver::freebob_driver_start (freebob_driver_t *driver)
{
    int retval = 0;

#ifdef FREEBOB_DRIVER_WITH_MIDI
    if (driver->midi_handle) {
        if ((retval = freebob_driver_midi_start(driver->midi_handle))) {
            printError("Could not start MIDI threads");
            return retval;
        }
    }
#endif

    if ((retval = freebob_streaming_start(driver->dev))) {
        printError("Could not start streaming threads");
#ifdef FREEBOB_DRIVER_WITH_MIDI
        if (driver->midi_handle) {
            freebob_driver_midi_stop(driver->midi_handle);
        }
#endif
        return retval;
    }

    return 0;
}

int
JackFreebobDriver::freebob_driver_stop (freebob_driver_t *driver)
{
    int retval = 0;

#ifdef FREEBOB_DRIVER_WITH_MIDI
    if (driver->midi_handle) {
        if ((retval = freebob_driver_midi_stop(driver->midi_handle))) {
            printError("Could not stop MIDI threads");
            return retval;
        }
    }
#endif
    if ((retval = freebob_streaming_stop(driver->dev))) {
        printError("Could not stop streaming threads");
        return retval;
    }

    return 0;
}

int
JackFreebobDriver::freebob_driver_restart (freebob_driver_t *driver)
{
    if (Stop())
        return -1;
    return Start();
}

int
JackFreebobDriver::SetBufferSize (jack_nframes_t nframes)
{
    printError("Buffer size change requested but not supported!!!");

    /*
    driver->period_size = nframes;
    driver->period_usecs =
    	(jack_time_t) floor ((((float) nframes) / driver->sample_rate)
    			     * 1000000.0f);
    */

    /* tell the engine to change its buffer size */
    //driver->engine->set_buffer_size (driver->engine, nframes);

    return -1; // unsupported
}

typedef void (*JackDriverFinishFunction) (jack_driver_t *);

freebob_driver_t *
JackFreebobDriver::freebob_driver_new (char *name,
                                       freebob_jack_settings_t *params)
{
    freebob_driver_t *driver;

    assert(params);

    if (freebob_get_api_version() != 1) {
        printMessage("Incompatible libfreebob version! (%s)", freebob_get_version());
        return NULL;
    }

    printMessage("Starting Freebob backend (%s)", freebob_get_version());

    driver = (freebob_driver_t*)calloc (1, sizeof (freebob_driver_t));

    /* Setup the jack interfaces */
    jack_driver_nt_init ((jack_driver_nt_t *) driver);

    /*	driver->nt_attach    = (JackDriverNTAttachFunction)   freebob_driver_attach;
    	driver->nt_detach    = (JackDriverNTDetachFunction)   freebob_driver_detach;
    	driver->nt_start     = (JackDriverNTStartFunction)    freebob_driver_start;
    	driver->nt_stop      = (JackDriverNTStopFunction)     freebob_driver_stop;
    	driver->nt_run_cycle = (JackDriverNTRunCycleFunction) freebob_driver_run_cycle;
    	driver->null_cycle   = (JackDriverNullCycleFunction)  freebob_driver_null_cycle;
    	driver->write        = (JackDriverReadFunction)       freebob_driver_write;
    	driver->read         = (JackDriverReadFunction)       freebob_driver_read;
    	driver->nt_bufsize   = (JackDriverNTBufSizeFunction)  freebob_driver_bufsize;
    */

    /* copy command line parameter contents to the driver structure */
    memcpy(&driver->settings, params, sizeof(freebob_jack_settings_t));

    /* prepare all parameters */
    driver->sample_rate = params->sample_rate;
    driver->period_size = params->period_size;
    fBeginDateUst = 0;

    driver->period_usecs =
        (jack_time_t) floor ((((float) driver->period_size) * 1000000.0f) / driver->sample_rate);

// 	driver->client = client;
    driver->engine = NULL;

    memset(&driver->device_options, 0, sizeof(driver->device_options));
    driver->device_options.sample_rate = params->sample_rate;
    driver->device_options.period_size = params->period_size;
    driver->device_options.nb_buffers = params->buffer_size;
    driver->device_options.node_id = params->node_id;
    driver->device_options.port = params->port;
    driver->capture_frame_latency = params->capture_frame_latency;
    driver->playback_frame_latency = params->playback_frame_latency;

    if (!params->capture_ports) {
        driver->device_options.directions |= FREEBOB_IGNORE_CAPTURE;
    }

    if (!params->playback_ports) {
        driver->device_options.directions |= FREEBOB_IGNORE_PLAYBACK;
    }

    debugPrint(DEBUG_LEVEL_STARTUP, " Driver compiled on %s %s", __DATE__, __TIME__);
    debugPrint(DEBUG_LEVEL_STARTUP, " Created driver %s", name);
    debugPrint(DEBUG_LEVEL_STARTUP, "            period_size: %d", driver->period_size);
    debugPrint(DEBUG_LEVEL_STARTUP, "            period_usecs: %d", driver->period_usecs);
    debugPrint(DEBUG_LEVEL_STARTUP, "            sample rate: %d", driver->sample_rate);

    return (freebob_driver_t *) driver;
}

void
JackFreebobDriver::freebob_driver_delete (freebob_driver_t *driver)
{
    free (driver);
}

#ifdef FREEBOB_DRIVER_WITH_MIDI
/*
 * MIDI support
 */

// the thread that will queue the midi events from the seq to the stream buffers

void *
JackFreebobDriver::freebob_driver_midi_queue_thread(void *arg)
{
    freebob_driver_midi_handle_t *m = (freebob_driver_midi_handle_t *)arg;
    assert(m);
    snd_seq_event_t *ev;
    unsigned char work_buffer[MIDI_TRANSMIT_BUFFER_SIZE];
    int bytes_to_send;
    int b;
    int i;

    printMessage("MIDI queue thread started");

    while (1) {
        // get next event, if one is present
        while ((snd_seq_event_input(m->seq_handle, &ev) > 0)) {
            // get the port this event is originated from
            freebob_midi_port_t *port = NULL;
            for (i = 0;i < m->nb_output_ports;i++) {
                if (m->output_ports[i]->seq_port_nr == ev->dest.port) {
                    port = m->output_ports[i];
                    break;
                }
            }

            if (!port) {
                printError(" Could not find target port for event: dst=%d src=%d", ev->dest.port, ev->source.port);
                break;
            }

            // decode it to the work buffer
            if ((bytes_to_send = snd_midi_event_decode ( port->parser,
                                 work_buffer,
                                 MIDI_TRANSMIT_BUFFER_SIZE,
                                 ev)) < 0) { // failed
                printError(" Error decoding event for port %d (errcode=%d)", port->seq_port_nr, bytes_to_send);
                bytes_to_send = 0;
                //return -1;
            }

            for (b = 0;b < bytes_to_send;b++) {
                freebob_sample_t tmp_event = work_buffer[b];
                if (freebob_streaming_write(m->dev, port->stream_nr, &tmp_event, 1) < 1) {
                    printError(" Midi send buffer overrun");
                }
            }
        }

        // sleep for some time
        usleep(MIDI_THREAD_SLEEP_TIME_USECS);
    }
    return NULL;
}

// the dequeue thread (maybe we need one thread per stream)
void *
JackFreebobDriver::freebob_driver_midi_dequeue_thread (void *arg)
{
    freebob_driver_midi_handle_t *m = (freebob_driver_midi_handle_t *)arg;

    int i;
    int s;
    int samples_read;

    assert(m);

    while (1) {
        // read incoming events

        for (i = 0;i < m->nb_input_ports;i++) {
            unsigned int buff[64];
            freebob_midi_port_t *port = m->input_ports[i];
            if (!port) {
                printError(" something went wrong when setting up the midi input port map (%d)", i);
            }

            do {
                samples_read = freebob_streaming_read(m->dev, port->stream_nr, buff, 64);

                for (s = 0;s < samples_read;s++) {
                    unsigned int *byte = (buff + s) ;
                    snd_seq_event_t ev;
                    if ((snd_midi_event_encode_byte(port->parser, (*byte) & 0xFF, &ev)) > 0) {
                        // a midi message is complete, send it out to ALSA
                        snd_seq_ev_set_subs(&ev);
                        snd_seq_ev_set_direct(&ev);
                        snd_seq_ev_set_source(&ev, port->seq_port_nr);
                        snd_seq_event_output_direct(port->seq_handle, &ev);
                    }
                }
            } while (samples_read > 0);
        }

        // sleep for some time
        usleep(MIDI_THREAD_SLEEP_TIME_USECS);
    }
    return NULL;
}

freebob_driver_midi_handle_t *
JackFreebobDriver::freebob_driver_midi_init(freebob_driver_t *driver)
{
    char buf[256];
    channel_t chn;
    int nchannels;
    int i = 0;

    freebob_device_t *dev = driver->dev;
    assert(dev);

    freebob_driver_midi_handle_t *m = calloc(1, sizeof(freebob_driver_midi_handle_t));
    if (!m) {
        printError("not enough memory to create midi structure");
        return NULL;
    }

    if (snd_seq_open(&m->seq_handle, "default", SND_SEQ_OPEN_DUPLEX, SND_SEQ_NONBLOCK) < 0) {
        printError("Error opening ALSA sequencer.");
        free(m);
        return NULL;
    }

    snd_seq_set_client_name(m->seq_handle, "FreeBoB Jack MIDI");

    // find out the number of midi in/out ports we need to setup
    nchannels = freebob_streaming_get_nb_capture_streams(dev);
    m->nb_input_ports = 0;

    for (chn = 0; chn < nchannels; chn++) {
        if (freebob_streaming_get_capture_stream_type(dev, chn) == freebob_stream_type_midi) {
            m->nb_input_ports++;
        }
    }

    m->input_ports = calloc(m->nb_input_ports, sizeof(freebob_midi_port_t *));
    if (!m->input_ports) {
        printError("not enough memory to create midi structure");
        free(m);
        return NULL;
    }

    i = 0;
    for (chn = 0; chn < nchannels; chn++) {
        if (freebob_streaming_get_capture_stream_type(dev, chn) == freebob_stream_type_midi) {
            m->input_ports[i] = calloc(1, sizeof(freebob_midi_port_t));
            if (!m->input_ports[i]) {
                // fixme
                printError("Could not allocate memory for seq port");
                continue;
            }

            freebob_streaming_get_capture_stream_name(dev, chn, buf, sizeof(buf));
            printMessage("Register MIDI IN port %s", buf);

            m->input_ports[i]->seq_port_nr = snd_seq_create_simple_port(m->seq_handle, buf,
                                             SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
                                             SND_SEQ_PORT_TYPE_MIDI_GENERIC);

            if (m->input_ports[i]->seq_port_nr < 0) {
                printError("Could not create seq port");
                m->input_ports[i]->stream_nr = -1;
                m->input_ports[i]->seq_port_nr = -1;
            } else {
                m->input_ports[i]->stream_nr = chn;
                m->input_ports[i]->seq_handle = m->seq_handle;
                if (snd_midi_event_new  ( ALSA_SEQ_BUFF_SIZE, &(m->input_ports[i]->parser)) < 0) {
                    printError("could not init parser for MIDI IN port %d", i);
                    m->input_ports[i]->stream_nr = -1;
                    m->input_ports[i]->seq_port_nr = -1;
                }
            }

            i++;
        }
    }

    // playback
    nchannels = freebob_streaming_get_nb_playback_streams(dev);

    m->nb_output_ports = 0;

    for (chn = 0; chn < nchannels; chn++) {
        if (freebob_streaming_get_playback_stream_type(dev, chn) == freebob_stream_type_midi) {
            m->nb_output_ports++;
        }
    }

    m->output_ports = calloc(m->nb_output_ports, sizeof(freebob_midi_port_t *));
    if (!m->output_ports) {
        printError("not enough memory to create midi structure");
        for (i = 0; i < m->nb_input_ports; i++) {
            free(m->input_ports[i]);
        }
        free(m->input_ports);
        free(m);
        return NULL;
    }

    i = 0;
    for (chn = 0; chn < nchannels; chn++) {
        if (freebob_streaming_get_playback_stream_type(dev, chn) == freebob_stream_type_midi) {
            m->output_ports[i] = calloc(1, sizeof(freebob_midi_port_t));
            if (!m->output_ports[i]) {
                // fixme
                printError("Could not allocate memory for seq port");
                continue;
            }

            freebob_streaming_get_playback_stream_name(dev, chn, buf, sizeof(buf));
            printMessage("Register MIDI OUT port %s", buf);

            m->output_ports[i]->seq_port_nr = snd_seq_create_simple_port(m->seq_handle, buf,
                                              SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
                                              SND_SEQ_PORT_TYPE_MIDI_GENERIC);

            if (m->output_ports[i]->seq_port_nr < 0) {
                printError("Could not create seq port");
                m->output_ports[i]->stream_nr = -1;
                m->output_ports[i]->seq_port_nr = -1;
            } else {
                m->output_ports[i]->stream_nr = chn;
                m->output_ports[i]->seq_handle = m->seq_handle;
                if (snd_midi_event_new  ( ALSA_SEQ_BUFF_SIZE, &(m->output_ports[i]->parser)) < 0) {
                    printError("could not init parser for MIDI OUT port %d", i);
                    m->output_ports[i]->stream_nr = -1;
                    m->output_ports[i]->seq_port_nr = -1;
                }
            }

            i++;
        }
    }

    m->dev = dev;
    m->driver = driver;
    return m;
}

int
JackFreebobDriver::freebob_driver_midi_start (freebob_driver_midi_handle_t *m)
{
    assert(m);
    // start threads

    m->queue_thread_realtime = (m->driver->engine->control->real_time ? 1 : 0);
    m->queue_thread_priority =
        m->driver->engine->control->client_priority +
        FREEBOB_RT_PRIORITY_MIDI_RELATIVE;

    if (m->queue_thread_priority > 98) {
        m->queue_thread_priority = 98;
    }
    if (m->queue_thread_realtime) {
        printMessage("MIDI threads running with Realtime scheduling, priority %d",
                     m->queue_thread_priority);
    } else {
        printMessage("MIDI threads running without Realtime scheduling");
    }

    if (jack_client_create_thread(NULL, &m->queue_thread, m->queue_thread_priority, m->queue_thread_realtime, freebob_driver_midi_queue_thread, (void *)m)) {
        printError(" cannot create midi queueing thread");
        return -1;
    }

    if (jack_client_create_thread(NULL, &m->dequeue_thread, m->queue_thread_priority, m->queue_thread_realtime, freebob_driver_midi_dequeue_thread, (void *)m)) {
        printError(" cannot create midi dequeueing thread");
        return -1;
    }
    return 0;
}

int
JackFreebobDriver::freebob_driver_midi_stop (freebob_driver_midi_handle_t *m)
{
    assert(m);

    pthread_cancel (m->queue_thread);
    pthread_join (m->queue_thread, NULL);

    pthread_cancel (m->dequeue_thread);
    pthread_join (m->dequeue_thread, NULL);
    return 0;
}

void
JackFreebobDriver::freebob_driver_midi_finish (freebob_driver_midi_handle_t *m)
{
    assert(m);

    int i;
    // TODO: add state info here, if not stopped then stop

    for (i = 0;i < m->nb_input_ports;i++) {
        free(m->input_ports[i]);
    }
    free(m->input_ports);

    for (i = 0;i < m->nb_output_ports;i++) {
        free(m->output_ports[i]);
    }
    free(m->output_ports);
    free(m);
}
#endif

int JackFreebobDriver::Attach()
{
    JackPort* port;
    jack_port_id_t port_index;

    char buf[REAL_JACK_PORT_NAME_SIZE];
    char portname[REAL_JACK_PORT_NAME_SIZE];
    jack_latency_range_t range;

    freebob_driver_t* driver = (freebob_driver_t*)fDriver;

    jack_log("JackFreebobDriver::Attach fBufferSize %ld fSampleRate %ld", fEngineControl->fBufferSize, fEngineControl->fSampleRate);

    g_verbose = (fEngineControl->fVerbose ? 1 : 0);
    driver->device_options.verbose = (fEngineControl->fVerbose ? 1 : 0);

    /* packetizer thread options */
    driver->device_options.realtime = (fEngineControl->fRealTime ? 1 : 0);

    driver->device_options.packetizer_priority = fEngineControl->fServerPriority +
            FREEBOB_RT_PRIORITY_PACKETIZER_RELATIVE;
    if (driver->device_options.packetizer_priority > 98) {
        driver->device_options.packetizer_priority = 98;
    }

    // initialize the thread
    driver->dev = freebob_streaming_init(&driver->device_info, driver->device_options);

    if (!driver->dev) {
        printError("FREEBOB: Error creating virtual device");
        return -1;
    }

#ifdef FREEBOB_DRIVER_WITH_MIDI
    driver->midi_handle = freebob_driver_midi_init(driver);
    if (!driver->midi_handle) {
        printError("-----------------------------------------------------------");
        printError("Error creating midi device!");
        printError("FreeBob will run without MIDI support.");
        printError("Consult the above error messages to solve the problem. ");
        printError("-----------------------------------------------------------\n\n");
    }
#endif

    if (driver->device_options.realtime) {
        printMessage("Streaming thread running with Realtime scheduling, priority %d",
                     driver->device_options.packetizer_priority);
    } else {
        printMessage("Streaming thread running without Realtime scheduling");
    }

    /* ports */

    // capture
    driver->capture_nchannels = freebob_streaming_get_nb_capture_streams(driver->dev);
    driver->capture_nchannels_audio = 0;

    for (unsigned int i = 0; i < driver->capture_nchannels; i++) {

        freebob_streaming_get_capture_stream_name(driver->dev, i, portname, sizeof(portname));
        snprintf(buf, sizeof(buf), "%s:%s", fClientControl.fName, portname);

        if (freebob_streaming_get_capture_stream_type(driver->dev, i) != freebob_stream_type_audio) {
            printMessage ("Don't register capture port %s", buf);
        } else {
            printMessage ("Registering capture port %s", buf);

            if (fEngine->PortRegister(fClientControl.fRefNum, buf,
                              JACK_DEFAULT_AUDIO_TYPE,
                              CaptureDriverFlags,
                              fEngineControl->fBufferSize, &port_index) < 0) {
                jack_error("driver: cannot register port for %s", buf);
                return -1;
            }
            port = fGraphManager->GetPort(port_index);
            range.min = range.max = driver->period_size + driver->capture_frame_latency;
            port->SetLatencyRange(JackCaptureLatency, &range);
            fCapturePortList[i] = port_index;
            jack_log("JackFreebobDriver::Attach fCapturePortList[i] %ld ", port_index);
            driver->capture_nchannels_audio++;
        }
    }

    // playback
    driver->playback_nchannels = freebob_streaming_get_nb_playback_streams(driver->dev);
    driver->playback_nchannels_audio = 0;

    for (unsigned int i = 0; i < driver->playback_nchannels; i++) {

        freebob_streaming_get_playback_stream_name(driver->dev, i, portname, sizeof(portname));
        snprintf(buf, sizeof(buf), "%s:%s", fClientControl.fName, portname);

        if (freebob_streaming_get_playback_stream_type(driver->dev, i) != freebob_stream_type_audio) {
            printMessage ("Don't register playback port %s", buf);
        } else {
            printMessage ("Registering playback port %s", buf);
            if (fEngine->PortRegister(fClientControl.fRefNum, buf,
                              JACK_DEFAULT_AUDIO_TYPE,
                              PlaybackDriverFlags,
                              fEngineControl->fBufferSize, &port_index) < 0) {
                jack_error("driver: cannot register port for %s", buf);
                return -1;
            }
            port = fGraphManager->GetPort(port_index);
            // Add one buffer more latency if "async" mode is used...
            range.min = range.max = (driver->period_size * (driver->device_options.nb_buffers - 1)) + ((fEngineControl->fSyncMode) ? 0 : fEngineControl->fBufferSize) + driver->playback_frame_latency;
            port->SetLatencyRange(JackPlaybackLatency, &range);
            fPlaybackPortList[i] = port_index;
            jack_log("JackFreebobDriver::Attach fPlaybackPortList[i] %ld ", port_index);
            driver->playback_nchannels_audio++;
        }
    }

    fCaptureChannels = driver->capture_nchannels_audio;
    fPlaybackChannels = driver->playback_nchannels_audio;

    assert(fCaptureChannels < DRIVER_PORT_NUM);
    assert(fPlaybackChannels < DRIVER_PORT_NUM);

    // this makes no sense...
    assert(fCaptureChannels + fPlaybackChannels > 0);
    return 0;
}

int JackFreebobDriver::Detach()
{
    freebob_driver_t* driver = (freebob_driver_t*)fDriver;
    jack_log("JackFreebobDriver::Detach");

    // finish the libfreebob streaming
    freebob_streaming_finish(driver->dev);
    driver->dev = NULL;

#ifdef FREEBOB_DRIVER_WITH_MIDI
    if (driver->midi_handle) {
        freebob_driver_midi_finish(driver->midi_handle);
    }
    driver->midi_handle = NULL;
#endif

    return JackAudioDriver::Detach();  // Generic JackAudioDriver Detach
}

int JackFreebobDriver::Open(freebob_jack_settings_t *params)
{
    // Generic JackAudioDriver Open
    if (JackAudioDriver::Open(
                params->period_size, params->sample_rate,
                params->playback_ports, params->playback_ports,
                0, 0, 0, "", "",
                params->capture_frame_latency, params->playback_frame_latency) != 0) {
        return -1;
    }

    fDriver = (jack_driver_t *)freebob_driver_new ((char*)"freebob_pcm", params);

    if (fDriver) {
        // FreeBoB driver may have changed the in/out values
        fCaptureChannels = ((freebob_driver_t *)fDriver)->capture_nchannels_audio;
        fPlaybackChannels = ((freebob_driver_t *)fDriver)->playback_nchannels_audio;
        return 0;
    } else {
        JackAudioDriver::Close();
        return -1;
    }
}

int JackFreebobDriver::Close()
{
    // Generic audio driver close
    int res = JackAudioDriver::Close();

    freebob_driver_delete((freebob_driver_t*)fDriver);
    return res;
}

int JackFreebobDriver::Start()
{
    int res = JackAudioDriver::Start();
    if (res >= 0) {
        res = freebob_driver_start((freebob_driver_t *)fDriver);
        if (res < 0) {
            JackAudioDriver::Stop();
        }
    }
    return res;
}

int JackFreebobDriver::Stop()
{
    int res = freebob_driver_stop((freebob_driver_t *)fDriver);
    if (JackAudioDriver::Stop() < 0) {
        res = -1;
    }
    return res;
}

int JackFreebobDriver::Read()
{
    printEnter();

    /* Taken from freebob_driver_run_cycle */
    freebob_driver_t* driver = (freebob_driver_t*)fDriver;
    int wait_status = 0;
    fDelayedUsecs = 0.f;

retry:

    jack_nframes_t nframes = freebob_driver_wait (driver, -1, &wait_status,
                             &fDelayedUsecs);

    if ((wait_status < 0)) {
        printError( "wait status < 0! (= %d)", wait_status);
        return -1;
    }

    if (nframes == 0) {
        /* we detected an xrun and restarted: notify
         * clients about the delay.
         */
        jack_log("FreeBoB XRun");
        NotifyXRun(fBeginDateUst, fDelayedUsecs);
        goto retry; /* recoverable error*/
    }

    if (nframes != fEngineControl->fBufferSize)
        jack_log("JackFreebobDriver::Read warning nframes = %ld", nframes);

    // Has to be done before read
    JackDriver::CycleIncTime();

    printExit();
    return freebob_driver_read((freebob_driver_t *)fDriver, fEngineControl->fBufferSize);
}

int JackFreebobDriver::Write()
{
    printEnter();
    int res = freebob_driver_write((freebob_driver_t *)fDriver, fEngineControl->fBufferSize);
    printExit();
    return res;
}

void
JackFreebobDriver::jack_driver_init (jack_driver_t *driver)
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
JackFreebobDriver::jack_driver_nt_init (jack_driver_nt_t * driver)
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

    const jack_driver_desc_t *
    driver_get_descriptor () {
        jack_driver_desc_t * desc;
        jack_driver_desc_filler_t filler;
        jack_driver_param_value_t value;

        desc = jack_driver_descriptor_construct("freebob", JackDriverMaster, "Linux FreeBob API based audio backend", &filler);

        strcpy(value.str, "hw:0");
        jack_driver_descriptor_add_parameter(desc, &filler, "device", 'd', JackDriverParamString, &value, NULL, "The FireWire device to use. Format is: 'hw:port[,node]'.", NULL);

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

        return desc;
    }

    Jack::JackDriverClientInterface* driver_initialize(Jack::JackLockedEngine* engine, Jack::JackSynchro* table, const JSList* params) {
        unsigned int port = 0;
        unsigned int node_id = -1;
        int nbitems;

        const JSList * node;
        const jack_driver_param_t * param;

        freebob_jack_settings_t cmlparams;

        const char *device_name = "hw:0";

        cmlparams.period_size_set = 0;
        cmlparams.sample_rate_set = 0;
        cmlparams.buffer_size_set = 0;
        cmlparams.port_set = 0;
        cmlparams.node_id_set = 0;

        /* default values */
        cmlparams.period_size = 1024;
        cmlparams.sample_rate = 48000;
        cmlparams.buffer_size = 3;
        cmlparams.port = 0;
        cmlparams.node_id = -1;
        cmlparams.playback_ports = 0;
        cmlparams.capture_ports = 0;
        cmlparams.playback_frame_latency = 0;
        cmlparams.capture_frame_latency = 0;

        for (node = params; node; node = jack_slist_next (node)) {
            param = (jack_driver_param_t *) node->data;

            switch (param->character) {
                case 'd':
                    device_name = param->value.str;
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
                case 'C':
                    cmlparams.capture_ports = 1;
                    break;
                case 'P':
                    cmlparams.playback_ports = 1;
                    break;
                case 'D':
                    cmlparams.capture_ports = 1;
                    cmlparams.playback_ports = 1;
                    break;
                case 'I':
                    cmlparams.capture_frame_latency = param->value.ui;
                    break;
                case 'O':
                    cmlparams.playback_frame_latency = param->value.ui;
                    break;
                    // ignore these for now
                case 'i':
                    break;
                case 'o':
                    break;
            }
        }

        /* duplex is the default */
        if (!cmlparams.playback_ports && !cmlparams.capture_ports) {
            cmlparams.playback_ports = TRUE;
            cmlparams.capture_ports = TRUE;
        }

        nbitems = sscanf(device_name, "hw:%u,%u", &port, &node_id);
        if (nbitems < 2) {
            nbitems = sscanf(device_name, "hw:%u", &port);

            if (nbitems < 1) {
                printError("device (-d) argument not valid\n");
                return NULL;
            } else {
                cmlparams.port = port;
                cmlparams.port_set = 1;

                cmlparams.node_id = -1;
                cmlparams.node_id_set = 0;
            }
        } else {
            cmlparams.port = port;
            cmlparams.port_set = 1;

            cmlparams.node_id = node_id;
            cmlparams.node_id_set = 1;
        }

        jack_error("Freebob using Firewire port %d, node %d", cmlparams.port, cmlparams.node_id);

        Jack::JackFreebobDriver* freebob_driver = new Jack::JackFreebobDriver("system", "freebob_pcm", engine, table);
        Jack::JackDriverClientInterface* threaded_driver = new Jack::JackThreadedDriver(freebob_driver);
        // Special open for FreeBoB driver...
        if (freebob_driver->Open(&cmlparams) == 0) {
            return threaded_driver;
        } else {
            delete threaded_driver; // Delete the decorated driver
            return NULL;
        }
    }

#ifdef __cplusplus
}
#endif


