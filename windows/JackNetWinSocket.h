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

#ifndef __JackNetWinSocket__
#define __JackNetWinSocket__

#include "JackNetSocket.h"
#ifdef __MINGW32__
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdint.h>
#endif

namespace Jack
{

#define E(code, s) { code, s }
#define NET_ERROR_CODE WSAGetLastError()
#define StrError PrintError

    typedef uint32_t uint;
    typedef int SOCKLEN;
    typedef struct _win_net_error win_net_error_t;

    struct _win_net_error
    {
        int code;
        const char* msg;
    };

    SERVER_EXPORT const char* PrintError(int error);

    //JeckNetWinSocket***************************************************************************
    class SERVER_EXPORT JackNetWinSocket
    {
        private:
            int fSockfd;
            int fPort;
            SOCKADDR_IN fSendAddr;
            SOCKADDR_IN fRecvAddr;
        public:
            JackNetWinSocket();
            JackNetWinSocket(const char* ip, int port);
            JackNetWinSocket(const JackNetWinSocket&);
            ~JackNetWinSocket();

            JackNetWinSocket& operator=(const JackNetWinSocket&);

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
            int SetOption(int level, int optname, const void* optval, SOCKLEN optlen);
            int GetOption(int level, int optname, void* optval, SOCKLEN* optlen);

            //timeout
            int SetTimeOut(int usec);

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

