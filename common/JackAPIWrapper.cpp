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

#include "types.h"
#include "jack.h"
#include "JackExports.h"
#include <dlfcn.h>
#include <stdarg.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

    EXPORT int jack_client_name_size (void);
    EXPORT char* jack_get_client_name (jack_client_t *client);
    EXPORT int jack_internal_client_new (const char *client_name,		// deprecated
                                         const char *load_name,
                                         const char *load_init);
    EXPORT void jack_internal_client_close (const char *client_name);	// deprecated
    EXPORT int jack_is_realtime (jack_client_t *client);
    EXPORT void jack_on_shutdown (jack_client_t *client,
                                  void (*function)(void *arg), void *arg);
    EXPORT int jack_set_process_callback (jack_client_t *client,
                                          JackProcessCallback process_callback,
                                          void *arg);
    //
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
    EXPORT void jack_set_error_function (void (*func)(const char *));
	EXPORT void jack_set_info_function (void (*func)(const char *));
	
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

    EXPORT int jack_acquire_real_time_scheduling (pthread_t thread, int priority);
    EXPORT int jack_client_create_thread (jack_client_t* client,
                                          pthread_t *thread,
                                          int priority,
                                          int realtime, 	// boolean
                                          void *(*start_routine)(void*),
                                          void *arg);
    EXPORT int jack_drop_real_time_scheduling (pthread_t thread);

    EXPORT char * jack_get_internal_client_name (jack_client_t *client,
            jack_intclient_t intclient);
    EXPORT jack_intclient_t jack_internal_client_handle (jack_client_t *client,
            const char *client_name,
            jack_status_t *status);
    EXPORT jack_intclient_t jack_internal_client_load (jack_client_t *client,
            const char *client_name,
            jack_options_t options,
            jack_status_t *status, ...);
    EXPORT jack_status_t jack_internal_client_unload (jack_client_t *client,
            jack_intclient_t intclient);

    EXPORT jack_client_t * jack_client_open (const char *client_name,
            jack_options_t options,
            jack_status_t *status, ...);
    EXPORT jack_client_t * jack_client_new (const char *client_name);
    EXPORT int jack_client_close (jack_client_t *client);

#ifdef __cplusplus
}
#endif

#define JACK_LIB "libjack.so.0.0"
#define JACKMP_LIB "libjackmp.so"

// client
static bool gInitedLib = false;
static void* gLibrary = 0;
static bool init_library();
static bool open_library();
static void close_library();

static void (*error_fun)(const char *) = 0;
static void (*info_fun)(const char *) = 0;

static void jack_log(const char *fmt,...)
{
	/*
	va_list ap;
	va_start(ap, fmt);
	f//printf(stderr,"Jack: ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr,"\n");
	va_end(ap);
	*/
}

// Function definition

typedef void* (*jack_port_get_buffer_fun_def)(jack_port_t* port, jack_nframes_t frames);
static jack_port_get_buffer_fun_def jack_port_get_buffer_fun = 0;
EXPORT void* jack_port_get_buffer(jack_port_t* port, jack_nframes_t frames)
{
	jack_log("jack_port_get_buffer");
    return (*jack_port_get_buffer_fun)(port, frames);
}

typedef const char* (*jack_port_name_fun_def)(const jack_port_t* port);
static jack_port_name_fun_def jack_port_name_fun = 0;
EXPORT const char* jack_port_name(const jack_port_t* port)
{
	jack_log("jack_port_name");
    return (*jack_port_name_fun)(port);
}

typedef const char* (*jack_port_short_name_fun_def) (const jack_port_t* port);
static jack_port_short_name_fun_def jack_port_short_name_fun = 0;
EXPORT const char* jack_port_short_name(const jack_port_t* port)
{
	jack_log("jack_port_short_name");
    return (*jack_port_short_name_fun)(port);
}

typedef int (*jack_port_flags_fun_def)(const jack_port_t* port);
static jack_port_flags_fun_def jack_port_flags_fun = 0;
EXPORT int jack_port_flags(const jack_port_t* port)
{
	jack_log("jack_port_flags");
    return (*jack_port_flags_fun)(port);
}

typedef const char* (*jack_port_type_fun_def)(const jack_port_t* port);
static jack_port_type_fun_def jack_port_type_fun = 0;
EXPORT const char* jack_port_type(const jack_port_t* port)
{
	jack_log("jack_port_type");
    return (*jack_port_type_fun)(port);
}

typedef int (*jack_port_connected_fun_def)(const jack_port_t* port);
static jack_port_connected_fun_def jack_port_connected_fun = 0;
EXPORT int jack_port_connected(const jack_port_t* port)
{
	jack_log("jack_port_connected");
    return (*jack_port_connected_fun)(port);
}

typedef int (*jack_port_connected_to_fun_def)(const jack_port_t* port, const char* portname);
static jack_port_connected_to_fun_def jack_port_connected_to_fun = 0;
EXPORT int jack_port_connected_to(const jack_port_t* port, const char* portname)
{
	jack_log("jack_port_connected_to");
    return (*jack_port_connected_to_fun)(port, portname);
}

typedef int (*jack_port_tie_fun_def)(jack_port_t* src, jack_port_t* dst);
static jack_port_tie_fun_def jack_port_tie_fun = 0;
EXPORT int jack_port_tie(jack_port_t* src, jack_port_t* dst)
{
	jack_log("jack_port_tie");
    return (*jack_port_tie_fun)(src, dst);
}

typedef int (*jack_port_untie_fun_def)(jack_port_t* port);
static jack_port_untie_fun_def jack_port_untie_fun = 0;
EXPORT int jack_port_untie(jack_port_t* port)
{
	jack_log("jack_port_untie");
    return (*jack_port_untie_fun)(port);
}

typedef jack_nframes_t (*jack_port_get_latency_fun_def)(jack_port_t* port);
static jack_port_get_latency_fun_def jack_port_get_latency_fun = 0;
EXPORT jack_nframes_t jack_port_get_latency(jack_port_t* port)
{
	jack_log("jack_port_get_latency");
    return (*jack_port_get_latency)(port);
}

typedef void (*jack_port_set_latency_fun_def)(jack_port_t* port, jack_nframes_t frames);
static jack_port_set_latency_fun_def jack_port_set_latency_fun = 0;
EXPORT void jack_port_set_latency(jack_port_t* port, jack_nframes_t frames)
{
	jack_log("jack_port_set_latency");
    (*jack_port_set_latency_fun)(port, frames);
}

typedef int (*jack_recompute_total_latency_fun_def)(jack_client_t* ext_client, jack_port_t* port);
static jack_recompute_total_latency_fun_def jack_recompute_total_latency_fun = 0;
EXPORT int jack_recompute_total_latency(jack_client_t* ext_client, jack_port_t* port)
{
	jack_log("jack_recompute_total_latency");
    return (*jack_recompute_total_latency_fun)(ext_client, port);
}

typedef int (*jack_recompute_total_latencies_fun_def)(jack_client_t* ext_client);
static jack_recompute_total_latencies_fun_def jack_recompute_total_latencies_fun = 0;
EXPORT int jack_recompute_total_latencies(jack_client_t* ext_client)
{
	jack_log("jack_recompute_total_latencies");
    return (*jack_recompute_total_latencies_fun)(ext_client);
}

