/*
Copyright (C) 2009 Grame

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

/*
    Completed from Julien Pommier (PianoTeq : http://www.pianoteq.com/) code.
*/

#include "jack.h"
#include <math.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <cassert>
#include <iostream>

/* dynamically load libjack and forward all registered calls to libjack 
   (similar to what relaytool is trying to do, but more portably..)
*/

using std::cerr;

int libjack_is_present = 0;     // public symbol, similar to what relaytool does.
static void *libjack_handle = 0;

static void __attribute__((constructor)) tryload_libjack()
{
    if (getenv("SKIP_LIBJACK") == 0) { // just in case libjack is causing troubles..
        libjack_handle = dlopen("libjack.so.0", RTLD_LAZY);
    }
    libjack_is_present = (libjack_handle != 0);
}

void *load_jack_function(const char *fn_name) 
{
    void *fn = 0;
    if (!libjack_handle) { 
        std::cerr << "libjack not found, so do not try to load " << fn_name << " ffs !\n";
        return 0;
    }
    fn = dlsym(libjack_handle, fn_name);
    if (!fn) { 
        std::cerr << "could not dlsym(" << libjack_handle << "), " << dlerror() << "\n"; 
    }
    return fn;
}

#define DECL_FUNCTION(return_type, fn_name, arguments_types, arguments) \
  typedef return_type (*fn_name##_ptr_t)arguments_types;                \
  return_type fn_name arguments_types {                                 \
    static fn_name##_ptr_t fn = 0;                                      \
    if (fn == 0) { fn = (fn_name##_ptr_t)load_jack_function(#fn_name); } \
    if (fn) return (*fn)arguments;                                      \
    else return 0;                                                      \
  }

#define DECL_VOID_FUNCTION(fn_name, arguments_types, arguments)         \
  typedef void (*fn_name##_ptr_t)arguments_types;                       \
  void fn_name arguments_types {                                        \
    static fn_name##_ptr_t fn = 0;                                      \
    if (fn == 0) { fn = (fn_name##_ptr_t)load_jack_function(#fn_name); } \
    if (fn) (*fn)arguments;                                             \
  }

DECL_VOID_FUNCTION(jack_get_version, (int *major_ptr, int *minor_ptr, int *micro_ptr, int *proto_ptr), (major_ptr, minor_ptr, micro_ptr, proto_ptr));
DECL_FUNCTION(const char *, jack_get_version_string, (), ());      
DECL_FUNCTION(jack_client_t *, jack_client_open, (const char *client_name, jack_options_t options, jack_status_t *status, ...), 
              (client_name, options, status));
DECL_FUNCTION(int, jack_client_close, (jack_client_t *client), (client));
DECL_FUNCTION(int, jack_client_new, (const char *client_name), (client_name));
DECL_FUNCTION(int, jack_client_name_size, (), ());
DECL_FUNCTION(char*, jack_get_client_name, (jack_client_t *client), (client));
DECL_FUNCTION(int, jack_internal_client_new, (const char *client_name,
                                            const char *load_name,
                                            const char *load_init), (client_name, load_name, load_init));
DECL_VOID_FUNCTION(jack_internal_client_close, (const char *client_name), (client_name));
DECL_FUNCTION(int, jack_is_realtime, (jack_client_t *client), (client));
DECL_VOID_FUNCTION(jack_on_shutdown, (jack_client_t *client, JackShutdownCallback shutdown_callback, void *arg), (client, function, arg));
DECL_VOID_FUNCTION(jack_on_info_shutdown, (jack_client_t* ext_client, JackInfoShutdownCallback callback, void* arg, (client, function, arg))
DECL_FUNCTION(int, jack_set_process_callback, (jack_client_t *client,
                                            JackProcessCallback process_callback,
                                            void *arg), (client, process_callback, arg));
DECL_FUNCTION(jack_nframes_t, jack_thread_wait, (jack_client_t *client, int status), (client, status));      
                                      
//
DECL_FUNCTION(jack_nframes_t, jack_cycle_wait, (jack_client_t *client), (client));   
DECL_VOID_FUNCTION(jack_cycle_signal, (jack_client_t *client, , int status), (client, status));                                          
DECL_FUNCTION(int, jack_set_process_thread, (jack_client_t *client,
                                            JackThreadCallback fun,
                                            void *arg), (client, fun, arg));
DECL_FUNCTION(int, jack_set_thread_init_callback, (jack_client_t *client,
                                            JackThreadInitCallback thread_init_callback,
                                            void *arg), (client, thread_init_callback, arg));
DECL_FUNCTION(int, jack_set_freewheel_callback, (jack_client_t *client,
                                            JackFreewheelCallback freewheel_callback,
                                            void *arg), (client, freewheel_callback, arg));
DECL_FUNCTION(int, jack_set_freewheel, (jack_client_t *client, int onoff), (client, onoff));   
DECL_FUNCTION(int, jack_set_buffer_size, (jack_client_t *client, jack_nframes_t nframes), (client, nframes));   
DECL_FUNCTION(int, jack_set_buffer_size_callback, (jack_client_t *client,
                                            JackBufferSizeCallback bufsize_callback,
                                            void *arg), (client, bufsize_callback, arg));
DECL_FUNCTION(int, jack_set_sample_rate_callback, (jack_client_t *client,
                                            JackSampleRateCallback srate_callback,
                                            void *arg), (client, srate_callback, arg));
DECL_FUNCTION(int, jack_set_client_registration_callback, (jack_client_t *client,
                                            JackClientRegistrationCallback registration_callback,
                                            void *arg), (client, registration_callback, arg));
DECL_FUNCTION(int, jack_set_port_registration_callback, (jack_client_t *client,
                                            JackPortRegistrationCallback registration_callback,
                                            void *arg), (client, registration_callback, arg));
DECL_FUNCTION(int, jack_set_port_connect_callback, (jack_client_t *client,
                                            JackPortConnectCallback connect_callback,
                                            void *arg), (client, connect_callback, arg));
DECL_FUNCTION(int, jack_set_port_rename_callback, (jack_client_t *client,
                                            JackPortRenameCallback rename_callback,
                                            void *arg), (client, rename_callback, arg));
DECL_FUNCTION(int, jack_set_graph_order_callback, (jack_client_t *client,
                                            JackGraphOrderCallback graph_callback,
                                            void *arg), (client, graph_callback, arg));
DECL_FUNCTION(int, jack_set_xrun_callback, (jack_client_t *client,
                                            JackXRunCallback xrun_callback,
                                            void *arg), (client, xrun_callback, arg));
DECL_FUNCTION(int, jack_activate, (jack_client_t *client), (client));
DECL_FUNCTION(int, jack_deactivate, (jack_client_t *client), (client));
DECL_FUNCTION(jack_port_t *, jack_port_register, (jack_client_t *client, const char *port_name, const char *port_type,
                                                  unsigned long flags, unsigned long buffer_size),
              (client, port_name, port_type, flags, buffer_size));
DECL_FUNCTION(int, jack_port_unregister, (jack_client_t *client, jack_port_t* port), (client, port));
DECL_FUNCTION(void *, jack_port_get_buffer, (jack_port_t *port, jack_nframes_t nframes), (port, nframes));
DECL_FUNCTION(const char*, jack_port_name, (const jack_port_t *port), (port));
DECL_FUNCTION(const char*, jack_port_short_name, (const jack_port_t *port), (port));
DECL_FUNCTION(int, jack_port_flags, (const jack_port_t *port), (port));
DECL_FUNCTION(const char*, jack_port_type, (const jack_port_t *port), (port));
DECL_FUNCTION(jack_port_type_id_t, jack_port_type_id, (const jack_port_t *port), (port));
DECL_FUNCTION(int, jack_port_is_mine, (const jack_client_t *client, const jack_port_t* port), (client, port));
DECL_FUNCTION(int, jack_port_connected, (const jack_port_t *port), (port));
DECL_FUNCTION(int, jack_port_connected_to, (const jack_port_t *port, const char *port_name), (port, port_name));
DECL_FUNCTION(const char**, jack_port_get_connections, (const jack_port_t *port), (port));
DECL_FUNCTION(const char**, jack_port_get_all_connections, (const jack_port_t *port), (port));
DECL_FUNCTION(int, jack_port_tie, (jack_port_t *src, jack_port_t *dst), (src, dst));
DECL_FUNCTION(int, jack_port_untie, (jack_port_t *port), (port));
DECL_FUNCTION(jack_nframes_t, jack_port_get_latency, (jack_port_t *port));
DECL_FUNCTION(jack_nframes_t, jack_port_get_total_latency ,(jack_client_t *), (jack_port_t *port));
DECL_VOID_FUNCTION(jack_port_set_latency, (jack_port_t *), (jack_nframes_t));
DECL_FUNCTION(int, jack_recompute_total_latency, (jack_client_t*), (jack_port_t* port));
DECL_FUNCTION(int, jack_recompute_total_latencies, (jack_client_t*));

DECL_FUNCTION(int, jack_port_set_name, (jack_port_t *port), (const char *port_name));
DECL_FUNCTION(int, jack_port_set_alias, (jack_port_t *port), (const char *alias));
DECL_FUNCTION(int, jack_port_unset_alias, (jack_port_t *port), (const char *alias));
DECL_FUNCTION(int, jack_port_get_aliases, (const jack_port_t *port), (char* const aliases[2]));
DECL_FUNCTION(int, jack_port_request_monitor, (jack_port_t *port), (int onoff));
DECL_FUNCTION(int, jack_port_request_monitor_by_name, (jack_client_t *client), (const char *port_name), (int onoff));
DECL_FUNCTION(int, jack_port_ensure_monitor, (jack_port_t *port), (int onoff));
DECL_FUNCTION(int, jack_port_monitoring_input, (jack_port_t *port));
DECL_FUNCTION(int, jack_connect, (jack_client_t *), (const char *source_port), (const char *destination_port));
DECL_FUNCTION(int, jack_disconnect, (jack_client_t *), (const char *source_port), (const char *destination_port));
DECL_FUNCTION(int, jack_port_disconnect, (jack_client_t *), (jack_port_t *));
DECL_FUNCTION(int, jack_port_name_size,(void));
DECL_FUNCTION(int, jack_port_type_size,(void));
            
DECL_FUNCTION(jack_nframes_t, jack_get_sample_rate, (jack_client_t *client), (client));
DECL_FUNCTION(jack_nframes_t, jack_get_buffer_size, (jack_client_t *client), (client));
DECL_FUNCTION(const char**, jack_get_ports, (jack_client_t *client, const char *port_name_pattern, const char *	type_name_pattern,
                                             unsigned long flags), (client, port_name_pattern, type_name_pattern, flags));
DECL_FUNCTION(jack_port_t *, jack_port_by_name, (jack_client_t *), (const char *port_name));
DECL_FUNCTION(jack_port_t *, jack_port_by_id, (jack_client_t *client), (jack_port_id_t port_id));

DECL_FUNCTION(int, jack_engine_takeover_timebase, (jack_client_t *));
DECL_FUNCTION(jack_nframes_t, jack_frames_since_cycle_start, (const jack_client_t *));
DECL_FUNCTION(jack_time_t, jack_get_time());
DECL_FUNCTION(jack_nframes_t, jack_time_to_frames, (const jack_client_t *client), (jack_time_t time));
DECL_FUNCTION(jack_time_t, jack_frames_to_time, (const jack_client_t *client), (jack_nframes_t frames));
DECL_FUNCTION(jack_nframes_t, jack_frame_time, (const jack_client_t *));
DECL_FUNCTION(jack_nframes_t, jack_last_frame_time, (const jack_client_t *client));
DECL_FUNCTION(float, jack_cpu_load, (jack_client_t *client));
DECL_FUNCTION(pthread_t, jack_client_thread_id, (jack_client_t *));
DECL_VOID_FUNCTION(jack_set_error_function, (print_function));
DECL_VOID_FUNCTION(jack_set_info_function, (print_function));

DECL_FUNCTION(float, jack_get_max_delayed_usecs, (jack_client_t *client));
DECL_FUNCTION(float, jack_get_xrun_delayed_usecs, (jack_client_t *client));
DECL_VOID_FUNCTION(jack_reset_max_delayed_usecs, (jack_client_t *client));

DECL_FUNCTION(int, jack_release_timebase, (jack_client_t *client));
DECL_FUNCTION(int, jack_set_sync_callback, (jack_client_t *client, (JackSyncCallback sync_callback), (void *arg));
DECL_FUNCTION(int, jack_set_sync_timeout, (jack_client_t *client), (jack_time_t timeout));
DECL_FUNCTION(int, jack_set_timebase_callback, (jack_client_t *client), (int conditional), (JackTimebaseCallback timebase_callback), (void *arg));
DECL_FUNCTION(int, jack_transport_locate, (jack_client_t *client), (jack_nframes_t frame));
DECL_FUNCTION(jack_transport_state_t, jack_transport_query, (const jack_client_t *client), (jack_position_t *pos));
DECL_FUNCTION(jack_nframes_t, jack_get_current_transport_frame, (const jack_client_t *client));
DECL_FUNCTION(int, jack_transport_reposition, (jack_client_t *client), (jack_position_t *pos));
DECL_VOID_FUNCTION(jack_transport_start, (jack_client_t *client));
DECL_VOID_FUNCTION(jack_transport_stop, (jack_client_t *client));
DECL_VOID_FUNCTION(jack_get_transport_info, (jack_client_t *client), (jack_transport_info_t *tinfo));
DECL_VOID_FUNCTION(jack_set_transport_info, (jack_client_t *client), (jack_transport_info_t *tinfo));

DECL_FUNCTION(int, jack_client_real_time_priority, (jack_client_t*));
DECL_FUNCTION(int, jack_client_max_real_time_priority, (jack_client_t*));
DECL_FUNCTION(int, jack_acquire_real_time_scheduling, (pthread_t thread), (int priority));
DECL_FUNCTION(int, jack_client_create_thread, (jack_client_t* client),
                                      (pthread_t *thread),
                                      (int priority),
                                      (int realtime), 	// boolean
                                      (thread_routine routine),
                                      (void *arg));
DECL_FUNCTION(int, jack_drop_real_time_scheduling, (pthread_t thread));

DECL_FUNCTION(int, jack_client_stop_thread, (jack_client_t* client), (pthread_t thread));
DECL_FUNCTION(int, jack_client_kill_thread, (jack_client_t* client), (pthread_t thread));
#ifndef WIN32
DECL_VOID_FUNCTION(jack_set_thread_creator, (jack_thread_creator_t jtc));
#endif
DECL_FUNCTION(char *, jack_get_internal_client_name, (jack_client_t *client, (jack_intclient_t intclient));
DECL_FUNCTION(jack_intclient_t, jack_internal_client_handle, (jack_client_t *client), (const char *client_name), (jack_status_t *status));
DECL_FUNCTION(jack_intclient_t, jack_internal_client_load, (jack_client_t *client), (const char *client_name), (jack_options_t options), (jack_status_t *status), ...));

DECL_FUNCTION(jack_status_t, jack_internal_client_unload, (jack_client_t *client), jack_intclient_t intclient));
DECL_VOID_FUNCTION(jack_free, (void* ptr));

// MIDI

DECL_FUNCTION(jack_nframes_t, jack_midi_get_event_count, (void* port_buffer));
DECL_FUNCTION(int jack_midi_event_get(jack_midi_event_t* event, void* port_buffer, jack_nframes_t event_index);
DECL_VOID_FUNCTION(jack_midi_clear_buffer, (void* port_buffer));
DECL_FUNCTION(size_t, jack_midi_max_event_size, (void* port_buffer));
DECL_FUNCTION(jack_midi_data_t*, jack_midi_event_reserve, (void* port_buffer), (jack_nframes_t time), (size_t data_size));
DECL_FUNCTIO(int jack_midi_event_write, (void* port_buffer), (jack_nframes_t time), (const jack_midi_data_t* data), (size_t data_size));
DECL_FUNCTION(jack_nframes_t, jack_midi_get_lost_event_count, (void* port_buffer));
