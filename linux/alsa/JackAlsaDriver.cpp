/*
Copyright (C) 2001 Paul Davis
Copyright (C) 2004 Grame

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

#define __STDC_FORMAT_MACROS   // For inttypes.h to work in C++

#include <iostream>
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
#include <string.h>

#include "JackAlsaDriver.h"
#include "JackEngineControl.h"
#include "JackClientControl.h"
#include "JackPort.h"
#include "JackGraphManager.h"
#include "JackLockedEngine.h"
#ifdef __ANDROID__
#include "JackAndroidThread.h"
#else
#include "JackPosixThread.h"
#endif
#include "JackCompilerDeps.h"
#include "JackServerGlobals.h"

static struct jack_constraint_enum_str_descriptor midi_constraint_descr_array[] =
{
    { "none", "no MIDI driver" },
    { "seq", "ALSA Sequencer driver" },
    { "raw", "ALSA RawMIDI driver" },
    { 0 }
};

static struct jack_constraint_enum_char_descriptor dither_constraint_descr_array[] =
{
    { 'n', "none" },
    { 'r', "rectangular" },
    { 's', "shaped" },
    { 't', "triangular" },
    { 0 }
};

namespace Jack
{

int JackAlsaDriver::SetBufferSize(jack_nframes_t buffer_size)
{
    jack_log("JackAlsaDriver::SetBufferSize %ld", buffer_size);
    int res = alsa_driver_reset_parameters((alsa_driver_t *)fDriver, buffer_size,
                                           ((alsa_driver_t *)fDriver)->user_nperiods,
                                           ((alsa_driver_t *)fDriver)->frame_rate);

    if (res == 0) { // update fEngineControl and fGraphManager
        JackAudioDriver::SetBufferSize(buffer_size);  // Generic change, never fails
        // ALSA specific
        UpdateLatencies();
    } else {
        // Restore old values
        alsa_driver_reset_parameters((alsa_driver_t *)fDriver, fEngineControl->fBufferSize,
                                     ((alsa_driver_t *)fDriver)->user_nperiods,
                                     ((alsa_driver_t *)fDriver)->frame_rate);
    }

    return res;
}

void JackAlsaDriver::UpdateLatencies()
{
    jack_latency_range_t range;
    alsa_driver_t* alsa_driver = (alsa_driver_t*)fDriver;

    for (int i = 0; i < fCaptureChannels; i++) {
        range.min = range.max = alsa_driver->frames_per_cycle + alsa_driver->capture_frame_latency;
        fGraphManager->GetPort(fCapturePortList[i])->SetLatencyRange(JackCaptureLatency, &range);
    }

    for (int i = 0; i < fPlaybackChannels; i++) {
        // Add one buffer more latency if "async" mode is used...
        range.min = range.max = (alsa_driver->frames_per_cycle * (alsa_driver->user_nperiods - 1)) +
                         ((fEngineControl->fSyncMode) ? 0 : fEngineControl->fBufferSize) + alsa_driver->playback_frame_latency;
        fGraphManager->GetPort(fPlaybackPortList[i])->SetLatencyRange(JackPlaybackLatency, &range);
        // Monitor port
        if (fWithMonitorPorts) {
            range.min = range.max = alsa_driver->frames_per_cycle;
            fGraphManager->GetPort(fMonitorPortList[i])->SetLatencyRange(JackCaptureLatency, &range);
        }
    }
}

int JackAlsaDriver::Attach()
{
    JackPort* port;
    jack_port_id_t port_index;
    unsigned long port_flags = (unsigned long)CaptureDriverFlags;
    char name[REAL_JACK_PORT_NAME_SIZE+1];
    char alias[REAL_JACK_PORT_NAME_SIZE+1];

    assert(fCaptureChannels < DRIVER_PORT_NUM);
    assert(fPlaybackChannels < DRIVER_PORT_NUM);

    alsa_driver_t* alsa_driver = (alsa_driver_t*)fDriver;

    if (alsa_driver->has_hw_monitoring)
        port_flags |= JackPortCanMonitor;

    // ALSA driver may have changed the values
    JackAudioDriver::SetBufferSize(alsa_driver->frames_per_cycle);
    JackAudioDriver::SetSampleRate(alsa_driver->frame_rate);

    jack_log("JackAlsaDriver::Attach fBufferSize %ld fSampleRate %ld", fEngineControl->fBufferSize, fEngineControl->fSampleRate);

    for (int i = 0; i < fCaptureChannels; i++) {
        snprintf(alias, sizeof(alias), "%s:%s:out%d", fAliasName, fCaptureDriverName, i + 1);
        snprintf(name, sizeof(name), "%s:capture_%d", fClientControl.fName, i + 1);
        if (fEngine->PortRegister(fClientControl.fRefNum, name, JACK_DEFAULT_AUDIO_TYPE, (JackPortFlags)port_flags, fEngineControl->fBufferSize, &port_index) < 0) {
            jack_error("driver: cannot register port for %s", name);
            return -1;
        }
        port = fGraphManager->GetPort(port_index);
        port->SetAlias(alias);
        fCapturePortList[i] = port_index;
        jack_log("JackAlsaDriver::Attach fCapturePortList[i] %ld ", port_index);
    }

    port_flags = (unsigned long)PlaybackDriverFlags;

    for (int i = 0; i < fPlaybackChannels; i++) {
        snprintf(alias, sizeof(alias), "%s:%s:in%d", fAliasName, fPlaybackDriverName, i + 1);
        snprintf(name, sizeof(name), "%s:playback_%d", fClientControl.fName, i + 1);
        if (fEngine->PortRegister(fClientControl.fRefNum, name, JACK_DEFAULT_AUDIO_TYPE, (JackPortFlags)port_flags, fEngineControl->fBufferSize, &port_index) < 0) {
            jack_error("driver: cannot register port for %s", name);
            return -1;
        }
        port = fGraphManager->GetPort(port_index);
        port->SetAlias(alias);
        fPlaybackPortList[i] = port_index;
        jack_log("JackAlsaDriver::Attach fPlaybackPortList[i] %ld ", port_index);

        // Monitor ports
        if (fWithMonitorPorts) {
            jack_log("Create monitor port");
            snprintf(name, sizeof(name), "%s:monitor_%d", fClientControl.fName, i + 1);
            if (fEngine->PortRegister(fClientControl.fRefNum, name, JACK_DEFAULT_AUDIO_TYPE, MonitorDriverFlags, fEngineControl->fBufferSize, &port_index) < 0) {
                jack_error("ALSA: cannot register monitor port for %s", name);
            } else {
                fMonitorPortList[i] = port_index;
            }
        }
    }

    UpdateLatencies();

    if (alsa_driver->midi) {
        int err = (alsa_driver->midi->attach)(alsa_driver->midi);
        if (err)
            jack_error ("ALSA: cannot attach MIDI: %d", err);
    }

    return 0;
}

int JackAlsaDriver::Detach()
{
    alsa_driver_t* alsa_driver = (alsa_driver_t*)fDriver;
    if (alsa_driver->midi)
        (alsa_driver->midi->detach)(alsa_driver->midi);

    return JackAudioDriver::Detach();
}

extern "C" char* get_control_device_name(const char * device_name)
{
    char * ctl_name;
    const char * comma;

    /* the user wants a hw or plughw device, the ctl name
     * should be hw:x where x is the card identification.
     * We skip the subdevice suffix that starts with comma */

    if (strncasecmp(device_name, "plughw:", 7) == 0) {
        /* skip the "plug" prefix" */
        device_name += 4;
    }

    comma = strchr(device_name, ',');
    if (comma == NULL) {
        ctl_name = strdup(device_name);
        if (ctl_name == NULL) {
            jack_error("strdup(\"%s\") failed.", device_name);
        }
    } else {
        ctl_name = strndup(device_name, comma - device_name);
        if (ctl_name == NULL) {
            jack_error("strndup(\"%s\", %u) failed.", device_name, (unsigned int)(comma - device_name));
        }
    }

    return ctl_name;
}

