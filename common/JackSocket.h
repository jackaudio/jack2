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

#ifndef __JackSocket__
#define __JackSocket__

#include "JackChannelTransaction.h"

#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h> 
#include <sys/time.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace Jack
{

/*!
\brief Client socket.
*/

class JackClientSocket : public JackChannelTransaction
{

    private:

        int fSocket;

    public:

        JackClientSocket(): fSocket( -1)
        {}
        JackClientSocket(int socket);
        virtual ~JackClientSocket()
        {}

        int Connect(const char* dir, int which);
        int Connect(const char* dir, const char* name, int which);
        int Close();
        int Read(void* data, int len);
        int Write(void* data, int len);
        int GetFd()
        {
            return fSocket;
        }
        void SetReadTimeOut(long sec);
        void SetWriteTimeOut(long sec);
        
        void SetNonBlocking(bool onoff);
};

/*!
\brief Server socket.
*/

class JackServerSocket
{

    private:

        int fSocket;
        char fName[256];

    public:

        JackServerSocket(): fSocket( -1)
        {}
        virtual ~JackServerSocket()
        {}

        int Bind(const char* dir, int which);
        int Bind(const char* dir, const char* name, int which);
        JackClientSocket* Accept();
        int Close();
        int GetFd()
        {
            return fSocket;
        }
};

} // end of namespace

#endif

