/*
Copyright (C) 2001-2003 Paul Davis
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

#include "JackClient.h"
#include "JackError.h"
#include "JackGraphManager.h"
#include "JackEngineControl.h"
#include "JackClientControl.h"
#include "JackGlobals.h"
#include "JackTime.h"
#include "JackExports.h"
#ifdef __APPLE__
	#include "JackMachThread.h"
#elif WIN32
	#include "JackWinThread.h"
#else
	#include "JackPosixThread.h"
#endif
#include <math.h>

#ifdef __CLIENTDEBUG__
	#include "JackLibGlobals.h"
#endif

using namespace Jack;

#ifdef __cplusplus
extern "C"
{
#endif

    EXPORT int jack_client_name_size (void);
    EXPORT char* jack_get_client_name (jack_client_t *client);
    EXPORT int jack_internal_client_new (const char *client_name,
                                         const char *load_name,
                                         const char *load_init);
    EXPORT jack_client_t* my_jack_internal_client_new(const char* client_name);
    EXPORT void jack_internal_client_close (const char *client_name);
    EXPORT void my_jack_internal_client_close (jack_client_t* client);
    EXPORT int jack_is_realtime (jack_client_t *client);
    EXPORT void jack_on_shutdown (jack_client_t *client,
                                  void (*function)(void *arg), void *arg);
    EXPORT int jack_set_process_callback (jack_client_t *client,
                                          JackProcessCallback process_callback,
                                          void *arg);
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
    EXPORT int jack_set_port_registration_callback (jack_client_t *,
            JackPortRegistrationCallback
            registration_callback, void *arg);
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
    EXPORT int jack_port_lock (jack_client_t *, jack_port_t *);
    EXPORT int jack_port_unlock (jack_client_t *, jack_port_t *);
    EXPORT jack_nframes_t jack_port_get_latency (jack_port_t *port);
    EXPORT jack_nframes_t jack_port_get_total_latency (jack_client_t *,
            jack_port_t *port);
    EXPORT void jack_port_set_latency (jack_port_t *, jack_nframes_t);
    EXPORT int jack_recompute_total_latencies (jack_client_t*);
    EXPORT int jack_port_set_name (jack_port_t *port, const char *port_name);
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
    EXPORT jack_nframes_t jack_frame_time (const jack_client_t *);
    EXPORT jack_nframes_t jack_last_frame_time (const jack_client_t *client);
    EXPORT float jack_cpu_load (jack_client_t *client);
    EXPORT pthread_t jack_client_thread_id (jack_client_t *);
    EXPORT void jack_set_error_function (void (*func)(const char *));

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
			
#ifdef __cplusplus
}
#endif

#ifdef WIN32 
/* missing on Windows : see http://bugs.mysql.com/bug.php?id=15936 */
inline double rint(double nr)
{
    double f = floor(nr);
    double c = ceil(nr);
    return (((c -nr) >= (nr - f)) ? f : c);
}
#endif

static inline bool CheckPort(jack_port_id_t port_index)
{
    return (port_index < PORT_NUM);
}

static inline bool CheckBufferSize(jack_nframes_t buffer_size)
{
    return (buffer_size <= BUFFER_SIZE_MAX);
}

static inline void WaitGraphChange()
{
    if (GetGraphManager()->IsPendingChange()) {
        JackLog("WaitGraphChange...\n");
        JackSleep(GetEngineControl()->fPeriodUsecs * 2);
    }
}

static void default_jack_error_callback(const char *desc)
{
    fprintf(stderr, "%s\n", desc);
}

void (*jack_error_callback)(const char *desc) = &default_jack_error_callback;

EXPORT void jack_set_error_function (void (*func)(const char *))
{
    jack_error_callback = func;
}

EXPORT void* jack_port_get_buffer(jack_port_t* port, jack_nframes_t frames)
{
#ifdef __CLIENTDEBUG__
	JackLibGlobals::CheckContext();
#endif
    jack_port_id_t myport = (jack_port_id_t)port;
    if (!CheckPort(myport)) {
        jack_error("jack_port_get_buffer called with an incorrect port %ld", myport);
        return NULL;
    } else {
        return GetGraphManager()->GetBuffer(myport, frames);
    }
}

EXPORT const char* jack_port_name(const jack_port_t* port)
{
#ifdef __CLIENTDEBUG__
	JackLibGlobals::CheckContext();
#endif
    jack_port_id_t myport = (jack_port_id_t)port;
    if (!CheckPort(myport)) {
        jack_error("jack_port_name called with an incorrect port %ld", myport);
        return NULL;
    } else {
        return GetGraphManager()->GetPort(myport)->GetName();
    }
}

