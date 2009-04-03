/*
Copyright (C) 2001-2003 Paul Davis
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

#include "JackClient.h"
#include "JackError.h"
#include "JackGraphManager.h"
#include "JackEngineControl.h"
#include "JackClientControl.h"
#include "JackGlobals.h"
#include "JackTime.h"
#include "JackCompilerDeps.h"
#include "JackPortType.h"
#include "JackPlatformPlug.h"
#include <math.h>

#ifdef __CLIENTDEBUG__
#include "JackLibGlobals.h"
#endif

using namespace Jack;

#ifdef __cplusplus
extern "C"
{
#endif

    typedef void (*print_function)(const char *);
    typedef void *(*thread_routine)(void*);

    EXPORT
    void
    jack_get_version(
        int *major_ptr,
        int *minor_ptr,
        int *micro_ptr,
        int *proto_ptr);

    EXPORT
    const char *
    jack_get_version_string();

    EXPORT jack_client_t * jack_client_open_aux (const char *client_name,
            jack_options_t options,
            jack_status_t *status, va_list ap);
    EXPORT jack_client_t * jack_client_open (const char *client_name,
            jack_options_t options,
            jack_status_t *status, ...);
    EXPORT jack_client_t * jack_client_new (const char *client_name);
    EXPORT int jack_client_name_size (void);
    EXPORT char* jack_get_client_name (jack_client_t *client);
    EXPORT int jack_internal_client_new (const char *client_name,
                                         const char *load_name,
                                         const char *load_init);
    EXPORT void jack_internal_client_close (const char *client_name);
    EXPORT int jack_is_realtime (jack_client_t *client);
    EXPORT void jack_on_shutdown (jack_client_t *client,
                                  JackShutdownCallback shutdown_callback, void *arg);
    EXPORT int jack_set_process_callback (jack_client_t *client,
                                          JackProcessCallback process_callback,
                                          void *arg);
    EXPORT jack_nframes_t jack_thread_wait(jack_client_t *client, int status);

    // new
    EXPORT jack_nframes_t jack_cycle_wait (jack_client_t*);
    EXPORT void jack_cycle_signal (jack_client_t*, int status);
    EXPORT int jack_set_process_thread(jack_client_t* client, JackThreadCallback fun, void *arg);

    EXPORT int jack_set_thread_init_callback (jack_client_t *client,
            JackThreadInitCallback thread_init_callback,
            void *arg);
    EXPORT int jack_set_freewheel_callback (jack_client_t *client,
                                            JackFreewheelCallback freewheel_callback,
                                            void *arg);
    EXPORT int jack_set_freewheel(jack_client_t* client, int onoff);
    EXPORT int jack_set_buffer_size (jack_client_t *client, jack_nframes_t nframes);
    EXPORT int jack_set_buffer_size_callback (jack_client_t *client,
            JackBufferSizeCallback bufsize_callback,
            void *arg);
    EXPORT int jack_set_sample_rate_callback (jack_client_t *client,
            JackSampleRateCallback srate_callback,
            void *arg);
    EXPORT int jack_set_client_registration_callback (jack_client_t *,
            JackClientRegistrationCallback
            registration_callback, void *arg);
    EXPORT int jack_set_port_registration_callback (jack_client_t *,
            JackPortRegistrationCallback
            registration_callback, void *arg);
    EXPORT int jack_set_port_connect_callback (jack_client_t *,
            JackPortConnectCallback
            connect_callback, void *arg);
    EXPORT int jack_set_port_rename_callback (jack_client_t *,
				    JackPortRenameCallback
				    rename_callback, void *arg);
    EXPORT int jack_set_graph_order_callback (jack_client_t *,
            JackGraphOrderCallback graph_callback,
            void *);
    EXPORT int jack_set_xrun_callback (jack_client_t *,
                                       JackXRunCallback xrun_callback, void *arg);
    EXPORT int jack_activate (jack_client_t *client);
    EXPORT int jack_deactivate (jack_client_t *client);
    EXPORT jack_port_t * jack_port_register (jack_client_t *client,
            const char *port_name,
            const char *port_type,
            unsigned long flags,
            unsigned long buffer_size);
    EXPORT int jack_port_unregister (jack_client_t *, jack_port_t *);
    EXPORT void * jack_port_get_buffer (jack_port_t *, jack_nframes_t);
    EXPORT const char * jack_port_name (const jack_port_t *port);
    EXPORT const char * jack_port_short_name (const jack_port_t *port);
    EXPORT int jack_port_flags (const jack_port_t *port);
    EXPORT const char * jack_port_type (const jack_port_t *port);
    EXPORT jack_port_type_id_t jack_port_type_id (const jack_port_t *port);
    EXPORT int jack_port_is_mine (const jack_client_t *, const jack_port_t *port);
    EXPORT int jack_port_connected (const jack_port_t *port);
    EXPORT int jack_port_connected_to (const jack_port_t *port,
                                       const char *port_name);
    EXPORT const char ** jack_port_get_connections (const jack_port_t *port);
    EXPORT const char ** jack_port_get_all_connections (const jack_client_t *client,
            const jack_port_t *port);
    EXPORT int jack_port_tie (jack_port_t *src, jack_port_t *dst);
    EXPORT int jack_port_untie (jack_port_t *port);
    EXPORT jack_nframes_t jack_port_get_latency (jack_port_t *port);
    EXPORT jack_nframes_t jack_port_get_total_latency (jack_client_t *,
            jack_port_t *port);
    EXPORT void jack_port_set_latency (jack_port_t *, jack_nframes_t);
    EXPORT int jack_recompute_total_latency (jack_client_t*, jack_port_t* port);
    EXPORT int jack_recompute_total_latencies (jack_client_t*);
    EXPORT int jack_port_set_name (jack_port_t *port, const char *port_name);
    EXPORT int jack_port_set_alias (jack_port_t *port, const char *alias);
    EXPORT int jack_port_unset_alias (jack_port_t *port, const char *alias);
    EXPORT int jack_port_get_aliases (const jack_port_t *port, char* const aliases[2]);
    EXPORT int jack_port_request_monitor (jack_port_t *port, int onoff);
    EXPORT int jack_port_request_monitor_by_name (jack_client_t *client,
            const char *port_name, int onoff);
    EXPORT int jack_port_ensure_monitor (jack_port_t *port, int onoff);
    EXPORT int jack_port_monitoring_input (jack_port_t *port);
    EXPORT int jack_connect (jack_client_t *,
                             const char *source_port,
                             const char *destination_port);
    EXPORT int jack_disconnect (jack_client_t *,
                                const char *source_port,
                                const char *destination_port);
    EXPORT int jack_port_disconnect (jack_client_t *, jack_port_t *);
    EXPORT int jack_port_name_size(void);
    EXPORT int jack_port_type_size(void);
    EXPORT jack_nframes_t jack_get_sample_rate (jack_client_t *);
    EXPORT jack_nframes_t jack_get_buffer_size (jack_client_t *);
    EXPORT const char ** jack_get_ports (jack_client_t *,
                                         const char *port_name_pattern,
                                         const char *type_name_pattern,
                                         unsigned long flags);
    EXPORT jack_port_t * jack_port_by_name (jack_client_t *, const char *port_name);
    EXPORT jack_port_t * jack_port_by_id (jack_client_t *client,
                                          jack_port_id_t port_id);
    EXPORT int jack_engine_takeover_timebase (jack_client_t *);
    EXPORT jack_nframes_t jack_frames_since_cycle_start (const jack_client_t *);
    EXPORT jack_time_t jack_get_time();
    EXPORT jack_nframes_t jack_time_to_frames(const jack_client_t *client, jack_time_t time);
    EXPORT jack_time_t jack_frames_to_time(const jack_client_t *client, jack_nframes_t frames);
    EXPORT jack_nframes_t jack_frame_time (const jack_client_t *);
    EXPORT jack_nframes_t jack_last_frame_time (const jack_client_t *client);
    EXPORT float jack_cpu_load (jack_client_t *client);
    EXPORT pthread_t jack_client_thread_id (jack_client_t *);
    EXPORT void jack_set_error_function (print_function);
    EXPORT void jack_set_info_function (print_function);

    EXPORT float jack_get_max_delayed_usecs (jack_client_t *client);
    EXPORT float jack_get_xrun_delayed_usecs (jack_client_t *client);
    EXPORT void jack_reset_max_delayed_usecs (jack_client_t *client);

    EXPORT int jack_release_timebase (jack_client_t *client);
    EXPORT int jack_set_sync_callback (jack_client_t *client,
                                       JackSyncCallback sync_callback,
                                       void *arg);
    EXPORT int jack_set_sync_timeout (jack_client_t *client,
                                      jack_time_t timeout);
    EXPORT int jack_set_timebase_callback (jack_client_t *client,
                                           int conditional,
                                           JackTimebaseCallback timebase_callback,
                                           void *arg);
    EXPORT int jack_transport_locate (jack_client_t *client,
                                      jack_nframes_t frame);
    EXPORT jack_transport_state_t jack_transport_query (const jack_client_t *client,
            jack_position_t *pos);
    EXPORT jack_nframes_t jack_get_current_transport_frame (const jack_client_t *client);
    EXPORT int jack_transport_reposition (jack_client_t *client,
                                          jack_position_t *pos);
    EXPORT void jack_transport_start (jack_client_t *client);
    EXPORT void jack_transport_stop (jack_client_t *client);
    EXPORT void jack_get_transport_info (jack_client_t *client,
                                         jack_transport_info_t *tinfo);
    EXPORT void jack_set_transport_info (jack_client_t *client,
                                         jack_transport_info_t *tinfo);

    EXPORT int jack_client_real_time_priority (jack_client_t*);
    EXPORT int jack_client_max_real_time_priority (jack_client_t*);
    EXPORT int jack_acquire_real_time_scheduling (pthread_t thread, int priority);
    EXPORT int jack_client_create_thread (jack_client_t* client,
                                          pthread_t *thread,
                                          int priority,
                                          int realtime, 	// boolean
                                          thread_routine routine,
                                          void *arg);
    EXPORT int jack_drop_real_time_scheduling (pthread_t thread);

    EXPORT int jack_client_stop_thread (jack_client_t* client, pthread_t thread);
    EXPORT int jack_client_kill_thread (jack_client_t* client, pthread_t thread);
#ifndef WIN32
    EXPORT void jack_set_thread_creator (jack_thread_creator_t jtc);
#endif
    EXPORT char * jack_get_internal_client_name (jack_client_t *client,
            jack_intclient_t intclient);
    EXPORT jack_intclient_t jack_internal_client_handle (jack_client_t *client,
            const char *client_name,
            jack_status_t *status);
    EXPORT jack_intclient_t jack_internal_client_load (jack_client_t *client,
            const char *client_name,
            jack_options_t options,
            jack_status_t *status, ...);
    EXPORT jack_intclient_t jack_internal_client_load_aux (jack_client_t *client,
            const char *client_name,
            jack_options_t options,
            jack_status_t *status, va_list ap);

    EXPORT jack_status_t jack_internal_client_unload (jack_client_t *client,
            jack_intclient_t intclient);

#ifdef __cplusplus
}
#endif

static inline bool CheckPort(jack_port_id_t port_index)
{
    return (port_index > 0 && port_index < PORT_NUM);
}

static inline bool CheckBufferSize(jack_nframes_t buffer_size)
{
    return (buffer_size <= BUFFER_SIZE_MAX);
}

static inline void WaitGraphChange()
{
    /*
    TLS key that is set only in RT thread, so never waits for pending
    graph change in RT context (just read the current graph state).
    */

    if (jack_tls_get(JackGlobals::fRealTime) == NULL) {
        JackGraphManager* manager = GetGraphManager();
        JackEngineControl* control = GetEngineControl();
        assert(manager);
        assert(control);
        if (manager->IsPendingChange()) {
            jack_log("WaitGraphChange...");
            JackSleep(int(control->fPeriodUsecs * 1.1f));
        }
    }
}

