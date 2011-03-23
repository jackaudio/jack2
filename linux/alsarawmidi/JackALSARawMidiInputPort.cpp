#include <memory>

#include "JackALSARawMidiInputPort.h"
#include "JackMidiUtil.h"

using Jack::JackALSARawMidiInputPort;

JackALSARawMidiInputPort::JackALSARawMidiInputPort(snd_rawmidi_info_t *info,
                                                   size_t index,
                                                   size_t max_bytes,
                                                   size_t max_messages):
    JackALSARawMidiPort(info, index)
{
    alsa_event = 0;
    jack_event = 0;
    receive_queue = new JackALSARawMidiReceiveQueue(rawmidi, max_bytes);
    std::auto_ptr<JackALSARawMidiReceiveQueue> receive_ptr(receive_queue);
    thread_queue = new JackMidiAsyncQueue(max_bytes, max_messages);
    std::auto_ptr<JackMidiAsyncQueue> thread_ptr(thread_queue);
    write_queue = new JackMidiBufferWriteQueue();
    std::auto_ptr<JackMidiBufferWriteQueue> write_ptr(write_queue);
    raw_queue = new JackMidiRawInputWriteQueue(thread_queue, max_bytes,
                                               max_messages);
    write_ptr.release();
    thread_ptr.release();
    receive_ptr.release();
}

JackALSARawMidiInputPort::~JackALSARawMidiInputPort()
{
    delete raw_queue;
    delete receive_queue;
    delete thread_queue;
    delete write_queue;
}

jack_nframes_t
JackALSARawMidiInputPort::EnqueueALSAEvent()
{
    switch (raw_queue->EnqueueEvent(alsa_event)) {
    case JackMidiWriteQueue::BUFFER_FULL:
        // Processing events early might free up some space in the raw queue.
        raw_queue->Process();
        switch (raw_queue->EnqueueEvent(alsa_event)) {
        case JackMidiWriteQueue::BUFFER_TOO_SMALL:
            jack_error("JackALSARawMidiInputPort::Process - **BUG** "
                       "JackMidiRawInputWriteQueue::EnqueueEvent returned "
                       "`BUFFER_FULL` and then returned `BUFFER_TOO_SMALL` "
                       "after a `Process()` call.");
            // Fallthrough on purpose
        case JackMidiWriteQueue::OK:
            return 0;
        default:
            ;
        }
        break;
    case JackMidiWriteQueue::BUFFER_TOO_SMALL:
        jack_error("JackALSARawMidiInputPort::Execute - The thread queue "
                   "couldn't enqueue a %d-byte packet.  Dropping event.",
                   alsa_event->size);
        // Fallthrough on purpose
    case JackMidiWriteQueue::OK:
        return 0;
    default:
        ;
    }
    jack_nframes_t alsa_time = alsa_event->time;
    jack_nframes_t next_time = raw_queue->Process();
    return (next_time < alsa_time) ? next_time : alsa_time;
}

jack_nframes_t
JackALSARawMidiInputPort::ProcessALSA()
{
    unsigned short revents = ProcessPollEvents();
    jack_nframes_t frame;
    if (alsa_event) {
        frame = EnqueueALSAEvent();
        if (frame) {
            return frame;
        }
    }
    if (revents & POLLIN) {
        for (alsa_event = receive_queue->DequeueEvent(); alsa_event;
             alsa_event = receive_queue->DequeueEvent()) {
            frame = EnqueueALSAEvent();
            if (frame) {
                return frame;
            }
        }
    }
    return raw_queue->Process();
}

void
JackALSARawMidiInputPort::ProcessJack(JackMidiBuffer *port_buffer,
                                      jack_nframes_t frames)
{
    write_queue->ResetMidiBuffer(port_buffer, frames);
    if (! jack_event) {
        jack_event = thread_queue->DequeueEvent();
    }
    for (; jack_event; jack_event = thread_queue->DequeueEvent()) {

        // We add `frames` so that MIDI events align with audio as closely as
        // possible.
        switch (write_queue->EnqueueEvent(jack_event->time + frames,
                                          jack_event->size,
                                          jack_event->buffer)) {
        case JackMidiWriteQueue::BUFFER_TOO_SMALL:
            jack_error("JackALSARawMidiInputPort::Process - The write queue "
                       "couldn't enqueue a %d-byte event.  Dropping event.",
                       jack_event->size);
            // Fallthrough on purpose
        case JackMidiWriteQueue::OK:
            continue;
        default:
            ;
        }
        break;
    }
}