EXPORT const char* jack_port_short_name(const jack_port_t* port)
{
#ifdef __CLIENTDEBUG__
	JackLibGlobals::CheckContext();
#endif
    jack_port_id_t myport = (jack_port_id_t)port;
    if (!CheckPort(myport)) {
        jack_error("jack_port_short_name called with an incorrect port %ld", myport);
        return NULL;
    } else {
        return GetGraphManager()->GetPort(myport)->GetShortName();
    }
}

EXPORT int jack_port_flags(const jack_port_t* port)
{
#ifdef __CLIENTDEBUG__
	JackLibGlobals::CheckContext();
#endif
    jack_port_id_t myport = (jack_port_id_t)port;
    if (!CheckPort(myport)) {
        jack_error("jack_port_flags called with an incorrect port %ld", myport);
        return -1;
    } else {
        return GetGraphManager()->GetPort(myport)->Flags();
    }
}

EXPORT const char* jack_port_type(const jack_port_t* port)
{
#ifdef __CLIENTDEBUG__
	JackLibGlobals::CheckContext();
#endif
    jack_port_id_t myport = (jack_port_id_t)port;
    if (!CheckPort(myport)) {
        jack_error("jack_port_flags called an incorrect port %ld", myport);
        return NULL;
    } else {
        return GetGraphManager()->GetPort(myport)->Type();
    }
}

EXPORT int jack_port_connected(const jack_port_t* port)
{
#ifdef __CLIENTDEBUG__
	JackLibGlobals::CheckContext();
#endif
    jack_port_id_t myport = (jack_port_id_t)port;
    if (!CheckPort(myport)) {
        jack_error("jack_port_connected called with an incorrect port %ld", myport);
        return -1;
    } else {
        WaitGraphChange();
        return GetGraphManager()->GetConnectionsNum(myport);
    }
}

EXPORT int jack_port_connected_to(const jack_port_t* port, const char* portname)
{
#ifdef __CLIENTDEBUG__
	JackLibGlobals::CheckContext();
#endif
    jack_port_id_t myport = (jack_port_id_t)port;
    if (!CheckPort(myport)) {
        jack_error("jack_port_connected_to called with an incorrect port %ld", myport);
        return -1;
    } else if (portname == NULL) {
        jack_error("jack_port_connected_to called with a NULL port name");
        return -1;
    } else {
        WaitGraphChange();
        return GetGraphManager()->ConnectedTo(myport, portname);
    }
}

EXPORT int jack_port_tie(jack_port_t* src, jack_port_t* dst)
{
#ifdef __CLIENTDEBUG__
	JackLibGlobals::CheckContext();
#endif
    jack_port_id_t mysrc = (jack_port_id_t)src;
    if (!CheckPort(mysrc)) {
        jack_error("jack_port_tie called with a NULL src port");
        return -1;
    }
    jack_port_id_t mydst = (jack_port_id_t)dst;
    if (!CheckPort(mydst)) {
        jack_error("jack_port_tie called with a NULL dst port");
        return -1;
    }
    if (GetGraphManager()->GetPort(mysrc)->GetRefNum() != GetGraphManager()->GetPort(mydst)->GetRefNum()) {
        jack_error("jack_port_tie called with ports not belonging to the same client");
        return -1;
    }
    return GetGraphManager()->GetPort(mydst)->Tie(mysrc);
}

EXPORT int jack_port_untie(jack_port_t* port)
{
#ifdef __CLIENTDEBUG__
	JackLibGlobals::CheckContext();
#endif
    jack_port_id_t myport = (jack_port_id_t)port;
    if (!CheckPort(myport)) {
        jack_error("jack_port_untie called with an incorrect port %ld", myport);
        return -1;
    } else {
        return GetGraphManager()->GetPort(myport)->UnTie();
    }
}

EXPORT jack_nframes_t jack_port_get_latency(jack_port_t* port)
{
#ifdef __CLIENTDEBUG__
	JackLibGlobals::CheckContext();
#endif
    jack_port_id_t myport = (jack_port_id_t)port;
    if (!CheckPort(myport)) {
        jack_error("jack_port_get_latency called with an incorrect port %ld", myport);
        return 0;
    } else {
        WaitGraphChange();
        return GetGraphManager()->GetPort(myport)->GetLatency();
    }
}

