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
#include "JackPortType.h"
#include <math.h>

using namespace Jack;

#ifdef __cplusplus
extern "C"
{
#endif

    typedef void (*print_function)(const char*);
    typedef void *(*thread_routine)(void*);

    LIB_EXPORT
    void
    jack_get_version(
        int *major_ptr,
        int *minor_ptr,
        int *micro_ptr,
        int *proto_ptr);

    LIB_EXPORT
    const char*
    jack_get_version_string();

    jack_client_t * jack_client_new_aux(const char* client_name,
            jack_options_t options,
            jack_status_t *status);

    LIB_EXPORT jack_client_t * jack_client_open(const char* client_name,
            jack_options_t options,
            jack_status_t *status, ...);
    LIB_EXPORT jack_client_t * jack_client_new(const char* client_name);
    LIB_EXPORT int jack_client_name_size(void);
    LIB_EXPORT char* jack_get_client_name(jack_client_t *client);
    LIB_EXPORT int jack_internal_client_new(const char* client_name,
                                         const char* load_name,
                                         const char* load_init);
    LIB_EXPORT void jack_internal_client_close(const char* client_name);
    LIB_EXPORT int jack_is_realtime(jack_client_t *client);
    LIB_EXPORT void jack_on_shutdown(jack_client_t *client,
                                  JackShutdownCallback shutdown_callback, void *arg);
    LIB_EXPORT void jack_on_info_shutdown(jack_client_t *client,
                                  JackInfoShutdownCallback shutdown_callback, void *arg);
    LIB_EXPORT int jack_set_process_callback(jack_client_t *client,
                                          JackProcessCallback process_callback,
                                          void *arg);
    LIB_EXPORT jack_nframes_t jack_thread_wait(jack_client_t *client, int status);

    // new
    LIB_EXPORT jack_nframes_t jack_cycle_wait(jack_client_t*);
    LIB_EXPORT void jack_cycle_signal(jack_client_t*, int status);
    LIB_EXPORT int jack_set_process_thread(jack_client_t* client, JackThreadCallback fun, void *arg);

    LIB_EXPORT int jack_set_thread_init_callback(jack_client_t *client,
            JackThreadInitCallback thread_init_callback,
            void *arg);
    LIB_EXPORT int jack_set_freewheel_callback(jack_client_t *client,
                                            JackFreewheelCallback freewheel_callback,
                                            void *arg);
    LIB_EXPORT int jack_set_freewheel(jack_client_t* client, int onoff);
    LIB_EXPORT int jack_set_buffer_size(jack_client_t *client, jack_nframes_t nframes);
    LIB_EXPORT int jack_set_buffer_size_callback(jack_client_t *client,
            JackBufferSizeCallback bufsize_callback,
            void *arg);
    LIB_EXPORT int jack_set_sample_rate_callback(jack_client_t *client,
            JackSampleRateCallback srate_callback,
            void *arg);
    LIB_EXPORT int jack_set_client_registration_callback(jack_client_t *,
            JackClientRegistrationCallback
            registration_callback, void *arg);
    LIB_EXPORT int jack_set_port_registration_callback(jack_client_t *,
            JackPortRegistrationCallback
            registration_callback, void *arg);
    LIB_EXPORT int jack_set_port_connect_callback(jack_client_t *,
            JackPortConnectCallback
            connect_callback, void *arg);
    LIB_EXPORT int jack_set_port_rename_callback(jack_client_t *,
                                    JackPortRenameCallback
                                    rename_callback, void *arg);
    LIB_EXPORT int jack_set_graph_order_callback(jack_client_t *,
            JackGraphOrderCallback graph_callback,
            void *);
    LIB_EXPORT int jack_set_xrun_callback(jack_client_t *,
                                       JackXRunCallback xrun_callback, void *arg);
    LIB_EXPORT int jack_set_latency_callback(jack_client_t *client,
			       JackLatencyCallback latency_callback, void *arg);

    LIB_EXPORT int jack_activate(jack_client_t *client);
    LIB_EXPORT int jack_deactivate(jack_client_t *client);
    LIB_EXPORT jack_port_t * jack_port_register(jack_client_t *client,
            const char* port_name,
            const char* port_type,
            unsigned long flags,
            unsigned long buffer_size);
    LIB_EXPORT int jack_port_unregister(jack_client_t *, jack_port_t *);
    LIB_EXPORT void * jack_port_get_buffer(jack_port_t *, jack_nframes_t);
    LIB_EXPORT const char*  jack_port_name(const jack_port_t *port);
    LIB_EXPORT const char*  jack_port_short_name(const jack_port_t *port);
    LIB_EXPORT int jack_port_flags(const jack_port_t *port);
    LIB_EXPORT const char*  jack_port_type(const jack_port_t *port);
    LIB_EXPORT jack_port_type_id_t jack_port_type_id(const jack_port_t *port);
    LIB_EXPORT int jack_port_is_mine(const jack_client_t *, const jack_port_t *port);
    LIB_EXPORT int jack_port_connected(const jack_port_t *port);
    LIB_EXPORT int jack_port_connected_to(const jack_port_t *port,
                                       const char* port_name);
    LIB_EXPORT const char* * jack_port_get_connections(const jack_port_t *port);
    LIB_EXPORT const char* * jack_port_get_all_connections(const jack_client_t *client,
            const jack_port_t *port);
    LIB_EXPORT int jack_port_tie(jack_port_t *src, jack_port_t *dst);
    LIB_EXPORT int jack_port_untie(jack_port_t *port);

    // Old latency API
    LIB_EXPORT jack_nframes_t jack_port_get_latency(jack_port_t *port);
    LIB_EXPORT jack_nframes_t jack_port_get_total_latency(jack_client_t *,
            jack_port_t *port);
    LIB_EXPORT void jack_port_set_latency(jack_port_t *, jack_nframes_t);
    LIB_EXPORT int jack_recompute_total_latency(jack_client_t*, jack_port_t* port);

    // New latency API
    LIB_EXPORT void jack_port_get_latency_range(jack_port_t *port, jack_latency_callback_mode_t mode, jack_latency_range_t *range);
    LIB_EXPORT void jack_port_set_latency_range(jack_port_t *port, jack_latency_callback_mode_t mode, jack_latency_range_t *range);
    LIB_EXPORT int jack_recompute_total_latencies(jack_client_t*);

    LIB_EXPORT int jack_port_set_name(jack_port_t *port, const char* port_name);
    LIB_EXPORT int jack_port_set_alias(jack_port_t *port, const char* alias);
    LIB_EXPORT int jack_port_unset_alias(jack_port_t *port, const char* alias);
    LIB_EXPORT int jack_port_get_aliases(const jack_port_t *port, char* const aliases[2]);
    LIB_EXPORT int jack_port_request_monitor(jack_port_t *port, int onoff);
    LIB_EXPORT int jack_port_request_monitor_by_name(jack_client_t *client,
            const char* port_name, int onoff);
    LIB_EXPORT int jack_port_ensure_monitor(jack_port_t *port, int onoff);
    LIB_EXPORT int jack_port_monitoring_input(jack_port_t *port);
    LIB_EXPORT int jack_connect(jack_client_t *,
                             const char* source_port,
                             const char* destination_port);
    LIB_EXPORT int jack_disconnect(jack_client_t *,
                                const char* source_port,
                                const char* destination_port);
    LIB_EXPORT int jack_port_disconnect(jack_client_t *, jack_port_t *);
    LIB_EXPORT int jack_port_name_size(void);
    LIB_EXPORT int jack_port_type_size(void);
    LIB_EXPORT size_t jack_port_type_get_buffer_size(jack_client_t *client, const char* port_type);
    LIB_EXPORT jack_nframes_t jack_get_sample_rate(jack_client_t *);
    LIB_EXPORT jack_nframes_t jack_get_buffer_size(jack_client_t *);
    LIB_EXPORT const char* * jack_get_ports(jack_client_t *,
                                         const char* port_name_pattern,
                                         const char* type_name_pattern,
                                         unsigned long flags);
    LIB_EXPORT jack_port_t * jack_port_by_name(jack_client_t *, const char* port_name);
    LIB_EXPORT jack_port_t * jack_port_by_id(jack_client_t *client,
                                          jack_port_id_t port_id);
    LIB_EXPORT int jack_engine_takeover_timebase(jack_client_t *);
    LIB_EXPORT jack_nframes_t jack_frames_since_cycle_start(const jack_client_t *);
    LIB_EXPORT jack_time_t jack_get_time();
    LIB_EXPORT jack_nframes_t jack_time_to_frames(const jack_client_t *client, jack_time_t usecs);
    LIB_EXPORT jack_time_t jack_frames_to_time(const jack_client_t *client, jack_nframes_t frames);
    LIB_EXPORT jack_nframes_t jack_frame_time(const jack_client_t *);
    LIB_EXPORT jack_nframes_t jack_last_frame_time(const jack_client_t *client);
    LIB_EXPORT int jack_get_cycle_times(const jack_client_t *client,
                                        jack_nframes_t *current_frames,
                                        jack_time_t    *current_usecs,
                                        jack_time_t    *next_usecs,
                                        float          *period_usecs);
    LIB_EXPORT float jack_cpu_load(jack_client_t *client);
    LIB_EXPORT jack_native_thread_t jack_client_thread_id(jack_client_t *);
    LIB_EXPORT void jack_set_error_function(print_function);
    LIB_EXPORT void jack_set_info_function(print_function);

    LIB_EXPORT float jack_get_max_delayed_usecs(jack_client_t *client);
    LIB_EXPORT float jack_get_xrun_delayed_usecs(jack_client_t *client);
    LIB_EXPORT void jack_reset_max_delayed_usecs(jack_client_t *client);

    LIB_EXPORT int jack_release_timebase(jack_client_t *client);
    LIB_EXPORT int jack_set_sync_callback(jack_client_t *client,
                                       JackSyncCallback sync_callback,
                                       void *arg);
    LIB_EXPORT int jack_set_sync_timeout(jack_client_t *client,
                                      jack_time_t timeout);
    LIB_EXPORT int jack_set_timebase_callback(jack_client_t *client,
                                           int conditional,
                                           JackTimebaseCallback timebase_callback,
                                           void *arg);
    LIB_EXPORT int jack_transport_locate(jack_client_t *client,
                                      jack_nframes_t frame);
    LIB_EXPORT jack_transport_state_t jack_transport_query(const jack_client_t *client,
            jack_position_t *pos);
    LIB_EXPORT jack_nframes_t jack_get_current_transport_frame(const jack_client_t *client);
    LIB_EXPORT int jack_transport_reposition(jack_client_t *client,
                                          const jack_position_t *pos);
    LIB_EXPORT void jack_transport_start(jack_client_t *client);
    LIB_EXPORT void jack_transport_stop(jack_client_t *client);
    LIB_EXPORT void jack_get_transport_info(jack_client_t *client,
                                         jack_transport_info_t *tinfo);
    LIB_EXPORT void jack_set_transport_info(jack_client_t *client,
                                         jack_transport_info_t *tinfo);

    LIB_EXPORT int jack_client_real_time_priority(jack_client_t*);
    LIB_EXPORT int jack_client_max_real_time_priority(jack_client_t*);
    LIB_EXPORT int jack_acquire_real_time_scheduling(jack_native_thread_t thread, int priority);
    LIB_EXPORT int jack_client_create_thread(jack_client_t* client,
                                          jack_native_thread_t *thread,
                                          int priority,
                                          int realtime,         // boolean
                                          thread_routine routine,
                                          void *arg);
    LIB_EXPORT int jack_drop_real_time_scheduling(jack_native_thread_t thread);

    LIB_EXPORT int jack_client_stop_thread(jack_client_t* client, jack_native_thread_t thread);
    LIB_EXPORT int jack_client_kill_thread(jack_client_t* client, jack_native_thread_t thread);
#ifndef WIN32
    LIB_EXPORT void jack_set_thread_creator(jack_thread_creator_t jtc);
#endif
    LIB_EXPORT char * jack_get_internal_client_name(jack_client_t *client,
            jack_intclient_t intclient);
    LIB_EXPORT jack_intclient_t jack_internal_client_handle(jack_client_t *client,
            const char* client_name,
            jack_status_t *status);
    LIB_EXPORT jack_intclient_t jack_internal_client_load(jack_client_t *client,
            const char* client_name,
            jack_options_t options,
            jack_status_t *status, ...);

    LIB_EXPORT jack_status_t jack_internal_client_unload(jack_client_t *client,
            jack_intclient_t intclient);
    LIB_EXPORT void jack_free(void* ptr);

    LIB_EXPORT int jack_set_session_callback(jack_client_t* ext_client, JackSessionCallback session_callback, void* arg);
    LIB_EXPORT jack_session_command_t *jack_session_notify(jack_client_t* ext_client, const char* target, jack_session_event_type_t ev_type, const char* path);
    LIB_EXPORT int jack_session_reply(jack_client_t* ext_client, jack_session_event_t *event);
    LIB_EXPORT void jack_session_event_free(jack_session_event_t* ev);
    LIB_EXPORT char* jack_client_get_uuid (jack_client_t *client);
    LIB_EXPORT char* jack_get_uuid_for_client_name(jack_client_t* ext_client, const char* client_name);
    LIB_EXPORT char* jack_get_client_name_by_uuid(jack_client_t* ext_client, const char* client_uuid);
    LIB_EXPORT int jack_reserve_client_name(jack_client_t* ext_client, const char* name, const char* uuid);
    LIB_EXPORT void jack_session_commands_free(jack_session_command_t *cmds);
    LIB_EXPORT int jack_client_has_session_callback(jack_client_t *client, const char* client_name);

#ifdef __cplusplus
}
#endif

