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

#include <memory>
#include <stdexcept>

#include "JackMidiUtil.h"
#include "JackTime.h"
#include "JackWinMMEOutputPort.h"
#include "JackGlobals.h"
#include "JackEngineControl.h"

using Jack::JackWinMMEOutputPort;

///////////////////////////////////////////////////////////////////////////////
// Static callbacks
///////////////////////////////////////////////////////////////////////////////

void CALLBACK
JackWinMMEOutputPort::HandleMessageEvent(HMIDIOUT handle, UINT message,
                                         DWORD_PTR port, DWORD_PTR param1,
                                         DWORD_PTR param2)
{
    ((JackWinMMEOutputPort *) port)->HandleMessage(message, param1, param2);
}

///////////////////////////////////////////////////////////////////////////////
// Class
///////////////////////////////////////////////////////////////////////////////

JackWinMMEOutputPort::JackWinMMEOutputPort(const char *alias_name,
                                           const char *client_name,
                                           const char *driver_name,
                                           UINT index,
                                           size_t max_bytes,
                                           size_t max_messages)
{
    read_queue = new JackMidiBufferReadQueue();
    std::auto_ptr<JackMidiBufferReadQueue> read_queue_ptr(read_queue);
    thread_queue = new JackMidiAsyncQueue(max_bytes, max_messages);
    std::auto_ptr<JackMidiAsyncQueue> thread_queue_ptr(thread_queue);
    thread = new JackThread(this);
    std::auto_ptr<JackThread> thread_ptr(thread);
    char error_message[MAXERRORLENGTH];
    MMRESULT result = midiOutOpen(&handle, index, (DWORD_PTR)HandleMessageEvent,
                                  (DWORD_PTR)this, CALLBACK_FUNCTION);
    if (result != MMSYSERR_NOERROR) {
        GetOutErrorString(result, error_message);
        goto raise_exception;
    }
    thread_queue_semaphore = CreateSemaphore(NULL, 0, max_messages, NULL);
    if (thread_queue_semaphore == NULL) {
        GetOSErrorString(error_message);
        goto close_handle;
    }
    sysex_semaphore = CreateSemaphore(NULL, 0, 1, NULL);
    if (sysex_semaphore == NULL) {
        GetOSErrorString(error_message);
        goto destroy_thread_queue_semaphore;
    }
    MIDIOUTCAPS capabilities;
    char *name_tmp;
    result = midiOutGetDevCaps(index, &capabilities, sizeof(capabilities));
    if (result != MMSYSERR_NOERROR) {
        WriteOutError("JackWinMMEOutputPort [constructor]", "midiOutGetDevCaps",
                     result);
        name_tmp = (char*)driver_name;
    } else {
        name_tmp = capabilities.szPname;
    }
    snprintf(alias, sizeof(alias) - 1, "%s:%s:out%d", alias_name, name_tmp,
             index + 1);
    snprintf(name, sizeof(name) - 1, "%s:playback_%d", client_name, index + 1);
    read_queue_ptr.release();
    thread_queue_ptr.release();
    thread_ptr.release();
    return;

 destroy_thread_queue_semaphore:
    if (! CloseHandle(thread_queue_semaphore)) {
        WriteOSError("JackWinMMEOutputPort [constructor]", "CloseHandle");
    }
 close_handle:
    result = midiOutClose(handle);
    if (result != MMSYSERR_NOERROR) {
        WriteOutError("JackWinMMEOutputPort [constructor]", "midiOutClose",
                     result);
    }
 raise_exception:
    throw std::runtime_error(error_message);
}

JackWinMMEOutputPort::~JackWinMMEOutputPort()
{
    MMRESULT result = midiOutReset(handle);
    if (result != MMSYSERR_NOERROR) {
        WriteOutError("JackWinMMEOutputPort [destructor]", "midiOutReset",
                     result);
    }
    result = midiOutClose(handle);
    if (result != MMSYSERR_NOERROR) {
        WriteOutError("JackWinMMEOutputPort [destructor]", "midiOutClose",
                     result);
    }
    if (! CloseHandle(sysex_semaphore)) {
        WriteOSError("JackWinMMEOutputPort [destructor]", "CloseHandle");
    }
    if (! CloseHandle(thread_queue_semaphore)) {
        WriteOSError("JackWinMMEOutputPort [destructor]", "CloseHandle");
    }
    delete read_queue;
    delete thread_queue;
    delete thread;
}

