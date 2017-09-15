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

#include "JackSocket.h"
#include "JackConstants.h"
#include "JackTools.h"
#include "JackError.h"
#include "promiscuous.h"
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>

namespace Jack
{

static void BuildName(const char* client_name, char* res, const char* dir, int which, int size, bool promiscuous)
{
    char ext_client_name[SYNC_MAX_NAME_SIZE + 1];
    JackTools::RewriteName(client_name, ext_client_name);
    if (promiscuous) {
	    snprintf(res, size, "%s/jack_%s_%d", dir, ext_client_name, which);
    } else {
	    snprintf(res, size, "%s/jack_%s_%d_%d", dir, ext_client_name, JackTools::GetUID(), which);
    }
}

JackClientSocket::JackClientSocket(): JackClientRequestInterface(), fSocket(-1), fTimeOut(0)
{
    const char* promiscuous = getenv("JACK_PROMISCUOUS_SERVER");
    fPromiscuous = (promiscuous != NULL);
    fPromiscuousGid = jack_group2gid(promiscuous);
}

JackClientSocket::JackClientSocket(int socket): JackClientRequestInterface(), fSocket(socket),fTimeOut(0), fPromiscuous(false), fPromiscuousGid(-1)
{}

#if defined(__sun__) || defined(sun)

void JackClientSocket::SetReadTimeOut(long sec)
{
    int	flags;
    fTimeOut = sec;

    if ((flags = fcntl(fSocket, F_GETFL, 0)) < 0) {
		jack_error("JackClientSocket::SetReadTimeOut error in fcntl F_GETFL");
		return;
	}

	flags |= O_NONBLOCK;
	if (fcntl(fSocket, F_SETFL, flags) < 0) {
		jack_error("JackClientSocket::SetReadTimeOut error in fcntl F_SETFL");
		return;
	}
}

void JackClientSocket::SetWriteTimeOut(long sec)
{
    int	flags;
    fTimeOut = sec;

    if ((flags = fcntl(fSocket, F_GETFL, 0)) < 0) {
		jack_error("JackClientSocket::SetWriteTimeOut error in fcntl F_GETFL");
		return;
	}

	flags |= O_NONBLOCK;
	if (fcntl(fSocket, F_SETFL, flags) < 0) {
		jack_error("JackClientSocket::SetWriteTimeOut error in fcntl F_SETFL");
		return;
	}
}

#else

void JackClientSocket::SetReadTimeOut(long sec)
{
    struct timeval timout;
    timout.tv_sec = sec;
    timout.tv_usec = 0;
    if (setsockopt(fSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timout, sizeof(timeval)) < 0) {
        jack_error("SetReadTimeOut fd = %ld err = %s", fSocket, strerror(errno));
    }
}

void JackClientSocket::SetWriteTimeOut(long sec)
{
    struct timeval timout;
    timout.tv_sec = sec ;
    timout.tv_usec = 0;
    if (setsockopt(fSocket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timout, sizeof(timeval)) < 0) {
        jack_error("SetWriteTimeOut fd = %ld err = %s", fSocket, strerror(errno));
    }
}

#endif

void JackClientSocket::SetNonBlocking(bool onoff)
{
    if (onoff) {
        long flags = 0;
        if (fcntl(fSocket, F_SETFL, flags | O_NONBLOCK) < 0) {
            jack_error("SetNonBlocking fd = %ld err = %s", fSocket, strerror(errno));
        }
    }
}

int JackClientSocket::Connect(const char* dir, const char* name, int which) // A revoir : utilisation de "which"
{
    struct sockaddr_un addr;

    if ((fSocket = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        jack_error("Cannot create socket err = %s", strerror(errno));
        return -1;
    }

    addr.sun_family = AF_UNIX;
    BuildName(name, addr.sun_path, dir, which, sizeof(addr.sun_path), fPromiscuous);
    jack_log("JackClientSocket::Connect : addr.sun_path %s", addr.sun_path);

    if (connect(fSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        jack_error("Cannot connect to server socket err = %s", strerror(errno));
        close(fSocket);
        return -1;
    }

#ifdef __APPLE__
    int on = 1;
    if (setsockopt(fSocket, SOL_SOCKET, SO_NOSIGPIPE, (const char*)&on, sizeof(on)) < 0) {
        jack_log("setsockopt SO_NOSIGPIPE fd = %ld err = %s", fSocket, strerror(errno));
    }
#endif

    return 0;
}

int JackClientSocket::Close()
{
    jack_log("JackClientSocket::Close");
    if (fSocket > 0) {
        shutdown(fSocket, SHUT_RDWR);
        close(fSocket);
        fSocket = -1;
        return 0;
    } else {
        return -1;
    }
}

int JackClientSocket::Read(void* data, int len)
{
    int res;

#if defined(__sun__) || defined(sun)
    if (fTimeOut > 0) {

        struct timeval tv;
	    fd_set fdset;
        ssize_t	res;

        tv.tv_sec = fTimeOut;
	    tv.tv_usec = 0;

	    FD_ZERO(&fdset);
	    FD_SET(fSocket, &fdset);

	    do {
		    res = select(fSocket + 1, &fdset, NULL, NULL, &tv);
	    } while (res < 0 && errno == EINTR);

	    if (res < 0) {
		    return res;
        } else if (res == 0) {
		    return -1;
	    }
    }
#endif

    if ((res = read(fSocket, data, len)) != len) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            jack_error("JackClientSocket::Read time out");
            return 0;  // For a non blocking socket, a read failure is not considered as an error
        } else if (res != 0) {
            jack_error("Cannot read socket fd = %d err = %s", fSocket, strerror(errno));
            //return 0;
            return -1;
        } else {
            jack_error("Cannot read socket fd = %d err = %s", fSocket, strerror(errno));
            return -1;
        }
    } else {
        return 0;
    }
}

int JackClientSocket::Write(void* data, int len)
{
    int res;

#if defined(__sun__) || defined(sun)
    if (fTimeOut > 0) {

        struct timeval tv;
	    fd_set fdset;
        ssize_t res;

        tv.tv_sec = fTimeOut;
	    tv.tv_usec = 0;

	    FD_ZERO(&fdset);
	    FD_SET(fSocket, &fdset);

	    do {
		    res = select(fSocket + 1, NULL, &fdset, NULL, &tv);
	    } while (res < 0 && errno == EINTR);

	    if (res < 0) {
		    return res;
        } else if (res == 0) {
		   return -1;
	    }
   }
#endif

    if ((res = write(fSocket, data, len)) != len) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            jack_log("JackClientSocket::Write time out");
            return 0;  // For a non blocking socket, a write failure is not considered as an error
        } else if (res != 0) {
            jack_error("Cannot write socket fd = %ld err = %s", fSocket, strerror(errno));
            //return 0;
            return -1;
        } else {
            jack_error("Cannot write socket fd = %ld err = %s", fSocket, strerror(errno));
            return -1;
        }
    } else {
        return 0;
    }
}