typedef int (*jack_port_set_name_fun_def)(jack_port_t* port, const char* name);
static jack_port_set_name_fun_def jack_port_set_name_fun = 0;
EXPORT int jack_port_set_name(jack_port_t* port, const char* name)
{
	jack_log("jack_port_set_name");
    return (*jack_port_set_name_fun)(port, name);
}

typedef int (*jack_port_set_alias_fun_def)(jack_port_t* port, const char* alias);
static jack_port_set_alias_fun_def jack_port_set_alias_fun = 0;
EXPORT int jack_port_set_alias(jack_port_t* port, const char* alias)
{
	jack_log("jack_port_set_alias");
    return (*jack_port_set_alias_fun)(port, alias);
}

typedef int (*jack_port_unset_alias_fun_def)(jack_port_t* port, const char* alias);
static jack_port_unset_alias_fun_def jack_port_unset_alias_fun = 0;
EXPORT int jack_port_unset_alias(jack_port_t* port, const char* alias)
{
	jack_log("jack_port_unset_alias");
    return (*jack_port_unset_alias_fun)(port, alias);
}

typedef int (*jack_port_get_aliases_fun_def)(jack_port_t* port, char* const aliases[2]);
static jack_port_get_aliases_fun_def jack_port_get_aliases_fun = 0;
EXPORT int jack_port_get_aliases(jack_port_t* port, char* const aliases[2])
{
	jack_log("jack_port_get_aliases");
    return (*jack_port_get_aliases_fun)(port, aliases);
}

typedef int (*jack_port_request_monitor_fun_def)(jack_port_t* port, int onoff);
static jack_port_request_monitor_fun_def jack_port_request_monitor_fun = 0;
EXPORT int jack_port_request_monitor(jack_port_t* port, int onoff)
{
	jack_log("jack_port_request_monitor");
    return (*jack_port_request_monitor_fun)(port, onoff);
}

typedef int (*jack_port_request_monitor_by_name_fun_def)(jack_client_t* ext_client, const char* port_name, int onoff);
static jack_port_request_monitor_by_name_fun_def jack_port_request_monitor_by_name_fun = 0;
EXPORT int jack_port_request_monitor_by_name(jack_client_t* ext_client, const char* port_name, int onoff)
{
	jack_log("jack_port_request_monitor_by_name");
    return (*jack_port_request_monitor_by_name_fun)(ext_client, port_name, onoff);
}

typedef int (*jack_port_ensure_monitor_fun_def)(jack_port_t* port, int onoff);
static jack_port_ensure_monitor_fun_def jack_port_ensure_monitor_fun = 0;
EXPORT int jack_port_ensure_monitor(jack_port_t* port, int onoff)
{
	jack_log("jack_port_ensure_monitor");
    return (*jack_port_ensure_monitor_fun)(port, onoff);
}

typedef int (*jack_port_monitoring_input_fun_def)(jack_port_t* port);
static jack_port_monitoring_input_fun_def jack_port_monitoring_input_fun = 0;
EXPORT int jack_port_monitoring_input(jack_port_t* port)
{
	jack_log("jack_port_monitoring_input");
    return (*jack_port_monitoring_input_fun)(port);
}

typedef int (*jack_is_realtime_fun_def)(jack_client_t* ext_client);
static jack_is_realtime_fun_def jack_is_realtime_fun = 0;
EXPORT int jack_is_realtime(jack_client_t* ext_client)
{
	jack_log("jack_is_realtime");
    return (*jack_is_realtime_fun)(ext_client);
}

typedef void (*shutdown_fun)(void* arg);
typedef void (*jack_on_shutdown_fun_def)(jack_client_t* ext_client, shutdown_fun callback, void* arg);
static jack_on_shutdown_fun_def jack_on_shutdown_fun = 0;
EXPORT void jack_on_shutdown(jack_client_t* ext_client, shutdown_fun callback, void* arg)
{
	jack_log("jack_on_shutdown");
	(*jack_on_shutdown_fun)(ext_client, callback, arg);
}

typedef int (*jack_set_process_callback_fun_def)(jack_client_t* ext_client, JackProcessCallback callback, void* arg);
static jack_set_process_callback_fun_def jack_set_process_callback_fun = 0;
EXPORT int jack_set_process_callback(jack_client_t* ext_client, JackProcessCallback callback, void* arg)
{
	jack_log("jack_set_process_callback");
    return (*jack_set_process_callback_fun)(ext_client, callback, arg);
}

typedef int (*jack_set_freewheel_callback_fun_def)(jack_client_t* ext_client, JackFreewheelCallback freewheel_callback, void* arg);
static jack_set_freewheel_callback_fun_def jack_set_freewheel_callback_fun = 0;
EXPORT int jack_set_freewheel_callback(jack_client_t* ext_client, JackFreewheelCallback freewheel_callback, void* arg)
{
	jack_log("jack_set_freewheel_callback");
    return (*jack_set_freewheel_callback_fun)(ext_client, freewheel_callback, arg);
}

typedef int (*jack_set_freewheel_fun_def)(jack_client_t* ext_client, int onoff);
static jack_set_freewheel_fun_def jack_set_freewheel_fun = 0;
EXPORT int jack_set_freewheel(jack_client_t* ext_client, int onoff)
{
	jack_log("jack_set_freewheel");
    return (*jack_set_freewheel_fun)(ext_client, onoff);
}

typedef int (*jack_set_buffer_size_fun_def)(jack_client_t* ext_client, jack_nframes_t buffer_size);
static jack_set_buffer_size_fun_def jack_set_buffer_size_fun = 0;
EXPORT int jack_set_buffer_size(jack_client_t* ext_client, jack_nframes_t buffer_size)
{
	jack_log("jack_set_buffer_size");
    return (*jack_set_buffer_size_fun)(ext_client, buffer_size);
}

typedef int (*jack_set_buffer_size_callback_fun_def)(jack_client_t* ext_client, JackBufferSizeCallback bufsize_callback, void* arg);
static jack_set_buffer_size_callback_fun_def jack_set_buffer_size_callback_fun = 0;
EXPORT int jack_set_buffer_size_callback(jack_client_t* ext_client, JackBufferSizeCallback bufsize_callback, void* arg)
{
	jack_log("jack_set_buffer_size_callback");
    return (*jack_set_buffer_size_callback_fun)(ext_client, bufsize_callback, arg);
}

typedef int (*jack_set_sample_rate_callback_fun_def)(jack_client_t* ext_client, JackSampleRateCallback srate_callback, void* arg);
static jack_set_sample_rate_callback_fun_def jack_set_sample_rate_callback_fun = 0;
EXPORT int jack_set_sample_rate_callback(jack_client_t* ext_client, JackSampleRateCallback srate_callback, void* arg)
{
	jack_log("jack_set_sample_rate_callback");
    return (*jack_set_sample_rate_callback_fun)(ext_client, srate_callback, arg);
}

typedef int (*jack_set_client_registration_callback_fun_def)(jack_client_t* ext_client, JackClientRegistrationCallback registration_callback, void* arg);
static jack_set_client_registration_callback_fun_def jack_set_client_registration_callback_fun = 0;
EXPORT int jack_set_client_registration_callback(jack_client_t* ext_client, JackClientRegistrationCallback registration_callback, void* arg)
{
	jack_log("jack_set_client_registration_callback");
    return (*jack_set_client_registration_callback_fun)(ext_client, registration_callback, arg);
}

