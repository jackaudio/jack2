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
#include "JackError.h"
#include <string.h>
#include <stdio.h>
#include <pthread.h>

namespace Jack
{

JackClientSocket::JackClientSocket(int socket): fSocket(socket)
{}

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

void JackClientSocket::SetNonBlocking(bool onoff)
{   
    int flag = (onoff) ? 1 : 0;
    if (ioctl(fSocket, FIONBIO, &flag) < 0) {
        jack_error("SetNonBlocking fd = %ld err = %s", fSocket, strerror(errno));
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
    snprintf(addr.sun_path, sizeof(addr.sun_path) - 1, "%s/jack_%s_%d", dir, name, which);
    jack_log("Connect: addr.sun_path %s", addr.sun_path);

    if (connect(fSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        jack_error("Cannot connect to server socket err = %s", strerror(errno));
        close(fSocket);
        return -1;
    }

#ifdef __APPLE__
    int on = 1 ;
    if (setsockopt(fSocket, SOL_SOCKET, SO_NOSIGPIPE, (const char*)&on, sizeof(on)) < 0) {
        jack_log("setsockopt SO_NOSIGPIPE fd = %ld err = %s", fSocket, strerror(errno));
    }
#endif

    return 0;
}

int JackClientSocket::Connect(const char* dir, int which)
{
    struct sockaddr_un addr;

    if ((fSocket = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        jack_error("Cannot create socket err = %s", strerror(errno));
        return -1;
    }

    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path) - 1, "%s/jack_%d", dir, which);
    jack_log("Connect: addr.sun_path %s", addr.sun_path);

    if (connect(fSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        jack_error("Cannot connect to server socket err = %s", strerror(errno));
        close(fSocket);
        return -1;
    }

#ifdef __APPLE__
    int on = 1 ;
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
    if (read(fSocket, data, len) != len) {
        jack_error("Cannot read socket fd = %d err = %s", fSocket, strerror(errno));
        if (errno == EWOULDBLOCK) {
            jack_error("JackClientSocket::Read time out");
            return 0;  // For a non blocking socket, a read failure is not considered as an error
        } else {
            return -1;
        }
    } else {
        return 0;
    }
}

int JackClientSocket::Write(void* data, int len)
{
    if (write(fSocket, data, len) != len) {
        jack_error("Cannot write socket fd = %ld err = %s", fSocket, strerror(errno));
        if (errno == EWOULDBLOCK) {
            jack_log("JackClientSocket::Write time out");
            return 0;  // For a non blocking socket, a write failure is not considered as an error
        } else {
            return -1;
        }
    } else {
        return 0;
    }
}

int JackServerSocket::Bind(const char* dir, const char* name, int which) // A revoir : utilisation de "which"
{
    struct sockaddr_un addr;

    if ((fSocket = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        jack_error("Cannot create server socket err = %s", strerror(errno));
        return -1;
    }

    addr.sun_family = AF_UNIX;

    // TO CORRECT: always reuse the same name for now...
    snprintf(fName, sizeof(addr.sun_path) - 1, "%s/jack_%s_%d", dir, name, which);
    strncpy(addr.sun_path, fName, sizeof(addr.sun_path) - 1);
    /*
    if (access(addr.sun_path, F_OK) == 0) {
    	goto error;
    }
    */

    jack_log("Bind: addr.sun_path %s", addr.sun_path);
    unlink(fName); // Security...

    if (bind(fSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        jack_error("Cannot bind server to socket err = %s", strerror(errno));
        goto error;
    }

    if (listen(fSocket, 1) < 0) {
        jack_error("Cannot enable listen on server socket err = %s", strerror(errno));
        goto error;
    }

    return 0;

error:
    unlink(fName);
    close(fSocket);
    return -1;
}

int JackServerSocket::Bind(const char* dir, int which) // A revoir : utilisation de "which"
{
    struct sockaddr_un addr;

    if ((fSocket = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        jack_error("Cannot create server socket err = %s", strerror(errno));
        return -1;
    }

    addr.sun_family = AF_UNIX;

    /*
    for (int i = 0; i < 999; i++) {
    	snprintf(addr.sun_path, sizeof(addr.sun_path) - 1,"%s/jack_%d", dir, i);
    	snprintf(fName, sizeof(addr.sun_path) - 1,"%s/jack_%d", dir, i);
    	if (access(addr.sun_path, F_OK) != 0) {
    		break;
    	}
    }
    */

    // TO CORRECT: always reuse the same name for now...
    snprintf(fName, sizeof(addr.sun_path) - 1, "%s/jack_%d", dir, which);
    strncpy(addr.sun_path, fName, sizeof(addr.sun_path) - 1);
    /*
    if (access(addr.sun_path, F_OK) == 0) {
    	goto error;
    }
    */

    jack_log("Bind: addr.sun_path %s", addr.sun_path);
    unlink(fName); // Security...

    if (bind(fSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        jack_error("Cannot bind server to socket err = %s", strerror(errno));
        goto error;
    }

    if (listen(fSocket, 1) < 0) {
        jack_error("Cannot enable listen on server socket err = %s", strerror(errno));
        goto error;
    }

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

    int fd = accept(fSocket, (struct sockaddr*) & client_addr, &client_addrlen);
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