EXPORT void jack_set_error_function (print_function func)
{
    jack_error_callback = func;
}

EXPORT void jack_set_info_function (print_function func)
{
    jack_info_callback = func;
}

EXPORT jack_client_t* jack_client_new(const char* client_name)
{
    assert(JackGlobals::fOpenMutex);
    JackGlobals::fOpenMutex->Lock();
    jack_error("jack_client_new: deprecated");
    int options = JackUseExactName;
    if (getenv("JACK_START_SERVER") == NULL)
        options |= JackNoStartServer;
    jack_client_t* res = jack_client_open_aux(client_name, (jack_options_t)options, NULL, NULL);
    JackGlobals::fOpenMutex->Unlock();
    return res;
}

EXPORT void* jack_port_get_buffer(jack_port_t* port, jack_nframes_t frames)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
#if defined(__x86_64__) || defined(__ppc64__)
    uint64_t port_aux = (uint64_t)port;
#else
    uint32_t port_aux = (uint32_t)port;
#endif
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_get_buffer called with an incorrect port %ld", myport);
        return NULL;
    } else {
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->GetBuffer(myport, frames) : NULL);
    }
}

EXPORT const char* jack_port_name(const jack_port_t* port)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
#if defined(__x86_64__) || defined(__ppc64__)
    uint64_t port_aux = (uint64_t)port;