typedef int (*jack_set_port_registration_callback_fun_def)(jack_client_t* ext_client, JackPortRegistrationCallback registration_callback, void* arg);
static jack_set_port_registration_callback_fun_def jack_set_port_registration_callback_fun = 0;
EXPORT int jack_set_port_registration_callback(jack_client_t* ext_client, JackPortRegistrationCallback registration_callback, void* arg)
{
	jack_log("jack_set_port_registration_callback");
    return (*jack_set_port_registration_callback_fun)(ext_client, registration_callback, arg);
}

typedef int (*jack_set_port_connect_callback_fun_def)(jack_client_t* ext_client, JackPortConnectCallback connect_callback, void* arg);
static jack_set_port_connect_callback_fun_def jack_set_port_connect_callback_fun = 0;
EXPORT int jack_set_port_connect_callback(jack_client_t* ext_client, JackPortConnectCallback connect_callback, void* arg)
{
	jack_log("jack_set_port_connect_callback");
    return (*jack_set_port_connect_callback_fun)(ext_client, connect_callback, arg);
}

typedef int (*jack_set_graph_order_callback_fun_def)(jack_client_t* ext_client, JackGraphOrderCallback graph_callback, void* arg);
static jack_set_graph_order_callback_fun_def jack_set_graph_order_callback_fun = 0;
EXPORT int jack_set_graph_order_callback(jack_client_t* ext_client, JackGraphOrderCallback graph_callback, void* arg)
{
	jack_log("jack_set_graph_order_callback");
    return (*jack_set_graph_order_callback_fun)(ext_client, graph_callback, arg);
}

typedef int (*jack_set_xrun_callback_fun_def)(jack_client_t* ext_client, JackXRunCallback xrun_callback, void* arg);
static jack_set_xrun_callback_fun_def jack_set_xrun_callback_fun = 0;
EXPORT int jack_set_xrun_callback(jack_client_t* ext_client, JackXRunCallback xrun_callback, void* arg)
{
	jack_log("jack_set_xrun_callback");
    return (*jack_set_xrun_callback_fun)(ext_client, xrun_callback, arg);
}

typedef int (*jack_set_thread_init_callback_fun_def)(jack_client_t* ext_client, JackThreadInitCallback init_callback, void *arg);
static jack_set_thread_init_callback_fun_def jack_set_thread_init_callback_fun = 0;
EXPORT int jack_set_thread_init_callback(jack_client_t* ext_client, JackThreadInitCallback init_callback, void *arg)
{
	jack_log("jack_set_thread_init_callback");
    return (*jack_set_thread_init_callback_fun)(ext_client, init_callback, arg);
}

typedef int (*jack_activate_fun_def)(jack_client_t* ext_client);
static jack_activate_fun_def jack_activate_fun = 0;
EXPORT int jack_activate(jack_client_t* ext_client)
{
	jack_log("jack_activate");
    return (*jack_activate_fun)(ext_client);
}

typedef int (*jack_deactivate_fun_def)(jack_client_t* ext_client);
static jack_deactivate_fun_def jack_deactivate_fun = 0;
EXPORT int jack_deactivate(jack_client_t* ext_client)
{
	jack_log("jack_deactivate");
    return (*jack_deactivate_fun)(ext_client);
}

typedef jack_port_t* (*jack_port_register_fun_def)(jack_client_t* ext_client, const char* port_name, const char* port_type, unsigned long flags, unsigned long buffer_size);
static jack_port_register_fun_def jack_port_register_fun = 0;
EXPORT jack_port_t* jack_port_register(jack_client_t* ext_client, const char* port_name, const char* port_type, unsigned long flags, unsigned long buffer_size)
{
	jack_log("jack_port_register");
    return (*jack_port_register_fun)(ext_client, port_name, port_type, flags, buffer_size);
}

typedef int (*jack_port_unregister_fun_def)(jack_client_t* ext_client, jack_port_t* port);
static jack_port_unregister_fun_def jack_port_unregister_fun = 0;
EXPORT int jack_port_unregister(jack_client_t* ext_client, jack_port_t* port)
{
	jack_log("jack_port_unregister");
    return (*jack_port_unregister_fun)(ext_client, port);
}

typedef int (*jack_port_is_mine_fun_def)(const jack_client_t* ext_client, const jack_port_t* port);
static jack_port_is_mine_fun_def jack_port_is_mine_fun = 0;
EXPORT int jack_port_is_mine(const jack_client_t* ext_client, const jack_port_t* port)
{
	jack_log("jack_port_is_mine");
    return (*jack_port_is_mine_fun)(ext_client, port);
}

typedef const char** (*jack_port_get_connections_fun_def)(const jack_port_t* port);
static jack_port_get_connections_fun_def jack_port_get_connections_fun = 0;
EXPORT const char** jack_port_get_connections(const jack_port_t* port)
{
	jack_log("jack_port_get_connections");
    return (*jack_port_get_connections_fun)(port);
}

// Calling client does not need to "own" the port
typedef const char** (*jack_port_get_all_connections_fun_def)(const jack_client_t* ext_client, const jack_port_t* port);
static jack_port_get_all_connections_fun_def jack_port_get_all_connections_fun = 0;
EXPORT const char** jack_port_get_all_connections(const jack_client_t* ext_client, const jack_port_t* port)
{
	jack_log("jack_port_get_all_connections");
    return (*jack_port_get_all_connections_fun)(ext_client, port);
}

typedef jack_nframes_t (*jack_port_get_total_latency_fun_def)(jack_client_t* ext_client, jack_port_t* port);
static jack_port_get_total_latency_fun_def jack_port_get_total_latency_fun = 0;
EXPORT jack_nframes_t jack_port_get_total_latency(jack_client_t* ext_client, jack_port_t* port)
{
	jack_log("jack_port_get_total_latency");
    return (*jack_port_get_total_latency_fun)(ext_client, port);
}

typedef int (*jack_connect_fun_def)(jack_client_t* ext_client, const char* src, const char* dst);
static jack_connect_fun_def jack_connect_fun = 0;
EXPORT int jack_connect(jack_client_t* ext_client, const char* src, const char* dst)
{
	jack_log("jack_connect");
    return (*jack_connect_fun)(ext_client, src, dst);
}

typedef int (*jack_disconnect_fun_def)(jack_client_t* ext_client, const char* src, const char* dst);
static jack_disconnect_fun_def jack_disconnect_fun = 0;
EXPORT int jack_disconnect(jack_client_t* ext_client, const char* src, const char* dst)
{
	jack_log("jack_disconnect");
    return (*jack_disconnect_fun)(ext_client, src, dst);
}

typedef int (*jack_port_disconnect_fun_def)(jack_client_t* ext_client, jack_port_t* src);
static jack_port_disconnect_fun_def jack_port_disconnect_fun = 0;
EXPORT int jack_port_disconnect(jack_client_t* ext_client, jack_port_t* src)
{
	jack_log("jack_port_disconnect");
    return (*jack_port_disconnect_fun)(ext_client, src);
}

