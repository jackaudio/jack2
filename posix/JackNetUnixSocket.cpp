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

#include "JackNetUnixSocket.h"
#include "JackError.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <unistd.h>
#include <fcntl.h>

namespace Jack
{
    //utility *********************************************************************************************************
    int GetHostName(char * name, int size)
    {
        if (gethostname(name, size) == SOCKET_ERROR) {
            jack_error("Can't get 'hostname' : %s", strerror(NET_ERROR_CODE));
            strcpy(name, "default");
            return SOCKET_ERROR;
        }
        return 0;
    }

    //construct/destruct***********************************************************************************************
    JackNetUnixSocket::JackNetUnixSocket()
    {
        fSockfd = 0;
        fPort = 0;
        fTimeOut = 0;
        fSendAddr.sin_family = AF_INET;
        fSendAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        memset(&fSendAddr.sin_zero, 0, 8);
        fRecvAddr.sin_family = AF_INET;
        fRecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        memset(&fRecvAddr.sin_zero, 0, 8);
    }

    JackNetUnixSocket::JackNetUnixSocket(const char* ip, int port)
    {
        fSockfd = 0;
        fPort = port;
        fTimeOut = 0;
        fSendAddr.sin_family = AF_INET;
        fSendAddr.sin_port = htons(port);
        inet_aton(ip, &fSendAddr.sin_addr);
        memset(&fSendAddr.sin_zero, 0, 8);
        fRecvAddr.sin_family = AF_INET;
        fRecvAddr.sin_port = htons(port);
        fRecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        memset(&fRecvAddr.sin_zero, 0, 8);
    }

    JackNetUnixSocket::JackNetUnixSocket(const JackNetUnixSocket& socket)
    {
        fSockfd = 0;
        fTimeOut = 0;
        fPort = socket.fPort;
        fSendAddr = socket.fSendAddr;
        fRecvAddr = socket.fRecvAddr;
    }

    JackNetUnixSocket::~JackNetUnixSocket()
    {
        Close();
    }

