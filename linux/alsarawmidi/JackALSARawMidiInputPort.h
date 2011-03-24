#ifndef __JackALSARawMidiInputPort__
#define __JackALSARawMidiInputPort__

#include "JackALSARawMidiPort.h"
#include "JackALSARawMidiReceiveQueue.h"
#include "JackMidiAsyncQueue.h"
#include "JackMidiBufferWriteQueue.h"
#include "JackMidiRawInputWriteQueue.h"

namespace Jack {

    class JackALSARawMidiInputPort: public JackALSARawMidiPort {

    private:

        jack_midi_event_t *alsa_event;
        jack_midi_event_t *jack_event;
        JackMidiRawInputWriteQueue *raw_queue;
        JackALSARawMidiReceiveQueue *receive_queue;
        JackMidiAsyncQueue *thread_queue;
        JackMidiBufferWriteQueue *write_queue;

        jack_nframes_t
        EnqueueALSAEvent();

    public:

        JackALSARawMidiInputPort(snd_rawmidi_info_t *info, size_t index,
                                 size_t max_bytes=4096,
                                 size_t max_messages=1024);
        ~JackALSARawMidiInputPort();

        bool
        ProcessALSA(jack_nframes_t *frame);

        bool
        ProcessJack(JackMidiBuffer *port_buffer, jack_nframes_t frames);

    };

}

#endif