static inline bool CheckPort(jack_port_id_t port_index)
{
    return (port_index > 0 && port_index < PORT_NUM_MAX);
}

static inline bool CheckBufferSize(jack_nframes_t buffer_size)
{
    return (buffer_size >= 1 && buffer_size <= BUFFER_SIZE_MAX);
}

static inline void WaitGraphChange()
{
    /*
    TLS key that is set only in RT thread, so never waits for pending
    graph change in RT context (just read the current graph state).
    */

    if (jack_tls_get(JackGlobals::fRealTimeThread) == NULL) {
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

LIB_EXPORT void jack_set_error_function(print_function func)
{
    jack_error_callback = (func == NULL) ? &default_jack_error_callback : func;
}

LIB_EXPORT void jack_set_info_function(print_function func)
{
    jack_info_callback = (func == NULL) ? &default_jack_info_callback : func;
}

LIB_EXPORT jack_client_t* jack_client_new(const char* client_name)
{
    JackGlobals::CheckContext("jack_client_new");

    try {
        assert(JackGlobals::fOpenMutex);
        JackGlobals::fOpenMutex->Lock();
        jack_error("jack_client_new: deprecated");
        int options = JackUseExactName;
        if (getenv("JACK_START_SERVER") == NULL) {
            options |= JackNoStartServer;
        }
        jack_client_t* res = jack_client_new_aux(client_name, (jack_options_t)options, NULL);
        JackGlobals::fOpenMutex->Unlock();
        return res;
    } catch (std::bad_alloc& e) {
        jack_error("Memory allocation error...");
        return NULL;
    } catch (...) {
        jack_error("Unknown error...");
        return NULL;
    }
}

LIB_EXPORT void* jack_port_get_buffer(jack_port_t* port, jack_nframes_t frames)
{
    JackGlobals::CheckContext("jack_port_get_buffer");

    uintptr_t port_aux = (uintptr_t)port;
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_get_buffer called with an incorrect port %ld", myport);
        return NULL;
    } else {
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->GetBuffer(myport, frames) : NULL);
    }
}

