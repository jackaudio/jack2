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

#include <cassert>
#include <stdexcept>
#include <string>

#include "JackALSARawMidiPort.h"
#include "JackALSARawMidiUtil.h"
#include "JackError.h"

using Jack::JackALSARawMidiPort;

JackALSARawMidiPort::JackALSARawMidiPort(snd_rawmidi_info_t *info,
                                         size_t index, unsigned short io_mask)
{
    int card = snd_rawmidi_info_get_card(info);
    unsigned int device = snd_rawmidi_info_get_device(info);
    unsigned int subdevice = snd_rawmidi_info_get_subdevice(info);
    char device_id[32];
    snprintf(device_id, sizeof(device_id), "hw:%d,%d,%d", card, device,
             subdevice);
    const char *alias_suffix;
    const char *error_message;
    snd_rawmidi_t **in;
    const char *name_prefix;
    snd_rawmidi_t **out;
    if (snd_rawmidi_info_get_stream(info) == SND_RAWMIDI_STREAM_OUTPUT) {
        alias_suffix = "out";
        in = 0;
        name_prefix = "system:midi_playback_";
        out = &rawmidi;
    } else {
        alias_suffix = "in";
        in = &rawmidi;
        name_prefix = "system:midi_capture_";
        out = 0;
    }
    const char *func;
    int code = snd_rawmidi_open(in, out, device_id, SND_RAWMIDI_NONBLOCK);
    if (code) {
        error_message = snd_strerror(code);
        func = "snd_rawmidi_open";
        goto handle_error;
    }
    snd_rawmidi_params_t *params;
    code = snd_rawmidi_params_malloc(&params);
    if (code) {
        error_message = snd_strerror(code);
        func = "snd_rawmidi_params_malloc";
        goto close;
    }
    code = snd_rawmidi_params_current(rawmidi, params);
    if (code) {
        error_message = snd_strerror(code);
        func = "snd_rawmidi_params_current";
        goto free_params;
    }
    code = snd_rawmidi_params_set_avail_min(rawmidi, params, 1);
    if (code) {
        error_message = snd_strerror(code);
        func = "snd_rawmidi_params_set_avail_min";
        goto free_params;
    }

    // Minimum buffer size allowed by ALSA
    code = snd_rawmidi_params_set_buffer_size(rawmidi, params, 32);
    if (code) {
        error_message = snd_strerror(code);
        func = "snd_rawmidi_params_set_buffer_size";
        goto free_params;
    }

    code = snd_rawmidi_params_set_no_active_sensing(rawmidi, params, 1);
    if (code) {
        error_message = snd_strerror(code);
        func = "snd_rawmidi_params_set_no_active_sensing";
        goto free_params;
    }
    code = snd_rawmidi_params(rawmidi, params);
    if (code) {
        error_message = snd_strerror(code);
        func = "snd_rawmidi_params";
        goto free_params;
    }
    snd_rawmidi_params_free(params);
    alsa_poll_fd_count = snd_rawmidi_poll_descriptors_count(rawmidi);
    if (! alsa_poll_fd_count) {
        error_message = "returned '0' count for poll descriptors";
        func = "snd_rawmidi_poll_descriptors_count";
        goto close;
    }
    try {
        CreateNonBlockingPipe(fds);
    } catch (std::exception e) {
        error_message = e.what();
        func = "CreateNonBlockingPipe";
        goto close;
    }
    snprintf(alias, sizeof(alias), "system:%d-%d %s %d %s", card + 1,
             device + 1, snd_rawmidi_info_get_name(info), subdevice + 1,
             alias_suffix);
    snprintf(name, sizeof(name), "%s%zu", name_prefix, index + 1);
    this->io_mask = io_mask;
    return;
 free_params:
    snd_rawmidi_params_free(params);
 close:
    snd_rawmidi_close(rawmidi);
 handle_error:
    throw std::runtime_error(std::string(func) + ": " + error_message);
}

