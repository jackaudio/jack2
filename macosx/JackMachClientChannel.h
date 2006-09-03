/*
Copyright (C) 2004-2006 Grame  

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

#ifndef __JackMachClientChannel__
#define __JackMachClientChannel__

#include "JackChannel.h"
#include "JackMachPort.h"
#include "JackThread.h"

namespace Jack
{

class JackLibClient;

/*!
\brief JackClientChannel using Mach IPC.
*/

class JackMachClientChannel : public JackClientChannelInterface, public JackRunnableInterface
{

    private:

        JackMachPort fClientPort;    /*! Mach port to communicate with the server : from server to client */
        JackMachPort fServerPort;    /*! Mach port to communicate with the server : from client to server */
        mach_port_t	fPrivatePort;
        JackThread*	fThread;		 /*! Thread to execute the event loop */

    public:

        JackMachClientChannel();
        virtual ~JackMachClientChannel();

        int Open(const char* name, JackClient* obj);
        void Close();

        int Start();
        void Stop();

        void ClientNew(const char* name, int* shared_engine, int* shared_client, int* shared_ports, int* result);
        void ClientClose(int refnum, int* result);

        void ClientActivate(int refnum, int* result);
        void ClientDeactivate(int refnum, int* result);

        void PortRegister(int refnum, const char* name, unsigned int flags, unsigned int buffer_size, jack_port_id_t* port_index, int* result);
        void PortUnRegister(int refnum, jack_port_id_t port_index, int* result);

        void PortConnect(int refnum, const char* src, const char* dst, int* result);
        void PortDisconnect(int refnum, const char* src, const char* dst, int* result);

        void PortConnect(int refnum, jack_port_id_t src, jack_port_id_t dst, int* result);
        void PortDisconnect(int refnum, jack_port_id_t src, jack_port_id_t dst, int* result);

        void SetBufferSize(jack_nframes_t nframes, int* result);
        void SetFreewheel(int onoff, int* result);

        void ReleaseTimebase(int refnum, int* result);
        void SetTimebaseCallback(int refnum, int conditional, int* result);

        bool Execute();
};


} // end of namespace

#endif