LIB_EXPORT const char* jack_port_name(const jack_port_t* port)
{
    JackGlobals::CheckContext("jack_port_name");

    uintptr_t port_aux = (uintptr_t)port;
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_name called with an incorrect port %ld", myport);
        return NULL;
    } else {
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->GetPort(myport)->GetName() : NULL);
    }
}

LIB_EXPORT const char* jack_port_short_name(const jack_port_t* port)
{
    JackGlobals::CheckContext("jack_port_short_name");

    uintptr_t port_aux = (uintptr_t)port;
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_short_name called with an incorrect port %ld", myport);
        return NULL;
    } else {
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->GetPort(myport)->GetShortName() : NULL);
    }
}

LIB_EXPORT int jack_port_flags(const jack_port_t* port)
{
    JackGlobals::CheckContext("jack_port_flags");

    uintptr_t port_aux = (uintptr_t)port;
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_flags called with an incorrect port %ld", myport);
        return -1;
    } else {
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->GetPort(myport)->GetFlags() : -1);
    }
}

LIB_EXPORT const char* jack_port_type(const jack_port_t* port)
{
    JackGlobals::CheckContext("jack_port_type");

    uintptr_t port_aux = (uintptr_t)port;
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_flags called an incorrect port %ld", myport);
        return NULL;
    } else {
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->GetPort(myport)->GetType() : NULL);
    }
}

LIB_EXPORT jack_port_type_id_t jack_port_type_id(const jack_port_t *port)
{
    JackGlobals::CheckContext("jack_port_type_id");

    uintptr_t port_aux = (uintptr_t)port;
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_type_id called an incorrect port %ld", myport);
        return 0;
    } else {
        JackGraphManager* manager = GetGraphManager();
        return (manager ? GetPortTypeId(manager->GetPort(myport)->GetType()) : 0);
    }
}

LIB_EXPORT int jack_port_connected(const jack_port_t* port)
{
    JackGlobals::CheckContext("jack_port_connected");

    uintptr_t port_aux = (uintptr_t)port;
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

LIB_EXPORT int jack_port_connected_to(const jack_port_t* port, const char* port_name)
{
    JackGlobals::CheckContext("jack_port_connected_to");

    uintptr_t port_aux = (uintptr_t)port;
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

LIB_EXPORT int jack_port_tie(jack_port_t* src, jack_port_t* dst)
{
    JackGlobals::CheckContext("jack_port_tie");

    uintptr_t src_aux = (uintptr_t)src;
    jack_port_id_t mysrc = (jack_port_id_t)src_aux;
    if (!CheckPort(mysrc)) {
        jack_error("jack_port_tie called with a NULL src port");
        return -1;
    }
    uintptr_t dst_aux = (uintptr_t)dst;
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

LIB_EXPORT int jack_port_untie(jack_port_t* port)
{
    JackGlobals::CheckContext("jack_port_untie");

    uintptr_t port_aux = (uintptr_t)port;
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_untie called with an incorrect port %ld", myport);
        return -1;
    } else {
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->GetPort(myport)->UnTie() : -1);
    }
}

LIB_EXPORT jack_nframes_t jack_port_get_latency(jack_port_t* port)
{
    JackGlobals::CheckContext("jack_port_get_latency");

    uintptr_t port_aux = (uintptr_t)port;
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

LIB_EXPORT void jack_port_set_latency(jack_port_t* port, jack_nframes_t frames)
{
    JackGlobals::CheckContext("jack_port_set_latency");

    uintptr_t port_aux = (uintptr_t)port;
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_set_latency called with an incorrect port %ld", myport);
    } else {
        JackGraphManager* manager = GetGraphManager();
        if (manager)
            manager->GetPort(myport)->SetLatency(frames);
    }
}

LIB_EXPORT void jack_port_get_latency_range(jack_port_t *port, jack_latency_callback_mode_t mode, jack_latency_range_t *range)
{
    JackGlobals::CheckContext("jack_port_get_latency_range");

    uintptr_t port_aux = (uintptr_t)port;
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_get_latency_range called with an incorrect port %ld", myport);
    } else {
        WaitGraphChange();
        JackGraphManager* manager = GetGraphManager();
        if (manager)
            manager->GetPort(myport)->GetLatencyRange(mode, range);
    }
}

LIB_EXPORT void jack_port_set_latency_range(jack_port_t *port, jack_latency_callback_mode_t mode, jack_latency_range_t *range)
{
    JackGlobals::CheckContext("jack_port_set_latency_range");

    uintptr_t port_aux = (uintptr_t)port;
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_set_latency_range called with an incorrect port %ld", myport);
    } else {
        WaitGraphChange();
        JackGraphManager* manager = GetGraphManager();
        if (manager)
            manager->GetPort(myport)->SetLatencyRange(mode, range);
    }
}

LIB_EXPORT int jack_recompute_total_latency(jack_client_t* ext_client, jack_port_t* port)
{
    JackGlobals::CheckContext("jack_recompute_total_latency");


    JackClient* client = (JackClient*)ext_client;
    uintptr_t port_aux = (uintptr_t)port;
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (client == NULL) {
        jack_error("jack_recompute_total_latency called with a NULL client");
        return -1;
    } else if (!CheckPort(myport)) {
        jack_error("jack_recompute_total_latency called with a NULL port");
        return -1;
    } else {
        WaitGraphChange();
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->ComputeTotalLatency(myport) : -1);
    }
}

LIB_EXPORT int jack_recompute_total_latencies(jack_client_t* ext_client)
{
    JackGlobals::CheckContext("jack_recompute_total_latencies");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_recompute_total_latencies called with a NULL client");
        return -1;
    } else {
        return client->ComputeTotalLatencies();
    }
}

