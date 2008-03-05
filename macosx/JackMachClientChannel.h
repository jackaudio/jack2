/*
Copyright (C) 2004-2008 Grame

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

        int Open(const char* server_name, const char* name, char* name_res, JackClient* client, jack_options_t options, jack_status_t* status);
        void Close();

        int Start();
        void Stop();

        int ServerCheck(const char* server_name);

        void ClientCheck(const char* name, char* name_res, int protocol, int options, int* status, int* result);
        void ClientOpen(const char* name, int* shared_engine, int* shared_client, int* shared_graph, int* result);
        void ClientClose(int refnum, int* result);

        void ClientActivate(int refnum, int* result);
        void ClientDeactivate(int refnum, int* result);

        void PortRegister(int refnum, const char* name, const char* type, unsigned int flags, unsigned int buffer_size, unsigned int* port_index, int* result);
        void PortUnRegister(int refnum, jack_port_id_t port_index, int* result);

        void PortConnect(int refnum, const char* src, const char* dst, int* result);
        void PortDisconnect(int refnum, const char* src, const char* dst, int* result);

        void PortConnect(int refnum, jack_port_id_t src, jack_port_id_t dst, int* result);
        void PortDisconnect(int refnum, jack_port_id_t src, jack_port_id_t dst, int* result);

        void SetBufferSize(jack_nframes_t buffer_size, int* result);
        void SetFreewheel(int onoff, int* result);

        void ReleaseTimebase(int refnum, int* result);
        void SetTimebaseCallback(int refnum, int conditional, int* result);

        void GetInternalClientName(int refnum, int int_ref, char* name_res, int* result);
        void InternalClientHandle(int refnum, const char* client_name, int* status, int* int_ref, int* result);
        void InternalClientLoad(int refnum, const char* client_name,  const char* so_name, const char* objet_data, int options, int* status, int* int_ref, int* result);
        void InternalClientUnload(int refnum, int int_ref, int* status, int* result);

        // JackRunnableInterface interface
        bool Execute();
};


} // end of namespace

#endif

