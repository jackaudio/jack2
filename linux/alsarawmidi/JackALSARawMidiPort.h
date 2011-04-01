#ifndef __JackALSARawMidiPort__
#define __JackALSARawMidiPort__

#include <alsa/asoundlib.h>
#include <poll.h>

#include "JackConstants.h"

namespace Jack {

    class JackALSARawMidiPort {

    private:

        char alias[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
        char name[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
        int num_fds;
        struct pollfd *poll_fds;

    protected:

        snd_rawmidi_t *rawmidi;

        bool
        ProcessPollEvents(unsigned short *revents);

        void
        SetPollEventMask(unsigned short events);

    public:

        JackALSARawMidiPort(snd_rawmidi_info_t *info, size_t index);

        virtual
        ~JackALSARawMidiPort();

        const char *
        GetAlias();

        const char *
        GetName();

        int
        GetPollDescriptorCount();

        bool
        PopulatePollDescriptors(struct pollfd *poll_fd);

    };

}

#endif