LIB_EXPORT int jack_port_set_name(jack_port_t* port, const char* name)
{
    JackGlobals::CheckContext("jack_port_set_name");

    uintptr_t port_aux = (uintptr_t)port;
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_set_name called with an incorrect port %ld", myport);
        return -1;
    } else if (name == NULL) {
        jack_error("jack_port_set_name called with a NULL port name");
        return -1;
    } else {
        JackClient* client = NULL;
        for (int i = 0; i < CLIENT_NUM; i++) {
            // Find a valid client
            if ((client = JackGlobals::fClientTable[i])) {
                break;
            }
        }
        return (client) ? client->PortRename(myport, name) : -1;
    }
}

LIB_EXPORT int jack_port_set_alias(jack_port_t* port, const char* name)
{
    JackGlobals::CheckContext("jack_port_set_alias");

    uintptr_t port_aux = (uintptr_t)port;
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

LIB_EXPORT int jack_port_unset_alias(jack_port_t* port, const char* name)
{
    JackGlobals::CheckContext("jack_port_unset_alias");

    uintptr_t port_aux = (uintptr_t)port;
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

LIB_EXPORT int jack_port_get_aliases(const jack_port_t* port, char* const aliases[2])
{
    JackGlobals::CheckContext("jack_port_get_aliases");

    uintptr_t port_aux = (uintptr_t)port;
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_get_aliases called with an incorrect port %ld", myport);
        return -1;
    } else {
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->GetPort(myport)->GetAliases(aliases) : -1);
    }
}

LIB_EXPORT int jack_port_request_monitor(jack_port_t* port, int onoff)
{
    JackGlobals::CheckContext("jack_port_request_monitor");

    uintptr_t port_aux = (uintptr_t)port;
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_request_monitor called with an incorrect port %ld", myport);
        return -1;
    } else {
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->RequestMonitor(myport, onoff) : -1);
    }
}

LIB_EXPORT int jack_port_request_monitor_by_name(jack_client_t* ext_client, const char* port_name, int onoff)
{
    JackGlobals::CheckContext("jack_port_request_monitor_by_name");

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

LIB_EXPORT int jack_port_ensure_monitor(jack_port_t* port, int onoff)
{
    JackGlobals::CheckContext("jack_port_ensure_monitor");

    uintptr_t port_aux = (uintptr_t)port;
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_ensure_monitor called with an incorrect port %ld", myport);
        return -1;
    } else {
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->GetPort(myport)->EnsureMonitor(onoff) : -1);
    }
}

LIB_EXPORT int jack_port_monitoring_input(jack_port_t* port)
{
    JackGlobals::CheckContext("jack_port_monitoring_input");

    uintptr_t port_aux = (uintptr_t)port;
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_monitoring_input called with an incorrect port %ld", myport);
        return -1;
    } else {
        JackGraphManager* manager = GetGraphManager();
        return (manager ? manager->GetPort(myport)->MonitoringInput() : -1);
    }
}

LIB_EXPORT int jack_is_realtime(jack_client_t* ext_client)
{
    JackGlobals::CheckContext("jack_is_realtime");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_is_realtime called with a NULL client");
        return -1;
    } else {
        JackEngineControl* control = GetEngineControl();
        return (control ? control->fRealTime : -1);
    }
}

LIB_EXPORT void jack_on_shutdown(jack_client_t* ext_client, JackShutdownCallback callback, void* arg)
{
    JackGlobals::CheckContext("jack_on_shutdown");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_on_shutdown called with a NULL client");
    } else {
        client->OnShutdown(callback, arg);
    }
}

LIB_EXPORT void jack_on_info_shutdown(jack_client_t* ext_client, JackInfoShutdownCallback callback, void* arg)
{
    JackGlobals::CheckContext("jack_on_info_shutdown");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_on_info_shutdown called with a NULL client");
    } else {
        client->OnInfoShutdown(callback, arg);
    }
}

LIB_EXPORT int jack_set_process_callback(jack_client_t* ext_client, JackProcessCallback callback, void* arg)
{
    JackGlobals::CheckContext("jack_set_process_callback");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_process_callback called with a NULL client");
        return -1;
    } else {
        return client->SetProcessCallback(callback, arg);
    }
}

LIB_EXPORT jack_nframes_t jack_thread_wait(jack_client_t* ext_client, int status)
{
    JackGlobals::CheckContext("jack_thread_wait");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_thread_wait called with a NULL client");
        return 0;
    } else {
        jack_error("jack_thread_wait: deprecated, use jack_cycle_wait/jack_cycle_signal");
        return 0;
    }
}

LIB_EXPORT jack_nframes_t jack_cycle_wait(jack_client_t* ext_client)
{
    JackGlobals::CheckContext("jack_cycle_wait");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_cycle_wait called with a NULL client");
        return 0;
    } else {
        return client->CycleWait();
    }
}

LIB_EXPORT void jack_cycle_signal(jack_client_t* ext_client, int status)
{
    JackGlobals::CheckContext("jack_cycle_signal");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_cycle_signal called with a NULL client");
    } else {
        client->CycleSignal(status);
    }
}

LIB_EXPORT int jack_set_process_thread(jack_client_t* ext_client, JackThreadCallback fun, void *arg)
{
    JackGlobals::CheckContext("jack_set_process_thread");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_process_thread called with a NULL client");
        return -1;
    } else {
        return client->SetProcessThread(fun, arg);
    }
}

LIB_EXPORT int jack_set_freewheel_callback(jack_client_t* ext_client, JackFreewheelCallback freewheel_callback, void* arg)
{
    JackGlobals::CheckContext("jack_set_freewheel_callback");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_freewheel_callback called with a NULL client");
        return -1;
    } else {
        return client->SetFreewheelCallback(freewheel_callback, arg);
    }
}

LIB_EXPORT int jack_set_freewheel(jack_client_t* ext_client, int onoff)
{
    JackGlobals::CheckContext("jack_set_freewheel");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_freewheel called with a NULL client");
        return -1;
    } else {
        return client->SetFreeWheel(onoff);
    }
}

LIB_EXPORT int jack_set_buffer_size(jack_client_t* ext_client, jack_nframes_t buffer_size)
{
    JackGlobals::CheckContext("jack_set_buffer_size");

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

LIB_EXPORT int jack_set_buffer_size_callback(jack_client_t* ext_client, JackBufferSizeCallback bufsize_callback, void* arg)
{
    JackGlobals::CheckContext("jack_set_buffer_size_callback");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_buffer_size_callback called with a NULL client");
        return -1;
    } else {
        return client->SetBufferSizeCallback(bufsize_callback, arg);
    }
}

LIB_EXPORT int jack_set_sample_rate_callback(jack_client_t* ext_client, JackSampleRateCallback srate_callback, void* arg)
{
    JackGlobals::CheckContext("jack_set_sample_rate_callback");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_sample_rate_callback called with a NULL client");
        return -1;
    } else {
        return client->SetSampleRateCallback(srate_callback, arg);
    }
}