#else
    uint32_t port_aux = (uint32_t)port;
#endif
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_name called with an incorrect port %ld", myport);
        return NULL;
    } else {
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->GetPort(myport)->GetName() : NULL);
    }
}

EXPORT const char* jack_port_short_name(const jack_port_t* port)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
#if defined(__x86_64__) || defined(__ppc64__)
    uint64_t port_aux = (uint64_t)port;
#else
    uint32_t port_aux = (uint32_t)port;
#endif
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_short_name called with an incorrect port %ld", myport);
        return NULL;
    } else {
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->GetPort(myport)->GetShortName() : NULL);
    }
}

EXPORT int jack_port_flags(const jack_port_t* port)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
#if defined(__x86_64__) || defined(__ppc64__)
    uint64_t port_aux = (uint64_t)port;
#else
    uint32_t port_aux = (uint32_t)port;
#endif
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_flags called with an incorrect port %ld", myport);
        return -1;
    } else {
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->GetPort(myport)->GetFlags() : -1);
    }
}

EXPORT const char* jack_port_type(const jack_port_t* port)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
 #if defined(__x86_64__) || defined(__ppc64__)
    uint64_t port_aux = (uint64_t)port;
#else
    uint32_t port_aux = (uint32_t)port;
#endif
    jack_port_id_t myport = (jack_port_id_t)port_aux;
   if (!CheckPort(myport)) {
        jack_error("jack_port_flags called an incorrect port %ld", myport);
        return NULL;
    } else {
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->GetPort(myport)->GetType() : NULL);
    }
}

EXPORT jack_port_type_id_t jack_port_type_id(const jack_port_t *port)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
#if defined(__x86_64__) || defined(__ppc64__)
    uint64_t port_aux = (uint64_t)port;
#else
    uint32_t port_aux = (uint32_t)port;
#endif
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_type_id called an incorrect port %ld", myport);
        return 0;
    } else {
        JackGraphManager* manager = GetGraphManager();
        return (manager ? GetPortTypeId(manager->GetPort(myport)->GetType()) : 0);
    }
}

EXPORT int jack_port_connected(const jack_port_t* port)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
#if defined(__x86_64__) || defined(__ppc64__)
    uint64_t port_aux = (uint64_t)port;
#else
    uint32_t port_aux = (uint32_t)port;
#endif
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_connected called with an incorrect port %ld", myport);
        return -1;
    } else {
        WaitGraphChange();
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->GetConnectionsNum(myport) : -1);
    }
}

EXPORT int jack_port_connected_to(const jack_port_t* port, const char* port_name)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
#if defined(__x86_64__) || defined(__ppc64__)
    uint64_t port_aux = (uint64_t)port;
#else
    uint32_t port_aux = (uint32_t)port;
#endif
    jack_port_id_t src = (jack_port_id_t)port_aux;
    if (!CheckPort(src)) {
        jack_error("jack_port_connected_to called with an incorrect port %ld", src);
        return -1;
    } else if (port_name == NULL) {
        jack_error("jack_port_connected_to called with a NULL port name");
        return -1;
    } else {
        WaitGraphChange();
        JackGraphManager* manager = GetGraphManager();
        jack_port_id_t dst = (manager ? manager->GetPort(port_name) : NO_PORT);
        if (dst == NO_PORT) {
            jack_error("Unknown destination port port_name = %s", port_name);
            return 0;
        } else {
            return manager->IsConnected(src, dst);
        }
    }
}

EXPORT int jack_port_tie(jack_port_t* src, jack_port_t* dst)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
#if defined(__x86_64__) || defined(__ppc64__)
    uint64_t src_aux = (uint64_t)src;
#else
    uint32_t src_aux = (uint32_t)src;
#endif
    jack_port_id_t mysrc = (jack_port_id_t)src_aux;
    if (!CheckPort(mysrc)) {
        jack_error("jack_port_tie called with a NULL src port");
        return -1;
    }
#if defined(__x86_64__) || defined(__ppc64__)
    uint64_t dst_aux = (uint64_t)dst;
#else
    uint32_t dst_aux = (uint32_t)dst;
#endif
    jack_port_id_t mydst = (jack_port_id_t)dst_aux;
    if (!CheckPort(mydst)) {
        jack_error("jack_port_tie called with a NULL dst port");
        return -1;
    }
    JackGraphManager* manager = GetGraphManager();
    if (manager && manager->GetPort(mysrc)->GetRefNum() != manager->GetPort(mydst)->GetRefNum()) {
        jack_error("jack_port_tie called with ports not belonging to the same client");
        return -1;
    } else {
        return manager->GetPort(mydst)->Tie(mysrc);
    }
}