bool
JackWinMMEOutputPort::Execute()
{
    for (;;) {
        if (! Wait(thread_queue_semaphore)) {
            jack_log("JackWinMMEOutputPort::Execute BREAK");

            break;
        }
        jack_midi_event_t *event = thread_queue->DequeueEvent();
        if (! event) {
            break;
        }
        jack_time_t frame_time = GetTimeFromFrames(event->time);
        jack_time_t current_time = GetMicroSeconds();
        if (frame_time > current_time) {
            LARGE_INTEGER due_time;

            // 100 ns resolution
            due_time.QuadPart =
                -((LONGLONG) ((frame_time - current_time) * 10));
            if (! SetWaitableTimer(timer, &due_time, 0, NULL, NULL, 0)) {
                WriteOSError("JackWinMMEOutputPort::Execute",
                             "SetWaitableTimer");
                break;
            }

            // Debugging code
            jack_log("JackWinMMEOutputPort::Execute - waiting at %f for %f "
                     "milliseconds before sending message (current frame: %d, "
                     "send frame: %d)",
                     ((double) current_time) / 1000.0,
                     ((double) (frame_time - current_time)) / 1000.0,
                     GetFramesFromTime(current_time), event->time);
            // End debugging code

            if (! Wait(timer)) {
                break;
            }

            // Debugging code
            jack_time_t wakeup_time = GetMicroSeconds();
            jack_log("JackWinMMEOutputPort::Execute - woke up at %f.",
                     ((double) wakeup_time) / 1000.0);
            if (wakeup_time > frame_time) {
                jack_log("JackWinMMEOutputPort::Execute - overslept by %f "
                         "milliseconds.",
                         ((double) (wakeup_time - frame_time)) / 1000.0);
            } else if (wakeup_time < frame_time) {
                jack_log("JackWinMMEOutputPort::Execute - woke up %f "
                         "milliseconds too early.",
                         ((double) (frame_time - wakeup_time)) / 1000.0);
            }
            // End debugging code

        }
        jack_midi_data_t *data = event->buffer;
        DWORD message = 0;
        MMRESULT result;
        size_t size = event->size;
        switch (size) {
        case 3:
            message |= (((DWORD) data[2]) << 16);
            // Fallthrough on purpose.
        case 2:
            message |= (((DWORD) data[1]) << 8);
            // Fallthrough on purpose.
        case 1:
            message |= (DWORD) data[0];
            result = midiOutShortMsg(handle, message);
            if (result != MMSYSERR_NOERROR) {
                WriteOutError("JackWinMMEOutputPort::Execute",
                              "midiOutShortMsg", result);
            }
            continue;
        }
        MIDIHDR header;
        header.dwBufferLength = size;
        header.dwFlags = 0;
        header.lpData = (LPSTR) data;
        result = midiOutPrepareHeader(handle, &header, sizeof(MIDIHDR));
        if (result != MMSYSERR_NOERROR) {
            WriteOutError("JackWinMMEOutputPort::Execute",
                          "midiOutPrepareHeader", result);
            continue;
        }
        result = midiOutLongMsg(handle, &header, sizeof(MIDIHDR));
        if (result != MMSYSERR_NOERROR) {
            WriteOutError("JackWinMMEOutputPort::Execute", "midiOutLongMsg",
                          result);
            continue;
        }

        // System exclusive messages may be sent synchronously or
        // asynchronously.  The choice is up to the WinMME driver.  So, we wait
        // until the message is sent, regardless of the driver's choice.
        if (! Wait(sysex_semaphore)) {
            break;
        }

        result = midiOutUnprepareHeader(handle, &header, sizeof(MIDIHDR));
        if (result != MMSYSERR_NOERROR) {
            WriteOutError("JackWinMMEOutputPort::Execute",
                          "midiOutUnprepareHeader", result);
            break;
        }
    }
    return false;
}

void
JackWinMMEOutputPort::GetOutErrorString(MMRESULT error, LPTSTR text)
{
    MMRESULT result = midiOutGetErrorText(error, text, MAXERRORLENGTH);
    if (result != MMSYSERR_NOERROR) {
        snprintf(text, MAXERRORLENGTH, "Unknown MM error code '%d'", error);
    }
}