LIB_EXPORT int jack_set_client_registration_callback(jack_client_t* ext_client, JackClientRegistrationCallback registration_callback, void* arg)
{
    JackGlobals::CheckContext("jack_set_client_registration_callback");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_client_registration_callback called with a NULL client");
        return -1;
    } else {
        return client->SetClientRegistrationCallback(registration_callback, arg);
    }
}

LIB_EXPORT int jack_set_port_registration_callback(jack_client_t* ext_client, JackPortRegistrationCallback registration_callback, void* arg)
{
    JackGlobals::CheckContext("jack_set_port_registration_callback");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_port_registration_callback called with a NULL client");
        return -1;
    } else {
        return client->SetPortRegistrationCallback(registration_callback, arg);
    }
}

LIB_EXPORT int jack_set_port_connect_callback(jack_client_t* ext_client, JackPortConnectCallback portconnect_callback, void* arg)
{
    JackGlobals::CheckContext("jack_set_port_connect_callback");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_port_connect_callback called with a NULL client");
        return -1;
    } else {
        return client->SetPortConnectCallback(portconnect_callback, arg);
    }
}

LIB_EXPORT int jack_set_port_rename_callback(jack_client_t* ext_client, JackPortRenameCallback rename_callback, void* arg)
{
    JackGlobals::CheckContext("jack_set_port_rename_callback");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_port_rename_callback called with a NULL client");
        return -1;
    } else {
        return client->SetPortRenameCallback(rename_callback, arg);
    }
}

LIB_EXPORT int jack_set_graph_order_callback(jack_client_t* ext_client, JackGraphOrderCallback graph_callback, void* arg)
{
    JackGlobals::CheckContext("jack_set_graph_order_callback");

    JackClient* client = (JackClient*)ext_client;
    jack_log("jack_set_graph_order_callback ext_client %x client %x ", ext_client, client);
    if (client == NULL) {
        jack_error("jack_set_graph_order_callback called with a NULL client");
        return -1;
    } else {
        return client->SetGraphOrderCallback(graph_callback, arg);
    }
}

LIB_EXPORT int jack_set_xrun_callback(jack_client_t* ext_client, JackXRunCallback xrun_callback, void* arg)
{
    JackGlobals::CheckContext("jack_set_xrun_callback");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_xrun_callback called with a NULL client");
        return -1;
    } else {
        return client->SetXRunCallback(xrun_callback, arg);
    }
}

LIB_EXPORT int jack_set_latency_callback(jack_client_t* ext_client, JackLatencyCallback latency_callback, void *arg)
{
    JackGlobals::CheckContext("jack_set_latency_callback");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_latency_callback called with a NULL client");
        return -1;
    } else {
        return client->SetLatencyCallback(latency_callback, arg);
    }
}

LIB_EXPORT int jack_set_thread_init_callback(jack_client_t* ext_client, JackThreadInitCallback init_callback, void *arg)
{
    JackGlobals::CheckContext("jack_set_thread_init_callback");

    JackClient* client = (JackClient*)ext_client;
    jack_log("jack_set_thread_init_callback ext_client %x client %x ", ext_client, client);
    if (client == NULL) {
        jack_error("jack_set_thread_init_callback called with a NULL client");
        return -1;
    } else {
        return client->SetInitCallback(init_callback, arg);
    }
}

LIB_EXPORT int jack_activate(jack_client_t* ext_client)
{
    JackGlobals::CheckContext("jack_activate");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_activate called with a NULL client");
        return -1;
    } else {
        return client->Activate();
    }
}

LIB_EXPORT int jack_deactivate(jack_client_t* ext_client)
{
    JackGlobals::CheckContext("jack_deactivate");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_deactivate called with a NULL client");
        return -1;
    } else {
        return client->Deactivate();
    }
}

LIB_EXPORT jack_port_t* jack_port_register(jack_client_t* ext_client, const char* port_name, const char* port_type, unsigned long flags, unsigned long buffer_size)
{
    JackGlobals::CheckContext("jack_port_register");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_port_register called with a NULL client");
        return NULL;
    } else if ((port_name == NULL) || (port_type == NULL)) {
        jack_error("jack_port_register called with a NULL port name or a NULL port_type");
        return NULL;
    } else {
        return (jack_port_t *)((uintptr_t)client->PortRegister(port_name, port_type, flags, buffer_size));
    }
}

LIB_EXPORT int jack_port_unregister(jack_client_t* ext_client, jack_port_t* port)
{
    JackGlobals::CheckContext("jack_port_unregister");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_port_unregister called with a NULL client");
        return -1;
    }
    uintptr_t port_aux = (uintptr_t)port;
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_unregister called with an incorrect port %ld", myport);
        return -1;
    }
    return client->PortUnRegister(myport);
}

LIB_EXPORT int jack_port_is_mine(const jack_client_t* ext_client, const jack_port_t* port)
{
    JackGlobals::CheckContext("jack_port_is_mine");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_port_is_mine called with a NULL client");
        return -1;
    }
    uintptr_t port_aux = (uintptr_t)port;
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_is_mine called with an incorrect port %ld", myport);
        return -1;
    }
    return client->PortIsMine(myport);
}

LIB_EXPORT const char** jack_port_get_connections(const jack_port_t* port)
{
    JackGlobals::CheckContext("jack_port_get_connections");

    uintptr_t port_aux = (uintptr_t)port;
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
LIB_EXPORT const char** jack_port_get_all_connections(const jack_client_t* ext_client, const jack_port_t* port)
{
    JackGlobals::CheckContext("jack_port_get_all_connections");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_port_get_all_connections called with a NULL client");
        return NULL;
    }

    uintptr_t port_aux = (uintptr_t)port;
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

LIB_EXPORT jack_nframes_t jack_port_get_total_latency(jack_client_t* ext_client, jack_port_t* port)
{
    JackGlobals::CheckContext("jack_port_get_total_latency");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_port_get_total_latency called with a NULL client");
        return 0;
    }

    uintptr_t port_aux = (uintptr_t)port;
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

LIB_EXPORT int jack_connect(jack_client_t* ext_client, const char* src, const char* dst)
{
    JackGlobals::CheckContext("jack_connect");

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

LIB_EXPORT int jack_disconnect(jack_client_t* ext_client, const char* src, const char* dst)
{
    JackGlobals::CheckContext("jack_disconnect");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_disconnect called with a NULL client");
        return -1;
    } else if ((src == NULL) || (dst == NULL)) {
        jack_error("jack_disconnect called with a NULL port name");
        return -1;
    } else {
        return client->PortDisconnect(src, dst);
    }
}

LIB_EXPORT int jack_port_disconnect(jack_client_t* ext_client, jack_port_t* src)
{
    JackGlobals::CheckContext("jack_port_disconnect");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_port_disconnect called with a NULL client");
        return -1;
    }
    uintptr_t port_aux = (uintptr_t)src;
    jack_port_id_t myport = (jack_port_id_t)port_aux;
    if (!CheckPort(myport)) {
        jack_error("jack_port_disconnect called with an incorrect port %ld", myport);
        return -1;
    }
    return client->PortDisconnect(myport);
}