EXPORT int jack_port_untie(jack_port_t* port)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
#if defined(__x86_64__) || defined(__ppc64__)
    uint64_t port_aux = (uint64_t)port;
#else
    uint32_t port_aux = (uint32_t)port;
#endif
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_untie called with an incorrect port %ld", myport);
        return -1;
    } else {
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->GetPort(myport)->UnTie() : -1);
    }
}

EXPORT jack_nframes_t jack_port_get_latency(jack_port_t* port)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
#if defined(__x86_64__) || defined(__ppc64__)
    uint64_t port_aux = (uint64_t)port;
#else
    uint32_t port_aux = (uint32_t)port;
#endif
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_get_latency called with an incorrect port %ld", myport);
        return 0;
    } else {
        WaitGraphChange();
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->GetPort(myport)->GetLatency() : 0);
    }
}

EXPORT void jack_port_set_latency(jack_port_t* port, jack_nframes_t frames)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
#if defined(__x86_64__) || defined(__ppc64__)
    uint64_t port_aux = (uint64_t)port;
#else
    uint32_t port_aux = (uint32_t)port;
#endif
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_set_latency called with an incorrect port %ld", myport);
    } else {
        JackGraphManager* manager = GetGraphManager();
        if (manager)
            manager->GetPort(myport)->SetLatency(frames);
    }
}

EXPORT int jack_recompute_total_latency(jack_client_t* ext_client, jack_port_t* port)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif

    JackClient* client = (JackClient*)ext_client;
#if defined(__x86_64__) || defined(__ppc64__)
    uint64_t port_aux = (uint64_t)port;
#else
    uint32_t port_aux = (uint32_t)port;
#endif
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (client == NULL) {
        jack_error("jack_recompute_total_latencies called with a NULL client");
        return -1;
    } else if (!CheckPort(myport)) {
        jack_error("jack_recompute_total_latencies called with a NULL port");
        return -1;
    } else {
        WaitGraphChange();
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->ComputeTotalLatency(myport) : -1);
    }
}

EXPORT int jack_recompute_total_latencies(jack_client_t* ext_client)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_recompute_total_latencies called with a NULL client");
        return -1;
    } else {
        WaitGraphChange();
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->ComputeTotalLatencies() : -1);
    }
}

/*
This is unsafe if case of concurrent access, and should be "serialized" doing a server call.
*/

EXPORT int jack_port_set_name(jack_port_t* port, const char* name)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
#if defined(__x86_64__) || defined(__ppc64__)
    uint64_t port_aux = (uint64_t)port;
#else
    uint32_t port_aux = (uint32_t)port;
#endif
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_set_name called with an incorrect port %ld", myport);
        return -1;
    } else if (name == NULL) {
        jack_error("jack_port_set_name called with a NULL port name");
        return -1;
    } else {
        JackGraphManager* manager = GetGraphManager();
        int refnum;
        if (manager && ((refnum = manager->GetPort(myport)->GetRefNum()) > 0)) {
            JackClient* client = JackGlobals::fClientTable[refnum];
            assert(client);
            return client->PortRename(myport, name);
        } else {
            return -1;
        }
    }
}

EXPORT int jack_port_set_alias(jack_port_t* port, const char* name)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
#if defined(__x86_64__) || defined(__ppc64__)
    uint64_t port_aux = (uint64_t)port;
#else
    uint32_t port_aux = (uint32_t)port;
#endif
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_set_alias called with an incorrect port %ld", myport);
        return -1;
    } else if (name == NULL) {
        jack_error("jack_port_set_alias called with a NULL port name");
        return -1;
    } else {
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->GetPort(myport)->SetAlias(name) : -1);
    }
}

EXPORT int jack_port_unset_alias(jack_port_t* port, const char* name)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
#if defined(__x86_64__) || defined(__ppc64__)
    uint64_t port_aux = (uint64_t)port;
#else
    uint32_t port_aux = (uint32_t)port;
#endif
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_unset_alias called with an incorrect port %ld", myport);
        return -1;
    } else if (name == NULL) {
        jack_error("jack_port_unset_alias called with a NULL port name");
        return -1;
    } else {
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->GetPort(myport)->UnsetAlias(name) : -1);
    }
}

EXPORT int jack_port_get_aliases(const jack_port_t* port, char* const aliases[2])
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
#if defined(__x86_64__) || defined(__ppc64__)
    uint64_t port_aux = (uint64_t)port;
#else
    uint32_t port_aux = (uint32_t)port;
#endif
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_get_aliases called with an incorrect port %ld", myport);
        return -1;
    } else {
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->GetPort(myport)->GetAliases(aliases) : -1);
    }
}

EXPORT int jack_port_request_monitor(jack_port_t* port, int onoff)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
#if defined(__x86_64__) || defined(__ppc64__)
    uint64_t port_aux = (uint64_t)port;
#else
    uint32_t port_aux = (uint32_t)port;
#endif
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_request_monitor called with an incorrect port %ld", myport);
        return -1;
    } else {
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->RequestMonitor(myport, onoff) : -1);
    }
}

EXPORT int jack_port_request_monitor_by_name(jack_client_t* ext_client, const char* port_name, int onoff)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_port_request_monitor_by_name called with a NULL client");
        return -1;
    } else {
        JackGraphManager* manager = GetGraphManager();
        if (!manager)
            return -1;
        jack_port_id_t myport = manager->GetPort(port_name);
        if (!CheckPort(myport)) {
            jack_error("jack_port_request_monitor_by_name called with an incorrect port %s", port_name);
            return -1;
        } else {
            return manager->RequestMonitor(myport, onoff);
        }
    }
}

EXPORT int jack_port_ensure_monitor(jack_port_t* port, int onoff)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
 #if defined(__x86_64__) || defined(__ppc64__)
    uint64_t port_aux = (uint64_t)port;
#else
    uint32_t port_aux = (uint32_t)port;
#endif
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_ensure_monitor called with an incorrect port %ld", myport);
        return -1;
    } else {
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->GetPort(myport)->EnsureMonitor(onoff) : -1);
    }
}