EXPORT void jack_port_set_latency(jack_port_t* port, jack_nframes_t frames)
{
#ifdef __CLIENTDEBUG__
	JackLibGlobals::CheckContext();
#endif
    jack_port_id_t myport = (jack_port_id_t)port;
    if (!CheckPort(myport)) {
        jack_error("jack_port_set_latency called with an incorrect port %ld", myport);
    } else {
        GetGraphManager()->GetPort(myport)->SetLatency(frames);
    }
}

EXPORT int jack_recompute_total_latencies(jack_client_t* ext_client)
{
    // The latency computation is done each time jack_port_get_total_latency is called
    return 0;
}

EXPORT int jack_port_set_name(jack_port_t* port, const char* name)
{
#ifdef __CLIENTDEBUG__
	JackLibGlobals::CheckContext();
#endif
    jack_port_id_t myport = (jack_port_id_t)port;
    if (!CheckPort(myport)) {
        jack_error("jack_port_set_name called with an incorrect port %ld", myport);
        return -1;
    } else if (name == NULL) {
        jack_error("jack_port_set_name called with a NULL port name");
        return -1;
    } else {
        return GetGraphManager()->GetPort(myport)->SetName(name);
    }
}

EXPORT int jack_port_request_monitor(jack_port_t* port, int onoff)
{
#ifdef __CLIENTDEBUG__
	JackLibGlobals::CheckContext();
#endif
    jack_port_id_t myport = (jack_port_id_t)port;
    if (!CheckPort(myport)) {
        jack_error("jack_port_request_monitor called with an incorrect port %ld", myport);
        return -1;
    } else {
        return GetGraphManager()->RequestMonitor(myport, onoff);
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
        jack_port_id_t myport = GetGraphManager()->GetPort(port_name);
        if (!CheckPort(myport)) {
            jack_error("jack_port_request_monitor_by_name called with an incorrect port %s", port_name);
            return -1;
        } else {
            return GetGraphManager()->RequestMonitor(myport, onoff);
        }
    }
}

EXPORT int jack_port_ensure_monitor(jack_port_t* port, int onoff)
{
#ifdef __CLIENTDEBUG__
	JackLibGlobals::CheckContext();
#endif
    jack_port_id_t myport = (jack_port_id_t)port;
    if (!CheckPort(myport)) {
        jack_error("jack_port_ensure_monitor called with an incorrect port %ld", myport);
        return -1;
    } else {
        return GetGraphManager()->GetPort(myport)->EnsureMonitor(onoff);
    }
}

EXPORT int jack_port_monitoring_input(jack_port_t* port)
{
#ifdef __CLIENTDEBUG__
	JackLibGlobals::CheckContext();
#endif
    jack_port_id_t myport = (jack_port_id_t)port;
    if (!CheckPort(myport)) {
        jack_error("jack_port_monitoring_input called with an incorrect port %ld", myport);
        return -1;
    } else {
        return GetGraphManager()->GetPort(myport)->MonitoringInput();
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
        return GetEngineControl()->fRealTime;
    }
}

