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

/*
See : http://groups.google.com/group/comp.programming.threads/browse_thread/thread/652bcf186fbbf697/f63757846514e5e5

catch (...) {
    // Assuming thread cancellation, must rethrow
    throw;
}
*/

#define CATCH_EXCEPTION_RETURN                      \
    } catch (std::bad_alloc& e) {                    \
        jack_error("Memory allocation error...");   \
        return -1;                                  \
    } catch (...) {                                 \
        jack_error("Unknown error...");             \
        throw;                                      \
    }                                               \

#define CATCH_CLOSE_EXCEPTION_RETURN                      \
    } catch (std::bad_alloc& e) {                    \
        jack_error("Memory allocation error...");   \
        return -1;                                  \
    } catch (JackTemporaryException& e) {                       \
        jack_error("JackTemporaryException : now quits...");   \
        JackTools::KillServer();                     \
        return 0;                                   \
    } catch (...) {                                 \
        jack_error("Unknown error...");             \
        throw;                                      \
    }

#define CATCH_EXCEPTION                      \
    } catch (std::bad_alloc& e) {                    \
        jack_error("Memory allocation error...");   \
    } catch (...) {                                 \
        jack_error("Unknown error...");             \
        throw;                                      \
    }                                               \


/*!
\brief Locked Engine, access to methods is serialized using a mutex.
*/