static int card_to_num(const char* device)
{
    int err;
    char* ctl_name;
    snd_ctl_card_info_t *card_info;
    snd_ctl_t* ctl_handle;
    int i = -1;

    snd_ctl_card_info_alloca (&card_info);

    ctl_name = get_control_device_name(device);
    if (ctl_name == NULL) {
        jack_error("get_control_device_name() failed.");
        goto fail;
    }

    if ((err = snd_ctl_open (&ctl_handle, ctl_name, 0)) < 0) {
        jack_error ("control open \"%s\" (%s)", ctl_name,
                    snd_strerror(err));
        goto free;
    }

    if ((err = snd_ctl_card_info(ctl_handle, card_info)) < 0) {
        jack_error ("control hardware info \"%s\" (%s)",
                    device, snd_strerror (err));
        goto close;
    }

    i = snd_ctl_card_info_get_card(card_info);

close:
    snd_ctl_close(ctl_handle);

free:
    free(ctl_name);

fail:
    return i;
}

int JackAlsaDriver::Open(jack_nframes_t nframes,
                         jack_nframes_t user_nperiods,
                         jack_nframes_t samplerate,
                         bool hw_monitoring,
                         bool hw_metering,
                         bool capturing,
                         bool playing,
                         DitherAlgorithm dither,
                         bool soft_mode,
                         bool monitor,
                         int inchannels,
                         int outchannels,
                         bool shorts_first,
                         const char* capture_driver_name,
                         const char* playback_driver_name,
                         jack_nframes_t capture_latency,
                         jack_nframes_t playback_latency,
                         const char* midi_driver_name)
{
    // Generic JackAudioDriver Open
    if (JackAudioDriver::Open(nframes, samplerate, capturing, playing,
                              inchannels, outchannels, monitor, capture_driver_name, playback_driver_name,
                              capture_latency, playback_latency) != 0) {
        return -1;
    }

    alsa_midi_t *midi = 0;
#ifndef __ANDROID__
    if (strcmp(midi_driver_name, "seq") == 0)
        midi = alsa_seqmidi_new((jack_client_t*)this, 0);
    else if (strcmp(midi_driver_name, "raw") == 0)
        midi = alsa_rawmidi_new((jack_client_t*)this);
#endif

    if (JackServerGlobals::on_device_acquire != NULL) {
        int capture_card = card_to_num(capture_driver_name);
        int playback_card = card_to_num(playback_driver_name);
        char audio_name[32];

        if (capture_card >= 0) {
            snprintf(audio_name, sizeof(audio_name), "Audio%d", capture_card);
            if (!JackServerGlobals::on_device_acquire(audio_name)) {
                jack_error("Audio device %s cannot be acquired...", capture_driver_name);
                return -1;
            }
        }

        if (playback_card >= 0 && playback_card != capture_card) {
            snprintf(audio_name, sizeof(audio_name), "Audio%d", playback_card);
            if (!JackServerGlobals::on_device_acquire(audio_name)) {
                jack_error("Audio device %s cannot be acquired...", playback_driver_name);
                if (capture_card >= 0) {
                    snprintf(audio_name, sizeof(audio_name), "Audio%d", capture_card);
                    JackServerGlobals::on_device_release(audio_name);
                }
                return -1;
            }
        }
    }

    fDriver = alsa_driver_new ((char*)"alsa_pcm", (char*)playback_driver_name, (char*)capture_driver_name,
                               NULL,
                               nframes,
                               user_nperiods,
                               samplerate,
                               hw_monitoring,
                               hw_metering,
                               capturing,
                               playing,
                               dither,
                               soft_mode,
                               monitor,
                               inchannels,
                               outchannels,
                               shorts_first,
                               capture_latency,
                               playback_latency,
                               midi);
    if (fDriver) {
        // ALSA driver may have changed the in/out values
        fCaptureChannels = ((alsa_driver_t *)fDriver)->capture_nchannels;
        fPlaybackChannels = ((alsa_driver_t *)fDriver)->playback_nchannels;
        return 0;
    } else {
        Close();
        return -1;
    }
}

