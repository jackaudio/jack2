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


#ifndef __JackWinNamedPipe__
#define __JackWinNamedPipe__

#include <windows.h>

#include "JackChannel.h"

namespace Jack
{

class JackWinNamedPipeAux
{

    protected:

        HANDLE fNamedPipe;
        char fName[256];

        int ReadAux(void* data, int len);
        int WriteAux(void* data, int len);

    public:

        JackWinNamedPipeAux(): fNamedPipe(INVALID_HANDLE_VALUE)
        {}
        JackWinNamedPipeAux(HANDLE pipe): fNamedPipe(pipe)
        {}
        virtual ~JackWinNamedPipeAux()
        {}

};


class JackWinNamedPipe : public JackWinNamedPipeAux, public detail::JackChannelTransactionInterface
{

    public:

        JackWinNamedPipe():JackWinNamedPipeAux()
        {}
        JackWinNamedPipe(HANDLE pipe):JackWinNamedPipeAux(pipe)
        {}
        virtual ~JackWinNamedPipe()
        {}

        virtual int Read(void* data, int len)
        {
            return ReadAux(data, len);
        }
        virtual int Write(void* data, int len)
        {
            return WriteAux(data, len);
        }
};

/*!
\brief Client named pipe.
*/

class JackWinNamedPipeClient : public JackWinNamedPipeAux, public detail::JackClientRequestInterface
{

    protected:

        int ConnectAux();

    public:

        JackWinNamedPipeClient():JackWinNamedPipeAux()
        {}
        JackWinNamedPipeClient(HANDLE pipe, const char* name):JackWinNamedPipeAux(pipe)
        {
            strcpy(fName, name);
        }

        virtual ~JackWinNamedPipeClient()
        {}

        virtual int Connect(const char* dir, int which);
        virtual int Connect(const char* dir, const char* name, int which);
        virtual int Close();

        virtual int Read(void* data, int len)
        {
            return ReadAux(data, len);
        }
        virtual int Write(void* data, int len)
        {
            return WriteAux(data, len);
        }

        virtual void SetReadTimeOut(long sec);
        virtual void SetWriteTimeOut(long sec);
        
        virtual void SetNonBlocking(bool onoff);
};

class JackWinAsyncNamedPipeClient : public JackWinNamedPipeClient
{
        enum kIOState {kIdle = 0, kConnecting, kReading, kWriting};

    private:

        bool fPendingIO;
        kIOState fIOState;
        OVERLAPPED fOverlap;

    public:

        JackWinAsyncNamedPipeClient();
        JackWinAsyncNamedPipeClient(HANDLE pipe, const char* name, bool pending);
        virtual ~JackWinAsyncNamedPipeClient();

        virtual int Read(void* data, int len);
        virtual int Write(void* data, int len);

        HANDLE GetEvent()
        {
            return (HANDLE)fOverlap.hEvent;
        }

        kIOState GetIOState()
        {
            return fIOState;
        }

        bool GetPending()
        {
            return fPendingIO;
        }

        int FinishIO();
};

/*!
\brief Server named pipe.
*/

class JackWinNamedPipeServer : public JackWinNamedPipe
{
    private:

        int BindAux();

    public:

        JackWinNamedPipeServer(): JackWinNamedPipe()
        {}
        virtual ~JackWinNamedPipeServer()
        {}

        virtual int Bind(const char* dir, int which);
        virtual int Bind(const char* dir, const char* name, int which);
        virtual bool Accept();
        virtual JackWinNamedPipeClient* AcceptClient();
        int Close();
};

/*!
\brief Server async named pipe.
*/

class JackWinAsyncNamedPipeServer : public JackWinNamedPipeServer
{

    private:

        int BindAux();

    public:

        JackWinAsyncNamedPipeServer(): JackWinNamedPipeServer()
        {}
        virtual ~JackWinAsyncNamedPipeServer()
        {}

        int Bind(const char* dir, int which);
        int Bind(const char* dir, const char* name, int which);
        bool Accept();
        JackWinNamedPipeClient* AcceptClient();
        int Close();
};

} // end of namespace


#endif