    JackNetUnixSocket& JackNetUnixSocket::operator=(const JackNetUnixSocket& socket)
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
    int JackNetUnixSocket::NewSocket()
    {
        if (fSockfd) {
            Close();
            Reset();
        }
        fSockfd = socket(AF_INET, SOCK_DGRAM, 0);

        /* Enable address reuse */
        int res, on = 1;
    #ifdef __APPLE__
        if ((res = setsockopt(fSockfd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on))) < 0) {
    #else
        if ((res = setsockopt(fSockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0) {
    #endif
            StrError(NET_ERROR_CODE);
        }
        return fSockfd;
    }

    bool JackNetUnixSocket::IsLocal(char* ip)
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

    int JackNetUnixSocket::Bind()
    {
        return bind(fSockfd, reinterpret_cast<socket_address_t*>(&fRecvAddr), sizeof(socket_address_t));
    }
    int JackNetUnixSocket::Bind(const char *if_name)
    {
        struct ip_mreq multicast_req;
	int ret = Bind();
	if(!ret && strcmp(if_name,"any")) {
	    multicast_req.imr_multiaddr.s_addr = fSendAddr.sin_addr.s_addr;
	    ret = BindMCastIface(if_name, IP_MULTICAST_IF, &multicast_req);
	}
	return ret;
    }

    int JackNetUnixSocket::BindWith(const char* ip)
    {
        int addr_conv = inet_aton(ip, &fRecvAddr.sin_addr);
        if (addr_conv < 0)
            return addr_conv;
        return Bind();
    }

    int JackNetUnixSocket::BindWith(int port)
    {
        fRecvAddr.sin_port = htons(port);
        return Bind();
    }

    int JackNetUnixSocket::Connect()
    {
        return connect(fSockfd, reinterpret_cast<socket_address_t*>(&fSendAddr), sizeof(socket_address_t));
    }

    int JackNetUnixSocket::ConnectTo(const char* ip)
    {
        int addr_conv = inet_aton(ip, &fSendAddr.sin_addr);
        if (addr_conv < 0)
            return addr_conv;
        return Connect();
    }

    void JackNetUnixSocket::Close()
    {
        if (fSockfd)
            close(fSockfd);
        fSockfd = 0;
    }

    void JackNetUnixSocket::Reset()
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

    bool JackNetUnixSocket::IsSocket()
    {
        return(fSockfd) ? true : false;
    }

    //IP/PORT***********************************************************************************************************
    void JackNetUnixSocket::SetPort(int port)
    {
        fPort = port;
        fSendAddr.sin_port = htons(port);
        fRecvAddr.sin_port = htons(port);
    }

    int JackNetUnixSocket::GetPort()
    {
        return fPort;
    }

    //address***********************************************************************************************************
    int JackNetUnixSocket::SetAddress(const char* ip, int port)
    {
        int addr_conv = inet_aton(ip, &fSendAddr.sin_addr);
        if (addr_conv < 0)
            return addr_conv;
        fSendAddr.sin_port = htons(port);
        return 0;
    }

    char* JackNetUnixSocket::GetSendIP()
    {
        return inet_ntoa(fSendAddr.sin_addr);
    }

    char* JackNetUnixSocket::GetRecvIP()
    {
        return inet_ntoa(fRecvAddr.sin_addr);
    }

    //utility************************************************************************************************************
    int JackNetUnixSocket::GetName(char* name)
    {
        return gethostname(name, 255);
    }

    int JackNetUnixSocket::JoinMCastGroup(const char* ip)
    {
	return JoinMCastGroup(ip,"any");
    }
    int JackNetUnixSocket::JoinMCastGroup(const char* ip, const char *if_name)
    {
        struct ip_mreq multicast_req;
        inet_aton(ip, &multicast_req.imr_multiaddr);
	if(!strcmp(if_name,"any")) {
	    multicast_req.imr_interface.s_addr = htonl(INADDR_ANY);
	    return SetOption(IPPROTO_IP, IP_ADD_MEMBERSHIP, &multicast_req, sizeof(multicast_req));
	} else {
	    return BindMCastIface(if_name, IP_ADD_MEMBERSHIP, &multicast_req);
	}
    }
    int JackNetUnixSocket::BindMCastIface(const char *if_name, const int option, struct ip_mreq *mreq)
    {
	struct ifaddrs *ifas, *ifa;
	int specific = strcmp("all",if_name);
	int ret=-1;
	char *if_last="any";

	if (getifaddrs(&ifas) == -1) {
		jack_error("JackNetUnixSocket::BindMCastIface error in getifaddrs");
		return -1;
	}
	for (ifa = ifas; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL)
		    continue; // Address is mandatory
		if(!ifa->ifa_name || !strcmp(if_last,ifa->ifa_name))
		    continue; // Name as well, also skip already enabled interface
		if(!(ifa->ifa_flags & IFF_MULTICAST) || !(ifa->ifa_flags & IFF_RUNNING))
		    continue; // And non multicast or down interface
		if(ifa->ifa_addr->sa_family != AF_INET)
		    continue; // And for the moment we're dealing with IPv4 only
		if(!specific || !strcmp(ifa->ifa_name,if_name)) {
		    mreq->imr_interface.s_addr = ((struct sockaddr_in *)(ifa->ifa_addr))->sin_addr.s_addr;
		    ret = SetOption(IPPROTO_IP, option, mreq, sizeof(struct ip_mreq));
		    if(ret)
			break;
		    if_last = ifa->ifa_name;
		    jack_log("JackNetUnixSocket::BindMCastIface attaching to %s", if_last);
		}
	}
	freeifaddrs(ifas);
	if(!strcmp(if_last,"any"))
		jack_error("JackNetUnixSocket::BindMCastIface cannot find valid interface");
	return ret;
    }

    //options************************************************************************************************************
    int JackNetUnixSocket::SetOption(int level, int optname, const void* optval, socklen_t optlen)
    {
        return setsockopt(fSockfd, level, optname, optval, optlen);
    }

    int JackNetUnixSocket::GetOption(int level, int optname, void* optval, socklen_t* optlen)
    {
        return getsockopt(fSockfd, level, optname, optval, optlen);
    }

    //timeout************************************************************************************************************

#if defined(__sun__) || defined(sun)
    int JackNetUnixSocket::SetTimeOut(int us)
    {
        int	flags;
        fTimeOut = us;

        if ((flags = fcntl(fSockfd, F_GETFL, 0)) < 0) {
		    jack_error("JackNetUnixSocket::SetTimeOut error in fcntl F_GETFL");
		    return -1;
	    }

	    flags |= O_NONBLOCK;
	    if (fcntl(fSockfd, F_SETFL, flags) < 0) {
		    jack_error("JackNetUnixSocket::SetTimeOut error in fcntl F_SETFL");
		    return 1;
	    }

        return 0;
    }

    int JackNetUnixSocket::WaitRead()
    {
        if (fTimeOut > 0) {

            struct timeval tv;
	        fd_set fdset;
            ssize_t	res;

            tv.tv_sec = fTimeOut / 1000000;
	        tv.tv_usec = fTimeOut % 1000000;

	        FD_ZERO(&fdset);
	        FD_SET(fSockfd, &fdset);

	        do {
		        res = select(fSockfd + 1, &fdset, NULL, NULL, &tv);
	        } while(res < 0 && errno == EINTR);

	        if (res < 0) {
 		        return res;
            } else if (res == 0) {
                errno = ETIMEDOUT;
		        return -1;
	        }
        }

        return 0;
    }

    int JackNetUnixSocket::WaitWrite()
    {
        if (fTimeOut > 0) {

            struct timeval tv;
	        fd_set fdset;
            ssize_t	res;

            tv.tv_sec = fTimeOut / 1000000;
            tv.tv_usec = fTimeOut % 1000000;

	        FD_ZERO(&fdset);
	        FD_SET(fSockfd, &fdset);

	        do {
		        res = select(fSockfd + 1, NULL, &fdset, NULL, &tv);
	        } while(res < 0 && errno == EINTR);

	        if (res < 0) {
		        return res;
            } else if (res == 0) {
                errno = ETIMEDOUT;
		        return -1;
	        }
        }

        return 0;
    }

#else
    int JackNetUnixSocket::SetTimeOut(int us)
    {
        jack_log("JackNetUnixSocket::SetTimeout %d usecs", us);
        struct timeval timeout;

        //less than 1sec
        if (us < 1000000) {
            timeout.tv_sec = 0;
            timeout.tv_usec = us;
        } else {
        //more than 1sec
            float sec = float(us) / 1000000.f;
            timeout.tv_sec = (int)sec;
            float usec = (sec - float(timeout.tv_sec)) * 1000000;
            timeout.tv_usec =(int)usec;
        }
        return SetOption(SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    }
#endif

    //local loop**********************************************************************************************************
    int JackNetUnixSocket::SetLocalLoop()
    {
        char disable = 0;
        return SetOption(IPPROTO_IP, IP_MULTICAST_LOOP, &disable, sizeof(disable));
    }

    //network operations**************************************************************************************************
    int JackNetUnixSocket::SendTo(const void* buffer, size_t nbytes, int flags)
    {
    #if defined(__sun__) || defined(sun)
        if (WaitWrite() < 0)
            return -1;
    #endif
        return sendto(fSockfd, buffer, nbytes, flags, reinterpret_cast<socket_address_t*>(&fSendAddr), sizeof(socket_address_t));
    }

    int JackNetUnixSocket::SendTo(const void* buffer, size_t nbytes, int flags, const char* ip)
    {
        int addr_conv = inet_aton(ip, &fSendAddr.sin_addr);
        if (addr_conv < 1)
            return addr_conv;
    #if defined(__sun__) || defined(sun)
        if (WaitWrite() < 0)
            return -1;
    #endif
        return SendTo(buffer, nbytes, flags);
    }

    int JackNetUnixSocket::Send(const void* buffer, size_t nbytes, int flags)
    {
    #if defined(__sun__) || defined(sun)
        if (WaitWrite() < 0)
            return -1;
    #endif
        return send(fSockfd, buffer, nbytes, flags);
    }

    int JackNetUnixSocket::RecvFrom(void* buffer, size_t nbytes, int flags)
    {
        socklen_t addr_len = sizeof(socket_address_t);
    #if defined(__sun__) || defined(sun)
        if (WaitRead() < 0)
            return -1;
    #endif
        return recvfrom(fSockfd, buffer, nbytes, flags, reinterpret_cast<socket_address_t*>(&fRecvAddr), &addr_len);
    }

    int JackNetUnixSocket::Recv(void* buffer, size_t nbytes, int flags)
    {
    #if defined(__sun__) || defined(sun)
        if (WaitRead() < 0)
            return -1;
    #endif
        return recv(fSockfd, buffer, nbytes, flags);
    }

    int JackNetUnixSocket::CatchHost(void* buffer, size_t nbytes, int flags)
    {
        jack_log("JackNetUnixSocket::CatchHost");
        socklen_t addr_len = sizeof(socket_address_t);
    #if defined(__sun__) || defined(sun)
        if (WaitRead() < 0)
            return -1;
    #endif
        return recvfrom(fSockfd, buffer, nbytes, flags, reinterpret_cast<socket_address_t*>(&fSendAddr), &addr_len);
    }

    net_error_t JackNetUnixSocket::GetError()
    {
        switch(errno)
        {
            case EAGAIN:
            case ETIMEDOUT:
                return NET_NO_DATA;

            case ECONNABORTED:
            case ECONNREFUSED:
            case ECONNRESET:
            case EINVAL:
            case EHOSTDOWN:
            case EHOSTUNREACH:
            case ENETDOWN:
            case ENETUNREACH:
                return NET_CONN_ERROR;

            default:
                //return NET_OP_ERROR;
                return NET_CONN_ERROR;
        }
    }
}