int JackAlsaDriver::Close()
{
    // Generic audio driver close
    int res = JackAudioDriver::Close();

    if (fDriver) {
        alsa_driver_delete((alsa_driver_t*)fDriver);
    }

    if (JackServerGlobals::on_device_release != NULL)
    {
        char audio_name[32];
        int capture_card = card_to_num(fCaptureDriverName);
        if (capture_card >= 0) {
            snprintf(audio_name, sizeof(audio_name), "Audio%d", capture_card);
            JackServerGlobals::on_device_release(audio_name);
        }

        int playback_card = card_to_num(fPlaybackDriverName);
        if (playback_card >= 0 && playback_card != capture_card) {
            snprintf(audio_name, sizeof(audio_name), "Audio%d", playback_card);
            JackServerGlobals::on_device_release(audio_name);
        }
    }

    return res;
}

int JackAlsaDriver::Start()
{
    int res = JackAudioDriver::Start();
    if (res >= 0) {
        res = alsa_driver_start((alsa_driver_t *)fDriver);
        if (res < 0) {
            JackAudioDriver::Stop();
        }
    }
    return res;
}

int JackAlsaDriver::Stop()
{
    int res = alsa_driver_stop((alsa_driver_t *)fDriver);
    if (JackAudioDriver::Stop() < 0) {
        res = -1;
    }
    return res;
}

