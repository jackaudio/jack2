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
#include <sys/time.h>

using namespace std;

// See RFC 3493; The Open Group Base Specifications Issue 6 IEEE Std 1003.1, 2004 Edition
#define _sock4(x) (*(struct sockaddr_in *)&x)
#define _sock6(x) (*(struct sockaddr_in6*)&x)
#define _ss_addr_p(x) ((fFamily==AF_INET6) ? \
        (void*)&(((struct sockaddr_in6*)&x)->sin6_addr) \
      : (void*)&(((struct sockaddr_in *)&x)->sin_addr))
#define JNS_UNSPEC 0x0
#define JNS_PROBED 0x1
#define JNS_BOUND  0x2
#define JNS_CONNCD 0x4
#define JNS_MCAST  0x10

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
    : fFamily(AF_UNSPEC), fSockfd(0), fState(JNS_UNSPEC), fPort(0), fTimeOut(0)
    {
        Reset();
    }

    JackNetUnixSocket::JackNetUnixSocket(const char* ip, int port)
    : fFamily(AF_UNSPEC), fSockfd(0), fState(JNS_UNSPEC), fPort(0), fTimeOut(0)
    {
        Reset();
        fPort = port;
        if(NewSocket(ip)==SOCKET_ERROR)
            jack_error("JackNetUnixSocket::JackNetUnixSocket Cannot initialize (%s:%d): %s",ip,port,strerror(NET_ERROR_CODE));
    }

    JackNetUnixSocket::JackNetUnixSocket(const JackNetUnixSocket& socket)
    {
        Clone(socket);
    }

    JackNetUnixSocket::~JackNetUnixSocket()
    {
        Close();
    }

    JackNetUnixSocket& JackNetUnixSocket::operator=(const JackNetUnixSocket& socket)
    {
        if (this != &socket)
            Clone(socket);
        return *this;
    }

    //socket***********************************************************************************************************
    void JackNetUnixSocket::Clone(const JackNetUnixSocket& socket)
    {
        fSockfd = 0;
        fTimeOut = 0;
        fPort = socket.fPort;
        fFamily = socket.fFamily;
        fState = socket.fState;
        fState &= (JNS_MCAST | JNS_PROBED); // Reset all fields except mcast and probed
        memcpy(&fSendAddr, & socket.fSendAddr, sizeof(fSendAddr));
        memcpy(&fRecvAddr, & socket.fRecvAddr, sizeof(fRecvAddr));
    }
    /* When using Multicast always create socket with new (mip,port) or call NewSocket(mip) for autoinitialized sockets first.
    *  This makes sure proper workarounds for Multicast incompatibility will be activated while setting up address family.
    */
    int JackNetUnixSocket::ProbeAF(const char* ip, struct sockaddr_storage *addr, int (*call)(int,const struct sockaddr*,socklen_t))
    {
        struct addrinfo hint,*res, *ri;
        char sport[6];
        int ret;

        snprintf(sport,6,"%d",fPort);
        memset(&hint,0,sizeof(hint));
        hint.ai_family = fFamily;
        hint.ai_socktype = SOCK_DGRAM;
        hint.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG | AI_NUMERICSERV;
        if(ip == NULL) hint.ai_flags |= AI_PASSIVE;
        ret = getaddrinfo(ip,sport,&hint,&res);
        if(ret) {
            jack_error("JackNetUnixSocket::ProbeAF getaddrinfo(%s:%s): %s",ip,sport,gai_strerror(ret));
            return ret;
        }
#define GLIBC_BUG
        // An ugly GLIBC bug, which one may argue to be RFC bug. See http://sourceware.org/bugzilla/show_bug.cgi?id=14967
#ifdef GLIBC_BUG
        // Of course gai configuration can say to have v4 having precedence explicitly but... here we prefer ipv6. period
        if(!ip && res->ai_family == AF_INET && res->ai_next && res->ai_next->ai_family == AF_INET6) {
            jack_log("JackNetUnixSocket::ProbeAF swapping addrinfo entries to work around GLIBC getaddrinfo bug: AF_INET<->AF_INET6");
            // Swapping first two entries
            ri = res->ai_next;
            res->ai_next = ri->ai_next;
            ri->ai_next = res;
            res = ri;
        } else // << Remove up to this very line including once GLIBC is fixed <<
#endif
            ri = res;
        do {
            jack_log("JackNetUnixSocket::ProbeAF trying AF[%d], TYPE[%d], PROTO[%d]",
                                                ri->ai_family,ri->ai_socktype,ri->ai_protocol);
            ret = socket(ri->ai_family, ri->ai_socktype, ri->ai_protocol);
            if(ret < 0)
                continue;
            if(!(*call)(ret, ri->ai_addr, ri->ai_addrlen))
                break;
            close(ret);
            jack_log("JackNetUnixSocket::ProbeAF failed for AF[%d], %s",ri->ai_family,(ri->ai_next)?"trying next":"giving up");
        } while((ri = ri->ai_next) != NULL);
        // probe successfully made a *call on socket for ip represented by ri
        if(ri) {
            fSockfd = ret;
            fFamily = ri->ai_family;
            memcpy(addr,ri->ai_addr,ri->ai_addrlen);
            jack_log("JackNetUnixSocket::ProbeAF probed[%d] AF[%d] for %s",ret,fFamily,ip);
            fState |= JNS_PROBED;
            if(fFamily == AF_INET6 && IN6_IS_ADDR_MULTICAST(&((struct sockaddr_in6*)addr)->sin6_addr))
                fState |= JNS_MCAST;
        } else
            ret = SOCKET_ERROR;
        freeaddrinfo(res);
        return ret;
    }

    int JackNetUnixSocket::NewSocket()
    {
        return NewSocket(NULL);
    }
    int JackNetUnixSocket::NewSocket(const char *ip)
    {
        if(fFamily == AF_UNSPEC) {
            if(ip) {
                if(ProbeAF(ip,&fSendAddr,connect)<0)
                    return SOCKET_ERROR;
            } else {
                if(ProbeAF(ip,&fRecvAddr,bind)<0)
                    return SOCKET_ERROR;
                // It should be with AF/port/ANY after bind
                memcpy(&fSendAddr,&fRecvAddr,sizeof(fRecvAddr));
            }
            // Now re-create the fd to get a brand-new unnamed socket
        }
        if (fSockfd) {
            Close();
            Reset();
        }
        fSockfd = socket(fFamily, SOCK_DGRAM, 0);

        /* Enable address reuse */
        int res, on = 1;
    #ifdef __APPLE__
        if ((res = setsockopt(fSockfd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on))) < 0) {
    #else
        if ((res = setsockopt(fSockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0) {
    #endif
            StrError(NET_ERROR_CODE);
        }
        
        int tos = 0;       /* see <netinet/in.h> */
        
        /*
        DSCP Field Hex/Bin/Dec	Layer 2 Prio	Traffic Type	Acronym	WMM Access Category
        0x38 / 111000 / 56	7	Network Control	NC	AC_VO
        0x30 / 110000 / 48	6	Voice	VO	AC_VO
        0x28 / 101000 / 40	5	Video	VI	AC_VI
        0x20 / 100000 / 32	4	Controlled Load	CL	AC_VI
        0x18 / 011000 / 24	3	Excellent Effort	EE	AC_BE
        0x10 / 010000 / 16	2	Spare	--	AC_BK
        0x08 / 001000 / 8	1	Background	BK	AC_BK
        0x00 / 000000 / 0	0	Best Effort	BE	AC_BE
        */
        
        socklen_t len = sizeof(tos);
        
        res = getsockopt(fSockfd, IPPROTO_IP, IP_TOS, &tos, &len);
        
        tos = 46 * 4;       // see <netinet/in.h> 
        res = setsockopt(fSockfd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos));

        return fSockfd;
    }

    bool JackNetUnixSocket::IsLocal(char* ip)
    {
        if (strcmp(ip, "127.0.0.1") == 0) {
            return true;
        } else if(!strcmp(ip,"::1"))
            return true;

        struct ifaddrs *ifas, *ifa;
        socklen_t len;

        if (getifaddrs(&ifas) == -1) {
                jack_error("JackNetUnixSocket::IsLocal error in getifaddrs");
                return false;
        }
        for (ifa = ifas; ifa != NULL; ifa = ifa->ifa_next) {
                if (ifa->ifa_addr == NULL)
                    continue; // Address is mandatory
                len = (ifa->ifa_addr->sa_family==AF_INET)?sizeof(struct sockaddr_in):sizeof(struct sockaddr_in6);
                if(!getnameinfo(ifa->ifa_addr, len, f_addr_buff, INET6_ADDRSTRLEN, NULL,0, NI_NUMERICSERV | NI_DGRAM | NI_NUMERICSERV | NI_DGRAM | NI_NUMERICHOST))
                    if(!strcmp(f_addr_buff,ip))
                        break;
        }
        freeifaddrs(ifas);
        return (ifa != NULL);
    }

    int JackNetUnixSocket::Bind()
    {
        int yes=1;
        if(fState & JNS_BOUND) return 0;
        // Multicast is incompatible with V4MAPPED or V4COMPAT addresses, if probe detected MC we need V6ONLY
        if(fFamily == AF_INET6 && IN6_IS_ADDR_UNSPECIFIED(&_sock6(fRecvAddr).sin6_addr) && fState & JNS_MCAST)
            if(SetOption(IPPROTO_IPV6, IPV6_V6ONLY, &yes, sizeof(yes))) return SOCKET_ERROR;
        if(::bind(fSockfd, reinterpret_cast<struct sockaddr*>(&fRecvAddr), sizeof(fRecvAddr))) return SOCKET_ERROR;
        fState |= JNS_BOUND;
        return 0;
    }
    int JackNetUnixSocket::Bind(const char *if_name)
    {
        int ret = Bind();
        if(!ret && strcmp(if_name,"any")) {
            if(fFamily == AF_INET) {
                // 'all' for this case will lead to 'last valid interface', which is not that one might expect
                if(strcmp(if_name,"all"))
                    ret = BindMCastIface(if_name, IP_MULTICAST_IF, &_sock4(fSendAddr).sin_addr);
                else
                    jack_error("Multicast Interface all not found, sending from default");
            } else if(fFamily == AF_INET6) {
                struct if_nameindex *if_ni = if_nameindex(); // In V6 world we do everything differently.
                if(if_ni) {
                    int i;
                    for (i=0; if_ni[i].if_index > 0; i++) {
                        if(if_ni[i].if_index == 1)
                            continue; // Skip loopback
                        if(!strcmp(if_ni[i].if_name,if_name)) {
                            ret = SetOption(IPPROTO_IPV6, IPV6_MULTICAST_IF, &if_ni[i].if_index, sizeof(if_ni[i].if_index));
                            jack_log("JackNetUnixSocket::Bind Multicasting from %s",if_ni[i].if_name);
                            break;
                        }
                    }
                    if(if_ni[i].if_index == 0) jack_error("Multicast Interface %s not found, sending from default",if_name);
                    if_freenameindex(if_ni);
                }
            }
        }
        return ret;
    }

    int JackNetUnixSocket::BindWith(const char* ip)
    {
        if(fFamily == AF_UNSPEC) {
            if(!fPort) return SOCKET_ERROR;
            if(ProbeAF(ip,&fRecvAddr,&bind)<0) return SOCKET_ERROR;
            fState |= JNS_BOUND;
            return 0;
        } else {
            if(SetRecvIP(ip)==-1) return SOCKET_ERROR;
            return Bind();
        }
    }

    int JackNetUnixSocket::BindWith(int port)
    {
        if(fFamily == AF_UNSPEC) {
            fPort = port;
            if(ProbeAF(NULL,&fRecvAddr,&bind)<0) return SOCKET_ERROR;
            fState |= JNS_BOUND;
            return 0;
        } else {
            SetPort(port);
            return Bind();
        }
    }

    int JackNetUnixSocket::Connect()
    {
        if(fFamily != AF_UNSPEC)
            return connect(fSockfd, (struct sockaddr*)&fSendAddr,sizeof(fSendAddr));
        jack_error("JackNetUnixSocket::Connect Family not initialized");
        return SOCKET_ERROR;
    }

    int JackNetUnixSocket::ConnectTo(const char* ip)
    {
        socklen_t l=sizeof(fRecvAddr);
        if(fPort==0) return SOCKET_ERROR;
        if(fState & JNS_PROBED) {
            Reset();
            fFamily=AF_UNSPEC;
        }
        if(fSockfd)
            Close();
        if(ProbeAF(ip,&fSendAddr,&connect)<0) return SOCKET_ERROR;
        fState |= JNS_CONNCD;
        return getsockname(fSockfd, (struct sockaddr *)&fRecvAddr, &l);
    }

    void JackNetUnixSocket::Close()
    {
        if (fSockfd) {
            close(fSockfd);
        }
        fSockfd = 0;
        fState = JNS_UNSPEC;
    }

    void JackNetUnixSocket::Reset()
    {
        memset(&fSendAddr, 0, sizeof(fSendAddr));
        fSendAddr.ss_family = fFamily;
        memset(&fRecvAddr, 0, sizeof(fRecvAddr));
        fRecvAddr.ss_family = fFamily;
        if(fPort)
            SetPort(fPort);
    }

    bool JackNetUnixSocket::IsSocket()
    {
        return(fSockfd) ? true : false;
    }

    //IP/PORT***********************************************************************************************************
    void JackNetUnixSocket::SetPort(int port)
    {
        fPort = port;
        switch(fFamily) { // Playing dumb here, in fact port section of both sockaddrs is compatible
            case AF_INET:
                _sock4(fSendAddr).sin_port = htons(port);
                _sock4(fRecvAddr).sin_port = htons(port);
                break;
            case AF_INET6:
                _sock6(fSendAddr).sin6_port = htons(port);
                _sock6(fRecvAddr).sin6_port = htons(port);
                break;
            default:
                jack_info("JackNetUnixSocket::SetPort: Family not initialized");
        }
    }

    int JackNetUnixSocket::GetPort()
    {
        return fPort;
    }

    //address***********************************************************************************************************
    int JackNetUnixSocket::SetAddress(const char* ip, int port)
    {
        if(fFamily == AF_UNSPEC) {
            fPort=port;
            return ProbeAF(ip,&fSendAddr,&connect);
        } else {
            SetPort(port);
            return inet_pton(fFamily, ip, _ss_addr_p(fSendAddr));
        }
    }
    int JackNetUnixSocket::SetSendIP(const char *ip)
    {
        if(fFamily == AF_UNSPEC) return -1;
        return inet_pton(fFamily, ip, _ss_addr_p(fSendAddr));
    }

    char* JackNetUnixSocket::GetSendIP()
    {
        return (char*)inet_ntop(fFamily, _ss_addr_p(fSendAddr),f_addr_buff,INET6_ADDRSTRLEN);
    }

    int JackNetUnixSocket::SetRecvIP(const char *ip)
    {
        if(fFamily == AF_UNSPEC) return -1;
        return inet_pton(fFamily, ip, _ss_addr_p(fRecvAddr));
    }

    char* JackNetUnixSocket::GetRecvIP()
    {
        return (char*)inet_ntop(fFamily, _ss_addr_p(fRecvAddr),f_addr_buff,INET6_ADDRSTRLEN);
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
    /** Glory and shame of IPv6 interoperability: Multicast
    * Not only API is completely incompatible, but even socket is incompatible. I.e. you cannot use V4MAPPED/COMPATIBLE
    * sockets/sockaddress for Multicast. Any V4 address is just a legacy unicast socket for IPv6. Moreover, IPv6 API
    * implementation itself is a big mess even though there's no time anymore and way back. Here we're using POSIX API.
    * When dealing with Multicast - Address Family should be set once and forever for used multicast address. This
    * implies we cannot use V4 addresses on V6 sockets. So multicast should have V6ONLY soscket option set to allow v6
    * adapter, master or manager coexist on the same host. See ProbeAF/Bind for these tricks.
    */
    int JackNetUnixSocket::JoinMCastGroup(const char* ip, const char *if_name)
    {
        int level, option, length;
        void *mreq;
        char addr[sizeof(in6_addr)];
        inet_pton(fFamily, ip, addr);
        if(!strcmp(if_name,"any")) { // UNSPEC binding we can do in-place using void pointers
            if(fFamily == AF_INET) {
                struct ip_mreq multicast_req;
                multicast_req.imr_multiaddr.s_addr = *(uint32_t*)(addr);
                multicast_req.imr_interface.s_addr = htonl(INADDR_ANY);
                level = IPPROTO_IP;
                option = IP_ADD_MEMBERSHIP;
                mreq=&multicast_req;
                length = sizeof(ip_mreq);
            } else if(fFamily == AF_INET6) {
                struct ipv6_mreq mreq6;
                memcpy(&mreq6.ipv6mr_multiaddr,addr,sizeof(in6_addr));
                mreq6.ipv6mr_interface = 0;
                level = IPPROTO_IPV6;
                option = IPV6_JOIN_GROUP;
                mreq = &mreq6;
                length = sizeof(ipv6_mreq);
            } else {
                jack_error("Unsupported family[%d]",fFamily);
                return -1;
            }
            return SetOption(level, option, mreq, length);
        } else { // For anything more complex need to call family-specific routine
            if(fFamily == AF_INET)
                return BindMCastIface(if_name, IP_ADD_MEMBERSHIP, (struct in_addr *)addr);
            else if(fFamily == AF_INET6)
                return BindMCast6Iface(if_name, (struct in6_addr *)addr);
            else {
                jack_error("Unsupported family[%d]",fFamily);
                return -1;
            }
        }
    }
    int JackNetUnixSocket::BindMCastIface(const char *if_name, const int option, struct in_addr *addr)
    {
        struct ifaddrs *ifas, *ifa;
        struct ip_mreq mreq;
        int specific = strcmp("all",if_name);
        int ret=-1;
        char *if_last=(char *)"any";

        if (getifaddrs(&ifas) == -1) {
                jack_error("JackNetUnixSocket::BindMCastIface error in getifaddrs");
                return -1;
        }
        mreq.imr_multiaddr.s_addr = addr->s_addr;
        for (ifa = ifas; ifa != NULL; ifa = ifa->ifa_next) {
                if (ifa->ifa_addr == NULL)
                    continue; // Address is mandatory
                if(!ifa->ifa_name || !strcmp(if_last,ifa->ifa_name))
                    continue; // Name as well, also skip already enabled interface
                if(!(ifa->ifa_flags & IFF_MULTICAST) || !(ifa->ifa_flags & IFF_RUNNING))
                    continue; // And non multicast or down interface
                if(ifa->ifa_addr->sa_family != AF_INET)
                    continue; // And non-matching family
                if(!specific || !strcmp(ifa->ifa_name,if_name)) {
                    mreq.imr_interface.s_addr = ((struct sockaddr_in *)(ifa->ifa_addr))->sin_addr.s_addr;
                    ret = SetOption(IPPROTO_IP, option, &mreq, sizeof(struct ip_mreq));
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
    int JackNetUnixSocket::BindMCast6Iface(const char *if_name, struct in6_addr *mip)
    {
        int i, ret=-1, specific=strcmp("all",if_name);
        struct if_nameindex *if_ni = if_nameindex();
        struct ipv6_mreq mreq;
        if(if_ni) {
            memcpy(&mreq.ipv6mr_multiaddr, mip, sizeof(struct in6_addr));
            for (i=0; if_ni[i].if_index > 0; i++) {
                if(if_ni[i].if_index == 1)
                    continue; // Skip loopback
                mreq.ipv6mr_interface = if_ni[i].if_index;
                if(!specific || !strcmp(if_ni[i].if_name,if_name)) {
                    ret = SetOption(IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, sizeof(mreq));
                    if(ret < 0)
                        break;
                }
            }
            if_freenameindex(if_ni);
        }
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
        int     flags;
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
            ssize_t     res;

            tv.tv_sec = fTimeOut / 1000000;
                tv.tv_usec = fTimeOut % 1000000;

                FD_ZERO(&fdset);
                FD_SET(fSockfd, &fdset);

	        do {
		        res = select(fSockfd + 1, &fdset, NULL, NULL, &tv);
	        } while (res < 0 && errno == EINTR);

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
            ssize_t     res;

            tv.tv_sec = fTimeOut / 1000000;
            tv.tv_usec = fTimeOut % 1000000;

                FD_ZERO(&fdset);
                FD_SET(fSockfd, &fdset);

	        do {
		        res = select(fSockfd + 1, NULL, &fdset, NULL, &tv);
	        } while (res < 0 && errno == EINTR);

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

        //less than 1 sec
        if (us < 1000000) {
            timeout.tv_sec = 0;
            timeout.tv_usec = us;
        } else {
        //more than 1 sec
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
        unsigned int dis6 = 0;
        switch(fFamily) {
        case AF_INET:
            return SetOption(IPPROTO_IP, IP_MULTICAST_LOOP, &disable, sizeof(disable));
        case AF_INET6:
            return SetOption(IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &dis6, sizeof(dis6));
        default:
            jack_error("JackNetUnixSocket::SetLocalLoop: Family not initialized");
            return -1;
        }
    }

    //network operations**************************************************************************************************
    int JackNetUnixSocket::SendTo(const void* buffer, size_t nbytes, int flags)
    {
    #if defined(__sun__) || defined(sun)
        if (WaitWrite() < 0) {
            return -1;
        }
    #endif
        int res;
        if ((res = sendto(fSockfd, buffer, nbytes, flags, reinterpret_cast<struct sockaddr *>(&fSendAddr), sizeof(fSendAddr))) < 0) {
            jack_error("SendTo fd = %ld err = %s", fSockfd, strerror(errno));
        }
        return res;
    }

    int JackNetUnixSocket::SendTo(const void* buffer, size_t nbytes, int flags, const char* ip)
    {
        int addr_conv = inet_pton(fFamily, ip, _ss_addr_p(fSendAddr));
        if (addr_conv < 1) {
            return addr_conv;
        }
	//fSendAddr.sin_port = htons(fPort);
    #if defined(__sun__) || defined(sun)
        if (WaitWrite() < 0) {
            return -1;
        }
    #endif
        return SendTo(buffer, nbytes, flags);
    }

    int JackNetUnixSocket::Send(const void* buffer, size_t nbytes, int flags)
    {
    #if defined(__sun__) || defined(sun)
        if (WaitWrite() < 0) {
            return -1;
        }
    #endif
        int res;
        if ((res = send(fSockfd, buffer, nbytes, flags)) < 0) {
            jack_error("Send fd = %ld err = %s", fSockfd, strerror(errno));
        }
        return res;
    }

    int JackNetUnixSocket::RecvFrom(void* buffer, size_t nbytes, int flags)
    {
        socklen_t addr_len = sizeof(fRecvAddr);
    #if defined(__sun__) || defined(sun)
        if (WaitRead() < 0) {
            return -1;
        }
    #endif
        int res;
        if ((res = recvfrom(fSockfd, buffer, nbytes, flags, reinterpret_cast<struct sockaddr *>(&fRecvAddr), &addr_len)) < 0) {
            jack_error("RecvFrom fd = %ld err = %s", fSockfd, strerror(errno));
        }
        return res;        
    }

    int JackNetUnixSocket::Recv(void* buffer, size_t nbytes, int flags)
    {
    #if defined(__sun__) || defined(sun)
        if (WaitRead() < 0) {
            return -1;
        }
    #endif
        int res;
        if ((res = recv(fSockfd, buffer, nbytes, flags)) < 0) {
            jack_error("Recv fd = %ld err = %s", fSockfd, strerror(errno));
        }
        return res;        
    }

    int JackNetUnixSocket::CatchHost(void* buffer, size_t nbytes, int flags)
    {
        jack_log("JackNetUnixSocket::CatchHost");
        socklen_t addr_len = sizeof(fSendAddr);
    #if defined(__sun__) || defined(sun)
        if (WaitRead() < 0) {
            return -1;
        }
    #endif
        int res;
        if ((res = recvfrom(fSockfd, buffer, nbytes, flags, reinterpret_cast<struct sockaddr *>(&fSendAddr), &addr_len)) < 0) {
            jack_log("CatchHost fd = %ld err = %s", fSockfd, strerror(errno));
        }
        return res;                
    }

    net_error_t JackNetUnixSocket::GetError()
    {
        switch (errno) {
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
        
    void JackNetUnixSocket::PrintError()
    {
        switch (errno) {
                
            case EAGAIN:
                jack_error("JackNetUnixSocket : EAGAIN");
                break;
            case ETIMEDOUT:
                jack_error("JackNetUnixSocket : ETIMEDOUT");
                break;
            case ECONNABORTED:
                jack_error("JackNetUnixSocket : ECONNABORTED");
                break;
            case ECONNREFUSED:
                jack_error("JackNetUnixSocket : ECONNREFUSED");
                break;
            case ECONNRESET:
                jack_error("JackNetUnixSocket : ECONNRESET");
                break;
            case EINVAL:
                jack_error("JackNetUnixSocket : EINVAL");
                break;
            case EHOSTDOWN:
                jack_error("JackNetUnixSocket : EHOSTDOWN");
                break;
            case EHOSTUNREACH:
                jack_error("JackNetUnixSocket : EHOSTUNREACH");
                break;
            case ENETDOWN:
                jack_error("JackNetUnixSocket : ENETDOWN");
                break;
            case ENETUNREACH:
                jack_error("JackNetUnixSocket : ENETUNREACH");
                break;
            default:
                jack_error("JackNetUnixSocket : %d", errno);
                break;
        }
    }    
}
