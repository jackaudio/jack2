/*
Copyright (C) 2008-2011 Romain Moret at Grame

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

#ifndef __JackNetUnixSocket__
#define __JackNetUnixSocket__

#include "JackNetSocket.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace Jack
{
#define NET_ERROR_CODE errno
#define SOCKET_ERROR -1
#define StrError strerror

    typedef struct sockaddr socket_address_t;
    typedef struct in_addr address_t;

    //JackNetUnixSocket********************************************
    class SERVER_EXPORT JackNetUnixSocket
    {
        private:

            int fSockfd;
            int fPort;
            int fTimeOut;

            struct sockaddr_in fSendAddr;
            struct sockaddr_in fRecvAddr;
        #if defined(__sun__) || defined(sun)
            int WaitRead();
            int WaitWrite();
        #endif

        public:

            JackNetUnixSocket();
            JackNetUnixSocket(const char* ip, int port);
            JackNetUnixSocket(const JackNetUnixSocket&);
            ~JackNetUnixSocket();

            JackNetUnixSocket& operator=(const JackNetUnixSocket& socket);

            //socket management
            int NewSocket();
            int Bind();
            int BindWith(const char* ip);
            int BindWith(int port);
            int Connect();
            int ConnectTo(const char* ip);
            void Close();
            void Reset();
            bool IsSocket();

            //IP/PORT management
            void SetPort(int port);
            int GetPort();

            //address management
            int SetAddress(const char* ip, int port);
            char* GetSendIP();
            char* GetRecvIP();

            //utility
            int GetName(char* name);
            int JoinMCastGroup(const char* mcast_ip);

            //options management
            int SetOption(int level, int optname, const void* optval, socklen_t optlen);
            int GetOption(int level, int optname, void* optval, socklen_t* optlen);

            //timeout
            int SetTimeOut(int us);

            //disable local loop
            int SetLocalLoop();

            bool IsLocal(char* ip);

            //network operations
            int SendTo(const void* buffer, size_t nbytes, int flags);
            int SendTo(const void* buffer, size_t nbytes, int flags, const char* ip);
            int Send(const void* buffer, size_t nbytes, int flags);
            int RecvFrom(void* buffer, size_t nbytes, int flags);
            int Recv(void* buffer, size_t nbytes, int flags);
            int CatchHost(void* buffer, size_t nbytes, int flags);

            //error management
            net_error_t GetError();
    };
}

#endif
