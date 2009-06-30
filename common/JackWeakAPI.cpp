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


DECL_FUNCTION(jack_nframes_t, jack_get_buffer_size, (jack_client_t *client), (client));
DECL_FUNCTION(jack_nframes_t, jack_get_sample_rate, (jack_client_t *client), (client));
DECL_FUNCTION(jack_nframes_t, jack_port_get_total_latency, (jack_client_t *client, jack_port_t *port), (client, port));
DECL_VOID_FUNCTION(jack_set_error_function, (void (*func)(const char *)), (func));


DECL_FUNCTION(const char**, jack_get_ports, (jack_client_t *client, const char *port_name_pattern, const char *	type_name_pattern,
                                             unsigned long flags), (client, port_name_pattern, type_name_pattern, flags));
DECL_FUNCTION(int, jack_connect, (jack_client_t *client, const char *source_port, const char *destination_port), (client, source_port, destination_port));
DECL_FUNCTION(int, jack_set_port_connect_callback, (jack_client_t *client, JackPortConnectCallback connect_callback, void *arg),
              (client, connect_callback, arg));
DECL_FUNCTION(jack_port_t *, jack_port_by_id, (jack_client_t *client, jack_port_id_t port_id), (client, port_id));
