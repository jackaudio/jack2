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
#include "JackALSARawMidiUtil.h"
#include "JackEngineControl.h"
#include "JackError.h"
#include "JackMidiUtil.h"
#include "driver_interface.h"

using Jack::JackALSARawMidiDriver;

JackALSARawMidiDriver::JackALSARawMidiDriver(const char *name,
                                             const char *alias,
                                             JackLockedEngine *engine,
                                             JackSynchro *table):
    JackMidiDriver(name, alias, engine, table)
{
    thread = new JackThread(this);
    fds[0] = -1;
    fds[1] = -1;
    input_ports = 0;
    output_ports = 0;
    output_port_timeouts = 0;
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
        fEngine->PortRegister(fClientControl.fRefNum, name,
                            JACK_DEFAULT_MIDI_TYPE,
                            CaptureDriverFlags, buffer_size, &index);
        if (index == NO_PORT) {
            jack_error("JackALSARawMidiDriver::Attach - cannot register input "
                       "port with name '%s'.", name);
            // XX: Do we need to deallocate ports?
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
        fEngine->PortRegister(fClientControl.fRefNum, name,
                            JACK_DEFAULT_MIDI_TYPE,
                            PlaybackDriverFlags, buffer_size, &index);
        if (index == NO_PORT) {
            jack_error("JackALSARawMidiDriver::Attach - cannot register "
                       "output port with name '%s'.", name);
            // XX: Do we need to deallocate ports?
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
        struct timespec timeout;
        struct timespec *timeout_ptr;
        if (! timeout_frame) {
            timeout_ptr = 0;
        } else {

            // The timeout value is relative to the time that
            // 'GetMicroSeconds()' is called, not the time that 'poll()' is
            // called.  This means that the amount of time that passes between
            // 'GetMicroSeconds()' and 'ppoll()' is time that will be lost
            // while waiting for 'poll() to timeout.
            //
            // I tried to replace the timeout with a 'timerfd' with absolute
            // times, but, strangely, it actually slowed things down, and made
            // the code a lot more complicated.
            //
            // I wonder about using the 'epoll' interface instead of 'ppoll()'.
            // The problem with the 'epoll' interface is that the timeout
            // resolution of 'epoll_wait()' is set in milliseconds.  We need
            // microsecond resolution.  Without microsecond resolution, we
            // impose the same jitter as USB MIDI.
            //
            // Another problem is that 'ppoll()' returns later than the wait
            // time.  The problem can be minimized with high precision timers.

            timeout_ptr = &timeout;
            jack_time_t next_time = GetTimeFromFrames(timeout_frame);
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
        int poll_result = ppoll(poll_fds, poll_fd_count, timeout_ptr, 0);

        // Getting the current frame value here allows us to use it for
        // incoming MIDI bytes.  This makes sense, as the data has already
        // arrived at this point.
        jack_nframes_t current_frame = GetCurrentFrame();

        if (poll_result == -1) {
            if (errno == EINTR) {
                continue;
            }
            jack_error("JackALSARawMidiDriver::Execute - poll error: %s",
                       strerror(errno));
            break;
        }
        jack_nframes_t port_timeout;
        timeout_frame = 0;
        if (! poll_result) {

            // No I/O events occurred.  So, only handle timeout events on
            // output ports.

            for (int i = 0; i < fPlaybackChannels; i++) {
                port_timeout = output_port_timeouts[i];
                if (port_timeout && (port_timeout <= current_frame)) {
                    if (! output_ports[i]->ProcessPollEvents(false, true,
                                                             &port_timeout)) {
                        jack_error("JackALSARawMidiDriver::Execute - a fatal "
                                   "error occurred while processing ALSA "
                                   "output events.");
                        goto cleanup;
                    }
                    output_port_timeouts[i] = port_timeout;
                }
                if (port_timeout && ((! timeout_frame) ||
                                     (port_timeout < timeout_frame))) {
                    timeout_frame = port_timeout;
                }
            }
            continue;
        }

        // See if it's time to shutdown.

        unsigned short revents = poll_fds[0].revents;
        if (revents) {
            if (revents & (~ POLLHUP)) {
                jack_error("JackALSARawMidiDriver::Execute - unexpected poll "
                           "event on pipe file descriptor.");
            }
            break;
        }

        // Handle I/O events *and* timeout events on output ports.

        for (int i = 0; i < fPlaybackChannels; i++) {
            port_timeout = output_port_timeouts[i];
            bool timeout = port_timeout && (port_timeout <= current_frame);
            if (! output_ports[i]->ProcessPollEvents(true, timeout,
                                                     &port_timeout)) {
                jack_error("JackALSARawMidiDriver::Execute - a fatal error "
                           "occurred while processing ALSA output events.");
                goto cleanup;
            }
            output_port_timeouts[i] = port_timeout;
            if (port_timeout && ((! timeout_frame) ||
                                 (port_timeout < timeout_frame))) {
                timeout_frame = port_timeout;
            }
        }

        // Handle I/O events on input ports.  We handle these last because we
        // already computed the arrival time above, and will impose a delay on
        // the events by 'period-size' frames anyway, which gives us a bit of
        // borrowed time.

        for (int i = 0; i < fCaptureChannels; i++) {
            if (! input_ports[i]->ProcessPollEvents(current_frame)) {
                jack_error("JackALSARawMidiDriver::Execute - a fatal error "
                           "occurred while processing ALSA input events.");
                goto cleanup;
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
FreeDeviceInfo(std::vector<snd_rawmidi_info_t *> *in_info_list,
               std::vector<snd_rawmidi_info_t *> *out_info_list)
{
    size_t length = in_info_list->size();
    for (size_t i = 0; i < length; i++) {
        snd_rawmidi_info_free(in_info_list->at(i));
    }
    length = out_info_list->size();
    for (size_t i = 0; i < length; i++) {
        snd_rawmidi_info_free(out_info_list->at(i));
    }
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
        FreeDeviceInfo(&in_info_list, &out_info_list);
        return -1;
    }
    size_t num_inputs = 0;
    size_t num_outputs = 0;
    if (potential_inputs) {
        try {
            input_ports = new JackALSARawMidiInputPort *[potential_inputs];
        } catch (std::exception e) {
            jack_error("JackALSARawMidiDriver::Open - while creating input "
                       "port array: %s", e.what());
            FreeDeviceInfo(&in_info_list, &out_info_list);
            return -1;
        }
    }
    if (potential_outputs) {
        try {
            output_ports = new JackALSARawMidiOutputPort *[potential_outputs];
        } catch (std::exception e) {
            jack_error("JackALSARawMidiDriver::Open - while creating output "
                       "port array: %s", e.what());
            FreeDeviceInfo(&in_info_list, &out_info_list);
            goto delete_input_ports;
        }
    }
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
    if (! (num_inputs || num_outputs)) {
        jack_error("JackALSARawMidiDriver::Open - none of the potential "
                   "inputs or outputs were successfully opened.");
    } else if (JackMidiDriver::Open(capturing, playing, num_inputs,
                                    num_outputs, monitor, capture_driver_name,
                                    playback_driver_name, capture_latency,
                                    playback_latency)) {
        jack_error("JackALSARawMidiDriver::Open - JackMidiDriver::Open error");
    } else {
        return 0;
    }
    if (output_ports) {
        for (size_t i = 0; i < num_outputs; i++) {
            delete output_ports[i];
        }
        delete[] output_ports;
        output_ports = 0;
    }
 delete_input_ports:
    if (input_ports) {
        for (size_t i = 0; i < num_inputs; i++) {
            delete input_ports[i];
        }
        delete[] input_ports;
        input_ports = 0;
    }
    return -1;
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
    } catch (std::exception e) {
        jack_error("JackALSARawMidiDriver::Start - creating poll descriptor "
                   "structures failed: %s", e.what());
        return -1;
    }
    if (fPlaybackChannels) {
        try {
            output_port_timeouts = new jack_nframes_t[fPlaybackChannels];
        } catch (std::exception e) {
            jack_error("JackALSARawMidiDriver::Start - creating array for "
                       "output port timeout values failed: %s", e.what());
            goto free_poll_descriptors;
        }
    }
    struct pollfd *poll_fd_iter;
    try {
        CreateNonBlockingPipe(fds);
    } catch (std::exception e) {
        jack_error("JackALSARawMidiDriver::Start - while creating wake pipe: "
                   "%s", e.what());
        goto free_output_port_timeouts;
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
        output_port_timeouts[i] = 0;
    }

    jack_info("JackALSARawMidiDriver::Start - starting ALSA thread ...");

    if (! thread->StartSync()) {

        jack_info("JackALSARawMidiDriver::Start - started ALSA thread.");

        return 0;
    }
    jack_error("JackALSARawMidiDriver::Start - failed to start MIDI "
               "processing thread.");

    DestroyNonBlockingPipe(fds);
    fds[1] = -1;
    fds[0] = -1;
 free_output_port_timeouts:
    delete[] output_port_timeouts;
    output_port_timeouts = 0;
 free_poll_descriptors:
    delete[] poll_fds;
    poll_fds = 0;
    return -1;
}

int
JackALSARawMidiDriver::Stop()
{
    jack_info("JackALSARawMidiDriver::Stop - stopping 'alsarawmidi' driver.");
    JackMidiDriver::Stop();

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
    if (output_port_timeouts) {
        delete[] output_port_timeouts;
        output_port_timeouts = 0;
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
    for (int i = 0; i < fPlaybackChannels; i++) {
        if (! output_ports[i]->ProcessJack(GetOutputBuffer(i), buffer_size)) {
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
        // X: There could be parameters here regarding setting I/O buffer
        // sizes.  I don't think MIDI drivers can accept parameters right
        // now without being set as the main driver.

        return jack_driver_descriptor_construct("alsarawmidi", JackDriverSlave, "Alternative ALSA raw MIDI backend.", NULL);
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