EXPORT int jack_port_monitoring_input(jack_port_t* port)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
#if defined(__x86_64__) || defined(__ppc64__)
    uint64_t port_aux = (uint64_t)port;
#else
    uint32_t port_aux = (uint32_t)port;
#endif
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_monitoring_input called with an incorrect port %ld", myport);
        return -1;
    } else {
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->GetPort(myport)->MonitoringInput() : -1);
    }
}

EXPORT int jack_is_realtime(jack_client_t* ext_client)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_is_realtime called with a NULL client");
        return -1;
    } else {
        JackEngineControl* control = GetEngineControl();
        return (control ? control->fRealTime : -1);
    }
}

EXPORT void jack_on_shutdown(jack_client_t* ext_client, JackShutdownCallback callback, void* arg)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_on_shutdown called with a NULL client");
    } else {
        client->OnShutdown(callback, arg);
    }
}

EXPORT int jack_set_process_callback(jack_client_t* ext_client, JackProcessCallback callback, void* arg)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_process_callback called with a NULL client");
        return -1;
    } else {
        return client->SetProcessCallback(callback, arg);
    }
}

EXPORT jack_nframes_t jack_thread_wait(jack_client_t* ext_client, int status)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_thread_wait called with a NULL client");
        return 0;
    } else {
        jack_error("jack_thread_wait: deprecated, use jack_cycle_wait/jack_cycle_signal");
        return -1;
    }
}

EXPORT jack_nframes_t jack_cycle_wait(jack_client_t* ext_client)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_cycle_wait called with a NULL client");
        return 0;
    } else {
        return client->CycleWait();
    }
}

EXPORT void jack_cycle_signal(jack_client_t* ext_client, int status)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_cycle_signal called with a NULL client");
    } else {
        client->CycleSignal(status);
    }
}

EXPORT int jack_set_process_thread(jack_client_t* ext_client, JackThreadCallback fun, void *arg)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_process_thread called with a NULL client");
        return -1;
    } else {
        return client->SetProcessThread(fun, arg);
    }
}

EXPORT int jack_set_freewheel_callback(jack_client_t* ext_client, JackFreewheelCallback freewheel_callback, void* arg)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_freewheel_callback called with a NULL client");
        return -1;
    } else {
        return client->SetFreewheelCallback(freewheel_callback, arg);
    }
}

EXPORT int jack_set_freewheel(jack_client_t* ext_client, int onoff)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_freewheel called with a NULL client");
        return -1;
    } else {
        return client->SetFreeWheel(onoff);
    }
}

EXPORT int jack_set_buffer_size(jack_client_t* ext_client, jack_nframes_t buffer_size)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_buffer_size called with a NULL client");
        return -1;
    } else if (!CheckBufferSize(buffer_size)) {
        return -1;
    } else {
        return client->SetBufferSize(buffer_size);
    }
}

EXPORT int jack_set_buffer_size_callback(jack_client_t* ext_client, JackBufferSizeCallback bufsize_callback, void* arg)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_buffer_size_callback called with a NULL client");
        return -1;
    } else {
        return client->SetBufferSizeCallback(bufsize_callback, arg);
    }
}

EXPORT int jack_set_sample_rate_callback(jack_client_t* ext_client, JackSampleRateCallback srate_callback, void* arg)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_sample_rate_callback called with a NULL client");
        return -1;
    } else {
        return client->SetSampleRateCallback(srate_callback, arg);
    }
}

EXPORT int jack_set_client_registration_callback(jack_client_t* ext_client, JackClientRegistrationCallback registration_callback, void* arg)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_client_registration_callback called with a NULL client");
        return -1;
    } else {
        return client->SetClientRegistrationCallback(registration_callback, arg);
    }
}

EXPORT int jack_set_port_registration_callback(jack_client_t* ext_client, JackPortRegistrationCallback registration_callback, void* arg)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_port_registration_callback called with a NULL client");
        return -1;
    } else {
        return client->SetPortRegistrationCallback(registration_callback, arg);
    }
}

EXPORT int jack_set_port_connect_callback(jack_client_t* ext_client, JackPortConnectCallback portconnect_callback, void* arg)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_port_connect_callback called with a NULL client");
        return -1;
    } else {
        return client->SetPortConnectCallback(portconnect_callback, arg);
    }
}

EXPORT int jack_set_port_rename_callback(jack_client_t* ext_client, JackPortRenameCallback rename_callback, void* arg)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_port_rename_callback called with a NULL client");
        return -1;
    } else {
        return client->SetPortRenameCallback(rename_callback, arg);
    }
}

EXPORT int jack_set_graph_order_callback(jack_client_t* ext_client, JackGraphOrderCallback graph_callback, void* arg)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    jack_log("jack_set_graph_order_callback ext_client %x client %x ", ext_client, client);
    if (client == NULL) {
        jack_error("jack_set_graph_order_callback called with a NULL client");
        return -1;
    } else {
        return client->SetGraphOrderCallback(graph_callback, arg);
    }
}

EXPORT int jack_set_xrun_callback(jack_client_t* ext_client, JackXRunCallback xrun_callback, void* arg)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_xrun_callback called with a NULL client");
        return -1;
    } else {
        return client->SetXRunCallback(xrun_callback, arg);
    }
}

EXPORT int jack_set_thread_init_callback(jack_client_t* ext_client, JackThreadInitCallback init_callback, void *arg)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    jack_log("jack_set_thread_init_callback ext_client %x client %x ", ext_client, client);
    if (client == NULL) {
        jack_error("jack_set_thread_init_callback called with a NULL client");
        return -1;
    } else {
        return client->SetInitCallback(init_callback, arg);
    }
}

EXPORT int jack_activate(jack_client_t* ext_client)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_activate called with a NULL client");
        return -1;
    } else {
        return client->Activate();
    }
}

EXPORT int jack_deactivate(jack_client_t* ext_client)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_deactivate called with a NULL client");
        return -1;
    } else {
        return client->Deactivate();
    }
}

