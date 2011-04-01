#ifndef __JackALSARawMidiOutputPort__
#define __JackALSARawMidiOutputPort__

#include "JackALSARawMidiPort.h"
#include "JackALSARawMidiSendQueue.h"
#include "JackMidiAsyncQueue.h"
#include "JackMidiBufferReadQueue.h"
#include "JackMidiRawOutputWriteQueue.h"

namespace Jack {

    class JackALSARawMidiOutputPort: public JackALSARawMidiPort {

    private:

        jack_midi_event_t *alsa_event;
        bool blocked;
        JackMidiRawOutputWriteQueue *raw_queue;
        JackMidiBufferReadQueue *read_queue;
        JackALSARawMidiSendQueue *send_queue;
        JackMidiAsyncQueue *thread_queue;

        jack_midi_event_t *
        DequeueALSAEvent(int read_fd);

    public:

        JackALSARawMidiOutputPort(snd_rawmidi_info_t *info, size_t index,
                                  size_t max_bytes=4096,
                                  size_t max_messages=1024);
        ~JackALSARawMidiOutputPort();

        bool
        ProcessALSA(int read_fd, jack_nframes_t *frame);

        bool
        ProcessJack(JackMidiBuffer *port_buffer, jack_nframes_t frames,
                    int write_fd);

    };

}

#endif