typedef jack_nframes_t (*jack_get_sample_rate_fun_def)(jack_client_t* ext_client);
static jack_get_sample_rate_fun_def jack_get_sample_rate_fun = 0;
EXPORT jack_nframes_t jack_get_sample_rate(jack_client_t* ext_client)
{
	jack_log("jack_get_sample_rate");
    return (*jack_get_sample_rate_fun)(ext_client);
}

typedef jack_nframes_t (*jack_get_buffer_size_fun_def)(jack_client_t* ext_client);
static jack_get_buffer_size_fun_def jack_get_buffer_size_fun = 0;
EXPORT jack_nframes_t jack_get_buffer_size(jack_client_t* ext_client)
{
	jack_log("jack_get_buffer_size");
    return (*jack_get_buffer_size_fun)(ext_client);
}

typedef const char** (*jack_get_ports_fun_def)(jack_client_t* ext_client, const char* port_name_pattern, const char* type_name_pattern, unsigned long flags);
static jack_get_ports_fun_def jack_get_ports_fun = 0;
EXPORT const char** jack_get_ports(jack_client_t* ext_client, const char* port_name_pattern, const char* type_name_pattern, unsigned long flags)
{
	jack_log("jack_get_ports");
    return (*jack_get_ports_fun)(ext_client, port_name_pattern, type_name_pattern, flags);
}

typedef jack_port_t* (*jack_port_by_name_fun_def)(jack_client_t* ext_client, const char* portname);
static jack_port_by_name_fun_def jack_port_by_name_fun = 0;
EXPORT jack_port_t* jack_port_by_name(jack_client_t* ext_client, const char* portname)
{
	jack_log("jack_port_by_name");
    return (*jack_port_by_name_fun)(ext_client, portname);
}

typedef jack_port_t* (*jack_port_by_id_fun_def)(const jack_client_t* ext_client, jack_port_id_t id);
static jack_port_by_id_fun_def jack_port_by_id_fun = 0;
EXPORT jack_port_t* jack_port_by_id(const jack_client_t* ext_client, jack_port_id_t id)
{
	jack_log("jack_port_by_id");
    return (*jack_port_by_id_fun)(ext_client, id);
}

typedef int (*jack_engine_takeover_timebase_fun_def)(jack_client_t* ext_client);
static jack_engine_takeover_timebase_fun_def jack_engine_takeover_timebase_fun = 0;
EXPORT int jack_engine_takeover_timebase(jack_client_t* ext_client)
{
	jack_log("jack_engine_takeover_timebase");
    return (*jack_engine_takeover_timebase_fun)(ext_client);
}

typedef jack_nframes_t (*jack_frames_since_cycle_start_fun_def)(const jack_client_t* ext_client);
static jack_frames_since_cycle_start_fun_def jack_frames_since_cycle_start_fun = 0;
EXPORT jack_nframes_t jack_frames_since_cycle_start(const jack_client_t* ext_client)
{
	jack_log("jack_frames_since_cycle_start");
    return (*jack_frames_since_cycle_start_fun)(ext_client);
}

typedef jack_time_t (*jack_get_time_fun_def)();
static jack_get_time_fun_def jack_get_time_fun = 0;
EXPORT jack_time_t jack_get_time()
{
	jack_log("jack_get_time");
    return (*jack_get_time_fun)();
}

typedef jack_nframes_t (*jack_time_to_frames_fun_def)(const jack_client_t* ext_client, jack_time_t time);
static jack_time_to_frames_fun_def jack_time_to_frames_fun = 0;
EXPORT jack_nframes_t jack_time_to_frames(const jack_client_t* ext_client, jack_time_t time)
{
	jack_log("jack_time_to_frames");
    return (*jack_time_to_frames_fun)(ext_client, time);
}

typedef jack_time_t (*jack_frames_to_time_fun_def)(const jack_client_t* ext_client, jack_nframes_t frames);
static jack_frames_to_time_fun_def jack_frames_to_time_fun = 0;
EXPORT jack_time_t jack_frames_to_time(const jack_client_t* ext_client, jack_nframes_t frames)
{
	jack_log("jack_frames_to_time");
    return (*jack_frames_to_time_fun)(ext_client, frames);
}

typedef jack_nframes_t (*jack_frame_time_fun_def)(const jack_client_t* ext_client);
static jack_frame_time_fun_def jack_frame_time_fun = 0;
EXPORT jack_nframes_t jack_frame_time(const jack_client_t* ext_client)
{
	jack_log("jack_frame_time");
    return (*jack_frame_time_fun)(ext_client);
}

typedef jack_nframes_t (*jack_last_frame_time_fun_def)(const jack_client_t* ext_client);
static jack_last_frame_time_fun_def jack_last_frame_time_fun = 0;
EXPORT jack_nframes_t jack_last_frame_time(const jack_client_t* ext_client)
{
	jack_log("jack_last_frame_time");
    return (*jack_last_frame_time_fun)(ext_client);
}

typedef float (*jack_cpu_load_fun_def)(jack_client_t* ext_client);
static jack_cpu_load_fun_def jack_cpu_load_fun = 0;
EXPORT float jack_cpu_load(jack_client_t* ext_client)
{
	jack_log("jack_cpu_load");
    return (*jack_cpu_load_fun)(ext_client);
}

typedef pthread_t (*jack_client_thread_id_fun_def)(jack_client_t* ext_client);
static jack_client_thread_id_fun_def jack_client_thread_id_fun = 0;
EXPORT pthread_t  jack_client_thread_id(jack_client_t* ext_client)
{
	jack_log("jack_client_thread_id");
    return (*jack_client_thread_id_fun)(ext_client);
}

typedef void (*jack_set_error_function_fun_def)(void (*func)(const char *));
static jack_set_error_function_fun_def jack_set_error_function_fun = 0;
EXPORT void jack_set_error_function(void (*func)(const char *))
{
	jack_log("jack_set_error_function");
	if (gLibrary) {
		(*jack_set_error_function_fun)(func);
	} else {
		error_fun = func; // Keep the function
	}
}

typedef void (*jack_set_info_function_fun_def)(void (*func)(const char *));
static jack_set_info_function_fun_def jack_set_info_function_fun = 0;
EXPORT void jack_set_info_function(void (*func)(const char *))
{
	jack_log("jack_set_info_function");
	if (gLibrary) {
		(*jack_set_error_function_fun)(func);
	} else {
		info_fun = func; // Keep the function
	}
}

typedef char* (*jack_get_client_name_fun_def)(jack_client_t* ext_client);
static jack_get_client_name_fun_def jack_get_client_name_fun = 0;
EXPORT char* jack_get_client_name (jack_client_t* ext_client)
{
	jack_log("jack_get_client_name");
    return (*jack_get_client_name_fun)(ext_client);
}

typedef int (*jack_internal_client_new_fun_def)(const char *client_name,
												const char *load_name,
												const char *load_init);
static jack_internal_client_new_fun_def jack_internal_client_new_fun = 0;
EXPORT int jack_internal_client_new (const char *client_name,
										const char *load_name,
										const char *load_init)
{
	jack_log("jack_internal_client_new");
    return (*jack_internal_client_new_fun)(client_name, load_name, load_init);
}

