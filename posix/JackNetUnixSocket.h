/*
 Copyright (C) 2008-2011 Romain Moret at Grame

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

    //JackNetUnixSocket********************************************
    class SERVER_EXPORT JackNetUnixSocket
    {
        protected:

            int fFamily;
            int fSockfd;
            int fState;
            int fPort;
            int fTimeOut;
            struct sockaddr_storage fSendAddr;
            struct sockaddr_storage fRecvAddr;
            void Clone(const JackNetUnixSocket& socket);
            int ProbeAF(const char* ip, struct sockaddr_storage *addr, int (*call)(int, const struct sockaddr*, socklen_t));
            int BindMCastIface(const char *if_name, const int option, struct in_addr *addr);
            int BindMCast6Iface(const char *if_name, struct in6_addr *addr);

        private:

            char f_addr_buff[INET6_ADDRSTRLEN];

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
            int NewSocket(const char *ip);
            int NewSocket();
            int Bind();
            int Bind(const char *if_name);
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
            int SetSendIP(const char *ip);
            char* GetRecvIP();
            int SetRecvIP(const char *ip);

            //utility
            int GetName(char* name);
            int JoinMCastGroup(const char* mcast_ip);
            int JoinMCastGroup(const char* mcast_ip, const char* if_name);

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
            void PrintError();
    };
}

#endif
