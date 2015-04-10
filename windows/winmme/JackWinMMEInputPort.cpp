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

#include <cassert>
#include <memory>
#include <stdexcept>

#include "JackError.h"
#include "JackTime.h"
#include "JackMidiUtil.h"
#include "JackWinMMEInputPort.h"
#include "JackMidiWriteQueue.h"

using Jack::JackWinMMEInputPort;

///////////////////////////////////////////////////////////////////////////////
// Static callbacks
///////////////////////////////////////////////////////////////////////////////

void CALLBACK
JackWinMMEInputPort::HandleMidiInputEvent(HMIDIIN handle, UINT message,
                                          DWORD port, DWORD param1,
                                          DWORD param2)
{
    ((JackWinMMEInputPort *) port)->ProcessWinMME(message, param1, param2);
}

///////////////////////////////////////////////////////////////////////////////
// Class
///////////////////////////////////////////////////////////////////////////////

JackWinMMEInputPort::JackWinMMEInputPort(const char *alias_name,
                                         const char *client_name,
                                         const char *driver_name, UINT index,
                                         size_t max_bytes, size_t max_messages)
{
    thread_queue = new JackMidiAsyncQueue(max_bytes, max_messages);
    std::auto_ptr<JackMidiAsyncQueue> thread_queue_ptr(thread_queue);
    write_queue = new JackMidiBufferWriteQueue();
    std::auto_ptr<JackMidiBufferWriteQueue> write_queue_ptr(write_queue);
    sysex_buffer = new jack_midi_data_t[max_bytes];
    char error_message[MAXERRORLENGTH];
    MMRESULT result = midiInOpen(&handle, index,
                                 (DWORD_PTR) HandleMidiInputEvent,
                                 (DWORD_PTR)this,
                                 CALLBACK_FUNCTION | MIDI_IO_STATUS);
    if (result != MMSYSERR_NOERROR) {
        GetInErrorString(result, error_message);
        goto delete_sysex_buffer;
    }
    sysex_header.dwBufferLength = max_bytes;
    sysex_header.dwFlags = 0;
    sysex_header.lpData = (LPSTR)sysex_buffer;
    result = midiInPrepareHeader(handle, &sysex_header, sizeof(MIDIHDR));
    if (result != MMSYSERR_NOERROR) {
        GetInErrorString(result, error_message);
        goto close_handle;
    }
    result = midiInAddBuffer(handle, &sysex_header, sizeof(MIDIHDR));
    if (result != MMSYSERR_NOERROR) {
        GetInErrorString(result, error_message);
        goto unprepare_header;
    }

    MIDIINCAPS capabilities;
    char *name_tmp;
    result = midiInGetDevCaps(index, &capabilities, sizeof(capabilities));
    if (result != MMSYSERR_NOERROR) {
        WriteInError("JackWinMMEInputPort [constructor]", "midiInGetDevCaps",
                   result);
        name_tmp = (char*) driver_name;
    } else {
        name_tmp = capabilities.szPname;
    }

    snprintf(alias, sizeof(alias) - 1, "%s:%s:in%d", alias_name, name_tmp,
             index + 1);
    snprintf(name, sizeof(name) - 1, "%s:capture_%d", client_name, index + 1);
    jack_event = 0;
    started = false;
    write_queue_ptr.release();
    thread_queue_ptr.release();
    return;

 unprepare_header:
    result = midiInUnprepareHeader(handle, &sysex_header, sizeof(MIDIHDR));
    if (result != MMSYSERR_NOERROR) {
        WriteInError("JackWinMMEInputPort [constructor]",
                     "midiInUnprepareHeader", result);
    }
 close_handle:
    result = midiInClose(handle);
    if (result != MMSYSERR_NOERROR) {
        WriteInError("JackWinMMEInputPort [constructor]", "midiInClose",
                     result);
    }
 delete_sysex_buffer:
    delete[] sysex_buffer;
    throw std::runtime_error(error_message);
}

JackWinMMEInputPort::~JackWinMMEInputPort()
{
    MMRESULT result = midiInReset(handle);
    if (result != MMSYSERR_NOERROR) {
        WriteInError("JackWinMMEInputPort [destructor]", "midiInReset", result);
    }
    result = midiInUnprepareHeader(handle, &sysex_header, sizeof(MIDIHDR));
    if (result != MMSYSERR_NOERROR) {
        WriteInError("JackWinMMEInputPort [destructor]",
                     "midiInUnprepareHeader", result);
    }
    result = midiInClose(handle);
    if (result != MMSYSERR_NOERROR) {
        WriteInError("JackWinMMEInputPort [destructor]", "midiInClose", result);
    }
    delete[] sysex_buffer;
    delete thread_queue;
    delete write_queue;
}

void
JackWinMMEInputPort::EnqueueMessage(DWORD timestamp, size_t length,
                                    jack_midi_data_t *data)
{
    jack_nframes_t frame =
        GetFramesFromTime(start_time + (((jack_time_t) timestamp) * 1000));

    // Debugging code
    jack_time_t current_time = GetMicroSeconds();
    jack_log("JackWinMMEInputPort::EnqueueMessage - enqueueing event at %f "
             "(frame: %d) with start offset '%d' scheduled for frame '%d'",
             ((double) current_time) / 1000.0, GetFramesFromTime(current_time),
             timestamp, frame);
    // End debugging code

    switch (thread_queue->EnqueueEvent(frame, length, data)) {
    case JackMidiWriteQueue::BUFFER_FULL:
        jack_error("JackWinMMEInputPort::EnqueueMessage - The thread queue "
                   "cannot currently accept a %d-byte event.  Dropping event.",
                   length);
        break;
    case JackMidiWriteQueue::BUFFER_TOO_SMALL:
        jack_error("JackWinMMEInputPort::EnqueueMessage - The thread queue "
                   "buffer is too small to enqueue a %d-byte event.  Dropping "
                   "event.", length);
        break;
    default:
        ;
    }
}

