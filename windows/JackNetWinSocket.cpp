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

#include "JackError.h"
#include "JackNetWinSocket.h"

namespace Jack
{
    //utility *********************************************************************************************************
    SERVER_EXPORT int GetHostName(char * name, int size)
    {
        if (gethostname(name, size) == SOCKET_ERROR) {
            jack_error("Can't get 'hostname' : %s", strerror(NET_ERROR_CODE));
            strcpy(name, "default");
            return -1;
        }
        return 0;
    }

    win_net_error_t NetErrorList[] =
    {
        E(0,                  "No error"),
        E(WSAEINTR,           "Interrupted system call"),
        E(WSAEBADF,           "Bad file number"),
        E(WSAEACCES,          "Permission denied"),
        E(WSAEFAULT,          "Bad address"),
        E(WSAEINVAL,          "Invalid argument"),
        E(WSAEMFILE,          "Too many open sockets"),
        E(WSAEWOULDBLOCK,     "Operation would block"),
        E(WSAEINPROGRESS,     "Operation now in progress"),
        E(WSAEALREADY,        "Operation already in progress"),
        E(WSAENOTSOCK,        "Socket operation on non-socket"),
        E(WSAEDESTADDRREQ,    "Destination address required"),
        E(WSAEMSGSIZE,        "Message too long"),
        E(WSAEPROTOTYPE,      "Protocol wrong type for socket"),
        E(WSAENOPROTOOPT,     "Bad protocol option"),
        E(WSAEPROTONOSUPPORT, "Protocol not supported"),
        E(WSAESOCKTNOSUPPORT, "Socket type not supported"),
        E(WSAEOPNOTSUPP,      "Operation not supported on socket"),
        E(WSAEPFNOSUPPORT,    "Protocol family not supported"),
        E(WSAEAFNOSUPPORT,    "Address family not supported"),
        E(WSAEADDRINUSE,      "Address already in use"),
        E(WSAEADDRNOTAVAIL,   "Can't assign requested address"),
        E(WSAENETDOWN,        "Network is down"),
        E(WSAENETUNREACH,     "Network is unreachable"),
        E(WSAENETRESET,       "Net connection reset"),
        E(WSAECONNABORTED,    "Software caused connection abort"),
        E(WSAECONNRESET,      "Connection reset by peer"),
        E(WSAENOBUFS,         "No buffer space available"),
        E(WSAEISCONN,         "Socket is already connected"),
        E(WSAENOTCONN,        "Socket is not connected"),
        E(WSAESHUTDOWN,       "Can't send after socket shutdown"),
        E(WSAETOOMANYREFS,    "Too many references, can't splice"),
        E(WSAETIMEDOUT,       "Connection timed out"),
        E(WSAECONNREFUSED,    "Connection refused"),
        E(WSAELOOP,           "Too many levels of symbolic links"),
        E(WSAENAMETOOLONG,    "File name too long"),
        E(WSAEHOSTDOWN,       "Host is down"),
        E(WSAEHOSTUNREACH,    "No route to host"),
        E(WSAENOTEMPTY,       "Directory not empty"),
        E(WSAEPROCLIM,        "Too many processes"),
        E(WSAEUSERS,          "Too many users"),
        E(WSAEDQUOT,          "Disc quota exceeded"),
        E(WSAESTALE,          "Stale NFS file handle"),
        E(WSAEREMOTE,         "Too many levels of remote in path"),
        E(WSASYSNOTREADY,     "Network system is unavailable"),
        E(WSAVERNOTSUPPORTED, "Winsock version out of range"),
        E(WSANOTINITIALISED,  "WSAStartup not yet called"),
        E(WSAEDISCON,         "Graceful shutdown in progress"),
        E(WSAHOST_NOT_FOUND,  "Host not found"),
        E(WSANO_DATA,         "No host data of that type was found"),
        { -1, NULL },
    };

    SERVER_EXPORT const char* PrintError(int error)
    {
        int i;
        for (i = 0; NetErrorList[i].code >= 0; ++i) {
            if (error == NetErrorList[i].code)
                return NetErrorList[i].msg;
        }
        return strerror(error);
    }

    //construct/destruct***********************************************************************************************
    JackNetWinSocket::JackNetWinSocket()
    {
        fSockfd = 0;
        fSendAddr.sin_family = AF_INET;
        fSendAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        memset(&fSendAddr.sin_zero, 0, 8);
        fRecvAddr.sin_family = AF_INET;
        fRecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        memset(&fRecvAddr.sin_zero, 0, 8);
    }