EXPORT jack_port_t* jack_port_register(jack_client_t* ext_client, const char* port_name, const char* port_type, unsigned long flags, unsigned long buffer_size)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_port_register called with a NULL client");
        return NULL;
    } else if ((port_name == NULL) || (port_type == NULL)) {
        jack_error("jack_port_register called with a NULL port name or a NULL port_type");
        return NULL;
    } else {
#if defined(__x86_64__) || defined(__ppc64__)
        return (jack_port_t *)((uint64_t)client->PortRegister(port_name, port_type, flags, buffer_size));
#else
        return (jack_port_t *)client->PortRegister(port_name, port_type, flags, buffer_size);
#endif
    }
}

EXPORT int jack_port_unregister(jack_client_t* ext_client, jack_port_t* port)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_port_unregister called with a NULL client");
        return -1;
    }
#if defined(__x86_64__) || defined(__ppc64__)
    uint64_t port_aux = (uint64_t)port;
#else
    uint32_t port_aux = (uint32_t)port;
#endif
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_unregister called with an incorrect port %ld", myport);
        return -1;
    }
    return client->PortUnRegister(myport);
}

EXPORT int jack_port_is_mine(const jack_client_t* ext_client, const jack_port_t* port)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_port_is_mine called with a NULL client");
        return -1;
    }
#if defined(__x86_64__) || defined(__ppc64__)
    uint64_t port_aux = (uint64_t)port;
#else
    uint32_t port_aux = (uint32_t)port;
#endif
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_is_mine called with an incorrect port %ld", myport);
        return -1;
    }
    return client->PortIsMine(myport);
}

EXPORT const char** jack_port_get_connections(const jack_port_t* port)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
#if defined(__x86_64__) || defined(__ppc64__)
    uint64_t port_aux = (uint64_t)port;
#else
    uint32_t port_aux = (uint32_t)port;
#endif
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_get_connections called with an incorrect port %ld", myport);
        return NULL;
    } else {
        WaitGraphChange();
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->GetConnections(myport) : NULL);
    }
}

// Calling client does not need to "own" the port
EXPORT const char** jack_port_get_all_connections(const jack_client_t* ext_client, const jack_port_t* port)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_port_get_all_connections called with a NULL client");
        return NULL;
    }

#if defined(__x86_64__) || defined(__ppc64__)
    uint64_t port_aux = (uint64_t)port;
#else
    uint32_t port_aux = (uint32_t)port;
#endif
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_get_all_connections called with an incorrect port %ld", myport);
        return NULL;
    } else {
        WaitGraphChange();
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->GetConnections(myport) : NULL);
    }
}

EXPORT jack_nframes_t jack_port_get_total_latency(jack_client_t* ext_client, jack_port_t* port)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_port_get_total_latency called with a NULL client");
        return 0;
    }

#if defined(__x86_64__) || defined(__ppc64__)
    uint64_t port_aux = (uint64_t)port;
#else
    uint32_t port_aux = (uint32_t)port;
#endif
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_get_total_latency called with an incorrect port %ld", myport);
        return 0;
    } else {
        WaitGraphChange();
        JackGraphManager* manager = GetGraphManager();
        if (manager) {
            manager->ComputeTotalLatency(myport);
            return manager->GetPort(myport)->GetTotalLatency();
        } else {
            return 0;
        }
    }
}

EXPORT int jack_connect(jack_client_t* ext_client, const char* src, const char* dst)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_connect called with a NULL client");
        return -1;
    } else if ((src == NULL) || (dst == NULL)) {
        jack_error("jack_connect called with a NULL port name");
        return -1;
    } else {
        return client->PortConnect(src, dst);
    }
}

EXPORT int jack_disconnect(jack_client_t* ext_client, const char* src, const char* dst)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_disconnect called with a NULL client");
        return -1;
    } else if ((src == NULL) || (dst == NULL)) {
        jack_error("jack_connect called with a NULL port name");
        return -1;
    } else {
        return client->PortDisconnect(src, dst);
    }
}

EXPORT int jack_port_disconnect(jack_client_t* ext_client, jack_port_t* src)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_port_disconnect called with a NULL client");
        return -1;
    }
#if defined(__x86_64__) || defined(__ppc64__)
    uint64_t port_aux = (uint64_t)src;
#else
    uint32_t port_aux = (uint32_t)src;
#endif
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_disconnect called with an incorrect port %ld", myport);
        return -1;
    }
    return client->PortDisconnect(myport);
}

EXPORT jack_nframes_t jack_get_sample_rate(jack_client_t* ext_client)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_get_sample_rate called with a NULL client");
        return 0;
    } else {
        JackEngineControl* control = GetEngineControl();
        return (control ? control->fSampleRate : 0);
    }
}

EXPORT jack_nframes_t jack_get_buffer_size(jack_client_t* ext_client)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_get_buffer_size called with a NULL client");
        return 0;
    } else {
        JackEngineControl* control = GetEngineControl();
        return (control ? control->fBufferSize : 0);
    }
}

EXPORT const char** jack_get_ports(jack_client_t* ext_client, const char* port_name_pattern, const char* type_name_pattern, unsigned long flags)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_get_ports called with a NULL client");
        return NULL;
    }
    JackGraphManager* manager = GetGraphManager();
    return (manager ? manager->GetPorts(port_name_pattern, type_name_pattern, flags) : NULL);
}

EXPORT jack_port_t* jack_port_by_name(jack_client_t* ext_client, const char* portname)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_get_ports called with a NULL client");
        return 0;
    }

    if (portname == NULL) {
        jack_error("jack_port_by_name called with a NULL port name");
        return NULL;
    } else {
        JackGraphManager* manager = GetGraphManager();
        if (!manager)
            return NULL;
        int res = manager->GetPort(portname); // returns a port index at least > 1
#if defined(__x86_64__) || defined(__ppc64__)
        return (res == NO_PORT) ? NULL : (jack_port_t*)((uint64_t)res);
#else
        return (res == NO_PORT) ? NULL : (jack_port_t*)res;
#endif
    }
}