LIB_EXPORT jack_nframes_t jack_get_sample_rate(jack_client_t* ext_client)
{
    JackGlobals::CheckContext("jack_get_sample_rate");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_get_sample_rate called with a NULL client");
        return 0;
    } else {
        JackEngineControl* control = GetEngineControl();
        return (control ? control->fSampleRate : 0);
    }
}

LIB_EXPORT jack_nframes_t jack_get_buffer_size(jack_client_t* ext_client)
{
    JackGlobals::CheckContext("jack_get_buffer_size");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_get_buffer_size called with a NULL client");
        return 0;
    } else {
        JackEngineControl* control = GetEngineControl();
        return (control ? control->fBufferSize : 0);
    }
}

LIB_EXPORT const char** jack_get_ports(jack_client_t* ext_client, const char* port_name_pattern, const char* type_name_pattern, unsigned long flags)
{
    JackGlobals::CheckContext("jack_get_ports");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_get_ports called with a NULL client");
        return NULL;
    }
    JackGraphManager* manager = GetGraphManager();
    return (manager ? manager->GetPorts(port_name_pattern, type_name_pattern, flags) : NULL);
}

LIB_EXPORT jack_port_t* jack_port_by_name(jack_client_t* ext_client, const char* portname)
{
    JackGlobals::CheckContext("jack_port_by_name");

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
        return (res == NO_PORT) ? NULL : (jack_port_t*)((uintptr_t)res);
    }
}

LIB_EXPORT jack_port_t* jack_port_by_id(jack_client_t* ext_client, jack_port_id_t id)
{
    JackGlobals::CheckContext("jack_port_by_id");

    /* jack_port_t* type is actually the port index */
    return (jack_port_t*)((uintptr_t)id);
}

LIB_EXPORT int jack_engine_takeover_timebase(jack_client_t* ext_client)
{
    JackGlobals::CheckContext("jack_engine_takeover_timebase");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_engine_takeover_timebase called with a NULL client");
        return -1;
    } else {
        jack_error("jack_engine_takeover_timebase: deprecated\n");
        return 0;
    }
}

LIB_EXPORT jack_nframes_t jack_frames_since_cycle_start(const jack_client_t* ext_client)
{
    JackGlobals::CheckContext("jack_frames_since_cycle_start");

    JackTimer timer;
    JackEngineControl* control = GetEngineControl();
    if (control) {
        control->ReadFrameTime(&timer);
        return timer.FramesSinceCycleStart(GetMicroSeconds(), control->fSampleRate);
    } else {
        return 0;
    }
}

LIB_EXPORT jack_time_t jack_get_time()
{
    JackGlobals::CheckContext("jack_get_time");

    return GetMicroSeconds();
}

LIB_EXPORT jack_time_t jack_frames_to_time(const jack_client_t* ext_client, jack_nframes_t frames)
{
    JackGlobals::CheckContext("jack_frames_to_time");

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

LIB_EXPORT jack_nframes_t jack_time_to_frames(const jack_client_t* ext_client, jack_time_t usecs)
{
    JackGlobals::CheckContext("jack_time_to_frames");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_time_to_frames called with a NULL client");
        return 0;
    } else {
        JackTimer timer;
        JackEngineControl* control = GetEngineControl();
        if (control) {
            control->ReadFrameTime(&timer);
            return timer.Time2Frames(usecs, control->fBufferSize);
        } else {
            return 0;
        }
    }
}

LIB_EXPORT jack_nframes_t jack_frame_time(const jack_client_t* ext_client)
{
    JackGlobals::CheckContext("jack_frame_time");

    return jack_time_to_frames(ext_client, GetMicroSeconds());
}

LIB_EXPORT jack_nframes_t jack_last_frame_time(const jack_client_t* ext_client)
{
    JackGlobals::CheckContext("jack_last_frame_time");

    JackEngineControl* control = GetEngineControl();
    return (control) ? control->fFrameTimer.ReadCurrentState()->CurFrame() : 0;
}

LIB_EXPORT int jack_get_cycle_times(const jack_client_t *client,
                                    jack_nframes_t *current_frames,
                                    jack_time_t    *current_usecs,
                                    jack_time_t    *next_usecs,
                                    float          *period_usecs)
{
    JackGlobals::CheckContext("jack_get_cycle_times");

    JackEngineControl* control = GetEngineControl();
    if (control) {
        JackTimer timer;
        control->ReadFrameTime(&timer);
        return timer.GetCycleTimes(current_frames, current_usecs, next_usecs, period_usecs);
    } else {
        return -1;
    }
}

LIB_EXPORT float jack_cpu_load(jack_client_t* ext_client)
{
    JackGlobals::CheckContext("jack_cpu_load");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_cpu_load called with a NULL client");
        return 0.0f;
    } else {
        JackEngineControl* control = GetEngineControl();
        return (control ? control->fCPULoad :  0.0f);
    }
}

LIB_EXPORT jack_native_thread_t jack_client_thread_id(jack_client_t* ext_client)
{
    JackGlobals::CheckContext("jack_client_thread_id");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_client_thread_id called with a NULL client");
        return (jack_native_thread_t)NULL;
    } else {
        return client->GetThreadID();
    }
}

LIB_EXPORT char* jack_get_client_name(jack_client_t* ext_client)
{
    JackGlobals::CheckContext("jack_get_client_name");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_get_client_name called with a NULL client");
        return NULL;
    } else {
        return client->GetClientControl()->fName;
    }
}

LIB_EXPORT int jack_client_name_size(void)
{
    return JACK_CLIENT_NAME_SIZE;
}

LIB_EXPORT int jack_port_name_size(void)
{
    return REAL_JACK_PORT_NAME_SIZE;
}

LIB_EXPORT int jack_port_type_size(void)
{
    return JACK_PORT_TYPE_SIZE;
}

LIB_EXPORT size_t jack_port_type_get_buffer_size(jack_client_t* ext_client, const char* port_type)
{
    JackGlobals::CheckContext("jack_port_type_get_buffer_size");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_port_type_get_buffer_size called with a NULL client");
        return 0;
    } else {
        jack_port_type_id_t port_id = GetPortTypeId(port_type);
        if (port_id == PORT_TYPES_MAX) {
            jack_error("jack_port_type_get_buffer_size called with an unknown port type = %s", port_type);
            return 0;
        } else {
            return GetPortType(port_id)->size();
        }
    }
}

// transport.h
LIB_EXPORT int jack_release_timebase(jack_client_t* ext_client)
{
    JackGlobals::CheckContext("jack_release_timebase");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_release_timebase called with a NULL client");
        return -1;
    } else {
        return client->ReleaseTimebase();
    }
}

LIB_EXPORT int jack_set_sync_callback(jack_client_t* ext_client, JackSyncCallback sync_callback, void *arg)
{
    JackGlobals::CheckContext("jack_set_sync_callback");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_sync_callback called with a NULL client");
        return -1;
    } else {
        return client->SetSyncCallback(sync_callback, arg);
    }
}

LIB_EXPORT int jack_set_sync_timeout(jack_client_t* ext_client, jack_time_t timeout)
{
    JackGlobals::CheckContext("jack_set_sync_timeout");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_sync_timeout called with a NULL client");
        return -1;
    } else {
        return client->SetSyncTimeout(timeout);
    }
}