    JackNetWinSocket::JackNetWinSocket(const char* ip, int port)
    {
        fSockfd = 0;
        fPort = port;
        fSendAddr.sin_family = AF_INET;
        fSendAddr.sin_port = htons(port);
        fSendAddr.sin_addr.s_addr = inet_addr(ip);
        memset(&fSendAddr.sin_zero, 0, 8);
        fRecvAddr.sin_family = AF_INET;
        fRecvAddr.sin_port = htons(port);
        fRecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        memset(&fRecvAddr.sin_zero, 0, 8);
    }

    JackNetWinSocket::JackNetWinSocket(const JackNetWinSocket& socket)
    {
        fSockfd = 0;
        fPort = socket.fPort;
        fSendAddr = socket.fSendAddr;
        fRecvAddr = socket.fRecvAddr;
    }

    JackNetWinSocket::~JackNetWinSocket()
    {
        Close();
    }

    JackNetWinSocket& JackNetWinSocket::operator=(const JackNetWinSocket& socket)
    {
        if (this != &socket) {
            fSockfd = 0;
            fPort = socket.fPort;
            fSendAddr = socket.fSendAddr;
            fRecvAddr = socket.fRecvAddr;
        }
        return *this;
    }

    //socket***********************************************************************************************************
    int JackNetWinSocket::NewSocket()
    {
        if (fSockfd) {
            Close();
            Reset();
        }
        fSockfd = socket(AF_INET, SOCK_DGRAM, 0);
        return fSockfd;
    }

    bool JackNetWinSocket::IsLocal(char* ip)
    {
        if (strcmp(ip, "127.0.0.1") == 0) {
            return true;
        }

        char host_name[32];
        gethostname(host_name, sizeof(host_name));

        struct hostent* host = gethostbyname(host_name);
        if (host) {
            for (int i = 0; host->h_addr_list[i] != 0; ++i) {
                struct in_addr addr;
                memcpy(&addr, host->h_addr_list[i], sizeof(struct in_addr));
                if (strcmp(inet_ntoa(addr), ip) == 0) {
                    return true;
                }
            }
            return false;
        } else {
            return false;
        }
    }

    int JackNetWinSocket::Bind()
    {
        return bind(fSockfd, reinterpret_cast<SOCKADDR*>(&fRecvAddr), sizeof(SOCKADDR));
    }

    int JackNetWinSocket::BindWith(const char* ip)
    {
        fRecvAddr.sin_addr.s_addr = inet_addr(ip);
        return Bind();
    }

    int JackNetWinSocket::BindWith(int port)
    {
        fRecvAddr.sin_port = htons(port);
        return Bind();
    }

    int JackNetWinSocket::Connect()
    {
        return connect(fSockfd, reinterpret_cast<SOCKADDR*>(&fSendAddr), sizeof(SOCKADDR));
    }

    int JackNetWinSocket::ConnectTo(const char* ip)
    {
        fSendAddr.sin_addr.s_addr = inet_addr(ip);
        return Connect();
    }

    void JackNetWinSocket::Close()
    {
        if (fSockfd)
            closesocket(fSockfd);
        fSockfd = 0;
    }

    void JackNetWinSocket::Reset()
    {
        fSendAddr.sin_family = AF_INET;
        fSendAddr.sin_port = htons(fPort);
        fSendAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        memset(&fSendAddr.sin_zero, 0, 8);
        fRecvAddr.sin_family = AF_INET;
        fRecvAddr.sin_port = htons(fPort);
        fRecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        memset(&fRecvAddr.sin_zero, 0, 8);
    }

    bool JackNetWinSocket::IsSocket()
    {
        return(fSockfd) ? true : false;
    }

    //IP/PORT***********************************************************************************************************
    void JackNetWinSocket::SetPort(int port)
    {
        fPort = port;
        fSendAddr.sin_port = htons(port);
        fRecvAddr.sin_port = htons(port);
    }

    int JackNetWinSocket::GetPort()
    {
        return fPort;
    }

    //address***********************************************************************************************************
    int JackNetWinSocket::SetAddress(const char* ip, int port)
    {
        fSendAddr.sin_addr.s_addr = inet_addr(ip);
        fSendAddr.sin_port = htons(port);
        return 0;
    }

    char* JackNetWinSocket::GetSendIP()
    {
        return inet_ntoa(fSendAddr.sin_addr);
    }

    char* JackNetWinSocket::GetRecvIP()
    {
        return inet_ntoa(fRecvAddr.sin_addr);
    }

