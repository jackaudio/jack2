/*
 Copyright (C) 2004-2008 Grame

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation; either version 2.1 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 */


#include "JackWinNamedPipe.h"
#include "JackError.h"
#include <assert.h>
#include <stdio.h>

#define BUFSIZE 4096

namespace Jack
{

int JackWinNamedPipeAux::ReadAux(void* data, int len)
{
    DWORD read;
    BOOL res = ReadFile(fNamedPipe, data, len, &read, NULL);
    if (res && read == (DWORD)len) {
        return 0;
    } else {
        jack_log("Cannot read named pipe name = %s err = %ld", fName, GetLastError());
        return -1;
    }
}

int JackWinNamedPipeAux::WriteAux(void* data, int len)
{
    DWORD written;
    BOOL res = WriteFile(fNamedPipe, data, len, &written, NULL);
    if (res && written == (DWORD)len) {
        return 0;
    } else {
        jack_log("Cannot write named pipe name = %s err = %ld", fName, GetLastError());
        return -1;
    }
}

/*
See :
    http://answers.google.com/answers/threadview?id=430173
    http://msdn.microsoft.com/en-us/library/windows/desktop/aa365800(v=vs.85).aspx
*/

/*
int JackWinNamedPipeClient::ConnectAux()
{
    fNamedPipe = CreateFile(fName, 			 // pipe name
                            GENERIC_READ |   // read and write access
                            GENERIC_WRITE,
                            0,               // no sharing
                            NULL,            // default security attributes
                            OPEN_EXISTING,   // opens existing pipe
                            0,               // default attributes
                            NULL);          // no template file

    if (fNamedPipe == INVALID_HANDLE_VALUE) {
        jack_error("Cannot connect to named pipe = %s err = %ld", fName, GetLastError());
        return -1;
    } else {
        return 0;
    }
}
*/

int JackWinNamedPipeClient::ConnectAux()
{
    jack_log("JackWinNamedPipeClient::ConnectAux : fName %s", fName);

    while (true) {

        fNamedPipe = CreateFile(fName, 			 // pipe name
                                GENERIC_READ |   // read and write access
                                GENERIC_WRITE,
                                0,               // no sharing
                                NULL,            // default security attributes
                                OPEN_EXISTING,   // opens existing pipe
                                0,               // default attributes
                                NULL);           // no template file

        // Break if the pipe handle is valid.
        if (fNamedPipe != INVALID_HANDLE_VALUE) {
            return 0;
        }

        // Exit if an error other than ERROR_PIPE_BUSY or ERROR_FILE_NOT_FOUND occurs.
        if ((GetLastError() != ERROR_PIPE_BUSY) && (GetLastError() != ERROR_FILE_NOT_FOUND)) {
            jack_error("Cannot connect to named pipe = %s err = %ld", fName, GetLastError());
            return -1;
        }

        // All pipe instances are busy, so wait for 2 seconds.
        if (!WaitNamedPipe(fName, 2000)) {
            jack_error("Cannot connect to named pipe after wait = %s err = %ld", fName, GetLastError());
            return -1;
        }
    }
}

int JackWinNamedPipeClient::Connect(const char* dir, int which)
{
    snprintf(fName, sizeof(fName), "\\\\.\\pipe\\%s_jack_%d", dir, which);
    return ConnectAux();
}

int JackWinNamedPipeClient::Connect(const char* dir, const char* name, int which)
{
    snprintf(fName, sizeof(fName), "\\\\.\\pipe\\%s_jack_%s_%d", dir, name, which);
    return ConnectAux();
}

int JackWinNamedPipeClient::Close()
{
    if (fNamedPipe != INVALID_HANDLE_VALUE) {
        CloseHandle(fNamedPipe);
        fNamedPipe = INVALID_HANDLE_VALUE;
        return 0;
    } else {
        return -1;
    }
}

void JackWinNamedPipeClient::SetReadTimeOut(long sec)
{
    /*
    COMMTIMEOUTS timeout;
    if (GetCommTimeouts(fNamedPipe, &timeout)) {
        jack_info("JackWinNamedPipeClient::SetReadTimeOut ReadIntervalTimeout = %d", timeout.ReadIntervalTimeout);
        jack_info("JackWinNamedPipeClient::SetReadTimeOut ReadTotalTimeoutMultiplier = %d", timeout.ReadTotalTimeoutMultiplier);
        jack_info("JackWinNamedPipeClient::SetReadTimeOut ReadTotalTimeoutConstant = %d", timeout.ReadTotalTimeoutConstant);
    } else {
        jack_error("JackWinNamedPipeClient::SetReadTimeOut err %d", GetLastError());
    }
    */
}

void JackWinNamedPipeClient::SetWriteTimeOut(long sec)
{
    /*
    COMMTIMEOUTS timeout;
    if (GetCommTimeouts(fNamedPipe, &timeout)) {
        jack_info("JackWinNamedPipeClient::SetWriteTimeOut WriteTotalTimeoutMultiplier = %d", timeout.WriteTotalTimeoutMultiplier);
        jack_info("JackWinNamedPipeClient::SetWriteTimeOut WriteTotalTimeoutConstant = %d", timeout.WriteTotalTimeoutConstant);
    }
    */
}

void JackWinNamedPipeClient::SetNonBlocking(bool onoff)
{}

JackWinAsyncNamedPipeClient::JackWinAsyncNamedPipeClient()
        : JackWinNamedPipeClient(), fPendingIO(false), fIOState(kIdle)
{
    fIOState = kIdle;
    fOverlap.hEvent = CreateEvent(NULL,     // default security attribute
                                  TRUE,     // manual-reset event
                                  TRUE,     // initial state = signaled
                                  NULL);    // unnamed event object
}

JackWinAsyncNamedPipeClient::JackWinAsyncNamedPipeClient(HANDLE pipe, const char* name, bool pending)
        : JackWinNamedPipeClient(pipe, name), fPendingIO(pending), fIOState(kIdle)
{
    fOverlap.hEvent = CreateEvent(NULL,     // default security attribute
                                  TRUE,     // manual-reset event
                                  TRUE,     // initial state = signaled
                                  NULL);	// unnamed event object

    if (!fPendingIO) {
        SetEvent(fOverlap.hEvent);
    }

    fIOState = (fPendingIO) ? kConnecting : kReading;
}

JackWinAsyncNamedPipeClient::~JackWinAsyncNamedPipeClient()
{
    CloseHandle(fOverlap.hEvent);
}

int JackWinAsyncNamedPipeClient::FinishIO()
{
    DWORD success, ret;
    success = GetOverlappedResult(fNamedPipe, 	// handle to pipe
                                  &fOverlap, 	// OVERLAPPED structure
                                  &ret,         // bytes transferred
                                  FALSE);       // do not wait

    switch (fIOState) {

        case kConnecting:
            if (!success) {
                jack_error("Conection error");
                return -1;
            } else {
                fIOState = kReading;
                // Prepare connection for new client ??
            }
            break;

        case kReading:
            if (!success || ret == 0) {
                return -1;
            }
            fIOState = kWriting;
            break;

        case kWriting:
            if (!success || ret == 0) {
                return -1;
            }
            fIOState = kReading;
            break;

        default:
            break;
    }

    return 0;
}

int JackWinAsyncNamedPipeClient::Read(void* data, int len)
{
    DWORD read;
    jack_log("JackWinNamedPipeClient::Read len = %ld", len);
    BOOL res = ReadFile(fNamedPipe, data, len, &read, &fOverlap);

    if (res && read != 0) {
        fPendingIO = false;
        fIOState = kWriting;
        return 0;
    } else if (!res && GetLastError() == ERROR_IO_PENDING) {
        fPendingIO = true;
        return 0;
    } else {
        jack_error("Cannot read named pipe err = %ld", GetLastError());
        return -1;
    }
}

int JackWinAsyncNamedPipeClient::Write(void* data, int len)
{
    DWORD written;
    jack_log("JackWinNamedPipeClient::Write len = %ld", len);
    BOOL res = WriteFile(fNamedPipe, data, len, &written, &fOverlap);

    if (res && written != 0) {
        fPendingIO = false;
        fIOState = kWriting;
        return 0;
    } else if (!res && GetLastError() == ERROR_IO_PENDING) {
        fPendingIO = true;
        return 0;
    } else {
        jack_error("Cannot write named pipe err = %ld", GetLastError());
        return -1;
    }
}

// Server side
int JackWinNamedPipeServer::BindAux()
{
    jack_log("JackWinNamedPipeServer::BindAux : fName %s", fName);

    if ((fNamedPipe = CreateNamedPipe(fName,
                                      PIPE_ACCESS_DUPLEX,       // read/write access
                                      PIPE_TYPE_MESSAGE |       // message type pipe
                                      PIPE_READMODE_MESSAGE |   // message-read mode
                                      PIPE_WAIT,                // blocking mode
                                      PIPE_UNLIMITED_INSTANCES, // max. instances
                                      BUFSIZE,  // output buffer size
                                      BUFSIZE,  // input buffer size
                                      INFINITE, // client time-out
                                      NULL)) == INVALID_HANDLE_VALUE) { // no security
        jack_error("Cannot bind server to pipe err = %ld", GetLastError());
        return -1;
    } else {
        return 0;
    }
}

int JackWinNamedPipeServer::Bind(const char* dir, int which)
{
     snprintf(fName, sizeof(fName), "\\\\.\\pipe\\%s_jack_%d", dir, which);
     return BindAux();
}

int JackWinNamedPipeServer::Bind(const char* dir, const char* name, int which)
{
    snprintf(fName, sizeof(fName), "\\\\.\\pipe\\%s_jack_%s_%d", dir, name, which);
    return BindAux();
}

bool JackWinNamedPipeServer::Accept()
{
    if (ConnectNamedPipe(fNamedPipe, NULL)) {
        return true;
    } else {
        jack_error("Cannot connect server pipe name = %s err = %ld", fName, GetLastError());
        if (GetLastError() == ERROR_PIPE_CONNECTED) {
            jack_error("Pipe already connnected = %s", fName);
            return true;
        } else {
            return false;
        }
    }
}

JackWinNamedPipeClient* JackWinNamedPipeServer::AcceptClient()
{
    if (ConnectNamedPipe(fNamedPipe, NULL)) {
        JackWinNamedPipeClient* client = new JackWinNamedPipeClient(fNamedPipe, fName);
        // Init the pipe to the default value
        fNamedPipe = INVALID_HANDLE_VALUE;
        return client;
    } else {
        switch (GetLastError()) {

            case ERROR_PIPE_CONNECTED:
                return new JackWinNamedPipeClient(fNamedPipe, fName);

            default:
                jack_error("Cannot connect server pipe name = %s  err = %ld", fName, GetLastError());
                return NULL;
        }
    }
}

int JackWinNamedPipeServer::Close()
{
    jack_log("JackWinNamedPipeServer::Close");

    if (fNamedPipe != INVALID_HANDLE_VALUE) {
        DisconnectNamedPipe(fNamedPipe);
        CloseHandle(fNamedPipe);
        fNamedPipe = INVALID_HANDLE_VALUE;
        return 0;
    } else {
        return -1;
    }
}

// Server side

int JackWinAsyncNamedPipeServer::BindAux()
{
    jack_log("JackWinAsyncNamedPipeServer::BindAux : fName %s", fName);

    if ((fNamedPipe = CreateNamedPipe(fName,
                                      PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,  // read/write access
                                      PIPE_TYPE_MESSAGE |  // message type pipe
                                      PIPE_READMODE_MESSAGE |  // message-read mode
                                      PIPE_WAIT,  // blocking mode
                                      PIPE_UNLIMITED_INSTANCES,  // max. instances
                                      BUFSIZE,  // output buffer size
                                      BUFSIZE,  // input buffer size
                                      INFINITE,  // client time-out
                                      NULL)) == INVALID_HANDLE_VALUE) { // no security a
        jack_error("Cannot bind server to pipe err = %ld", GetLastError());
        return -1;
    } else {
        return 0;
    }
}

int JackWinAsyncNamedPipeServer::Bind(const char* dir, int which)
{
    snprintf(fName, sizeof(fName), "\\\\.\\pipe\\%s_jack_%d", dir, which);
    return BindAux();
}

int JackWinAsyncNamedPipeServer::Bind(const char* dir, const char* name, int which)
{
    snprintf(fName, sizeof(fName), "\\\\.\\pipe\\%s_jack_%s_%d", dir, name, which);
    return BindAux();
}

bool JackWinAsyncNamedPipeServer::Accept()
{
    return false;
}

JackWinNamedPipeClient* JackWinAsyncNamedPipeServer::AcceptClient()
{
    if (ConnectNamedPipe(fNamedPipe, NULL)) {
        return new JackWinAsyncNamedPipeClient(fNamedPipe, fName, false);
    } else {
        switch (GetLastError()) {

            case ERROR_IO_PENDING:
                return new JackWinAsyncNamedPipeClient(fNamedPipe, fName, true);

            case ERROR_PIPE_CONNECTED:
                return new JackWinAsyncNamedPipeClient(fNamedPipe, fName, false);

            default:
                jack_error("Cannot connect server pipe name = %s err = %ld", fName, GetLastError());
                return NULL;
                break;
        }
    }
}

} // end of namespace

