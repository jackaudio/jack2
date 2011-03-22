#ifndef __JackALSARawMidiReceiveQueue__
#define __JackALSARawMidiReceiveQueue__

#include <alsa/asoundlib.h>

#include "JackMidiReceiveQueue.h"

namespace Jack {

    class JackALSARawMidiReceiveQueue: public JackMidiReceiveQueue {

    private:

        jack_midi_data_t *buffer;
        size_t buffer_size;
        jack_midi_event_t event;
        snd_rawmidi_t *rawmidi;

    public:

        JackALSARawMidiReceiveQueue(snd_rawmidi_t *rawmidi,
                                    size_t buffer_size=4096);
        ~JackALSARawMidiReceiveQueue();

        jack_midi_event_t *
        DequeueEvent();

    };

}

#endif
