#include <memory>

#include "JackALSARawMidiOutputPort.h"

using Jack::JackALSARawMidiOutputPort;

JackALSARawMidiOutputPort::JackALSARawMidiOutputPort(snd_rawmidi_info_t *info,
                                                     size_t index,
                                                     size_t max_bytes,
                                                     size_t max_messages):
    JackALSARawMidiPort(info, index)
{
    alsa_event = 0;
    blocked = false;
    read_queue = new JackMidiBufferReadQueue();
    std::auto_ptr<JackMidiBufferReadQueue> read_ptr(read_queue);
    send_queue = new JackALSARawMidiSendQueue(rawmidi);
    std::auto_ptr<JackALSARawMidiSendQueue> send_ptr(send_queue);
    thread_queue = new JackMidiAsyncQueue(max_bytes, max_messages);
    std::auto_ptr<JackMidiAsyncQueue> thread_ptr(thread_queue);
    raw_queue = new JackMidiRawOutputWriteQueue(send_queue, max_bytes,
                                                max_messages, max_messages);
    thread_ptr.release();
    send_ptr.release();
    read_ptr.release();
}

JackALSARawMidiOutputPort::~JackALSARawMidiOutputPort()
{
    delete raw_queue;
    delete read_queue;
    delete send_queue;
    delete thread_queue;
}

jack_midi_event_t *
JackALSARawMidiOutputPort::DequeueALSAEvent(int read_fd)
{
    jack_midi_event_t *event = thread_queue->DequeueEvent();
    if (event) {
        char c;
        ssize_t result = read(read_fd, &c, 1);
        if (! result) {
            jack_error("JackALSARawMidiOutputPort::DequeueALSAEvent - **BUG** "
                       "An event was dequeued from the thread queue, but no "
                       "byte was available for reading from the pipe file "
                       "descriptor.");
        } else if (result < 0) {
            jack_error("JackALSARawMidiOutputPort::DequeueALSAEvent - error "
                       "reading a byte from the pipe file descriptor: %s",
                       strerror(errno));
        }
    }
    return event;
}

jack_nframes_t
JackALSARawMidiOutputPort::ProcessALSA(int read_fd)
{
    unsigned short revents = ProcessPollEvents();
    if (blocked) {
        if (! (revents & POLLOUT)) {
            return 0;
        }
        blocked = false;
    }
    if (! alsa_event) {
        alsa_event = DequeueALSAEvent(read_fd);
    }
    for (; alsa_event; alsa_event = DequeueALSAEvent(read_fd)) {
        switch (raw_queue->EnqueueEvent(alsa_event)) {
        case JackMidiWriteQueue::BUFFER_FULL:
            // Try to free up some space by processing events early.
            raw_queue->Process();
            switch (raw_queue->EnqueueEvent(alsa_event)) {
            case JackMidiWriteQueue::BUFFER_TOO_SMALL:
                jack_error("JackALSARawMidiOutputPort::ProcessALSA - **BUG** "
                           "JackMidiRawOutputWriteQueue::EnqueueEvent "
                           "returned `BUFFER_FULL`, and then returned "
                           "`BUFFER_TOO_SMALL` after a Process() call.");
                // Fallthrough on purpose
            case JackMidiWriteQueue::OK:
                continue;
            default:
                ;
            }
            goto process_events;
        case JackMidiWriteQueue::BUFFER_TOO_SMALL:
            jack_error("JackALSARawMidiOutputPort::ProcessALSA - The raw "
                       "output queue couldn't enqueue a %d-byte event.  "
                       "Dropping event.", alsa_event->size);
            // Fallthrough on purpose
        case JackMidiWriteQueue::OK:
            continue;
        default:
            ;
        }
        break;
    }
 process_events:
    jack_nframes_t next_frame = raw_queue->Process();
    blocked = send_queue->IsBlocked();
    if (blocked) {
        SetPollEventMask(POLLERR | POLLNVAL | POLLOUT);
        return 0;
    }
    SetPollEventMask(POLLERR | POLLNVAL);
    return next_frame;
}

void
JackALSARawMidiOutputPort::ProcessJack(JackMidiBuffer *port_buffer,
                                       jack_nframes_t frames, int write_fd)
{
    read_queue->ResetMidiBuffer(port_buffer);
    for (jack_midi_event_t *event = read_queue->DequeueEvent(); event;
         event = read_queue->DequeueEvent()) {
        if (event->size > thread_queue->GetAvailableSpace()) {
            jack_error("JackALSARawMidiOutputPort::ProcessJack - The thread "
                       "queue doesn't have enough room to enqueue a %d-byte "
                       "event.  Dropping event.", event->size);
            continue;
        }
        char c = 1;

        jack_info("Attempting to write to file descriptor '%d'", write_fd);

        ssize_t result = write(write_fd, &c, 1);
        assert(result <= 1);
        if (! result) {
            jack_error("JackALSARawMidiOutputPort::ProcessJack - Couldn't "
                       "write a byte to the pipe file descriptor.  Dropping "
                       "event.");
        } else if (result < 0) {
            jack_error("JackALSARawMidiOutputPort::ProcessJack - error "
                       "writing a byte to the pipe file descriptor: %s",
                       strerror(errno));
        } else if (thread_queue->EnqueueEvent(event->time + frames,
                                              event->size, event->buffer) !=
                   JackMidiWriteQueue::OK) {
            jack_error("JackALSARawMidiOutputPort::ProcessJack - **BUG** The "
                       "thread queue said it had enough space to enqueue a "
                       "%d-byte event, but failed to enqueue the event.");
        }
    }
}
