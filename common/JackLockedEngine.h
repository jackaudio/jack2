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
#include "JackTools.h"
#include "JackException.h"

namespace Jack
{

#define TRY_CALL    \
    try {           \

#define CATCH_EXCEPTION_RETURN                      \
    } catch(std::bad_alloc& e) {                    \
        jack_error("Memory allocation error...");   \
        return -1;                                  \
    } catch(JackTemporaryException& e) {                       \
        jack_error("JackTemporaryException : now quits...");   \
        JackTools::KillServer();                     \
        return -1;                                  \
    } catch (...) {                                 \
        jack_error("Unknown error...");             \
        return -1;                                  \
    }                                               \

#define CATCH_ENGINE_EXCEPTION                      \
    } catch(std::bad_alloc& e) {                    \
        jack_error("Memory allocation error...");   \
    } catch (...) {                                 \
        jack_error("Unknown error...");             \
    }                                               \

/*!
\brief Locked Engine, access to methods is serialized using a mutex.
*/

class SERVER_EXPORT JackLockedEngine : public JackLockAble
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
            TRY_CALL
            return fEngine.Open();
            CATCH_EXCEPTION_RETURN
        }
        int Close()
        {
            // No lock needed
            TRY_CALL
            return fEngine.Close();
            CATCH_EXCEPTION_RETURN
        }

        // Client management
        int ClientCheck(const char* name, char* name_res, int protocol, int options, int* status)
        {
            TRY_CALL
            JackLock lock(this);
            return fEngine.ClientCheck(name, name_res, protocol, options, status);
            CATCH_EXCEPTION_RETURN
        }
        int ClientExternalOpen(const char* name, int pid, int* ref, int* shared_engine, int* shared_client, int* shared_graph_manager)
        {
            TRY_CALL
            JackLock lock(this);
            return fEngine.ClientExternalOpen(name, pid, ref, shared_engine, shared_client, shared_graph_manager);
            CATCH_EXCEPTION_RETURN
        }
        int ClientInternalOpen(const char* name, int* ref, JackEngineControl** shared_engine, JackGraphManager** shared_manager, JackClientInterface* client, bool wait)
        {
            TRY_CALL
            JackLock lock(this);
            return fEngine.ClientInternalOpen(name, ref, shared_engine, shared_manager, client, wait);
            CATCH_EXCEPTION_RETURN
        }

        int ClientExternalClose(int refnum)
        {
            TRY_CALL
            JackLock lock(this);
            return fEngine.ClientExternalClose(refnum);
            CATCH_EXCEPTION_RETURN
        }
        int ClientInternalClose(int refnum, bool wait)
        {
            TRY_CALL
            JackLock lock(this);
            return fEngine.ClientInternalClose(refnum, wait);
            CATCH_EXCEPTION_RETURN
        }

        int ClientActivate(int refnum, bool is_real_time)
        {
            TRY_CALL
            JackLock lock(this);
            return fEngine.ClientActivate(refnum, is_real_time);
            CATCH_EXCEPTION_RETURN
        }
        int ClientDeactivate(int refnum)
        {
            TRY_CALL
            JackLock lock(this);
            return fEngine.ClientDeactivate(refnum);
            CATCH_EXCEPTION_RETURN
        }

        // Internal client management
        int GetInternalClientName(int int_ref, char* name_res)
        {
            TRY_CALL
            JackLock lock(this);
            return fEngine.GetInternalClientName(int_ref, name_res);
            CATCH_EXCEPTION_RETURN
        }
        int InternalClientHandle(const char* client_name, int* status, int* int_ref)
        {
            TRY_CALL
            JackLock lock(this);
            return fEngine.InternalClientHandle(client_name, status, int_ref);
            CATCH_EXCEPTION_RETURN
        }
        int InternalClientUnload(int refnum, int* status)
        {
            TRY_CALL
            JackLock lock(this);
            return fEngine.InternalClientUnload(refnum, status);
            CATCH_EXCEPTION_RETURN
        }

        // Port management
        int PortRegister(int refnum, const char* name, const char *type, unsigned int flags, unsigned int buffer_size, jack_port_id_t* port)
        {
            TRY_CALL
            JackLock lock(this);
            return fEngine.PortRegister(refnum, name, type, flags, buffer_size, port);
            CATCH_EXCEPTION_RETURN
        }
        int PortUnRegister(int refnum, jack_port_id_t port)
        {
            TRY_CALL
            JackLock lock(this);
            return fEngine.PortUnRegister(refnum, port);
            CATCH_EXCEPTION_RETURN
        }

        int PortConnect(int refnum, const char* src, const char* dst)
        {
            TRY_CALL
            JackLock lock(this);
            return fEngine.PortConnect(refnum, src, dst);
            CATCH_EXCEPTION_RETURN
        }
        int PortDisconnect(int refnum, const char* src, const char* dst)
        {
            TRY_CALL
            JackLock lock(this);
            return fEngine.PortDisconnect(refnum, src, dst);
            CATCH_EXCEPTION_RETURN
        }

        int PortConnect(int refnum, jack_port_id_t src, jack_port_id_t dst)
        {
            TRY_CALL
            JackLock lock(this);
            return fEngine.PortConnect(refnum, src, dst);
            CATCH_EXCEPTION_RETURN
        }
        int PortDisconnect(int refnum, jack_port_id_t src, jack_port_id_t dst)
        {
            TRY_CALL
            JackLock lock(this);
            return fEngine.PortDisconnect(refnum, src, dst);
            CATCH_EXCEPTION_RETURN
        }

        int PortRename(int refnum, jack_port_id_t port, const char* name)
        {
            TRY_CALL
            JackLock lock(this);
            return fEngine.PortRename(refnum, port, name);
            CATCH_EXCEPTION_RETURN
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
            TRY_CALL
            JackLock lock(this);
            fEngine.NotifyXRun(refnum);
            CATCH_ENGINE_EXCEPTION
        }
        void NotifyGraphReorder()
        {
            TRY_CALL
            JackLock lock(this);
            fEngine.NotifyGraphReorder();
            CATCH_ENGINE_EXCEPTION
        }
        void NotifyBufferSize(jack_nframes_t buffer_size)
        {
            TRY_CALL
            JackLock lock(this);
            fEngine.NotifyBufferSize(buffer_size);
            CATCH_ENGINE_EXCEPTION
        }
        void NotifySampleRate(jack_nframes_t sample_rate)
        {
            TRY_CALL
            JackLock lock(this);
            fEngine.NotifySampleRate(sample_rate);
            CATCH_ENGINE_EXCEPTION
        }
        void NotifyFreewheel(bool onoff)
        {
            TRY_CALL
            JackLock lock(this);
            fEngine.NotifyFreewheel(onoff);
            CATCH_ENGINE_EXCEPTION
        }

        void NotifyFailure(int code, const char* reason)
        {
            TRY_CALL
            JackLock lock(this);
            fEngine.NotifyFailure(code, reason);
            CATCH_ENGINE_EXCEPTION
        }

        int GetClientPID(const char* name)
        {
            TRY_CALL
            JackLock lock(this);
            return fEngine.GetClientPID(name);
            CATCH_EXCEPTION_RETURN
        }

        int GetClientRefNum(const char* name)
        {
            TRY_CALL
            JackLock lock(this);
            return fEngine.GetClientRefNum(name);
            CATCH_EXCEPTION_RETURN
        }

};

} // end of namespace

#endif