EXPORT jack_port_t* jack_port_by_id(jack_client_t* ext_client, jack_port_id_t id)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    /* jack_port_t* type is actually the port index */
#if defined(__x86_64__) || defined(__ppc64__)
    return (jack_port_t*)((uint64_t)id);
#else
    return (jack_port_t*)id;
#endif
}

EXPORT int jack_engine_takeover_timebase(jack_client_t* ext_client)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_engine_takeover_timebase called with a NULL client");
        return -1;
    } else {
        jack_error("jack_engine_takeover_timebase: deprecated\n");
        return 0;
    }
}

EXPORT jack_nframes_t jack_frames_since_cycle_start(const jack_client_t* ext_client)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackTimer timer;
    JackEngineControl* control = GetEngineControl();
    if (control) {
        control->ReadFrameTime(&timer);
        return timer.FramesSinceCycleStart(GetMicroSeconds(), control->fSampleRate);
    } else {
        return 0;
    }
}

EXPORT jack_time_t jack_get_time()
{
    return GetMicroSeconds();
}

EXPORT jack_time_t jack_frames_to_time(const jack_client_t* ext_client, jack_nframes_t frames)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_frames_to_time called with a NULL client");
        return 0;
    } else {
        JackTimer timer;
        JackEngineControl* control = GetEngineControl();
        if (control) {
            control->ReadFrameTime(&timer);
            return timer.Frames2Time(frames, control->fBufferSize);
        } else {
            return 0;
        }
    }
}

EXPORT jack_nframes_t jack_time_to_frames(const jack_client_t* ext_client, jack_time_t time)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_time_to_frames called with a NULL client");
        return 0;
    } else {
        JackTimer timer;
        JackEngineControl* control = GetEngineControl();
        if (control) {
            control->ReadFrameTime(&timer);
            return timer.Time2Frames(time, control->fBufferSize);
        } else {
            return 0;
        }
    }
}

EXPORT jack_nframes_t jack_frame_time(const jack_client_t* ext_client)
{
    return jack_time_to_frames(ext_client, GetMicroSeconds());
}

EXPORT jack_nframes_t jack_last_frame_time(const jack_client_t* ext_client)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackTimer timer;
    JackEngineControl* control = GetEngineControl();
    if (control) {
        control->ReadFrameTime(&timer);
        return timer.CurFrame();
    } else {
        return 0;
    }
}

EXPORT float jack_cpu_load(jack_client_t* ext_client)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_cpu_load called with a NULL client");
        return 0.0f;
    } else {
        JackEngineControl* control = GetEngineControl();
        return (control ? control->fCPULoad :  0.0f);
    }
}

EXPORT pthread_t jack_client_thread_id(jack_client_t* ext_client)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_client_thread_id called with a NULL client");
        return (pthread_t)NULL;
    } else {
        return client->GetThreadID();
    }
}

EXPORT char* jack_get_client_name(jack_client_t* ext_client)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_get_client_name called with a NULL client");
        return NULL;
    } else {
        return client->GetClientControl()->fName;
    }
}

EXPORT int jack_client_name_size(void)
{
    return JACK_CLIENT_NAME_SIZE;
}

EXPORT int jack_port_name_size(void)
{
    return JACK_PORT_NAME_SIZE;
}

EXPORT int jack_port_type_size(void)
{
    return JACK_PORT_TYPE_SIZE;
}

// transport.h
EXPORT int jack_release_timebase(jack_client_t* ext_client)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_release_timebase called with a NULL client");
        return -1;
    } else {
        return client->ReleaseTimebase();
    }
}

EXPORT int jack_set_sync_callback(jack_client_t* ext_client, JackSyncCallback sync_callback, void *arg)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_sync_callback called with a NULL client");
        return -1;
    } else {
        return client->SetSyncCallback(sync_callback, arg);
    }
}

EXPORT int jack_set_sync_timeout(jack_client_t* ext_client, jack_time_t timeout)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_sync_timeout called with a NULL client");
        return -1;
    } else {
        return client->SetSyncTimeout(timeout);
    }
}

EXPORT int jack_set_timebase_callback(jack_client_t* ext_client, int conditional, JackTimebaseCallback timebase_callback, void* arg)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_timebase_callback called with a NULL client");
        return -1;
    } else {
        return client->SetTimebaseCallback(conditional, timebase_callback, arg);
    }
}

EXPORT int jack_transport_locate(jack_client_t* ext_client, jack_nframes_t frame)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_transport_locate called with a NULL client");
        return -1;
    } else {
        client->TransportLocate(frame);
        return 0;
    }
}

EXPORT jack_transport_state_t jack_transport_query(const jack_client_t* ext_client, jack_position_t* pos)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_transport_query called with a NULL client");
        return JackTransportStopped;
    } else {
        return client->TransportQuery(pos);
    }
}

EXPORT jack_nframes_t jack_get_current_transport_frame(const jack_client_t* ext_client)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_get_current_transport_frame called with a NULL client");
        return 0;
    } else {
        return client->GetCurrentTransportFrame();
    }
}

EXPORT int jack_transport_reposition(jack_client_t* ext_client, jack_position_t* pos)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_transport_reposition called with a NULL client");
        return -1;
    } else {
        client->TransportReposition(pos);
        return 0;
    }
}

EXPORT void jack_transport_start(jack_client_t* ext_client)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_transport_start called with a NULL client");
    } else {
        client->TransportStart();
    }
}

EXPORT void jack_transport_stop(jack_client_t* ext_client)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_transport_stop called with a NULL client");
    } else {
        client->TransportStop();
    }
}

// deprecated
EXPORT void jack_get_transport_info(jack_client_t* ext_client, jack_transport_info_t* tinfo)
{
    jack_error("jack_get_transport_info: deprecated");
    if (tinfo)
        memset(tinfo, 0, sizeof(jack_transport_info_t));
}

EXPORT void jack_set_transport_info(jack_client_t* ext_client, jack_transport_info_t* tinfo)
{
    jack_error("jack_set_transport_info: deprecated");
    if (tinfo)
        memset(tinfo, 0, sizeof(jack_transport_info_t));
}