void
JackWinMMEInputPort::GetInErrorString(MMRESULT error, LPTSTR text)
{
    MMRESULT result = midiInGetErrorText(error, text, MAXERRORLENGTH);
    if (result != MMSYSERR_NOERROR) {
        snprintf(text, MAXERRORLENGTH, "Unknown error code '%d'", error);
    }
}

void
JackWinMMEInputPort::ProcessJack(JackMidiBuffer *port_buffer,
                                 jack_nframes_t frames)
{
    write_queue->ResetMidiBuffer(port_buffer, frames);
    if (! jack_event) {
        jack_event = thread_queue->DequeueEvent();
    }
    for (; jack_event; jack_event = thread_queue->DequeueEvent()) {
        switch (write_queue->EnqueueEvent(jack_event, frames)) {
        case JackMidiWriteQueue::BUFFER_TOO_SMALL:
            jack_error("JackWinMMEMidiInputPort::Process - The buffer write "
                       "queue couldn't enqueue a %d-byte event. Dropping "
                       "event.", jack_event->size);
            // Fallthrough on purpose
        case JackMidiWriteQueue::OK:
            continue;
        default:
            break;
        }
        break;
    }
}

void
JackWinMMEInputPort::ProcessWinMME(UINT message, DWORD param1, DWORD param2)
{
    set_threaded_log_function();
    switch (message) {
    case MIM_CLOSE:
        jack_log("JackWinMMEInputPort::ProcessWinMME - MIDI device closed.");
        break;
    case MIM_MOREDATA:
        jack_log("JackWinMMEInputPort::ProcessWinMME - The MIDI input device "
                  "driver thinks that JACK is not processing messages fast "
                  "enough.");
        // Fallthrough on purpose.
    case MIM_DATA: {
        jack_midi_data_t message_buffer[3];
        jack_midi_data_t status = param1 & 0xff;
        int length = GetMessageLength(status);

        switch (length) {
        case 3:
             message_buffer[2] = (param1 >> 16)  & 0xff;
            // Fallthrough on purpose.
        case 2:
            message_buffer[1] = (param1 >> 8) & 0xff;
            // Fallthrough on purpose.
        case 1:
            message_buffer[0] = status;
            break;
        case 0:
            jack_error("JackWinMMEInputPort::ProcessWinMME - **BUG** MIDI "
                       "input driver sent an MIM_DATA message with a sysex "
                       "status byte.");
            return;
        case -1:
            jack_error("JackWinMMEInputPort::ProcessWinMME - **BUG** MIDI "
                       "input driver sent an MIM_DATA message with an invalid "
                       "status byte.");
            return;
        }
        EnqueueMessage(param2, (size_t) length, message_buffer);
        break;
    }
    case MIM_LONGDATA: {
        LPMIDIHDR header = (LPMIDIHDR) param1;
        size_t byte_count = header->dwBytesRecorded;
        if (! byte_count) {
            jack_log("JackWinMMEInputPort::ProcessWinMME - WinMME driver has "
                      "returned sysex header to us with no bytes.  The JACK "
                      "driver is probably being stopped.");
            break;
        }
        jack_midi_data_t *data = (jack_midi_data_t *) header->lpData;
        if ((data[0] != 0xf0) || (data[byte_count - 1] != 0xf7)) {
            jack_error("JackWinMMEInputPort::ProcessWinMME - Discarding "
                       "%d-byte sysex chunk.", byte_count);
        } else {
            EnqueueMessage(param2, byte_count, data);
        }
        // Is this realtime-safe?  This function isn't run in the JACK thread,
        // but we still want it to perform as quickly as possible.  Even if
        // this isn't realtime safe, it may not be avoidable.
        MMRESULT result = midiInAddBuffer(handle, &sysex_header,
                                          sizeof(MIDIHDR));
        if (result != MMSYSERR_NOERROR) {
            WriteInError("JackWinMMEInputPort::ProcessWinMME",
                         "midiInAddBuffer", result);
        }
        break;
    }
    case MIM_LONGERROR:
        jack_error("JackWinMMEInputPort::ProcessWinMME - Invalid or "
                   "incomplete sysex message received.");
        break;
    case MIM_OPEN:
        jack_log("JackWinMMEInputPort::ProcessWinMME - MIDI device opened.");
    }
}

bool
JackWinMMEInputPort::Start()
{
    if (! started) {
        start_time = GetMicroSeconds();
        MMRESULT result = midiInStart(handle);
        started = result == MMSYSERR_NOERROR;
        if (! started) {
            WriteInError("JackWinMMEInputPort::Start", "midiInStart", result);
        }
    }
    return started;
}

bool
JackWinMMEInputPort::Stop()
{
    if (started) {
        MMRESULT result = midiInStop(handle);
        started = result != MMSYSERR_NOERROR;
        if (started) {
            WriteInError("JackWinMMEInputPort::Stop", "midiInStop", result);
        }
    }
    return ! started;
}

void
JackWinMMEInputPort::WriteInError(const char *jack_func, const char *mm_func,
                                MMRESULT result)
{
    char error_message[MAXERRORLENGTH];
    GetInErrorString(result, error_message);
    jack_error("%s - %s: %s", jack_func, mm_func, error_message);
}