JackALSARawMidiPort::~JackALSARawMidiPort()
{
    DestroyNonBlockingPipe(fds);
    if (rawmidi) {
        int code = snd_rawmidi_close(rawmidi);
        if (code) {
            jack_error("JackALSARawMidiPort::~JackALSARawMidiPort - "
                       "snd_rawmidi_close: %s", snd_strerror(code));
        }
        rawmidi = 0;
    }
}

const char *
JackALSARawMidiPort::GetAlias()
{
    return alias;
}

int
JackALSARawMidiPort::GetIOPollEvent()
{
    unsigned short events;
    int code = snd_rawmidi_poll_descriptors_revents(rawmidi, alsa_poll_fds,
                                                    alsa_poll_fd_count,
                                                    &events);
    if (code) {
        jack_error("JackALSARawMidiPort::GetIOPollEvents - "
                   "snd_rawmidi_poll_descriptors_revents: %s",
                   snd_strerror(code));
        return -1;
    }
    if (events & POLLNVAL) {
        jack_error("JackALSARawMidiPort::GetIOPollEvents - the file "
                   "descriptor is invalid.");
        return -1;
    }
    if (events & POLLERR) {
        jack_error("JackALSARawMidiPort::GetIOPollEvents - an error has "
                   "occurred on the device or stream.");
        return -1;
    }
    return (events & io_mask) ? 1 : 0;
}

const char *
JackALSARawMidiPort::GetName()
{
    return name;
}

int
JackALSARawMidiPort::GetPollDescriptorCount()
{
    return alsa_poll_fd_count + 1;
}

int
JackALSARawMidiPort::GetQueuePollEvent()
{
    unsigned short events = queue_poll_fd->revents;
    if (events & POLLNVAL) {
        jack_error("JackALSARawMidiPort::GetQueuePollEvents - the file "
                   "descriptor is invalid.");
        return -1;
    }
    if (events & POLLERR) {
        jack_error("JackALSARawMidiPort::GetQueuePollEvents - an error has "
                   "occurred on the device or stream.");
        return -1;
    }
    int event = events & POLLIN ? 1 : 0;
    if (event) {
        char c;
        ssize_t result = read(fds[0], &c, 1);
        assert(result);
        if (result < 0) {
            jack_error("JackALSARawMidiPort::GetQueuePollEvents - error "
                       "reading a byte from the pipe file descriptor: %s",
                       strerror(errno));
            return -1;
        }
    }
    return event;
}

void
JackALSARawMidiPort::PopulatePollDescriptors(struct pollfd *poll_fd)
{
    alsa_poll_fds = poll_fd + 1;
    assert(snd_rawmidi_poll_descriptors(rawmidi, alsa_poll_fds,
                                        alsa_poll_fd_count) ==
           alsa_poll_fd_count);
    queue_poll_fd = poll_fd;
    queue_poll_fd->events = POLLERR | POLLIN | POLLNVAL;
    queue_poll_fd->fd = fds[0];
    SetIOEventsEnabled(true);
}

void
JackALSARawMidiPort::SetIOEventsEnabled(bool enabled)
{
    unsigned short mask = POLLNVAL | POLLERR | (enabled ? io_mask : 0);
    for (int i = 0; i < alsa_poll_fd_count; i++) {
        (alsa_poll_fds + i)->events = mask;
    }
}

bool
JackALSARawMidiPort::TriggerQueueEvent()
{
    char c;
    ssize_t result = write(fds[1], &c, 1);
    assert(result <= 1);
    switch (result) {
    case 1:
        return true;
    case 0:
        jack_error("JackALSARawMidiPort::TriggerQueueEvent - error writing a "
                   "byte to the pipe file descriptor: %s", strerror(errno));
        break;
    default:
        jack_error("JackALSARawMidiPort::TriggerQueueEvent - couldn't write a "
                   "byte to the pipe file descriptor.");
    }
    return false;
}