class SERVER_EXPORT JackLockedEngine
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
        
        void ShutDown()
        {
            // No lock needed
            TRY_CALL
            fEngine.ShutDown();
            CATCH_EXCEPTION
        }

        // Client management
        int ClientCheck(const char* name, int uuid, char* name_res, int protocol, int options, int* status)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            return fEngine.ClientCheck(name, uuid, name_res, protocol, options, status);
            CATCH_EXCEPTION_RETURN
        }
        int ClientExternalOpen(const char* name, int pid, int uuid, int* ref, int* shared_engine, int* shared_client, int* shared_graph_manager)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            return fEngine.ClientExternalOpen(name, pid, uuid, ref, shared_engine, shared_client, shared_graph_manager);
            CATCH_EXCEPTION_RETURN
        }
        int ClientInternalOpen(const char* name, int* ref, JackEngineControl** shared_engine, JackGraphManager** shared_manager, JackClientInterface* client, bool wait)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            return fEngine.ClientInternalOpen(name, ref, shared_engine, shared_manager, client, wait);
            CATCH_EXCEPTION_RETURN
        }

        int ClientExternalClose(int refnum)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            return (fEngine.CheckClient(refnum)) ? fEngine.ClientExternalClose(refnum) : -1;
            CATCH_CLOSE_EXCEPTION_RETURN
        }
        int ClientInternalClose(int refnum, bool wait)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            return (fEngine.CheckClient(refnum)) ? fEngine.ClientInternalClose(refnum, wait) : -1;
            CATCH_CLOSE_EXCEPTION_RETURN
        }

        int ClientActivate(int refnum, bool is_real_time)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            return (fEngine.CheckClient(refnum)) ? fEngine.ClientActivate(refnum, is_real_time) : -1;
            CATCH_EXCEPTION_RETURN
        }
        int ClientDeactivate(int refnum)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            return (fEngine.CheckClient(refnum)) ? fEngine.ClientDeactivate(refnum) : -1;
            CATCH_EXCEPTION_RETURN
        }

        // Internal client management
        int GetInternalClientName(int int_ref, char* name_res)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            return fEngine.GetInternalClientName(int_ref, name_res);
            CATCH_EXCEPTION_RETURN
        }
        int InternalClientHandle(const char* client_name, int* status, int* int_ref)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            return fEngine.InternalClientHandle(client_name, status, int_ref);
            CATCH_EXCEPTION_RETURN
        }
        int InternalClientUnload(int refnum, int* status)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            // Client is tested in fEngine.InternalClientUnload
            return fEngine.InternalClientUnload(refnum, status);
            CATCH_EXCEPTION_RETURN
        }

        // Port management
        int PortRegister(int refnum, const char* name, const char *type, unsigned int flags, unsigned int buffer_size, jack_port_id_t* port)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            return (fEngine.CheckClient(refnum)) ? fEngine.PortRegister(refnum, name, type, flags, buffer_size, port) : -1;
            CATCH_EXCEPTION_RETURN
        }
        int PortUnRegister(int refnum, jack_port_id_t port)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            return (fEngine.CheckClient(refnum)) ? fEngine.PortUnRegister(refnum, port) : -1;
            CATCH_EXCEPTION_RETURN
        }

        int PortConnect(int refnum, const char* src, const char* dst)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            return (fEngine.CheckClient(refnum)) ? fEngine.PortConnect(refnum, src, dst) : -1;
            CATCH_EXCEPTION_RETURN
        }
        int PortDisconnect(int refnum, const char* src, const char* dst)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            return (fEngine.CheckClient(refnum)) ? fEngine.PortDisconnect(refnum, src, dst) : -1;
            CATCH_EXCEPTION_RETURN
        }

        int PortConnect(int refnum, jack_port_id_t src, jack_port_id_t dst)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            return (fEngine.CheckClient(refnum)) ? fEngine.PortConnect(refnum, src, dst) : -1;
            CATCH_EXCEPTION_RETURN
        }
        int PortDisconnect(int refnum, jack_port_id_t src, jack_port_id_t dst)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            return (fEngine.CheckClient(refnum)) ? fEngine.PortDisconnect(refnum, src, dst) : -1;
            CATCH_EXCEPTION_RETURN
        }

        int PortRename(int refnum, jack_port_id_t port, const char* name)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            return (fEngine.CheckClient(refnum)) ? fEngine.PortRename(refnum, port, name) : -1;
            CATCH_EXCEPTION_RETURN
        }

        int ComputeTotalLatencies()
        {
            TRY_CALL
            JackLock lock(&fEngine);
            return fEngine.ComputeTotalLatencies();
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
            // RT : no lock
            fEngine.NotifyXRun(refnum);
        }

        void NotifyGraphReorder()
        {
            TRY_CALL
            JackLock lock(&fEngine);
            fEngine.NotifyGraphReorder();
            CATCH_EXCEPTION
        }

        void NotifyBufferSize(jack_nframes_t buffer_size)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            fEngine.NotifyBufferSize(buffer_size);
            CATCH_EXCEPTION
        }
        void NotifySampleRate(jack_nframes_t sample_rate)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            fEngine.NotifySampleRate(sample_rate);
            CATCH_EXCEPTION
        }
        void NotifyFreewheel(bool onoff)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            fEngine.NotifyFreewheel(onoff);
            CATCH_EXCEPTION
        }

        void NotifyFailure(int code, const char* reason)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            fEngine.NotifyFailure(code, reason);
            CATCH_EXCEPTION
        }

        int GetClientPID(const char* name)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            return fEngine.GetClientPID(name);
            CATCH_EXCEPTION_RETURN
        }

        int GetClientRefNum(const char* name)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            return fEngine.GetClientRefNum(name);
            CATCH_EXCEPTION_RETURN
        }

        void NotifyQuit()
        {
            // No lock needed
            TRY_CALL
            return fEngine.NotifyQuit();
            CATCH_EXCEPTION
        }

        void SessionNotify(int refnum, const char* target, jack_session_event_type_t type, const char *path, detail::JackChannelTransactionInterface *socket, JackSessionNotifyResult** result)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            fEngine.SessionNotify(refnum, target, type, path, socket, result);
            CATCH_EXCEPTION
        }

        int SessionReply(int refnum)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            return fEngine.SessionReply(refnum);
            CATCH_EXCEPTION_RETURN
        }

        int GetUUIDForClientName(const char *client_name, char *uuid_res)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            return fEngine.GetUUIDForClientName(client_name, uuid_res);
            CATCH_EXCEPTION_RETURN
        }
        int GetClientNameForUUID(const char *uuid, char *name_res)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            return fEngine.GetClientNameForUUID(uuid, name_res);
            CATCH_EXCEPTION_RETURN
        }
        int ReserveClientName(const char *name, const char *uuid)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            return fEngine.ReserveClientName(name, uuid);
            CATCH_EXCEPTION_RETURN
        }

        int ClientHasSessionCallback(const char *name)
        {
            TRY_CALL
            JackLock lock(&fEngine);
            return fEngine.ClientHasSessionCallback(name);
            CATCH_EXCEPTION_RETURN
        }
};

} // end of namespace

#endif

