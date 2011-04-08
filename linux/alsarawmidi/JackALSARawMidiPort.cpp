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

#include <stdexcept>
#include <string>

#include "JackALSARawMidiPort.h"
#include "JackError.h"

using Jack::JackALSARawMidiPort;

JackALSARawMidiPort::JackALSARawMidiPort(snd_rawmidi_info_t *info,
                                         size_t index)
{
    int card = snd_rawmidi_info_get_card(info);
    unsigned int device = snd_rawmidi_info_get_device(info);
    unsigned int subdevice = snd_rawmidi_info_get_subdevice(info);
    char device_id[32];
    snprintf(device_id, sizeof(device_id), "hw:%d,%d,%d", card, device,
             subdevice);
    const char *alias_prefix;
    const char *error_message;
    snd_rawmidi_t **in;
    snd_rawmidi_t **out;
    const char *name_suffix;
    if (snd_rawmidi_info_get_stream(info) == SND_RAWMIDI_STREAM_OUTPUT) {
        alias_prefix = "system:midi_playback_";
        in = 0;
        name_suffix = "out";
        out = &rawmidi;
    } else {
        alias_prefix = "system:midi_capture_";
        in = &rawmidi;
        name_suffix = "in";
        out = 0;
    }
    const char *device_name;
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

    // Smallest valid buffer size.
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
    num_fds = snd_rawmidi_poll_descriptors_count(rawmidi);
    if (! num_fds) {
        error_message = "returned '0' count for poll descriptors";
        func = "snd_rawmidi_poll_descriptors_count";
        goto close;
    }
    snprintf(alias, sizeof(alias), "%s%d", alias_prefix, index + 1);
    snprintf(name, sizeof(name), "system:%d-%d %s %d %s", card + 1, device + 1,
             snd_rawmidi_info_get_name(info), subdevice + 1, name_suffix);
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

const char *
JackALSARawMidiPort::GetName()
{
    return name;
}

int
JackALSARawMidiPort::GetPollDescriptorCount()
{
    return num_fds;
}

bool
JackALSARawMidiPort::PopulatePollDescriptors(struct pollfd *poll_fd)
{
    bool result = snd_rawmidi_poll_descriptors(rawmidi, poll_fd, num_fds) ==
        num_fds;
    if (result) {
        poll_fds = poll_fd;
    }
    return result;
}

bool
JackALSARawMidiPort::ProcessPollEvents(unsigned short *revents)
{
    int code = snd_rawmidi_poll_descriptors_revents(rawmidi, poll_fds, num_fds,
                                                    revents);
    if (code) {
        jack_error("JackALSARawMidiPort::ProcessPollEvents - "
                   "snd_rawmidi_poll_descriptors_revents: %s",
                   snd_strerror(code));
        return false;
    }
    if ((*revents) & POLLNVAL) {
        jack_error("JackALSARawMidiPort::ProcessPollEvents - the file "
                   "descriptor is invalid.");
        return false;
    }
    if ((*revents) & POLLERR) {
        jack_error("JackALSARawMidiPort::ProcessPollEvents - an error has "
                   "occurred on the device or stream.");
        return false;
    }
    return true;
}

void
JackALSARawMidiPort::SetPollEventMask(unsigned short events)
{
    for (int i = 0; i < num_fds; i++) {
        (poll_fds + i)->events = events;
    }
}