JackServerSocket::JackServerSocket(): fSocket( -1)
{
    const char* promiscuous = getenv("JACK_PROMISCUOUS_SERVER");
    fPromiscuous = (promiscuous != NULL);
    fPromiscuousGid = jack_group2gid(promiscuous);
}

int JackServerSocket::Bind(const char* dir, const char* name, int which) // A revoir : utilisation de "which"
{
    struct sockaddr_un addr;

    if ((fSocket = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        jack_error("Cannot create server socket err = %s", strerror(errno));
        return -1;
    }

    addr.sun_family = AF_UNIX;
    // Socket name has to be kept in fName to be "unlinked".
    BuildName(name, fName, dir, which, sizeof(addr.sun_path), fPromiscuous);
    strncpy(addr.sun_path, fName, sizeof(addr.sun_path) - 1);
   
    jack_log("JackServerSocket::Bind : addr.sun_path %s", addr.sun_path);
    unlink(fName); // Security...

    if (bind(fSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        jack_error("Cannot bind server to socket err = %s", strerror(errno));
        goto error;
    }

    if (listen(fSocket, 100) < 0) {
        jack_error("Cannot enable listen on server socket err = %s", strerror(errno));
        goto error;
    }

    if (fPromiscuous && (jack_promiscuous_perms(-1, fName, fPromiscuousGid) < 0))
        goto error;

    return 0;

error:
    unlink(fName);
    close(fSocket);
    return -1;
}

JackClientSocket* JackServerSocket::Accept()
{
    struct sockaddr_un client_addr;
    socklen_t client_addrlen;

    memset(&client_addr, 0, sizeof(client_addr));
    client_addrlen = sizeof(client_addr);

    int fd = accept(fSocket, (struct sockaddr*)&client_addr, &client_addrlen);
    if (fd < 0) {
        jack_error("Cannot accept new connection err = %s", strerror(errno));
        return 0;
    } else {
        return new JackClientSocket(fd);
    }
}

int JackServerSocket::Close()
{
    if (fSocket > 0) {
        jack_log("JackServerSocket::Close %s", fName);
        shutdown(fSocket, SHUT_RDWR);
        close(fSocket);
        unlink(fName);
        fSocket = -1;
        return 0;
    } else {
        return -1;
    }
}

} // end of namespace