    //utility************************************************************************************************************
    int JackNetWinSocket::GetName(char* name)
    {
        return gethostname(name, 255);
    }

    int JackNetWinSocket::JoinMCastGroup(const char* ip)
    {
        struct ip_mreq multicast_req;
        multicast_req.imr_multiaddr.s_addr = inet_addr(ip);
        multicast_req.imr_interface.s_addr = htonl(INADDR_ANY);
        //12 is IP_ADD_MEMBERSHIP in winsock2 (differs from winsock1...)
        return SetOption(IPPROTO_IP, 12, &multicast_req, sizeof(multicast_req));
    }

    //options************************************************************************************************************
    int JackNetWinSocket::SetOption(int level, int optname, const void* optval, SOCKLEN optlen)
    {
        return setsockopt(fSockfd, level, optname, static_cast<const char*>(optval), optlen);
    }

    int JackNetWinSocket::GetOption(int level, int optname, void* optval, SOCKLEN* optlen)
    {
        return getsockopt(fSockfd, level, optname, static_cast<char*>(optval), optlen);
    }

    //tiemout************************************************************************************************************
    int JackNetWinSocket::SetTimeOut(int usec)
    {
        jack_log("JackNetWinSocket::SetTimeout %d usec", usec);
        int millisec = usec / 1000;
        return SetOption(SOL_SOCKET, SO_RCVTIMEO, &millisec, sizeof(millisec));
    }

    //local loop*********************************************************************************************************
    int JackNetWinSocket::SetLocalLoop()
    {
        //char disable = 0;
        /*
        see http://msdn.microsoft.com/en-us/library/aa916098.aspx
        Default value is TRUE. When TRUE, data that is sent from the local interface to the multicast group to
        which the socket is joined, including data sent from the same socket, will be echoed to its receive buffer.
        */
        char disable = 1;
        return SetOption(IPPROTO_IP, IP_MULTICAST_LOOP, &disable, sizeof(disable));
    }

    //network operations*************************************************************************************************
    int JackNetWinSocket::SendTo(const void* buffer, size_t nbytes, int flags)
    {
        return sendto(fSockfd, reinterpret_cast<const char*>(buffer), nbytes, flags, reinterpret_cast<SOCKADDR*>(&fSendAddr), sizeof(SOCKADDR));
    }

    int JackNetWinSocket::SendTo(const void* buffer, size_t nbytes, int flags, const char* ip)
    {
        fSendAddr.sin_addr.s_addr = inet_addr(ip);
        return SendTo(buffer, nbytes, flags);
    }

    int JackNetWinSocket::Send(const void* buffer, size_t nbytes, int flags)
    {
        return send(fSockfd, reinterpret_cast<const char*>(buffer), nbytes, flags);
    }

    int JackNetWinSocket::RecvFrom(void* buffer, size_t nbytes, int flags)
    {
        SOCKLEN addr_len = sizeof(SOCKADDR);
        return recvfrom(fSockfd, reinterpret_cast<char*>(buffer), nbytes, flags, reinterpret_cast<SOCKADDR*>(&fRecvAddr), &addr_len);
    }

    int JackNetWinSocket::Recv(void* buffer, size_t nbytes, int flags)
    {
        return recv(fSockfd, reinterpret_cast<char*>(buffer), nbytes, flags);
    }

    int JackNetWinSocket::CatchHost(void* buffer, size_t nbytes, int flags)
    {
        SOCKLEN addr_len = sizeof(SOCKADDR);
        return recvfrom(fSockfd, reinterpret_cast<char*>(buffer), nbytes, flags, reinterpret_cast<SOCKADDR*>(&fSendAddr), &addr_len);
    }

    net_error_t JackNetWinSocket::GetError()
    {
        switch (NET_ERROR_CODE)
        {
            case WSABASEERR:
                return NET_NO_ERROR;
            case WSAETIMEDOUT:
                return NET_NO_DATA;
            case WSAEWOULDBLOCK:
                return NET_NO_DATA;
            case WSAECONNREFUSED:
                return NET_CONN_ERROR;
            case WSAECONNRESET:
                return NET_CONN_ERROR;
            case WSAEACCES:
                return NET_CONN_ERROR;
            case WSAECONNABORTED:
                return NET_CONN_ERROR;
            case WSAEHOSTDOWN:
                return NET_CONN_ERROR;
            case WSAEHOSTUNREACH:
                return NET_CONN_ERROR;
            default:
                return NET_OP_ERROR;
        }
    }
}