typedef void (*jack_internal_client_close_fun_def)(const char *client_name);
static jack_internal_client_close_fun_def jack_internal_client_close_fun = 0;
EXPORT void jack_internal_client_close (const char *client_name)
{
	jack_log("jack_internal_client_close");
	(*jack_internal_client_close_fun)(client_name);
}

typedef int (*jack_client_name_size_fun_def)(void);
static jack_client_name_size_fun_def jack_client_name_size_fun = 0;
EXPORT int jack_client_name_size(void)
{
	jack_log("jack_client_name_size");
    return (*jack_client_name_size_fun)();
}

typedef int (*jack_port_name_size_fun_def)(void);
static jack_port_name_size_fun_def jack_port_name_size_fun = 0;
EXPORT int jack_port_name_size(void)
{
	jack_log("jack_port_name_size");
    return (*jack_port_name_size_fun)();
}

typedef int (*jack_port_type_size_fun_def)(void);
static jack_port_type_size_fun_def jack_port_type_size_fun = 0;
EXPORT int jack_port_type_size(void)
{
	jack_log("jack_port_type_size");
    return (*jack_port_type_size_fun)();
}

// transport.h

typedef int (*jack_release_timebase_fun_def)(jack_client_t* ext_client);
static jack_release_timebase_fun_def jack_release_timebase_fun = 0;
EXPORT int jack_release_timebase(jack_client_t* ext_client)
{
	jack_log("jack_release_timebase");
    return (*jack_release_timebase_fun)(ext_client);
}

typedef int (*jack_set_sync_callback_fun_def)(jack_client_t* ext_client, JackSyncCallback sync_callback, void *arg);
static jack_set_sync_callback_fun_def jack_set_sync_callback_fun = 0;
EXPORT int jack_set_sync_callback(jack_client_t* ext_client, JackSyncCallback sync_callback, void *arg)
{
	jack_log("jack_set_sync_callback");
    return (*jack_set_sync_callback_fun)(ext_client, sync_callback, arg);
}

typedef int (*jack_set_sync_timeout_fun_def)(jack_client_t* ext_client, jack_time_t timeout);
static jack_set_sync_timeout_fun_def jack_set_sync_timeout_fun = 0;
EXPORT int jack_set_sync_timeout(jack_client_t* ext_client, jack_time_t timeout)
{
	jack_log("jack_set_sync_timeout");
    return (*jack_set_sync_timeout_fun)(ext_client, timeout);
}

typedef int (*jack_set_timebase_callback_fun_def)(jack_client_t* ext_client, int conditional, JackTimebaseCallback timebase_callback, void* arg);
static jack_set_timebase_callback_fun_def jack_set_timebase_callback_fun = 0;
EXPORT int jack_set_timebase_callback(jack_client_t* ext_client, int conditional, JackTimebaseCallback timebase_callback, void* arg)
{
	jack_log("jack_set_timebase_callback");
    return (*jack_set_timebase_callback_fun)(ext_client, conditional, timebase_callback, arg);
}

typedef int (*jack_transport_locate_fun_def)(jack_client_t* ext_client, jack_nframes_t frame);
static jack_transport_locate_fun_def jack_transport_locate_fun = 0;
EXPORT int jack_transport_locate(jack_client_t* ext_client, jack_nframes_t frame)
{
	jack_log("jack_transport_locate");
    return (*jack_transport_locate_fun)(ext_client, frame);
}

typedef jack_transport_state_t (*jack_transport_query_fun_def)(const jack_client_t* ext_client, jack_position_t* pos);
static jack_transport_query_fun_def jack_transport_query_fun = 0;
EXPORT jack_transport_state_t jack_transport_query(const jack_client_t* ext_client, jack_position_t* pos)
{
	jack_log("jack_transport_query");
    return (*jack_transport_query_fun)(ext_client, pos);
}

typedef jack_nframes_t (*jack_get_current_transport_frame_fun_def)(const jack_client_t* ext_client);
static jack_get_current_transport_frame_fun_def jack_get_current_transport_frame_fun = 0;
EXPORT jack_nframes_t jack_get_current_transport_frame(const jack_client_t* ext_client)
{
	jack_log("jack_get_current_transport_frame");
    return (*jack_get_current_transport_frame_fun)(ext_client);
}

typedef int (*jack_transport_reposition_fun_def)(jack_client_t* ext_client, jack_position_t* pos);
static jack_transport_reposition_fun_def jack_transport_reposition_fun = 0;
EXPORT int jack_transport_reposition(jack_client_t* ext_client, jack_position_t* pos)
{
	jack_log("jack_transport_reposition");
    return (*jack_transport_reposition_fun)(ext_client, pos);
}

typedef void (*jack_transport_start_fun_def)(jack_client_t* ext_client);
static jack_transport_start_fun_def jack_transport_start_fun = 0;
EXPORT void jack_transport_start(jack_client_t* ext_client)
{
	jack_log("jack_transport_start");
	(*jack_transport_start_fun)(ext_client);
}

typedef void (*jack_transport_stop_fun_def)(jack_client_t* ext_client);
static jack_transport_stop_fun_def jack_transport_stop_fun = 0;
EXPORT void jack_transport_stop(jack_client_t* ext_client)
{
	jack_log("jack_transport_stop");
	(*jack_transport_stop_fun)(ext_client);
}

// deprecated

typedef void (*jack_get_transport_info_fun_def)(jack_client_t* ext_client, jack_transport_info_t* tinfo);
static jack_get_transport_info_fun_def jack_get_transport_info_fun = 0;
EXPORT void jack_get_transport_info(jack_client_t* ext_client, jack_transport_info_t* tinfo)
{
	jack_log("jack_get_transport_info");
    (*jack_get_transport_info_fun)(ext_client, tinfo);
}

typedef void (*jack_set_transport_info_fun_def)(jack_client_t* ext_client, jack_transport_info_t* tinfo);
static jack_set_transport_info_fun_def jack_set_transport_info_fun = 0;
EXPORT void jack_set_transport_info(jack_client_t* ext_client, jack_transport_info_t* tinfo)
{
	jack_log("jack_set_transport_info");
    (*jack_set_transport_info_fun)(ext_client, tinfo);
}

// statistics.h

typedef float (*jack_get_max_delayed_usecs_fun_def)(jack_client_t* ext_client);
static jack_get_max_delayed_usecs_fun_def jack_get_max_delayed_usecs_fun = 0;
EXPORT float jack_get_max_delayed_usecs(jack_client_t* ext_client)
{
	jack_log("jack_get_max_delayed_usecs");
    return (*jack_get_max_delayed_usecs_fun)(ext_client);
}

typedef float (*jack_get_xrun_delayed_usecs_fun_def)(jack_client_t* ext_client);
static jack_get_xrun_delayed_usecs_fun_def jack_get_xrun_delayed_usecs_fun = 0;
EXPORT float jack_get_xrun_delayed_usecs(jack_client_t* ext_client)
{
	jack_log("jack_get_xrun_delayed_usecs");
    return (*jack_get_xrun_delayed_usecs_fun)(ext_client);
}

typedef void (*jack_reset_max_delayed_usecs_fun_def)(jack_client_t* ext_client);
static jack_reset_max_delayed_usecs_fun_def jack_reset_max_delayed_usecs_fun = 0;
EXPORT void jack_reset_max_delayed_usecs(jack_client_t* ext_client)
{
	jack_log("jack_reset_max_delayed_usecs");
    (*jack_reset_max_delayed_usecs_fun)(ext_client);
}