EXPORT void jack_on_shutdown(jack_client_t* ext_client, void (*function)(void* arg), void* arg)
{
#ifdef __CLIENTDEBUG__
	JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_on_shutdown called with a NULL client");
    } else {
        client->OnShutdown(function, arg);
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
    JackLog("jack_set_sample_rate_callback ext_client %x client %x \n", ext_client, client);
    if (client == NULL) {
        jack_error("jack_set_sample_rate_callback called with a NULL client");
        return -1;
    } else {
        JackLog("jack_set_sample_rate_callback: deprecated\n");
        return -1;
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

EXPORT int jack_set_graph_order_callback(jack_client_t* ext_client, JackGraphOrderCallback graph_callback, void* arg)
{
#ifdef __CLIENTDEBUG__
	JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    JackLog("jack_set_graph_order_callback ext_client %x client %x \n", ext_client, client);
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
    JackLog("jack_set_thread_init_callback ext_client %x client %x \n", ext_client, client);
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
        return (jack_port_t *)client->PortRegister(port_name, port_type, flags, buffer_size);
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
    jack_port_id_t myport = (jack_port_id_t)port;
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
    jack_port_id_t myport = (jack_port_id_t)port;
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
    jack_port_id_t myport = (jack_port_id_t)port;
    if (!CheckPort(myport)) {
        jack_error("jack_port_get_connections called with an incorrect port %ld", myport);
        return NULL;
    } else {
        WaitGraphChange();
        return GetGraphManager()->GetConnections(myport);
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

    jack_port_id_t myport = (jack_port_id_t)port;
    if (!CheckPort(myport)) {
        jack_error("jack_port_get_all_connections called with an incorrect port %ld", myport);
        return NULL;
    } else {
        WaitGraphChange();
        return GetGraphManager()->GetConnections(myport);
    }
}

// Does not use the client parameter
EXPORT int jack_port_lock(jack_client_t* ext_client, jack_port_t* port)
{
#ifdef __CLIENTDEBUG__
	JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_port_lock called with a NULL client");
        return -1;
    }

    jack_port_id_t myport = (jack_port_id_t)port;
    if (!CheckPort(myport)) {
        jack_error("jack_port_lock called with an incorrect port %ld", myport);
        return -1;
    } else {
        return (myport && client->PortIsMine(myport)) ? GetGraphManager()->GetPort(myport)->Lock() : -1;
    }
}

// Does not use the client parameter
EXPORT int jack_port_unlock(jack_client_t* ext_client, jack_port_t* port)
{
#ifdef __CLIENTDEBUG__
	JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_port_unlock called with a NULL client");
        return -1;
    }

    jack_port_id_t myport = (jack_port_id_t)port;
    if (!CheckPort(myport)) {
        jack_error("jack_port_unlock called with an incorrect port %ld", myport);
        return -1;
    } else {
        return (myport && client->PortIsMine(myport)) ? GetGraphManager()->GetPort(myport)->Unlock() : -1;
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

    jack_port_id_t myport = (jack_port_id_t)port;
    if (!CheckPort(myport)) {
        jack_error("jack_port_get_total_latency called with an incorrect port %ld", myport);
        return 0;
    } else {
        // The latency computation is done each time
        WaitGraphChange();
        return GetGraphManager()->GetTotalLatency(myport);
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

EXPORT int jack_port_connect(jack_client_t* ext_client, jack_port_t* src, jack_port_t* dst)
{
#ifdef __CLIENTDEBUG__
	JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_port_connect called with a NULL client");
        return -1;
    }
    jack_port_id_t mysrc = (jack_port_id_t)src;
    if (!CheckPort(mysrc)) {
        jack_error("jack_port_connect called with a NULL src port");
        return -1;
    }
    jack_port_id_t mydst = (jack_port_id_t)dst;
    if (!CheckPort(mydst)) {
        jack_error("jack_port_connect called with a NULL dst port");
        return -1;
    }
    return client->PortConnect(mysrc, mydst);
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
    jack_port_id_t myport = (jack_port_id_t)src;
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
        return GetEngineControl()->fSampleRate;
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
        return GetEngineControl()->fBufferSize;
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
    return GetGraphManager()->GetPorts(port_name_pattern, type_name_pattern, flags);
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
        int res = GetGraphManager()->GetPort(portname); // returns a port index at least > 1
        return (res == NO_PORT) ? NULL : (jack_port_t*)res;
    }
}

EXPORT jack_port_t* jack_port_by_id(const jack_client_t* ext_client, jack_port_id_t id)
{
#ifdef __CLIENTDEBUG__
	JackLibGlobals::CheckContext();
#endif
    /* jack_port_t* type is actually the port index */
    return (jack_port_t*)id;
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
        jack_error("jack_engine_takeover_timebase : not yet implemented\n");
        return 0;
    }
}

EXPORT jack_nframes_t jack_frames_since_cycle_start(const jack_client_t* ext_client)
{
#ifdef __CLIENTDEBUG__
	JackLibGlobals::CheckContext();
#endif
    JackTimer timer;
	GetEngineControl()->ReadFrameTime(&timer);
    return (jack_nframes_t) floor((((float)GetEngineControl()->fSampleRate) / 1000000.0f) * (GetMicroSeconds() - timer.fCurrentCallback));
}

EXPORT jack_nframes_t jack_frame_time(const jack_client_t* ext_client)
{
#ifdef __CLIENTDEBUG__
	JackLibGlobals::CheckContext();
#endif
    JackClient* client = (JackClient*)ext_client;
    if (client == NULL) {
        jack_error("jack_frame_time called with a NULL client");
        return 0;
    } else {
        JackTimer timer;
  		GetEngineControl()->ReadFrameTime(&timer);
        if (timer.fInitialized) {
            return timer.fFrames +
                   (long) rint(((double) ((jack_time_t)(GetMicroSeconds() - timer.fCurrentWakeup)) /
                                ((jack_time_t)(timer.fNextWakeUp - timer.fCurrentWakeup))) * GetEngineControl()->fBufferSize);
        } else {
            return 0;
        }
    }
}

EXPORT jack_nframes_t jack_last_frame_time(const jack_client_t* ext_client)
{
#ifdef __CLIENTDEBUG__
	JackLibGlobals::CheckContext();
#endif
    JackTimer timer;
	GetEngineControl()->ReadFrameTime(&timer);
    return timer.fFrames;
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
        return GetEngineControl()->fCPULoad;
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

EXPORT char* jack_get_client_name (jack_client_t* ext_client)
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
        return client->TransportLocate(frame);
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
        return client->TransportReposition(pos);
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
    JackLog("jack_get_transport_info : deprecated");
    if (tinfo)
        memset(tinfo, 0, sizeof(jack_transport_info_t));
}

EXPORT void jack_set_transport_info(jack_client_t* ext_client, jack_transport_info_t* tinfo)
{
    JackLog("jack_set_transport_info : deprecated");
    if (tinfo)
        memset(tinfo, 0, sizeof(jack_transport_info_t));
}

// statistics.h

EXPORT float jack_get_max_delayed_usecs(jack_client_t* ext_client)
{
#ifdef __CLIENTDEBUG__
	JackLibGlobals::CheckContext();
#endif
    JackLog("jack_get_max_delayed_usecs: not yet implemented\n");
    return 0.f;
}

EXPORT float jack_get_xrun_delayed_usecs(jack_client_t* ext_client)
{
#ifdef __CLIENTDEBUG__
	JackLibGlobals::CheckContext();
#endif
    JackLog("jack_get_xrun_delayed_usecs: not yet implemented\n");
    return 0.f;
}

EXPORT void jack_reset_max_delayed_usecs(jack_client_t* ext_client)
{
#ifdef __CLIENTDEBUG__
	JackLibGlobals::CheckContext();
#endif
    JackLog("jack_reset_max_delayed_usecs: not yet implemented\n");
}

// thread.h

EXPORT int jack_acquire_real_time_scheduling(pthread_t thread, int priority)
{
	#ifdef __APPLE__
		return JackMachThread::AcquireRealTimeImp(thread, 0, 500 * 1000, 500 * 1000);
	#elif WIN32
		return JackWinThread::AcquireRealTimeImp(thread, priority);
	#else
		return JackPosixThread::AcquireRealTimeImp(thread, priority);
	#endif	
}

EXPORT int jack_client_create_thread(jack_client_t* client,
                                     pthread_t *thread,
                                     int priority,
                                     int realtime,  	/* boolean */
                                     void *(*start_routine)(void*),
                                     void *arg)
{
	#ifdef __APPLE__
		return JackPosixThread::StartImp(thread, priority, realtime, start_routine, arg);
	#elif WIN32
		return JackWinThread::StartImp(thread, priority, realtime, (ThreadCallback)start_routine, arg);
	#else
		return JackPosixThread::StartImp(thread, priority, realtime, start_routine, arg);
	#endif
}

EXPORT int jack_drop_real_time_scheduling(pthread_t thread)
{
	#ifdef __APPLE__
		return JackMachThread::DropRealTimeImp(thread);
	#elif WIN32
		return JackWinThread::DropRealTimeImp(thread);
	#else
		return JackPosixThread::DropRealTimeImp(thread);
	#endif
}

// intclient.h

EXPORT char* jack_get_internal_client_name(jack_client_t* ext_client, jack_intclient_t intclient)
{
    JackLog("jack_get_internal_client_name: not yet implemented\n");
    return "";
}

EXPORT jack_intclient_t jack_internal_client_handle(jack_client_t* ext_client, const char* client_name, jack_status_t* status)
{
    JackLog("jack_internal_client_handle: not yet implemented\n");
    return 0;
}

EXPORT jack_intclient_t jack_internal_client_load(jack_client_t* ext_client, const char* client_name, jack_options_t options, jack_status_t* status, ...)
{
    JackLog("jack_internal_client_load: not yet implemented\n");
    return 0;
}

EXPORT jack_status_t jack_internal_client_unload(jack_client_t* ext_client, jack_intclient_t intclient)
{
    JackLog("jack_internal_client_unload: not yet implemented\n");
    return JackFailure;
}
