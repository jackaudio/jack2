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

#include "JackFifo.h"
#include "JackTools.h"
#include "JackError.h"
#include "JackPlatformPlug.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

namespace Jack
{

void JackFifo::BuildName(const char* client_name, const char* server_name, char* res)
{
    char ext_client_name[JACK_CLIENT_NAME_SIZE + 1];
    JackTools::RewriteName(client_name, ext_client_name);
    sprintf(res, "%s/jack_fifo.%d_%s_%s", jack_client_dir, JackTools::GetUID(), server_name, ext_client_name);
}

bool JackFifo::Signal()
{
    bool res;
    char c = 0;

    if (fFifo < 0) {
        jack_error("JackFifo::Signal name = %s already desallocated!!", fName);
        return false;
    }

    if (fFlush)
        return true;

    if ((res = (write(fFifo, &c, sizeof(c)) != sizeof(c)))) {
        jack_error("JackFifo::Signal name = %s err = %s", fName, strerror(errno));
    }
    return !res;
}

bool JackFifo::SignalAll()
{
    bool res;
    char c = 0;

    if (fFifo < 0) {
        jack_error("JackFifo::SignalAll name = %s already desallocated!!", fName);
        return false;
    }

    if (fFlush)
        return true;

    if ((res = (write(fFifo, &c, sizeof(c)) != sizeof(c)))) {
        jack_error("JackFifo::SignalAll name = %s err = %s", fName, strerror(errno));
    }
    return !res;
}

bool JackFifo::Wait()
{
    bool res;
    char c;

    if (fFifo < 0) {
        jack_error("JackFifo::Wait name = %s already desallocated!!", fName);
        return false;
    }

    if ((res = (read(fFifo, &c, sizeof(c)) != sizeof(c)))) {
        jack_error("JackFifo::Wait name = %s err = %s", fName, strerror(errno));
    }
    return !res;
}

#ifdef __APPLE__
#warning JackFifo::TimedWait not available : synchronous mode may not work correctly if FIFO are used
bool JackFifo::TimedWait(long usec)
{
    return Wait();
}
#else
// Does not work on OSX ??
bool JackFifo::TimedWait(long usec)
{
    int res;
    
    if (fFifo < 0) {
        jack_error("JackFifo::TimedWait name = %s already desallocated!!", fName);
        return false;
    }
   
    do {
        res = poll(&fPoll, 1, usec / 1000);
    } while (res < 0 && errno == EINTR);

    if (fPoll.revents & POLLIN) {
        return Wait();
    } else {
        // Wait failure but we still continue...
        jack_log("JackFifo::TimedWait name = %s usec = %ld err = %s", fName, usec, strerror(errno));
        return true;
    }
}
#endif

// Server side
bool JackFifo::Allocate(const char* name, const char* server_name, int value)
{
    struct stat statbuf;
    BuildName(name, server_name, fName);
    jack_log("JackFifo::Allocate name = %s", fName);

    if (stat(fName, &statbuf) < 0) {
        if (errno == ENOENT || errno == EPERM) {
            if (mkfifo(fName, 0666) < 0) {
                jack_error("Cannot create inter-client FIFO name = %s err = %s", name, strerror(errno));
                return false;
            }
        } else {
            jack_error("Cannot check on FIFO %s", name);
            return false;
        }
    } else {
        if (!S_ISFIFO(statbuf.st_mode)) {
            jack_error("FIFO name = %s already exists, but is not a FIFO", name);
            return false;
        }
    }

    if ((fFifo = open(fName, O_RDWR | O_CREAT, 0666)) < 0) {
        jack_error("Cannot open FIFO name = %s err = %s", name, strerror(errno));
        return false;
    } else {
        fPoll.fd = fFifo;
        fPoll.events = POLLERR | POLLIN | POLLHUP | POLLNVAL;
        return true;
    }
}

// Client side
bool JackFifo::ConnectAux(const char* name, const char* server_name, int access)
{
    BuildName(name, server_name, fName);
    jack_log("JackFifo::ConnectAux name = %s", fName);

    // Temporary...
    if (fFifo >= 0) {
        jack_log("Already connected name = %s", name);
        return true;
    }

    if ((fFifo = open(fName, access)) < 0) {
        jack_error("Connect: can't connect named fifo name = %s err = %s", fName, strerror(errno));
        return false;
    } else {
        fPoll.fd = fFifo;
        fPoll.events = POLLERR | POLLIN | POLLHUP | POLLNVAL;
        return true;
    }
}

bool JackFifo::Connect(const char* name, const char* server_name)
{
    return ConnectAux(name, server_name, O_RDWR);
}

bool JackFifo::ConnectOutput(const char* name, const char* server_name)
{
    return ConnectAux(name, server_name, O_WRONLY | O_NONBLOCK);
}

bool JackFifo::ConnectInput(const char* name, const char* server_name)
{
    return ConnectAux(name, server_name, O_RDONLY);
}

bool JackFifo::Disconnect()
{
    if (fFifo >= 0) {
        jack_log("JackFifo::Disconnect %s", fName);
        if (close(fFifo) != 0) {
            jack_error("Disconnect: can't disconnect named fifo name = %s err = %s", fName, strerror(errno));
            return false;
        } else {
            fFifo = -1;
            return true;
        }
    } else {
        return true;
    }
}

// Server side : destroy the fifo
void JackFifo::Destroy()
{
    if (fFifo > 0) {
        jack_log("JackFifo::Destroy name = %s", fName);
        unlink(fName);
        if (close(fFifo) != 0) {
            jack_error("Destroy: can't destroy fifo name = %s err = %s", fName, strerror(errno));
        }
        fFifo = -1;
    } else {
        jack_error("JackFifo::Destroy fifo < 0");
    }
}

} // end of namespace