// thread.h

typedef int (*jack_acquire_real_time_scheduling_fun_def)(pthread_t thread, int priority);
static jack_acquire_real_time_scheduling_fun_def jack_acquire_real_time_scheduling_fun = 0;
EXPORT int jack_acquire_real_time_scheduling(pthread_t thread, int priority)
{
	jack_log("jack_acquire_real_time_scheduling");
    return (*jack_acquire_real_time_scheduling_fun)(thread, priority);
}

typedef void *(*start_routine)(void*);
typedef int (*jack_client_create_thread_fun_def)(jack_client_t* client,
        pthread_t *thread,
        int priority,
        int realtime,  	// boolean
        start_routine callback,
        void *arg);
static jack_client_create_thread_fun_def jack_client_create_thread_fun = 0;
EXPORT int jack_client_create_thread(jack_client_t* client,
                                     pthread_t *thread,
                                     int priority,
                                     int realtime,  	// boolean
                                     start_routine callback,
                                     void *arg)
{
	jack_log("jack_client_create_thread");
    return (*jack_client_create_thread_fun)(client, thread, priority, realtime, callback, arg);
}

typedef int (*jack_drop_real_time_scheduling_fun_def)(pthread_t thread);
static jack_drop_real_time_scheduling_fun_def jack_drop_real_time_scheduling_fun = 0;
EXPORT int jack_drop_real_time_scheduling(pthread_t thread)
{
	jack_log("jack_client_create_thread");
    return (*jack_drop_real_time_scheduling_fun)(thread);
}

// intclient.h

typedef char* (*jack_get_internal_client_name_fun_def)(jack_client_t* ext_client, jack_intclient_t intclient);
static jack_get_internal_client_name_fun_def jack_get_internal_client_name_fun = 0;
EXPORT char* jack_get_internal_client_name(jack_client_t* ext_client, jack_intclient_t intclient)
{
	jack_log("jack_get_internal_client_name");
    return (*jack_get_internal_client_name_fun)(ext_client, intclient);
}

typedef jack_intclient_t (*jack_internal_client_handle_fun_def)(jack_client_t* ext_client, const char* client_name, jack_status_t* status);
static jack_internal_client_handle_fun_def jack_internal_client_handle_fun = 0;
EXPORT jack_intclient_t jack_internal_client_handle(jack_client_t* ext_client, const char* client_name, jack_status_t* status)
{
	jack_log("jack_internal_client_handle");
    return (*jack_internal_client_handle_fun)(ext_client, client_name, status);
}

typedef jack_intclient_t (*jack_internal_client_load_aux_fun_def)(jack_client_t* ext_client, const char* client_name, jack_options_t options, jack_status_t* status, va_list ap);
static jack_internal_client_load_aux_fun_def jack_internal_client_load_aux_fun = 0;
EXPORT jack_intclient_t jack_internal_client_load(jack_client_t* ext_client, const char* client_name, jack_options_t options, jack_status_t* status, ...)
{
	jack_log("jack_internal_client_load");
    va_list ap;
    va_start(ap, status);
    jack_intclient_t res =  (*jack_internal_client_load_aux_fun)(ext_client, client_name, options, status, ap);
    va_end(ap);
    return res;
}

typedef jack_status_t (*jack_internal_client_unload_fun_def)(jack_client_t* ext_client, jack_intclient_t intclient);
static jack_internal_client_unload_fun_def jack_internal_client_unload_fun = 0;
EXPORT jack_status_t jack_internal_client_unload(jack_client_t* ext_client, jack_intclient_t intclient)
{
	jack_log("jack_internal_client_unload");
    return (*jack_internal_client_unload_fun)(ext_client, intclient);
}


typedef jack_client_t * (*jack_client_open_aux_fun_def)(const char *client_name, jack_options_t options, jack_status_t *status, va_list ap);
static jack_client_open_aux_fun_def jack_client_open_aux_fun = 0;
EXPORT jack_client_t * jack_client_open(const char *client_name, jack_options_t options, jack_status_t *status, ...)
{
	// TODO : in "autostart mode", has to load jackdrc file and figure out which server has to be started...
	jack_log("jack_client_open");
	
	/*
	va_list ap;
	va_start (ap, status);
	
	if (try_start_server(&va, options, status)) {
        jack_error("jack server is not running or cannot be started");
        JackLibGlobals::Destroy(); // jack library destruction
        return 0;
    }
	
	va_end (ap);
	*/
	
    // Library check...
    if (!open_library())
        return 0;

    va_list ap;
    va_start(ap, status);
    jack_client_t* res = (*jack_client_open_aux_fun)(client_name, options, status, ap);
    va_end(ap);
    return res;
}

typedef jack_client_t * (*jack_client_new_fun_def)(const char *client_name);
static jack_client_new_fun_def jack_client_new_fun = 0;
EXPORT jack_client_t * jack_client_new(const char *client_name)
{
	jack_log("jack_client_new");
    // Library check...
    if (!open_library())
        return 0;

    return (*jack_client_new_fun)(client_name);
}

typedef int (*jack_client_close_fun_def)(jack_client_t *client);
static jack_client_close_fun_def jack_client_close_fun = 0;
EXPORT int jack_client_close(jack_client_t *client)
{
	jack_log("jack_client_close");
    int res = (*jack_client_close_fun)(client);
    close_library();
    return res;
}

// Library loader
static bool get_jack_library_in_directory(const char* dir_name, const char* library_name, char* library_res_name)
{
	jack_log("get_jack_library_in_directory");

    struct dirent * dir_entry;
    DIR * dir_stream = opendir(dir_name);
    if (!dir_stream)
        return false;

    while ((dir_entry = readdir(dir_stream))) {
        if (strncmp(library_name, dir_entry->d_name, strlen(library_name)) == 0) {
			jack_log("found");
        	sprintf(library_res_name, "%s/%s", dir_name, dir_entry->d_name);
            closedir(dir_stream);
            return true;
        }
    }
    closedir(dir_stream);
    return false;
}

static bool get_jack_library(const char* library_name, char* library_res_name)
{
    if (get_jack_library_in_directory("/usr/lib", library_name, library_res_name))
        return true;
    if (get_jack_library_in_directory("/usr/local/lib", library_name, library_res_name))
        return true;
    return false;
}

static bool open_library()
{
	jack_log("open_library %ld", gInitedLib);
    if (!gInitedLib) 
        gInitedLib = init_library();
	return gInitedLib;
}

static void close_library()
{
	jack_log("close_library");
    if (gInitedLib) {
        dlclose(gLibrary);
		gLibrary = 0;
		gInitedLib = false;
    }
}

static bool check_client(void* library)
{
    jack_client_t* client = 0;

	jack_log("check_library");

    // Get "new" and "close" entry points...
    jack_client_new_fun = (jack_client_new_fun_def)dlsym(library, "jack_client_new");
    jack_client_close_fun = (jack_client_close_fun_def)dlsym(library, "jack_client_close");
 
    // Try opening a client...
    if ((client = (*jack_client_new_fun)("dummy"))) { // server is running....
		jack_log("check_library 1  %x", jack_client_close_fun);
        (*jack_client_close_fun)(client);
		jack_log("check_library 2");
        return true;
    } else {
		jack_log("check_library NO");
        return false;
    }
}