LIB_EXPORT int jack_set_timebase_callback(jack_client_t* ext_client, int conditional, JackTimebaseCallback timebase_callback, void* arg)
{
    JackGlobals::CheckContext("jack_set_timebase_callback");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_set_timebase_callback called with a NULL client");
        return -1;
    } else {
        return client->SetTimebaseCallback(conditional, timebase_callback, arg);
    }
}

LIB_EXPORT int jack_transport_locate(jack_client_t* ext_client, jack_nframes_t frame)
{
    JackGlobals::CheckContext("jack_transport_locate");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_transport_locate called with a NULL client");
        return -1;
    } else {
        client->TransportLocate(frame);
        return 0;
    }
}

LIB_EXPORT jack_transport_state_t jack_transport_query(const jack_client_t* ext_client, jack_position_t* pos)
{
    JackGlobals::CheckContext("jack_transport_query");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_transport_query called with a NULL client");
        return JackTransportStopped;
    } else {
        return client->TransportQuery(pos);
    }
}

LIB_EXPORT jack_nframes_t jack_get_current_transport_frame(const jack_client_t* ext_client)
{
    JackGlobals::CheckContext("jack_get_current_transport_frame");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_get_current_transport_frame called with a NULL client");
        return 0;
    } else {
        return client->GetCurrentTransportFrame();
    }
}

LIB_EXPORT int jack_transport_reposition(jack_client_t* ext_client, const jack_position_t* pos)
{
    JackGlobals::CheckContext("jack_transport_reposition");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_transport_reposition called with a NULL client");
        return -1;
    } else {
        client->TransportReposition(pos);
        return 0;
    }
}

LIB_EXPORT void jack_transport_start(jack_client_t* ext_client)
{
    JackGlobals::CheckContext("jack_transport_start");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_transport_start called with a NULL client");
    } else {
        client->TransportStart();
    }
}

LIB_EXPORT void jack_transport_stop(jack_client_t* ext_client)
{
    JackGlobals::CheckContext("jack_transport_stop");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_transport_stop called with a NULL client");
    } else {
        client->TransportStop();
    }
}

// deprecated
LIB_EXPORT void jack_get_transport_info(jack_client_t* ext_client, jack_transport_info_t* tinfo)
{
    JackGlobals::CheckContext("jack_get_transport_info");

    jack_error("jack_get_transport_info: deprecated");
    if (tinfo)
        memset(tinfo, 0, sizeof(jack_transport_info_t));
}

LIB_EXPORT void jack_set_transport_info(jack_client_t* ext_client, jack_transport_info_t* tinfo)
{
    JackGlobals::CheckContext("jack_set_transport_info");

    jack_error("jack_set_transport_info: deprecated");
    if (tinfo)
        memset(tinfo, 0, sizeof(jack_transport_info_t));
}

// statistics.h
LIB_EXPORT float jack_get_max_delayed_usecs(jack_client_t* ext_client)
{
    JackGlobals::CheckContext("jack_get_max_delayed_usecs");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_get_max_delayed_usecs called with a NULL client");
        return 0.f;
    } else {
        JackEngineControl* control = GetEngineControl();
        return (control ? control->fMaxDelayedUsecs : 0.f);
    }
 }

LIB_EXPORT float jack_get_xrun_delayed_usecs(jack_client_t* ext_client)
{
    JackGlobals::CheckContext("jack_get_xrun_delayed_usecs");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_get_xrun_delayed_usecs called with a NULL client");
        return 0.f;
    } else {
        JackEngineControl* control = GetEngineControl();
        return (control ? control->fXrunDelayedUsecs : 0.f);
    }
}

LIB_EXPORT void jack_reset_max_delayed_usecs(jack_client_t* ext_client)
{
    JackGlobals::CheckContext("jack_reset_max_delayed_usecs");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_reset_max_delayed_usecs called with a NULL client");
    } else {
        JackEngineControl* control = GetEngineControl();
        control->ResetXRun();
    }
}

// thread.h
LIB_EXPORT int jack_client_real_time_priority(jack_client_t* ext_client)
{
    JackGlobals::CheckContext("jack_client_real_time_priority");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_client_real_time_priority called with a NULL client");
        return -1;
    } else {
        JackEngineControl* control = GetEngineControl();
        return (control->fRealTime) ? control->fClientPriority : -1;
    }
}

LIB_EXPORT int jack_client_max_real_time_priority(jack_client_t* ext_client)
{
    JackGlobals::CheckContext("jack_client_max_real_time_priority");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_client_max_real_time_priority called with a NULL client");
        return -1;
    } else {
        JackEngineControl* control = GetEngineControl();
        return (control->fRealTime) ? control->fMaxClientPriority : -1;
    }
}

LIB_EXPORT int jack_acquire_real_time_scheduling(jack_native_thread_t thread, int priority)
{
    JackEngineControl* control = GetEngineControl();
    return (control
        ? JackThread::AcquireRealTimeImp(thread, priority, control->fPeriod, control->fComputation, control->fConstraint)
        : -1);
}

LIB_EXPORT int jack_client_create_thread(jack_client_t* client,
                                     jack_native_thread_t *thread,
                                     int priority,
                                     int realtime,      /* boolean */
                                     thread_routine routine,
                                     void *arg)
{
    JackGlobals::CheckContext("jack_client_create_thread");

    JackEngineControl* control = GetEngineControl();
    int res = JackThread::StartImp(thread, priority, realtime, routine, arg);
    return (res == 0)
        ? ((realtime ? JackThread::AcquireRealTimeImp(*thread, priority, control->fPeriod, control->fComputation, control->fConstraint) : res))
        : res;
}

LIB_EXPORT int jack_drop_real_time_scheduling(jack_native_thread_t thread)
{
    return JackThread::DropRealTimeImp(thread);
}

LIB_EXPORT int jack_client_stop_thread(jack_client_t* client, jack_native_thread_t thread)
{
    JackGlobals::CheckContext("jack_client_stop_thread");

    return JackThread::StopImp(thread);
}

LIB_EXPORT int jack_client_kill_thread(jack_client_t* client, jack_native_thread_t thread)
{
    JackGlobals::CheckContext("jack_client_kill_thread");

    return JackThread::KillImp(thread);
}

#ifndef WIN32
LIB_EXPORT void jack_set_thread_creator (jack_thread_creator_t jtc)
{
    if (jtc == NULL) {
        JackGlobals::fJackThreadCreator = pthread_create;
	} else {
        JackGlobals::fJackThreadCreator = jtc;
	}
}
#endif

// intclient.h
LIB_EXPORT int jack_internal_client_new (const char* client_name,
                                     const char* load_name,
                                     const char* load_init)
{
    JackGlobals::CheckContext("jack_internal_client_new");

    jack_error("jack_internal_client_new: deprecated");
    return -1;
}

LIB_EXPORT void jack_internal_client_close (const char* client_name)
{
    JackGlobals::CheckContext("jack_internal_client_close");

    jack_error("jack_internal_client_close: deprecated");
}

