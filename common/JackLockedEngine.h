/*
Copyright (C) 2008 Grame

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

#ifndef __JackLockedEngine__
#define __JackLockedEngine__

#include "JackEngine.h"
#include "JackMutex.h"

namespace Jack
{

/*!
\brief Locked Engine, access to methods is serialized using a mutex.
*/

class EXPORT JackLockedEngine : public JackLockAble
{
    private:

        JackEngine fEngine;

    public:

        JackLockedEngine(JackGraphManager* manager, JackSynchro* table, JackEngineControl* controler):
            fEngine(manager, table, controler)
        {}
        ~JackLockedEngine()
        {}

        int Open()
        {
            // No lock needed
            return fEngine.Open();
        }
        int Close()
        {
            // No lock needed
            return fEngine.Close();
        }

        // Client management
        int ClientCheck(const char* name, char* name_res, int protocol, int options, int* status)
        {
            JackLock lock(this);
            return fEngine.ClientCheck(name, name_res, protocol, options, status);
        }
        int ClientExternalOpen(const char* name, int pid, int* ref, int* shared_engine, int* shared_client, int* shared_graph_manager)
        {
            JackLock lock(this);
            return fEngine.ClientExternalOpen(name, pid, ref, shared_engine, shared_client, shared_graph_manager);
        }
        int ClientInternalOpen(const char* name, int* ref, JackEngineControl** shared_engine, JackGraphManager** shared_manager, JackClientInterface* client, bool wait)
        {
            JackLock lock(this);
            return fEngine.ClientInternalOpen(name, ref, shared_engine, shared_manager, client, wait);
        }

        int ClientExternalClose(int refnum)
        {
            JackLock lock(this);
            return fEngine.ClientExternalClose(refnum);
        }
        int ClientInternalClose(int refnum, bool wait)
        {
            JackLock lock(this);
            return fEngine.ClientInternalClose(refnum, wait);
        }

        int ClientActivate(int refnum, bool state)
        {
            JackLock lock(this);
            return fEngine.ClientActivate(refnum, state);
        }
        int ClientDeactivate(int refnum)
        {
            JackLock lock(this);
            return fEngine.ClientDeactivate(refnum);
        }

        // Internal client management
        int GetInternalClientName(int int_ref, char* name_res)
        {
            JackLock lock(this);
            return fEngine.GetInternalClientName(int_ref, name_res);
        }
        int InternalClientHandle(const char* client_name, int* status, int* int_ref)
        {
            JackLock lock(this);
            return fEngine.InternalClientHandle(client_name, status, int_ref);
        }
        int InternalClientUnload(int refnum, int* status)
        {
            JackLock lock(this);
            return fEngine.InternalClientUnload(refnum, status);
        }

        // Port management
        int PortRegister(int refnum, const char* name, const char *type, unsigned int flags, unsigned int buffer_size, unsigned int* port)
        {
            JackLock lock(this);
            return fEngine.PortRegister(refnum, name, type, flags, buffer_size, port);
        }
        int PortUnRegister(int refnum, jack_port_id_t port)
        {
            JackLock lock(this);
            return fEngine.PortUnRegister(refnum, port);
        }

        int PortConnect(int refnum, const char* src, const char* dst)
        {
            JackLock lock(this);
            return fEngine.PortConnect(refnum, src, dst);
        }
        int PortDisconnect(int refnum, const char* src, const char* dst)
        {
            JackLock lock(this);
            return fEngine.PortDisconnect(refnum, src, dst);
        }

        int PortConnect(int refnum, jack_port_id_t src, jack_port_id_t dst)
        {
            JackLock lock(this);
            return fEngine.PortConnect(refnum, src, dst);
        }
        int PortDisconnect(int refnum, jack_port_id_t src, jack_port_id_t dst)
        {
            JackLock lock(this);
            return fEngine.PortDisconnect(refnum, src, dst);
        }

        // Graph
        bool Process(jack_time_t cur_cycle_begin, jack_time_t prev_cycle_end)
        {
            // RT : no lock
            return fEngine.Process(cur_cycle_begin, prev_cycle_end);
        }

        // Notifications
        void NotifyXRun(jack_time_t cur_cycle_begin, float delayed_usecs)
        {
            // RT : no lock
            fEngine.NotifyXRun(cur_cycle_begin, delayed_usecs);
        }

        void NotifyXRun(int refnum)
        {
            JackLock lock(this);
            fEngine.NotifyXRun(refnum);
        }
        void NotifyGraphReorder()
        {
            JackLock lock(this);
            fEngine.NotifyGraphReorder();
        }
        void NotifyBufferSize(jack_nframes_t buffer_size)
        {
            JackLock lock(this);
            fEngine.NotifyBufferSize(buffer_size);
        }
        void NotifySampleRate(jack_nframes_t sample_rate)
        {
            JackLock lock(this);
            fEngine.NotifySampleRate(sample_rate);
        }
        void NotifyFreewheel(bool onoff)
        {
            JackLock lock(this);
            fEngine.NotifyFreewheel(onoff);
        }
    
        int GetClientPID(const char* name)
        {
            JackLock lock(this);
            return fEngine.GetClientPID(name);
        }

};

} // end of namespace

#endif