static bool init_library()
{
    char library_res_name[256];
    void* jackLibrary = (get_jack_library(JACK_LIB, library_res_name)) ? dlopen(library_res_name, RTLD_LAZY) : 0;
    void* jackmpLibrary = (get_jack_library(JACKMP_LIB, library_res_name)) ? dlopen(library_res_name, RTLD_LAZY) : 0;

    if (jackLibrary) {

		jack_log("jackLibrary");

        if (check_client(jackLibrary)) { // jackd is running...
			jack_log("jackd is running");
            gLibrary = jackLibrary;
            if (jackmpLibrary) 
				dlclose(jackmpLibrary);
			jack_log("jackd is running OK");
        } else if (check_client(jackmpLibrary)) { // jackdmp is running...
            gLibrary = jackmpLibrary;
            if (jackLibrary) 
				dlclose(jackLibrary);
        } else {
            goto error;
        }

    } else if (jackmpLibrary) {

		jack_log("jackmpLibrary");

        if (check_client(jackmpLibrary)) { // jackd is running...
            gLibrary = jackmpLibrary;
        } else {
            goto error;
        }

    } else {
        jack_log("Jack libraries not found, failure...");
        goto error;
    }

    // Load entry points...    
	jack_client_open_aux_fun = (jack_client_open_aux_fun_def)dlsym(gLibrary, "jack_client_open_aux");
    jack_client_new_fun = (jack_client_new_fun_def)dlsym(gLibrary, "jack_client_new");
	jack_client_close_fun = (jack_client_close_fun_def)dlsym(gLibrary, "jack_client_close");
   	jack_client_name_size_fun = (jack_client_name_size_fun_def)dlsym(gLibrary, "jack_client_name_size");
	jack_get_client_name_fun = (jack_get_client_name_fun_def)dlsym(gLibrary, "jack_get_client_name"); 
	jack_internal_client_new_fun = (jack_internal_client_new_fun_def)dlsym(gLibrary, "jack_internal_client_new"); 
	jack_internal_client_close_fun = (jack_internal_client_close_fun_def)dlsym(gLibrary, "jack_internal_client_close");    
	jack_is_realtime_fun = (jack_is_realtime_fun_def)dlsym(gLibrary, "jack_is_realtime");
    jack_on_shutdown_fun = (jack_on_shutdown_fun_def)dlsym(gLibrary, "jack_on_shutdown");
    jack_set_process_callback_fun = (jack_set_process_callback_fun_def)dlsym(gLibrary, "jack_set_process_callback");
	jack_set_thread_init_callback_fun = (jack_set_thread_init_callback_fun_def)dlsym(gLibrary, "jack_set_thread_init_callback");
    jack_set_freewheel_callback_fun = (jack_set_freewheel_callback_fun_def)dlsym(gLibrary, "jack_set_freewheel_callback");
    jack_set_freewheel_fun = (jack_set_freewheel_fun_def)dlsym(gLibrary, "jack_set_freewheel");
    jack_set_buffer_size_fun = (jack_set_buffer_size_fun_def)dlsym(gLibrary, "jack_set_buffer_size");
    jack_set_buffer_size_callback_fun = (jack_set_buffer_size_callback_fun_def)dlsym(gLibrary, "jack_set_buffer_size_callback");
    jack_set_sample_rate_callback_fun = (jack_set_sample_rate_callback_fun_def)dlsym(gLibrary, "jack_set_sample_rate_callback");
    jack_set_client_registration_callback_fun = (jack_set_client_registration_callback_fun_def)dlsym(gLibrary, "jack_set_client_registration_callback");
    jack_set_port_registration_callback_fun = (jack_set_port_registration_callback_fun_def)dlsym(gLibrary, "jack_set_port_registration_callback");
    jack_set_port_connect_callback_fun = (jack_set_port_connect_callback_fun_def)dlsym(gLibrary, "jack_set_port_connect_callback");
    jack_set_graph_order_callback_fun = (jack_set_graph_order_callback_fun_def)dlsym(gLibrary, "jack_set_graph_order_callback");
    jack_set_xrun_callback_fun = (jack_set_xrun_callback_fun_def)dlsym(gLibrary, "jack_set_xrun_callback");
    jack_activate_fun = (jack_activate_fun_def)dlsym(gLibrary, "jack_activate");
    jack_deactivate_fun = (jack_deactivate_fun_def)dlsym(gLibrary, "jack_deactivate");
    jack_port_register_fun = (jack_port_register_fun_def)dlsym(gLibrary, "jack_port_register");
    jack_port_unregister_fun = (jack_port_unregister_fun_def)dlsym(gLibrary, "jack_port_unregister");
	jack_port_get_buffer_fun = (jack_port_get_buffer_fun_def)dlsym(gLibrary, "jack_port_get_buffer");
	jack_port_name_fun = (jack_port_name_fun_def)dlsym(gLibrary, "jack_port_name");
    jack_port_short_name_fun = (jack_port_short_name_fun_def)dlsym(gLibrary, "jack_port_short_name");
    jack_port_flags_fun = (jack_port_flags_fun_def)dlsym(gLibrary, "jack_port_flags");
    jack_port_type_fun = (jack_port_type_fun_def)dlsym(gLibrary, "jack_port_type");
    jack_port_is_mine_fun = (jack_port_is_mine_fun_def)dlsym(gLibrary, "jack_port_is_mine");
	jack_port_connected_fun = (jack_port_connected_fun_def)dlsym(gLibrary, "jack_port_connected");
    jack_port_connected_to_fun = (jack_port_connected_to_fun_def)dlsym(gLibrary, "jack_port_connected_to");
    jack_port_get_connections_fun = (jack_port_get_connections_fun_def)dlsym(gLibrary, "jack_port_get_connections");
    jack_port_get_all_connections_fun = (jack_port_get_all_connections_fun_def)dlsym(gLibrary, "jack_port_get_all_connections");
	jack_port_tie_fun = (jack_port_tie_fun_def)dlsym(gLibrary, "jack_port_tie");
    jack_port_untie_fun = (jack_port_untie_fun_def)dlsym(gLibrary, "jack_port_untie");
	jack_port_get_latency_fun = (jack_port_get_latency_fun_def)dlsym(gLibrary, "jack_port_get_latency");
    jack_port_get_total_latency_fun = (jack_port_get_total_latency_fun_def)dlsym(gLibrary, "jack_port_get_total_latency");
	jack_port_set_latency_fun = (jack_port_set_latency_fun_def)dlsym(gLibrary, "jack_port_set_latency");
	jack_recompute_total_latency_fun = (jack_recompute_total_latency_fun_def)dlsym(gLibrary, "jack_recompute_total_latency");
    jack_recompute_total_latencies_fun = (jack_recompute_total_latencies_fun_def)dlsym(gLibrary, "jack_recompute_total_latencies");
	jack_port_set_name_fun = (jack_port_set_name_fun_def)dlsym(gLibrary, "jack_port_set_name");
	jack_port_set_alias_fun = (jack_port_set_alias_fun_def)dlsym(gLibrary, "jack_port_set_alias");
	jack_port_unset_alias_fun = (jack_port_unset_alias_fun_def)dlsym(gLibrary, "jack_port_unset_alias");
	jack_port_get_aliases_fun = (jack_port_get_aliases_fun_def)dlsym(gLibrary, "jack_port_get_aliases");
	jack_port_request_monitor_fun = (jack_port_request_monitor_fun_def)dlsym(gLibrary, "jack_port_request_monitor");
    jack_port_request_monitor_by_name_fun = (jack_port_request_monitor_by_name_fun_def)dlsym(gLibrary, "jack_port_request_monitor_by_name");
    jack_port_ensure_monitor_fun = (jack_port_ensure_monitor_fun_def)dlsym(gLibrary, "jack_port_ensure_monitor");
    jack_port_monitoring_input_fun = (jack_port_monitoring_input_fun_def)dlsym(gLibrary, "jack_port_monitoring_input");
    jack_connect_fun = (jack_connect_fun_def)dlsym(gLibrary, "jack_connect");
    jack_disconnect_fun = (jack_disconnect_fun_def)dlsym(gLibrary, "jack_disconnect");
	jack_port_disconnect_fun = (jack_port_disconnect_fun_def)dlsym(gLibrary, "jack_port_disconnect");
	jack_port_name_size_fun = (jack_port_name_size_fun_def)dlsym(gLibrary, "jack_port_name_size");
	jack_port_type_size_fun = (jack_port_type_size_fun_def)dlsym(gLibrary, "jack_port_type_size");
    jack_get_sample_rate_fun = (jack_get_sample_rate_fun_def)dlsym(gLibrary, "jack_get_sample_rate");
    jack_get_buffer_size_fun = (jack_get_buffer_size_fun_def)dlsym(gLibrary, "jack_get_buffer_size");
    jack_get_ports_fun = (jack_get_ports_fun_def)dlsym(gLibrary, "jack_get_ports");
    jack_port_by_name_fun = (jack_port_by_name_fun_def)dlsym(gLibrary, "jack_port_by_name");
    jack_port_by_id_fun = (jack_port_by_id_fun_def)dlsym(gLibrary, "jack_port_by_id");
    jack_engine_takeover_timebase_fun = (jack_engine_takeover_timebase_fun_def)dlsym(gLibrary, "jack_engine_takeover_timebase");
    jack_frames_since_cycle_start_fun = (jack_frames_since_cycle_start_fun_def)dlsym(gLibrary, "jack_frames_since_cycle_start");
    jack_get_time_fun = (jack_get_time_fun_def)dlsym(gLibrary, "jack_get_time");
    jack_time_to_frames_fun = (jack_time_to_frames_fun_def)dlsym(gLibrary, "jack_time_to_frames");
	jack_frames_to_time_fun = (jack_frames_to_time_fun_def)dlsym(gLibrary, "jack_frames_to_time");
    jack_frame_time_fun = (jack_frame_time_fun_def)dlsym(gLibrary, "jack_frame_time");
    jack_last_frame_time_fun = (jack_last_frame_time_fun_def)dlsym(gLibrary, "jack_last_frame_time");
    jack_cpu_load_fun = (jack_cpu_load_fun_def)dlsym(gLibrary, "jack_cpu_load");
    jack_client_thread_id_fun = (jack_client_thread_id_fun_def)dlsym(gLibrary, "jack_client_thread_id");
	jack_set_error_function_fun = (jack_set_error_function_fun_def)dlsym(gLibrary, "jack_set_error_function");
	jack_set_info_function_fun = (jack_set_info_function_fun_def)dlsym(gLibrary, "jack_set_info_function");
	jack_get_max_delayed_usecs_fun = (jack_get_max_delayed_usecs_fun_def)dlsym(gLibrary, "jack_get_max_delayed_usecs");
    jack_get_xrun_delayed_usecs_fun = (jack_get_xrun_delayed_usecs_fun_def)dlsym(gLibrary, "jack_get_xrun_delayed_usecs");
    jack_reset_max_delayed_usecs_fun = (jack_reset_max_delayed_usecs_fun_def)dlsym(gLibrary, "jack_reset_max_delayed_usecs");
	jack_release_timebase_fun = (jack_release_timebase_fun_def)dlsym(gLibrary, "jack_release_timebase");
    jack_set_sync_callback_fun = (jack_set_sync_callback_fun_def)dlsym(gLibrary, "jack_set_sync_callback");
    jack_set_sync_timeout_fun = (jack_set_sync_timeout_fun_def)dlsym(gLibrary, "jack_set_sync_timeout");
    jack_set_timebase_callback_fun = (jack_set_timebase_callback_fun_def)dlsym(gLibrary, "jack_set_timebase_callback");
    jack_transport_locate_fun = (jack_transport_locate_fun_def)dlsym(gLibrary, "jack_transport_locate");
    jack_transport_query_fun = (jack_transport_query_fun_def)dlsym(gLibrary, "jack_transport_query");
    jack_get_current_transport_frame_fun = (jack_get_current_transport_frame_fun_def)dlsym(gLibrary, "jack_get_current_transport_frame");
    jack_transport_reposition_fun = (jack_transport_reposition_fun_def)dlsym(gLibrary, "jack_transport_reposition");
    jack_transport_start_fun = (jack_transport_start_fun_def)dlsym(gLibrary, "jack_transport_start");
    jack_transport_stop_fun = (jack_transport_stop_fun_def)dlsym(gLibrary, "jack_transport_stop");
    jack_get_transport_info_fun = (jack_get_transport_info_fun_def)dlsym(gLibrary, "jack_get_transport_info");
    jack_set_transport_info_fun = (jack_set_transport_info_fun_def)dlsym(gLibrary, "jack_set_transport_info");
    jack_acquire_real_time_scheduling_fun = (jack_acquire_real_time_scheduling_fun_def)dlsym(gLibrary, "jack_acquire_real_time_scheduling");
    jack_client_create_thread_fun = (jack_client_create_thread_fun_def)dlsym(gLibrary, "jack_client_create_thread");
    jack_drop_real_time_scheduling_fun = (jack_drop_real_time_scheduling_fun_def)dlsym(gLibrary, "jack_drop_real_time_scheduling");
    jack_get_internal_client_name_fun = (jack_get_internal_client_name_fun_def)dlsym(gLibrary, "jack_get_internal_client_name");
    jack_internal_client_handle_fun = (jack_internal_client_handle_fun_def)dlsym(gLibrary, "jack_internal_client_handle");
    jack_internal_client_load_aux_fun = (jack_internal_client_load_aux_fun_def)dlsym(gLibrary, "jack_internal_client_load_aux");
    jack_internal_client_unload_fun = (jack_internal_client_unload_fun_def)dlsym(gLibrary, "jack_internal_client_unload");
	
	// Functions were kept...
	if (error_fun)
		jack_set_error_function_fun(error_fun);
	if (info_fun)
		jack_set_info_function_fun(info_fun);

	jack_log("init_library OK");
    return true;

error:
    if (jackLibrary)
        dlclose(jackLibrary);
    if (jackmpLibrary)
        dlclose(jackmpLibrary);
	gLibrary = 0;
    return false;
}