LIB_EXPORT char* jack_get_internal_client_name(jack_client_t* ext_client, jack_intclient_t intclient)
{
    JackGlobals::CheckContext("jack_get_internal_client_name");

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

LIB_EXPORT jack_intclient_t jack_internal_client_handle(jack_client_t* ext_client, const char* client_name, jack_status_t* status)
{
    JackGlobals::CheckContext("jack_internal_client_handle");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_internal_client_handle called with a NULL client");
        return 0;
    } else {
        jack_status_t my_status;
        if (status == NULL)             /* no status from caller? */
            status = &my_status;        /* use local status word */
        *status = (jack_status_t)0;
        return client->InternalClientHandle(client_name, status);
    }
}

static jack_intclient_t jack_internal_client_load_aux(jack_client_t* ext_client, const char* client_name, jack_options_t options, jack_status_t* status, va_list ap)
{
    JackGlobals::CheckContext("jack_internal_client_load_aux");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_internal_client_load called with a NULL client");
        return 0;
    } else {
        jack_varargs_t va;
        jack_status_t my_status;

        if (status == NULL)             /* no status from caller? */
            status = &my_status;        /* use local status word */
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

LIB_EXPORT jack_intclient_t jack_internal_client_load(jack_client_t *client, const char* client_name, jack_options_t options, jack_status_t *status, ...)
{
    JackGlobals::CheckContext("jack_internal_client_load");

    va_list ap;
    va_start(ap, status);
    jack_intclient_t res = jack_internal_client_load_aux(client, client_name, options, status, ap);
    va_end(ap);
    return res;
}

LIB_EXPORT jack_status_t jack_internal_client_unload(jack_client_t* ext_client, jack_intclient_t intclient)
{
    JackGlobals::CheckContext("jack_internal_client_load");

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

LIB_EXPORT void jack_get_version(int *major_ptr,
                            int *minor_ptr,
                            int *micro_ptr,
                            int *proto_ptr)
{
    JackGlobals::CheckContext("jack_get_version");

    // FIXME: We need these comming from build system
    *major_ptr = 0;
    *minor_ptr = 0;
    *micro_ptr = 0;
    *proto_ptr = 0;
}

LIB_EXPORT const char* jack_get_version_string()
{
    JackGlobals::CheckContext("jack_get_version_string");

    return VERSION;
}

LIB_EXPORT void jack_free(void* ptr)
{
    JackGlobals::CheckContext("jack_free");

    if (ptr) {
        free(ptr);
    }
}

// session.h
LIB_EXPORT int jack_set_session_callback(jack_client_t* ext_client, JackSessionCallback session_callback, void* arg)
{
    JackGlobals::CheckContext("jack_set_session_callback");

    JackClient* client = (JackClient*)ext_client;
    jack_log("jack_set_session_callback ext_client %x client %x ", ext_client, client);
    if (client == NULL) {
        jack_error("jack_set_session_callback called with a NULL client");
        return -1;
    } else {
        return client->SetSessionCallback(session_callback, arg);
    }
}

LIB_EXPORT jack_session_command_t* jack_session_notify(jack_client_t* ext_client, const char* target, jack_session_event_type_t ev_type, const char* path)
{
    JackGlobals::CheckContext("jack_session_notify");

    JackClient* client = (JackClient*)ext_client;
    jack_log("jack_session_notify ext_client %x client %x ", ext_client, client);
    if (client == NULL) {
        jack_error("jack_session_notify called with a NULL client");
        return NULL;
    } else {
        return client->SessionNotify(target, ev_type, path);
    }
}

LIB_EXPORT int jack_session_reply(jack_client_t* ext_client, jack_session_event_t *event)
{
    JackGlobals::CheckContext("jack_session_reply");

    JackClient* client = (JackClient*)ext_client;
    jack_log("jack_session_reply ext_client %x client %x ", ext_client, client);
    if (client == NULL) {
        jack_error("jack_session_reply called with a NULL client");
        return -1;
    } else {
        return client->SessionReply(event);
    }
}

LIB_EXPORT void jack_session_event_free(jack_session_event_t* ev)
{
    JackGlobals::CheckContext("jack_session_event_free");

    if (ev) {
        if (ev->session_dir)
            free((void *)ev->session_dir);
        if (ev->client_uuid)
            free((void *)ev->client_uuid);
        if (ev->command_line)
            free(ev->command_line);
        free(ev);
    }
}

LIB_EXPORT char *jack_client_get_uuid(jack_client_t* ext_client)
{
    JackGlobals::CheckContext("jack_client_get_uuid");

    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_client_get_uuid called with a NULL client");
        return NULL;
    } else {
        char retval[16];
        snprintf(retval, sizeof(retval), "%d", client->GetClientControl()->fSessionID);
        return strdup(retval);
    }
}

LIB_EXPORT char* jack_get_uuid_for_client_name(jack_client_t* ext_client, const char* client_name)
{
    JackGlobals::CheckContext("jack_get_uuid_for_client_name");

    JackClient* client = (JackClient*)ext_client;
    jack_log("jack_get_uuid_for_client_name ext_client %x client %x ", ext_client, client);
    if (client == NULL) {
        jack_error("jack_get_uuid_for_client_name called with a NULL client");
        return NULL;
    } else {
        return client->GetUUIDForClientName(client_name);
    }
}

LIB_EXPORT char* jack_get_client_name_by_uuid(jack_client_t* ext_client, const char* client_uuid)
{
    JackGlobals::CheckContext("jack_get_client_name_by_uuid");

    JackClient* client = (JackClient*)ext_client;
    jack_log("jack_get_uuid_for_client_name ext_client %x client %x ", ext_client, client);
    if (client == NULL) {
        jack_error("jack_get_client_name_by_uuid called with a NULL client");
        return NULL;
    } else {
        return client->GetClientNameByUUID(client_uuid);
    }
}

LIB_EXPORT int jack_reserve_client_name(jack_client_t* ext_client, const char* client_name, const char* uuid)
{
    JackGlobals::CheckContext("jack_reserve_client_name");

    JackClient* client = (JackClient*)ext_client;
    jack_log("jack_reserve_client_name ext_client %x client %x ", ext_client, client);
    if (client == NULL) {
        jack_error("jack_reserve_client_name called with a NULL client");
        return -1;
    } else {
        return client->ReserveClientName(client_name, uuid);
    }
}

LIB_EXPORT void jack_session_commands_free(jack_session_command_t *cmds)
{
    JackGlobals::CheckContext("jack_session_commands_free");


    if (!cmds) {
        return;
    }

    int i = 0;
    while (1) {
        if (cmds[i].client_name) {
            free ((char *)cmds[i].client_name);
        }
        if (cmds[i].command) {
            free ((char *)cmds[i].command);
        }
        if (cmds[i].uuid) {
            free ((char *)cmds[i].uuid);
        } else {
            break;
        }

        i += 1;
    }

    free(cmds);
}

LIB_EXPORT int jack_client_has_session_callback(jack_client_t* ext_client, const char* client_name)
{
    JackGlobals::CheckContext("jack_client_has_session_callback");

    JackClient* client = (JackClient*)ext_client;
    jack_log("jack_client_has_session_callback ext_client %x client %x ", ext_client, client);
    if (client == NULL) {
        jack_error("jack_client_has_session_callback called with a NULL client");
        return -1;
    } else {
        return client->ClientHasSessionCallback(client_name);
    }
}