int JackAlsaDriver::Read()
{
    /* Taken from alsa_driver_run_cycle */
    int wait_status;
    jack_nframes_t nframes;
    fDelayedUsecs = 0.f;

retry:

    nframes = alsa_driver_wait((alsa_driver_t *)fDriver, -1, &wait_status, &fDelayedUsecs);

    if (wait_status < 0)
        return -1;		/* driver failed */

    if (nframes == 0) {
        /* we detected an xrun and restarted: notify
         * clients about the delay.
         */
        jack_log("ALSA XRun wait_status = %d", wait_status);
        NotifyXRun(fBeginDateUst, fDelayedUsecs);
        goto retry; /* recoverable error*/
    }

    if (nframes != fEngineControl->fBufferSize)
        jack_log("JackAlsaDriver::Read warning fBufferSize = %ld nframes = %ld", fEngineControl->fBufferSize, nframes);

    // Has to be done before read
    JackDriver::CycleIncTime();

    return alsa_driver_read((alsa_driver_t *)fDriver, fEngineControl->fBufferSize);
}

int JackAlsaDriver::Write()
{
    return alsa_driver_write((alsa_driver_t *)fDriver, fEngineControl->fBufferSize);
}

void JackAlsaDriver::ReadInputAux(jack_nframes_t orig_nframes, snd_pcm_sframes_t contiguous, snd_pcm_sframes_t nread)
{
    for (int chn = 0; chn < fCaptureChannels; chn++) {
        if (fGraphManager->GetConnectionsNum(fCapturePortList[chn]) > 0) {
            jack_default_audio_sample_t* buf = (jack_default_audio_sample_t*)fGraphManager->GetBuffer(fCapturePortList[chn], orig_nframes);
            alsa_driver_read_from_channel((alsa_driver_t *)fDriver, chn, buf + nread, contiguous);
        }
    }
}

void JackAlsaDriver::MonitorInputAux()
{
    for (int chn = 0; chn < fCaptureChannels; chn++) {
        JackPort* port = fGraphManager->GetPort(fCapturePortList[chn]);
        if (port->MonitoringInput()) {
            ((alsa_driver_t *)fDriver)->input_monitor_mask |= (1 << chn);
        }
    }
}

void JackAlsaDriver::ClearOutputAux()
{
    for (int chn = 0; chn < fPlaybackChannels; chn++) {
        jack_default_audio_sample_t* buf =
            (jack_default_audio_sample_t*)fGraphManager->GetBuffer(fPlaybackPortList[chn], fEngineControl->fBufferSize);
        memset(buf, 0, sizeof (jack_default_audio_sample_t) * fEngineControl->fBufferSize);
    }
}

void JackAlsaDriver::SetTimetAux(jack_time_t time)
{
    fBeginDateUst = time;
}

