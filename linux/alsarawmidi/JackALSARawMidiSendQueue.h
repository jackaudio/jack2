#ifndef __JackALSARawMidiSendQueue__
#define __JackALSARawMidiSendQueue__

#include <alsa/asoundlib.h>

#include "JackMidiSendQueue.h"

namespace Jack {

    class JackALSARawMidiSendQueue: public JackMidiSendQueue {

    private:

        bool blocked;
        snd_rawmidi_t *rawmidi;

    public:

        JackALSARawMidiSendQueue(snd_rawmidi_t *rawmidi);

        JackMidiWriteQueue::EnqueueResult
        EnqueueEvent(jack_nframes_t time, size_t size,
                     jack_midi_data_t *buffer);

        bool
        IsBlocked();

    };

}

#endif