void
JackWinMMEOutputPort::HandleMessage(UINT message, DWORD_PTR param1,
                                    DWORD_PTR param2)
{
    set_threaded_log_function();
    switch (message) {
    case MOM_CLOSE:
        jack_log("JackWinMMEOutputPort::HandleMessage - MIDI device closed.");
        break;
    case MOM_DONE:
        Signal(sysex_semaphore);
        break;
    case MOM_OPEN:
        jack_log("JackWinMMEOutputPort::HandleMessage - MIDI device opened.");
        break;
    case MOM_POSITIONCB:
        LPMIDIHDR header = (LPMIDIHDR) param1;
        jack_log("JackWinMMEOutputPort::HandleMessage - %d bytes out of %d "
                  "bytes of the current sysex message have been sent.",
                  header->dwOffset, header->dwBytesRecorded);
    }
}

bool
JackWinMMEOutputPort::Init()
{
    set_threaded_log_function();
    // XX: Can more be done?  Ideally, this thread should have the JACK server
    // thread priority + 1.
    if (thread->AcquireSelfRealTime(GetEngineControl()->fServerPriority)) {
        jack_error("JackWinMMEOutputPort::Init - could not acquire realtime "
                   "scheduling. Continuing anyway.");
    }
    return true;
}

void
JackWinMMEOutputPort::ProcessJack(JackMidiBuffer *port_buffer,
                                  jack_nframes_t frames)
{
    read_queue->ResetMidiBuffer(port_buffer);
    for (jack_midi_event_t *event = read_queue->DequeueEvent(); event;
        event = read_queue->DequeueEvent()) {

        switch (thread_queue->EnqueueEvent(event, frames)) {
        case JackMidiWriteQueue::BUFFER_FULL:
            jack_error("JackWinMMEOutputPort::ProcessJack - The thread queue "
                       "buffer is full.  Dropping event.");
            break;
        case JackMidiWriteQueue::BUFFER_TOO_SMALL:
            jack_error("JackWinMMEOutputPort::ProcessJack - The thread queue "
                       "couldn't enqueue a %d-byte event.  Dropping event.",
                       event->size);
            break;
        default:
            Signal(thread_queue_semaphore);
        }
    }
}

bool
JackWinMMEOutputPort::Signal(HANDLE semaphore)
{
    bool result = (bool) ReleaseSemaphore(semaphore, 1, NULL);
    if (! result) {
        WriteOSError("JackWinMMEOutputPort::Signal", "ReleaseSemaphore");
    }
    return result;
}

bool
JackWinMMEOutputPort::Start()
{
    if (thread->GetStatus() != JackThread::kIdle) {
        return true;
    }
    timer = CreateWaitableTimer(NULL, FALSE, NULL);
    if (! timer) {
        WriteOSError("JackWinMMEOutputPort::Start", "CreateWaitableTimer");
        return false;
    }
    if (! thread->StartSync()) {
        return true;
    }
    jack_error("JackWinMMEOutputPort::Start - failed to start MIDI processing "
               "thread.");
    if (! CloseHandle(timer)) {
        WriteOSError("JackWinMMEOutputPort::Start", "CloseHandle");
    }
    return false;
}

bool
JackWinMMEOutputPort::Stop()
{
    jack_log("JackWinMMEOutputPort::Stop - stopping MIDI output port "
              "processing thread.");

    int result;
    const char *verb;
    switch (thread->GetStatus()) {
    case JackThread::kIniting:
    case JackThread::kStarting:
        result = thread->Kill();
        verb = "kill";
        break;
    case JackThread::kRunning:
        Signal(thread_queue_semaphore);
        result = thread->Stop();
        verb = "stop";
        break;
    default:
        return true;
    }
    if (result) {
        jack_error("JackWinMMEOutputPort::Stop - could not %s MIDI processing "
                   "thread.", verb);
    }
    if (! CloseHandle(timer)) {
        WriteOSError("JackWinMMEOutputPort::Stop", "CloseHandle");
        result = -1;
    }
    return ! result;
}

bool
JackWinMMEOutputPort::Wait(HANDLE semaphore)
{
    DWORD result = WaitForSingleObject(semaphore, INFINITE);
    switch (result) {
    case WAIT_FAILED:
        WriteOSError("JackWinMMEOutputPort::Wait", "WaitForSingleObject");
        break;
    case WAIT_OBJECT_0:
        return true;
    default:
        jack_error("JackWinMMEOutputPort::Wait - unexpected result from "
                   "'WaitForSingleObject'.");
    }
    return false;
}

void
JackWinMMEOutputPort::WriteOutError(const char *jack_func, const char *mm_func,
                                   MMRESULT result)
{
    char error_message[MAXERRORLENGTH];
    GetOutErrorString(result, error_message);
    jack_error("%s - %s: %s", jack_func, mm_func, error_message);
}