void JackAlsaDriver::WriteOutputAux(jack_nframes_t orig_nframes, snd_pcm_sframes_t contiguous, snd_pcm_sframes_t nwritten)
{
    for (int chn = 0; chn < fPlaybackChannels; chn++) {
        // Output ports
        if (fGraphManager->GetConnectionsNum(fPlaybackPortList[chn]) > 0) {
            jack_default_audio_sample_t* buf = (jack_default_audio_sample_t*)fGraphManager->GetBuffer(fPlaybackPortList[chn], orig_nframes);
            alsa_driver_write_to_channel(((alsa_driver_t *)fDriver), chn, buf + nwritten, contiguous);
            // Monitor ports
            if (fWithMonitorPorts && fGraphManager->GetConnectionsNum(fMonitorPortList[chn]) > 0) {
                jack_default_audio_sample_t* monbuf = (jack_default_audio_sample_t*)fGraphManager->GetBuffer(fMonitorPortList[chn], orig_nframes);
                memcpy(monbuf + nwritten, buf + nwritten, contiguous * sizeof(jack_default_audio_sample_t));
            }
        }
    }
}

int JackAlsaDriver::is_realtime() const
{
    return fEngineControl->fRealTime;
}

int JackAlsaDriver::create_thread(pthread_t *thread, int priority, int realtime, void *(*start_routine)(void*), void *arg)
{
#ifdef __ANDROID__
    return JackAndroidThread::StartImp(thread, priority, realtime, start_routine, arg);
#else
    return JackPosixThread::StartImp(thread, priority, realtime, start_routine, arg);
#endif
}

jack_port_id_t JackAlsaDriver::port_register(const char *port_name, const char *port_type, unsigned long flags, unsigned long buffer_size)
{
    jack_port_id_t port_index;
    int res = fEngine->PortRegister(fClientControl.fRefNum, port_name, port_type, flags, buffer_size, &port_index);
    return (res == 0) ? port_index : 0;
}

int JackAlsaDriver::port_unregister(jack_port_id_t port_index)
{
    return fEngine->PortUnRegister(fClientControl.fRefNum, port_index);
}

void* JackAlsaDriver::port_get_buffer(int port, jack_nframes_t nframes)
{
    return fGraphManager->GetBuffer(port, nframes);
}

int  JackAlsaDriver::port_set_alias(int port, const char* name)
{
    return fGraphManager->GetPort(port)->SetAlias(name);
}

jack_nframes_t JackAlsaDriver::get_sample_rate() const
{
    return fEngineControl->fSampleRate;
}

jack_nframes_t JackAlsaDriver::frame_time() const
{
    JackTimer timer;
    fEngineControl->ReadFrameTime(&timer);
    return timer.Time2Frames(GetMicroSeconds(), fEngineControl->fBufferSize);
}

jack_nframes_t JackAlsaDriver::last_frame_time() const
{
    JackTimer timer;
    fEngineControl->ReadFrameTime(&timer);
    return timer.CurFrame();
}

} // end of namespace


