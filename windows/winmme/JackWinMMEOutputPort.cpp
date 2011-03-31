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
                                           const char *driver_name, UINT index,
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
    MMRESULT result = midiOutOpen(&handle, index, (DWORD)HandleMessageEvent, (DWORD)this,
                                  CALLBACK_FUNCTION);
    if (result != MMSYSERR_NOERROR) {
        GetErrorString(result, error_message);
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
    /*
    Devin : FIXME
    if (result != MMSYSERR_NOERROR) {
        WriteMMError("JackWinMMEOutputPort [constructor]", "midiOutGetDevCaps",
                     result);
        name_tmp = driver_name;
    } else {
        name_tmp = capabilities.szPname;
    }
    */
    snprintf(alias, sizeof(alias) - 1, "%s:%s:out%d", alias_name, driver_name,
             index + 1);
    snprintf(name, sizeof(name) - 1, "%s:playback_%d", client_name, index + 1);
    thread_ptr.release();
     thread_queue_ptr.release();
    return;

 destroy_thread_queue_semaphore:
    if (! CloseHandle(thread_queue_semaphore)) {
        WriteOSError("JackWinMMEOutputPort [constructor]", "CloseHandle");
    }
 close_handle:
    result = midiOutClose(handle);
    if (result != MMSYSERR_NOERROR) {
        WriteMMError("JackWinMMEOutputPort [constructor]", "midiOutClose",
                     result);
    }
 raise_exception:
    throw std::runtime_error(error_message);
}

JackWinMMEOutputPort::~JackWinMMEOutputPort()
{
    Stop();
    MMRESULT result = midiOutReset(handle);
    if (result != MMSYSERR_NOERROR) {
        WriteMMError("JackWinMMEOutputPort [destructor]", "midiOutReset",
                     result);
    }
    result = midiOutClose(handle);
    if (result != MMSYSERR_NOERROR) {
        WriteMMError("JackWinMMEOutputPort [destructor]", "midiOutClose",
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
}

bool
JackWinMMEOutputPort::Execute()
{
    for (;;) {
        if (! Wait(thread_queue_semaphore)) {
            break;
        }
        jack_midi_event_t *event = thread_queue->DequeueEvent();
        if (! event) {
            break;
        }
        jack_time_t frame_time = GetTimeFromFrames(event->time);
        for (jack_time_t current_time = GetMicroSeconds();
             frame_time > current_time; current_time = GetMicroSeconds()) {
            jack_time_t sleep_time = frame_time - current_time;

            // Windows has a millisecond sleep resolution for its Sleep calls.
            // This is unfortunate, as MIDI timing often requires a higher
            // resolution.  For now, we attempt to compensate by letting an
            // event be sent if we're less than 500 microseconds from sending
            // the event.  We assume that it's better to let an event go out
            // 499 microseconds early than let an event go out 501 microseconds
            // late.  Of course, that's assuming optimal sleep times, which is
            // a whole different Windows issue ...
            if (sleep_time < 500) {
                break;
            }

            if (sleep_time < 1000) {
                sleep_time = 1000;
            }
            JackSleep(sleep_time);
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
                WriteMMError("JackWinMMEOutputPort::Execute",
                             "midiOutShortMsg", result);
            }
            continue;
        }
        MIDIHDR header;
        header.dwBufferLength = size;
        header.dwBytesRecorded = size;
        header.dwFlags = 0;
        header.dwOffset = 0;
        header.dwUser = 0;
        header.lpData = (LPSTR)data;
        result = midiOutPrepareHeader(handle, &header, sizeof(MIDIHDR));
        if (result != MMSYSERR_NOERROR) {
            WriteMMError("JackWinMMEOutputPort::Execute",
                         "midiOutPrepareHeader", result);
            continue;
        }
        result = midiOutLongMsg(handle, &header, sizeof(MIDIHDR));
        if (result != MMSYSERR_NOERROR) {
            WriteMMError("JackWinMMEOutputPort::Execute", "midiOutLongMsg",
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
            WriteMMError("JackWinMMEOutputPort::Execute",
                         "midiOutUnprepareHeader", result);
            break;
        }
    }
 stop_execution:
    return false;
}

void
JackWinMMEOutputPort::GetMMErrorString(MMRESULT error, LPTSTR text)
{
    MMRESULT result = midiOutGetErrorText(error, text, MAXERRORLENGTH);
    if (result != MMSYSERR_NOERROR) {
        snprintf(text, MAXERRORLENGTH, "Unknown MM error code '%d'", error);
    }
}

void
JackWinMMEOutputPort::GetOSErrorString(LPTSTR text)
{
    DWORD error = GetLastError();
    /*
    Devin: FIXME
    if (! FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), &text,
                        MAXERRORLENGTH, NULL)) {
        snprintf(text, MAXERRORLENGTH, "Unknown OS error code '%d'", error);
    }
    */
}

void
JackWinMMEOutputPort::HandleMessage(UINT message, DWORD_PTR param1,
                                    DWORD_PTR param2)
{
    set_threaded_log_function();
    switch (message) {
    case MOM_CLOSE:
        jack_info("JackWinMMEOutputPort::HandleMessage - MIDI device closed.");
        break;
    case MOM_DONE:
        Signal(sysex_semaphore);
        break;
    case MOM_OPEN:
        jack_info("JackWinMMEOutputPort::HandleMessage - MIDI device opened.");
        break;
    case MOM_POSITIONCB:
        LPMIDIHDR header = (LPMIDIHDR) param2;
        jack_info("JackWinMMEOutputPort::HandleMessage - %d bytes out of %d "
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
    if (thread->AcquireSelfRealTime()) {
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
    bool result = thread->GetStatus() != JackThread::kIdle;
    if (! result) {
        result = ! thread->StartSync();
        if (! result) {
            jack_error("JackWinMMEOutputPort::Start - failed to start MIDI "
                       "processing thread.");
        }
    }
    return result;
}

bool
JackWinMMEOutputPort::Stop()
{

    jack_info("JackWinMMEOutputPort::Stop - stopping MIDI output port "
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
JackWinMMEOutputPort::WriteMMError(const char *jack_func, const char *mm_func,
                                   MMRESULT result)
{
    char error_message[MAXERRORLENGTH];
    GetMMErrorString(result, error_message);
    jack_error("%s - %s: %s", jack_func, mm_func, error_message);
}

void
JackWinMMEOutputPort::WriteOSError(const char *jack_func, const char *os_func)
{
    char error_message[MAXERRORLENGTH];
    // Devin : FIXME
    //GetOSErrorString(result, error_message);
    jack_error("%s - %s: %s", jack_func, os_func, error_message);
}

const char *
JackWinMMEOutputPort::GetAlias()
{
     return alias;
}
const char *
JackWinMMEOutputPort::GetName()
{
     return name;
}

void
JackWinMMEOutputPort::GetErrorString(MMRESULT error, LPTSTR text)
{
    MMRESULT result = midiInGetErrorText(error, text, MAXERRORLENGTH);
    if (result != MMSYSERR_NOERROR) {
        snprintf(text, MAXERRORLENGTH, "Unknown error code '%d'", error);
    }
}
