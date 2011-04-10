/*
Copyright (C) 2011 Devin Anderson

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

#include <memory>
#include <new>
#include <stdexcept>

#include <alsa/asoundlib.h>

#include "JackALSARawMidiDriver.h"
#include "JackEngineControl.h"
#include "JackError.h"
#include "JackMidiUtil.h"

using Jack::JackALSARawMidiDriver;

JackALSARawMidiDriver::JackALSARawMidiDriver(const char *name,
                                             const char *alias,
                                             JackLockedEngine *engine,
                                             JackSynchro *table):
    JackMidiDriver(name, alias, engine, table)
{
    thread = new JackThread(this);
    fCaptureChannels = 0;
    fds[0] = -1;
    fds[1] = -1;
    fPlaybackChannels = 0;
    input_ports = 0;
    output_ports = 0;
    poll_fds = 0;
}

JackALSARawMidiDriver::~JackALSARawMidiDriver()
{
    delete thread;
}

int
JackALSARawMidiDriver::Attach()
{
    const char *alias;
    jack_nframes_t buffer_size = fEngineControl->fBufferSize;
    jack_port_id_t index;
    jack_nframes_t latency = buffer_size;
    jack_latency_range_t latency_range;
    const char *name;
    JackPort *port;
    latency_range.max = latency;
    latency_range.min = latency;
    for (int i = 0; i < fCaptureChannels; i++) {
        JackALSARawMidiInputPort *input_port = input_ports[i];
        name = input_port->GetName();
        index = fGraphManager->AllocatePort(fClientControl.fRefNum, name,
                                            JACK_DEFAULT_MIDI_TYPE,
                                            CaptureDriverFlags, buffer_size);
        if (index == NO_PORT) {
            jack_error("JackALSARawMidiDriver::Attach - cannot register input "
                       "port with name '%s'.", name);
            // X: Do we need to deallocate ports?
            return -1;
        }
        alias = input_port->GetAlias();
        port = fGraphManager->GetPort(index);
        port->SetAlias(alias);
        port->SetLatencyRange(JackCaptureLatency, &latency_range);
        fCapturePortList[i] = index;

        jack_info("JackALSARawMidiDriver::Attach - input port registered "
                  "(name='%s', alias='%s').", name, alias);

    }
    if (! fEngineControl->fSyncMode) {
        latency += buffer_size;
        latency_range.max = latency;
        latency_range.min = latency;
    }
    for (int i = 0; i < fPlaybackChannels; i++) {
        JackALSARawMidiOutputPort *output_port = output_ports[i];
        name = output_port->GetName();
        index = fGraphManager->AllocatePort(fClientControl.fRefNum, name,
                                            JACK_DEFAULT_MIDI_TYPE,
                                            PlaybackDriverFlags, buffer_size);
        if (index == NO_PORT) {
            jack_error("JackALSARawMidiDriver::Attach - cannot register "
                       "output port with name '%s'.", name);
            // X: Do we need to deallocate ports?
            return -1;
        }
        alias = output_port->GetAlias();
        port = fGraphManager->GetPort(index);
        port->SetAlias(alias);
        port->SetLatencyRange(JackPlaybackLatency, &latency_range);
        fPlaybackPortList[i] = index;

        jack_info("JackALSARawMidiDriver::Attach - output port registered "
                  "(name='%s', alias='%s').", name, alias);

    }
    return 0;
}

int
JackALSARawMidiDriver::Close()
{
    // Generic MIDI driver close
    int result = JackMidiDriver::Close();

    if (input_ports) {
        for (int i = 0; i < fCaptureChannels; i++) {
            delete input_ports[i];
        }
        delete[] input_ports;
        input_ports = 0;
    }
    if (output_ports) {
        for (int i = 0; i < fPlaybackChannels; i++) {
            delete output_ports[i];
        }
        delete[] output_ports;
        output_ports = 0;
    }
    return result;
}

bool
JackALSARawMidiDriver::Execute()
{
    jack_nframes_t timeout_frame = 0;
    for (;;) {
        jack_nframes_t process_frame;
        unsigned short revents;
        jack_nframes_t *timeout_frame_ptr;
        if (! timeout_frame) {
            timeout_frame_ptr = 0;
        } else {
            timeout_frame_ptr = &timeout_frame;
        }
        if (Poll(timeout_frame_ptr) == -1) {
            if (errno == EINTR) {
                continue;
            }
            jack_error("JackALSARawMidiDriver::Execute - poll error: %s",
                       strerror(errno));
            break;
        }

        if (timeout_frame_ptr) {
            jack_info("JackALSARawMidiDriver::Execute - '%d', '%d'",
                      timeout_frame, GetCurrentFrame());
        }

        revents = poll_fds[0].revents;
        if (revents & POLLHUP) {
            // Driver is being stopped.
            break;
        }
        if (revents & (~ POLLIN)) {
            jack_error("JackALSARawMidiDriver::Execute - unexpected poll "
                       "event on pipe file descriptor.");
            break;
        }
        timeout_frame = 0;
        for (int i = 0; i < fPlaybackChannels; i++) {
            if (! output_ports[i]->ProcessALSA(fds[0], &process_frame)) {
                jack_error("JackALSARawMidiDriver::Execute - a fatal error "
                           "occurred while processing ALSA output events.");
                goto cleanup;
            }
            if (process_frame && ((! timeout_frame) ||
                                  (process_frame < timeout_frame))) {
                timeout_frame = process_frame;
            }
        }
        for (int i = 0; i < fCaptureChannels; i++) {
            if (! input_ports[i]->ProcessALSA(&process_frame)) {
                jack_error("JackALSARawMidiDriver::Execute - a fatal error "
                           "occurred while processing ALSA input events.");
                goto cleanup;
            }
            if (process_frame && ((! timeout_frame) ||
                                  (process_frame < timeout_frame))) {
                timeout_frame = process_frame;
            }
        }
    }
 cleanup:
    close(fds[0]);
    fds[0] = -1;

    jack_info("JackALSARawMidiDriver::Execute - ALSA thread exiting.");

    return false;
}

void
JackALSARawMidiDriver::
GetDeviceInfo(snd_ctl_t *control, snd_rawmidi_info_t *info,
              std::vector<snd_rawmidi_info_t *> *info_list)
{
    snd_rawmidi_info_set_subdevice(info, 0);
    int code = snd_ctl_rawmidi_info(control, info);
    if (code) {
        if (code != -ENOENT) {
            HandleALSAError("GetDeviceInfo", "snd_ctl_rawmidi_info", code);
        }
        return;
    }
    unsigned int count = snd_rawmidi_info_get_subdevices_count(info);
    for (unsigned int i = 0; i < count; i++) {
        snd_rawmidi_info_set_subdevice(info, i);
        int code = snd_ctl_rawmidi_info(control, info);
        if (code) {
            HandleALSAError("GetDeviceInfo", "snd_ctl_rawmidi_info", code);
            continue;
        }
        snd_rawmidi_info_t *info_copy;
        code = snd_rawmidi_info_malloc(&info_copy);
        if (code) {
            HandleALSAError("GetDeviceInfo", "snd_rawmidi_info_malloc", code);
            continue;
        }
        snd_rawmidi_info_copy(info_copy, info);
        try {
            info_list->push_back(info_copy);
        } catch (std::bad_alloc &e) {
            snd_rawmidi_info_free(info_copy);
            jack_error("JackALSARawMidiDriver::GetDeviceInfo - "
                       "std::vector::push_back: %s", e.what());
        }
    }
}

void
JackALSARawMidiDriver::HandleALSAError(const char *driver_func,
                                       const char *alsa_func, int code)
{
    jack_error("JackALSARawMidiDriver::%s - %s: %s", driver_func, alsa_func,
               snd_strerror(code));
}

bool
JackALSARawMidiDriver::Init()
{
    set_threaded_log_function();
    if (thread->AcquireSelfRealTime(fEngineControl->fServerPriority + 1)) {
        jack_error("JackALSARawMidiDriver::Init - could not acquire realtime "
                   "scheduling.  Continuing anyway.");
    }
    return true;
}

int
JackALSARawMidiDriver::Open(bool capturing, bool playing, int in_channels,
                            int out_channels, bool monitor,
                            const char *capture_driver_name,
                            const char *playback_driver_name,
                            jack_nframes_t capture_latency,
                            jack_nframes_t playback_latency)
{
    snd_rawmidi_info_t *info;
    int code = snd_rawmidi_info_malloc(&info);
    if (code) {
        HandleALSAError("Open", "snd_rawmidi_info_malloc", code);
        return -1;
    }
    std::vector<snd_rawmidi_info_t *> in_info_list;
    std::vector<snd_rawmidi_info_t *> out_info_list;
    for (int card = -1;;) {
        int code = snd_card_next(&card);
        if (code) {
            HandleALSAError("Open", "snd_card_next", code);
            continue;
        }
        if (card == -1) {
            break;
        }
        char name[32];
        snprintf(name, sizeof(name), "hw:%d", card);
        snd_ctl_t *control;
        code = snd_ctl_open(&control, name, SND_CTL_NONBLOCK);
        if (code) {
            HandleALSAError("Open", "snd_ctl_open", code);
            continue;
        }
        for (int device = -1;;) {
            code = snd_ctl_rawmidi_next_device(control, &device);
            if (code) {
                HandleALSAError("Open", "snd_ctl_rawmidi_next_device", code);
                continue;
            }
            if (device == -1) {
                break;
            }
            snd_rawmidi_info_set_device(info, device);
            snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_INPUT);
            GetDeviceInfo(control, info, &in_info_list);
            snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_OUTPUT);
            GetDeviceInfo(control, info, &out_info_list);
        }
        snd_ctl_close(control);
    }
    snd_rawmidi_info_free(info);
    size_t potential_inputs = in_info_list.size();
    size_t potential_outputs = out_info_list.size();
    if (! (potential_inputs || potential_outputs)) {
        jack_error("JackALSARawMidiDriver::Open - no ALSA raw MIDI input or "
                   "output ports found.");
        return -1;
    }

    // XXX: Can't use auto_ptr here.  These are arrays, and require the
    // delete[] operator.
    std::auto_ptr<JackALSARawMidiInputPort *> input_ptr;
    if (potential_inputs) {
        input_ports = new JackALSARawMidiInputPort *[potential_inputs];
        input_ptr.reset(input_ports);
    }
    std::auto_ptr<JackALSARawMidiOutputPort *> output_ptr;
    if (potential_outputs) {
        output_ports = new JackALSARawMidiOutputPort *[potential_outputs];
        output_ptr.reset(output_ports);
    }

    size_t num_inputs = 0;
    size_t num_outputs = 0;
    for (size_t i = 0; i < potential_inputs; i++) {
        snd_rawmidi_info_t *info = in_info_list.at(i);
        try {
            input_ports[num_inputs] = new JackALSARawMidiInputPort(info, i);
            num_inputs++;
        } catch (std::exception e) {
            jack_error("JackALSARawMidiDriver::Open - while creating new "
                       "JackALSARawMidiInputPort: %s", e.what());
        }
        snd_rawmidi_info_free(info);
    }
    for (size_t i = 0; i < potential_outputs; i++) {
        snd_rawmidi_info_t *info = out_info_list.at(i);
        try {
            output_ports[num_outputs] = new JackALSARawMidiOutputPort(info, i);
            num_outputs++;
        } catch (std::exception e) {
            jack_error("JackALSARawMidiDriver::Open - while creating new "
                       "JackALSARawMidiOutputPort: %s", e.what());
        }
        snd_rawmidi_info_free(info);
    }
    if (num_inputs || num_outputs) {
        if (! JackMidiDriver::Open(capturing, playing, num_inputs, num_outputs,
                                   monitor, capture_driver_name,
                                   playback_driver_name, capture_latency,
                                   playback_latency)) {
            if (potential_inputs) {
                input_ptr.release();
            }
            if (potential_outputs) {
                output_ptr.release();
            }
            return 0;
        }
        jack_error("JackALSARawMidiDriver::Open - JackMidiDriver::Open error");
    } else {
        jack_error("JackALSARawMidiDriver::Open - none of the potential "
                   "inputs or outputs were successfully opened.");
    }
    Close();
    return -1;
}

int
JackALSARawMidiDriver::Poll(const jack_nframes_t *wakeup_frame)
{
    struct timespec timeout;
    struct timespec *timeout_ptr;
    if (! wakeup_frame) {
        timeout_ptr = 0;
    } else {
        timeout_ptr = &timeout;
        jack_time_t next_time = GetTimeFromFrames(*wakeup_frame);
        jack_time_t now = GetMicroSeconds();
        if (next_time <= now) {
            timeout.tv_sec = 0;
            timeout.tv_nsec = 0;
        } else {
            jack_time_t wait_time = next_time - now;
            timeout.tv_sec = wait_time / 1000000;
            timeout.tv_nsec = (wait_time % 1000000) * 1000;
        }
    }
    return ppoll(poll_fds, poll_fd_count, timeout_ptr, 0);
}

int
JackALSARawMidiDriver::Read()
{
    jack_nframes_t buffer_size = fEngineControl->fBufferSize;
    for (int i = 0; i < fCaptureChannels; i++) {
        if (! input_ports[i]->ProcessJack(GetInputBuffer(i), buffer_size)) {
            return -1;
        }
    }
    return 0;
}

int
JackALSARawMidiDriver::Start()
{

    jack_info("JackALSARawMidiDriver::Start - Starting 'alsarawmidi' driver.");

    JackMidiDriver::Start();
    poll_fd_count = 1;
    for (int i = 0; i < fCaptureChannels; i++) {
        poll_fd_count += input_ports[i]->GetPollDescriptorCount();
    }
    for (int i = 0; i < fPlaybackChannels; i++) {
        poll_fd_count += output_ports[i]->GetPollDescriptorCount();
    }
    try {
        poll_fds = new pollfd[poll_fd_count];
    } catch (std::bad_alloc e) {
        jack_error("JackALSARawMidiDriver::Start - creating poll descriptor "
                   "structures failed: %s", e.what());
        return -1;
    }
    int flags;
    struct pollfd *poll_fd_iter;
    if (pipe(fds) == -1) {
        jack_error("JackALSARawMidiDriver::Start - while creating wake pipe: "
                   "%s", strerror(errno));
        goto free_poll_descriptors;
    }
    flags = fcntl(fds[0], F_GETFL);
    if (flags == -1) {
        jack_error("JackALSARawMidiDriver::Start = while getting flags for "
                   "read file descriptor: %s", strerror(errno));
        goto close_fds;
    }
    if (fcntl(fds[0], F_SETFL, flags | O_NONBLOCK) == -1) {
        jack_error("JackALSARawMidiDriver::Start - while setting non-blocking "
                   "mode for read file descriptor: %s", strerror(errno));
        goto close_fds;
    }
    flags = fcntl(fds[1], F_GETFL);
    if (flags == -1) {
        jack_error("JackALSARawMidiDriver::Start = while getting flags for "
                   "write file descriptor: %s", strerror(errno));
        goto close_fds;
    }
    if (fcntl(fds[1], F_SETFL, flags | O_NONBLOCK) == -1) {
        jack_error("JackALSARawMidiDriver::Start - while setting non-blocking "
                   "mode for write file descriptor: %s", strerror(errno));
        goto close_fds;
    }
    poll_fds[0].events = POLLERR | POLLIN | POLLNVAL;
    poll_fds[0].fd = fds[0];
    poll_fd_iter = poll_fds + 1;
    for (int i = 0; i < fCaptureChannels; i++) {
        JackALSARawMidiInputPort *input_port = input_ports[i];
        input_port->PopulatePollDescriptors(poll_fd_iter);
        poll_fd_iter += input_port->GetPollDescriptorCount();
    }
    for (int i = 0; i < fPlaybackChannels; i++) {
        JackALSARawMidiOutputPort *output_port = output_ports[i];
        output_port->PopulatePollDescriptors(poll_fd_iter);
        poll_fd_iter += output_port->GetPollDescriptorCount();
    }

    jack_info("JackALSARawMidiDriver::Start - starting ALSA thread ...");

    if (! thread->StartSync()) {

        jack_info("JackALSARawMidiDriver::Start - started ALSA thread.");

        return 0;
    }
    jack_error("JackALSARawMidiDriver::Start - failed to start MIDI "
               "processing thread.");
 close_fds:
    close(fds[1]);
    fds[1] = -1;
    close(fds[0]);
    fds[0] = -1;
 free_poll_descriptors:
    delete[] poll_fds;
    poll_fds = 0;
    return -1;
}

int
JackALSARawMidiDriver::Stop()
{
    jack_info("JackALSARawMidiDriver::Stop - stopping 'alsarawmidi' driver.");

    if (fds[1] != -1) {
        close(fds[1]);
        fds[1] = -1;
    }
    int result;
    const char *verb;
    switch (thread->GetStatus()) {
    case JackThread::kIniting:
    case JackThread::kStarting:
        result = thread->Kill();
        verb = "kill";
        break;
    case JackThread::kRunning:
        result = thread->Stop();
        verb = "stop";
        break;
    default:
        result = 0;
        verb = 0;
    }
    if (fds[0] != -1) {
        close(fds[0]);
        fds[0] = -1;
    }
    if (poll_fds) {
        delete[] poll_fds;
        poll_fds = 0;
    }
    if (result) {
        jack_error("JackALSARawMidiDriver::Stop - could not %s MIDI "
                   "processing thread.", verb);
    }
    return result;
}

int
JackALSARawMidiDriver::Write()
{
    jack_nframes_t buffer_size = fEngineControl->fBufferSize;
    int write_fd = fds[1];
    for (int i = 0; i < fPlaybackChannels; i++) {
        if (! output_ports[i]->ProcessJack(GetOutputBuffer(i), buffer_size,
                                           write_fd)) {
            return -1;
        }
    }
    return 0;
}

#ifdef __cplusplus
extern "C" {
#endif

    SERVER_EXPORT jack_driver_desc_t *
    driver_get_descriptor()
    {
        jack_driver_desc_t *desc =
            (jack_driver_desc_t *) malloc(sizeof(jack_driver_desc_t));
        if (desc) {
            strcpy(desc->desc, "Alternative ALSA raw MIDI backend.");
            strcpy(desc->name, "alsarawmidi");

            // X: There could be parameters here regarding setting I/O buffer
            // sizes.  I don't think MIDI drivers can accept parameters right
            // now without being set as the main driver.
            desc->nparams = 0;
            desc->params = 0;
        }
        return desc;
    }

    SERVER_EXPORT Jack::JackDriverClientInterface *
    driver_initialize(Jack::JackLockedEngine *engine, Jack::JackSynchro *table,
                      const JSList *params)
    {
        Jack::JackDriverClientInterface *driver =
            new Jack::JackALSARawMidiDriver("system_midi", "alsarawmidi",
                                            engine, table);
        if (driver->Open(1, 1, 0, 0, false, "midi in", "midi out", 0, 0)) {
            delete driver;
            driver = 0;
        }
        return driver;
    }

#ifdef __cplusplus
}
#endif