#ifdef __cplusplus
extern "C"
{
#endif

static
jack_driver_param_constraint_desc_t *
enum_alsa_devices()
{
    snd_ctl_t * handle;
    snd_ctl_card_info_t * info;
    snd_pcm_info_t * pcminfo_capture;
    snd_pcm_info_t * pcminfo_playback;
    int card_no = -1;
    jack_driver_param_value_t card_id;
    jack_driver_param_value_t device_id;
    char description[64];
    int device_no;
    bool has_capture;
    bool has_playback;
    jack_driver_param_constraint_desc_t * constraint_ptr;
    uint32_t array_size = 0;

    snd_ctl_card_info_alloca(&info);
    snd_pcm_info_alloca(&pcminfo_capture);
    snd_pcm_info_alloca(&pcminfo_playback);

    constraint_ptr = NULL;

    while(snd_card_next(&card_no) >= 0 && card_no >= 0)
    {
        snprintf(card_id.str, sizeof(card_id.str), "hw:%d", card_no);

        if (snd_ctl_open(&handle, card_id.str, 0) >= 0 &&
            snd_ctl_card_info(handle, info) >= 0)
        {
            snprintf(card_id.str, sizeof(card_id.str), "hw:%s", snd_ctl_card_info_get_id(info));
            if (!jack_constraint_add_enum(
                    &constraint_ptr,
                    &array_size,
                    &card_id,
                    snd_ctl_card_info_get_name(info)))
                goto fail;

            device_no = -1;

            while (snd_ctl_pcm_next_device(handle, &device_no) >= 0 && device_no != -1)
            {
                snprintf(device_id.str, sizeof(device_id.str), "%s,%d", card_id.str, device_no);

                snd_pcm_info_set_device(pcminfo_capture, device_no);
                snd_pcm_info_set_subdevice(pcminfo_capture, 0);
                snd_pcm_info_set_stream(pcminfo_capture, SND_PCM_STREAM_CAPTURE);
                has_capture = snd_ctl_pcm_info(handle, pcminfo_capture) >= 0;

                snd_pcm_info_set_device(pcminfo_playback, device_no);
                snd_pcm_info_set_subdevice(pcminfo_playback, 0);
                snd_pcm_info_set_stream(pcminfo_playback, SND_PCM_STREAM_PLAYBACK);
                has_playback = snd_ctl_pcm_info(handle, pcminfo_playback) >= 0;

                if (has_capture && has_playback)
                {
                    snprintf(description, sizeof(description),"%s (duplex)", snd_pcm_info_get_name(pcminfo_capture));
                }
                else if (has_capture)
                {
                    snprintf(description, sizeof(description),"%s (capture)", snd_pcm_info_get_name(pcminfo_capture));
                }
                else if (has_playback)
                {
                    snprintf(description, sizeof(description),"%s (playback)", snd_pcm_info_get_name(pcminfo_playback));
                }
                else
                {
                    continue;
                }

                if (!jack_constraint_add_enum(
                        &constraint_ptr,
                        &array_size,
                        &device_id,
                        description))
                    goto fail;
            }

            snd_ctl_close(handle);
        }
    }

    return constraint_ptr;
fail:
    jack_constraint_free(constraint_ptr);
    return NULL;
}

static int
dither_opt (char c, DitherAlgorithm* dither)
{
    switch (c) {
        case '-':
        case 'n':
            *dither = None;
            break;

        case 'r':
            *dither = Rectangular;
            break;

        case 's':
            *dither = Shaped;
            break;

        case 't':
            *dither = Triangular;
            break;

        default:
            fprintf (stderr, "ALSA driver: illegal dithering mode %c\n", c);
            return -1;
    }
    return 0;
}

SERVER_EXPORT const jack_driver_desc_t* driver_get_descriptor ()
{
    jack_driver_desc_t * desc;
    jack_driver_desc_filler_t filler;
    jack_driver_param_value_t value;

    desc = jack_driver_descriptor_construct("alsa", JackDriverMaster, "Linux ALSA API based audio backend", &filler);

    strcpy(value.str, "hw:0");
#ifdef __ANDROID__
    jack_driver_descriptor_add_parameter(desc, &filler, "device", 'd', JackDriverParamString, &value, NULL, "ALSA device name", NULL);
#else
    jack_driver_descriptor_add_parameter(desc, &filler, "device", 'd', JackDriverParamString, &value, enum_alsa_devices(), "ALSA device name", NULL);
#endif

    strcpy(value.str, "none");
    jack_driver_descriptor_add_parameter(desc, &filler, "capture", 'C', JackDriverParamString, &value, NULL, "Provide capture ports.  Optionally set device", NULL);
    jack_driver_descriptor_add_parameter(desc, &filler, "playback", 'P', JackDriverParamString, &value, NULL, "Provide playback ports.  Optionally set device", NULL);

    value.ui = 48000U;
    jack_driver_descriptor_add_parameter(desc, &filler, "rate", 'r', JackDriverParamUInt, &value, NULL, "Sample rate", NULL);

    value.ui = 1024U;
    jack_driver_descriptor_add_parameter(desc, &filler, "period", 'p', JackDriverParamUInt, &value, NULL, "Frames per period", NULL);

    value.ui = 2U;
    jack_driver_descriptor_add_parameter(desc, &filler, "nperiods", 'n', JackDriverParamUInt, &value, NULL, "Number of periods of playback latency", NULL);

    value.i = 0;
    jack_driver_descriptor_add_parameter(desc, &filler, "hwmon", 'H', JackDriverParamBool, &value, NULL, "Hardware monitoring, if available", NULL);

    value.i = 0;
    jack_driver_descriptor_add_parameter(desc, &filler, "hwmeter", 'M', JackDriverParamBool, &value, NULL, "Hardware metering, if available", NULL);

    value.i = 1;
    jack_driver_descriptor_add_parameter(desc, &filler, "duplex", 'D', JackDriverParamBool, &value, NULL, "Provide both capture and playback ports", NULL);

    value.i = 0;
    jack_driver_descriptor_add_parameter(desc, &filler, "softmode", 's', JackDriverParamBool, &value, NULL, "Soft-mode, no xrun handling", NULL);

    value.i = 0;
    jack_driver_descriptor_add_parameter(desc, &filler, "monitor", 'm', JackDriverParamBool, &value, NULL, "Provide monitor ports for the output", NULL);

    value.c = 'n';
    jack_driver_descriptor_add_parameter(
        desc,
        &filler,
        "dither",
        'z',
        JackDriverParamChar,
        &value,
        jack_constraint_compose_enum_char(
            JACK_CONSTRAINT_FLAG_STRICT | JACK_CONSTRAINT_FLAG_FAKE_VALUE,
            dither_constraint_descr_array),
        "Dithering mode",
        NULL);

    value.ui = 0;
    jack_driver_descriptor_add_parameter(desc, &filler, "inchannels", 'i', JackDriverParamUInt, &value, NULL, "Number of capture channels (defaults to hardware max)", NULL);
    jack_driver_descriptor_add_parameter(desc, &filler, "outchannels", 'o', JackDriverParamUInt, &value, NULL, "Number of playback channels (defaults to hardware max)", NULL);

    value.i = FALSE;
    jack_driver_descriptor_add_parameter(desc, &filler, "shorts", 'S', JackDriverParamBool, &value, NULL, "Try 16-bit samples before 32-bit", NULL);

    value.ui = 0;
    jack_driver_descriptor_add_parameter(desc, &filler, "input-latency", 'I', JackDriverParamUInt, &value, NULL, "Extra input latency (frames)", NULL);
    jack_driver_descriptor_add_parameter(desc, &filler, "output-latency", 'O', JackDriverParamUInt, &value, NULL, "Extra output latency (frames)", NULL);

    strcpy(value.str, "none");
    jack_driver_descriptor_add_parameter(
        desc,
        &filler,
        "midi-driver",
        'X',
        JackDriverParamString,
        &value,
        jack_constraint_compose_enum_str(
            JACK_CONSTRAINT_FLAG_STRICT | JACK_CONSTRAINT_FLAG_FAKE_VALUE,
            midi_constraint_descr_array),
        "ALSA MIDI driver",
        NULL);

    return desc;
}

static Jack::JackAlsaDriver* g_alsa_driver;

SERVER_EXPORT Jack::JackDriverClientInterface* driver_initialize(Jack::JackLockedEngine* engine, Jack::JackSynchro* table, const JSList* params)
{
    jack_nframes_t srate = 48000;
    jack_nframes_t frames_per_interrupt = 1024;
    unsigned long user_nperiods = 2;
    const char *playback_pcm_name = "hw:0";
    const char *capture_pcm_name = "hw:0";
    int hw_monitoring = FALSE;
    int hw_metering = FALSE;
    int capture = FALSE;
    int playback = FALSE;
    int soft_mode = FALSE;
    int monitor = FALSE;
    DitherAlgorithm dither = None;
    int user_capture_nchnls = 0;
    int user_playback_nchnls = 0;
    int shorts_first = FALSE;
    jack_nframes_t systemic_input_latency = 0;
    jack_nframes_t systemic_output_latency = 0;
    const JSList * node;
    const jack_driver_param_t * param;
    const char *midi_driver = "none";

    for (node = params; node; node = jack_slist_next (node)) {
        param = (const jack_driver_param_t *) node->data;

        switch (param->character) {

            case 'C':
                capture = TRUE;
                if (strcmp (param->value.str, "none") != 0) {
                    capture_pcm_name = strdup (param->value.str);
                    jack_log("capture device %s", capture_pcm_name);
                }
                break;

            case 'P':
                playback = TRUE;
                if (strcmp (param->value.str, "none") != 0) {
                    playback_pcm_name = strdup (param->value.str);
                    jack_log("playback device %s", playback_pcm_name);
                }
                break;

            case 'D':
                playback = TRUE;
                capture = TRUE;
                break;

            case 'd':
                if (strcmp (param->value.str, "none") != 0) {
                    playback_pcm_name = strdup (param->value.str);
                    capture_pcm_name = strdup (param->value.str);
                    jack_log("playback device %s", playback_pcm_name);
                    jack_log("capture device %s", capture_pcm_name);
                }
                break;

            case 'H':
                hw_monitoring = param->value.i;
                break;

            case 'm':
                monitor = param->value.i;
                break;

            case 'M':
                hw_metering = param->value.i;
                break;

            case 'r':
                srate = param->value.ui;
                jack_log("apparent rate = %d", srate);
                break;

            case 'p':
                frames_per_interrupt = param->value.ui;
                jack_log("frames per period = %d", frames_per_interrupt);
                break;

            case 'n':
                user_nperiods = param->value.ui;
                if (user_nperiods < 2) {    /* enforce minimum value */
                    user_nperiods = 2;
                }
                break;

            case 's':
                soft_mode = param->value.i;
                break;

            case 'z':
                if (dither_opt (param->value.c, &dither)) {
                    return NULL;
                }
                break;

            case 'i':
                user_capture_nchnls = param->value.ui;
                break;

            case 'o':
                user_playback_nchnls = param->value.ui;
                break;

            case 'S':
                shorts_first = param->value.i;
                break;

            case 'I':
                systemic_input_latency = param->value.ui;
                break;

            case 'O':
                systemic_output_latency = param->value.ui;
                break;

            case 'X':
                midi_driver = strdup(param->value.str);
                break;
        }
    }

    /* duplex is the default */
    if (!capture && !playback) {
        capture = TRUE;
        playback = TRUE;
    }

    g_alsa_driver = new Jack::JackAlsaDriver("system", "alsa_pcm", engine, table);
    Jack::JackDriverClientInterface* threaded_driver = new Jack::JackThreadedDriver(g_alsa_driver);
    // Special open for ALSA driver...
    if (g_alsa_driver->Open(frames_per_interrupt, user_nperiods, srate, hw_monitoring, hw_metering, capture, playback, dither, soft_mode, monitor,
                          user_capture_nchnls, user_playback_nchnls, shorts_first, capture_pcm_name, playback_pcm_name,
                          systemic_input_latency, systemic_output_latency, midi_driver) == 0) {
        return threaded_driver;
    } else {
        delete threaded_driver; // Delete the decorated driver
        return NULL;
    }
}

// Code to be used in alsa_driver.c

void ReadInput(jack_nframes_t orig_nframes, snd_pcm_sframes_t contiguous, snd_pcm_sframes_t nread)
{
    g_alsa_driver->ReadInputAux(orig_nframes, contiguous, nread);
}
void MonitorInput()
{
    g_alsa_driver->MonitorInputAux();
}
void ClearOutput()
{
    g_alsa_driver->ClearOutputAux();
}
void WriteOutput(jack_nframes_t orig_nframes, snd_pcm_sframes_t contiguous, snd_pcm_sframes_t nwritten)
{
    g_alsa_driver->WriteOutputAux(orig_nframes, contiguous, nwritten);
}
void SetTime(jack_time_t time)
{
    g_alsa_driver->SetTimetAux(time);
}

int Restart()
{
    int res;
    if ((res = g_alsa_driver->Stop()) == 0) {
        res = g_alsa_driver->Start();
    }
    return res;
}

#ifdef __cplusplus
}
#endif


