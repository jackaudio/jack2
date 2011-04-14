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

#ifndef __JackALSARawMidiDriver__
#define __JackALSARawMidiDriver__

#include <vector>

#include <alsa/asoundlib.h>
#include <poll.h>

#include "JackALSARawMidiInputPort.h"
#include "JackALSARawMidiOutputPort.h"
#include "JackMidiDriver.h"
#include "JackThread.h"

namespace Jack {

    class JackALSARawMidiDriver:
        public JackMidiDriver, public JackRunnableInterface {

    private:

        int fds[2];
        JackALSARawMidiInputPort **input_ports;
        JackALSARawMidiOutputPort **output_ports;
        jack_nframes_t *output_port_timeouts;
        nfds_t poll_fd_count;
        struct pollfd *poll_fds;
        JackThread *thread;

        void
        FreeDeviceInfo(std::vector<snd_rawmidi_info_t *> *in_info_list,
                       std::vector<snd_rawmidi_info_t *> *out_info_list);

        void
        GetDeviceInfo(snd_ctl_t *control, snd_rawmidi_info_t *info,
                      std::vector<snd_rawmidi_info_t *> *info_list);

        void
        HandleALSAError(const char *driver_func, const char *alsa_func,
                        int code);

    public:

        JackALSARawMidiDriver(const char *name, const char *alias,
                              JackLockedEngine *engine, JackSynchro *table);
        ~JackALSARawMidiDriver();

        int
        Attach();

        int
        Close();

        bool
        Execute();

        bool
        Init();

        int
        Open(bool capturing, bool playing, int in_channels, int out_channels,
             bool monitoring, const char *capture_driver_name,
             const char *playback_driver_name, jack_nframes_t capture_latency,
             jack_nframes_t playback_latency);

        int
        Read();

        int
        Start();

        int
        Stop();

        int
        Write();

    };

}

#endif