// statistics.h
EXPORT float jack_get_max_delayed_usecs(jack_client_t* ext_client)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_get_max_delayed_usecs called with a NULL client");
        return 0.f;
    } else {
        JackEngineControl* control = GetEngineControl();
        return (control ? control->fMaxDelayedUsecs : 0.f);
    }
 }

EXPORT float jack_get_xrun_delayed_usecs(jack_client_t* ext_client)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_get_xrun_delayed_usecs called with a NULL client");
        return 0.f;
    } else {
        JackEngineControl* control = GetEngineControl();
        return (control ? control->fXrunDelayedUsecs : 0.f);
    }
}

EXPORT void jack_reset_max_delayed_usecs(jack_client_t* ext_client)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_reset_max_delayed_usecs called with a NULL client");
    } else {
        JackEngineControl* control = GetEngineControl();
        control->ResetXRun();
    }
}

// thread.h
EXPORT int jack_client_real_time_priority(jack_client_t* ext_client)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_client_real_time_priority called with a NULL client");
        return -1;
    } else {
        JackEngineControl* control = GetEngineControl();
        return (control->fRealTime) ? control->fClientPriority : -1;
    }
}

EXPORT int jack_client_max_real_time_priority(jack_client_t* ext_client)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_client_max_real_time_priority called with a NULL client");
        return -1;
    } else {
        JackEngineControl* control = GetEngineControl();
       return (control->fRealTime) ? control->fMaxClientPriority : -1;
    }
}

EXPORT int jack_acquire_real_time_scheduling(pthread_t thread, int priority)
{
    JackEngineControl* control = GetEngineControl();
    return (control ? JackThread::AcquireRealTimeImp(thread, priority, GetEngineControl()->fPeriod, GetEngineControl()->fComputation, GetEngineControl()->fConstraint) : -1);
}

EXPORT int jack_client_create_thread(jack_client_t* client,
                                     pthread_t *thread,
                                     int priority,
                                     int realtime,  	/* boolean */
                                     thread_routine routine,
                                     void *arg)
{
    return JackThread::StartImp(thread, priority, realtime, routine, arg);
}

EXPORT int jack_drop_real_time_scheduling(pthread_t thread)
{
    return JackThread::DropRealTimeImp(thread);
}

EXPORT int jack_client_stop_thread(jack_client_t* client, pthread_t thread)
{
    return JackThread::StopImp(thread);
}

EXPORT int jack_client_kill_thread(jack_client_t* client, pthread_t thread)
{
    return JackThread::KillImp(thread);
}

#ifndef WIN32
EXPORT void jack_set_thread_creator (jack_thread_creator_t jtc)
{
    JackGlobals::fJackThreadCreator = jtc;
}
#endif

// intclient.h
EXPORT int jack_internal_client_new (const char *client_name,
                                     const char *load_name,
                                     const char *load_init)
{
    jack_error("jack_internal_client_new: deprecated");
    return -1;
}

EXPORT void jack_internal_client_close (const char *client_name)
{
    jack_error("jack_internal_client_close: deprecated");
}

EXPORT char* jack_get_internal_client_name(jack_client_t* ext_client, jack_intclient_t intclient)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_get_internal_client_name called with a NULL client");
        return NULL;
    } else if (intclient >= CLIENT_NUM) {
        jack_error("jack_get_internal_client_name: incorrect client");
        return NULL;
    } else {
        return client->GetInternalClientName(intclient);
    }
}

EXPORT jack_intclient_t jack_internal_client_handle(jack_client_t* ext_client, const char* client_name, jack_status_t* status)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_internal_client_handle called with a NULL client");
        return 0;
    } else {
        jack_status_t my_status;
        if (status == NULL)		/* no status from caller? */
            status = &my_status;	/* use local status word */
        *status = (jack_status_t)0;
        return client->InternalClientHandle(client_name, status);
    }
}

EXPORT jack_intclient_t jack_internal_client_load_aux(jack_client_t* ext_client, const char* client_name, jack_options_t options, jack_status_t* status, va_list ap)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_internal_client_load called with a NULL client");
        return 0;
    } else {
        jack_varargs_t va;
        jack_status_t my_status;

        if (status == NULL)			/* no status from caller? */
            status = &my_status;	/* use local status word */
        *status = (jack_status_t)0;

        /* validate parameters */
        if ((options & ~JackLoadOptions)) {
            int my_status1 = *status | (JackFailure | JackInvalidOption);
            *status = (jack_status_t)my_status1;
            return 0;
        }

        /* parse variable arguments */
        jack_varargs_parse(options, ap, &va);
        return client->InternalClientLoad(client_name, options, status, &va);
    }
}

EXPORT jack_intclient_t jack_internal_client_load(jack_client_t *client, const char *client_name, jack_options_t options, jack_status_t *status, ...)
{
    va_list ap;
    va_start(ap, status);
    jack_intclient_t res = jack_internal_client_load_aux(client, client_name, options, status, ap);
    va_end(ap);
    return res;
}

EXPORT jack_status_t jack_internal_client_unload(jack_client_t* ext_client, jack_intclient_t intclient)
{
#ifdef __CLIENTDEBUG__
    JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_internal_client_unload called with a NULL client");
        return (jack_status_t)(JackNoSuchClient | JackFailure);
    } else if (intclient >= CLIENT_NUM) {
        jack_error("jack_internal_client_unload: incorrect client");
        return (jack_status_t)(JackNoSuchClient | JackFailure);
    } else {
        jack_status_t my_status;
        client->InternalClientUnload(intclient, &my_status);
        return my_status;
    }
}

EXPORT
void
jack_get_version(
    int *major_ptr,
    int *minor_ptr,
    int *micro_ptr,
    int *proto_ptr)
{
    // FIXME: We need these comming from build system
    *major_ptr = 0;
    *minor_ptr = 0;
    *micro_ptr = 0;
    *proto_ptr = 0;
}

EXPORT
const char *
jack_get_version_string()
{
    return VERSION;
}
