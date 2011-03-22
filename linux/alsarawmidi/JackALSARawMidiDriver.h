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
        nfds_t poll_fd_count;
        struct pollfd *poll_fds;
        JackThread *thread;

        void
        GetDeviceInfo(snd_ctl_t *control, snd_rawmidi_info_t *info,
                      std::vector<snd_rawmidi_info_t *> *info_list);

        void
        HandleALSAError(const char *driver_func, const char *alsa_func,
                        int code);

        int
        Poll(jack_time_t wait_time);

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
