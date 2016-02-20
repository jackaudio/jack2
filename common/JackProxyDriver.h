/*
 Copyright (C) 2014 Cédric Schieli

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#ifndef __JackProxyDriver__
#define __JackProxyDriver__

#include "JackTimedDriver.h"

#define DEFAULT_UPSTREAM "default" /*!< Default upstream Jack server to connect to */
#define DEFAULT_CLIENT_NAME "proxy" /*!< Default client name to use when connecting to upstream Jack server */

#ifdef __APPLE__
    #define JACK_PROXY_CLIENT_LIB "libjack.0.dylib"
#elif defined(WIN32)
    #ifdef _WIN64
        #define JACK_PROXY_CLIENT_LIB "libjack64.dll"
    #else
        #define JACK_PROXY_CLIENT_LIB "libjack.dll"
    #endif
#else
    #define JACK_PROXY_CLIENT_LIB "libjack.so.0"
#endif

#define PROXY_DEF_SYMBOL(ret,name,...) ret (*name) (__VA_ARGS__)
#define PROXY_LOAD_SYMBOL(ret,name,...) name = (ret (*) (__VA_ARGS__)) GetJackProc(fHandle, #name); assert(name)

namespace Jack
{
    /*! \Brief This class describes the Proxy Backend

    It uses plain Jack API to connect to an upstream server. The latter is
    either running as the same user, or is running in promiscuous mode.

    The main use case is the multi-user, multi-session, shared workstation:

    - a classic server with hw driver is launched system-wide at boot time, in
      promiscuous mode, optionaly restricted to the audio group
    - in each user session, a jackdbus server is automatically started with
      JackProxyDriver as master driver, automatically connected to the
      system-wide one
    - optionaly, each user run PulseAudio with a pulse-jack bridge
    */

    class JackProxyDriver : public JackRestarterDriver
    {

        private:

            char fUpstream[JACK_CLIENT_NAME_SIZE+1];    /*<! the upstream server name */
            char fClientName[JACK_CLIENT_NAME_SIZE+1];  /*<! client name to use when connecting */
            const char* fPromiscuous;                   /*<! if not null, group or gid to use for promiscuous mode */

            //jack data
            jack_client_t* fClient;                  /*<! client handle */
            jack_port_t** fUpstreamCapturePorts;     /*<! ports registered for capture in the upstream server */
            jack_port_t** fUpstreamPlaybackPorts;    /*<! ports registered for playback in the upstream server */
            int* fUpstreamCapturePortConnected;      /*<! map of capture ports connected upstream, for optimization purpose */
            int* fUpstreamPlaybackPortConnected;     /*<! map of playback ports connected upstream, for optimization purpose */

            bool fAutoSave;                          /*<! wether the local connections should be saved/restored when upstream connection is restarted */
            bool fAutoConnect;                       /*<! wether the upstream ports should be automatically connected to upstream physical ports */
            bool fDetectPlaybackChannels;            /*<! wether the number of playback ports registered should match the number of upstream physical playback ports */
            bool fDetectCaptureChannels;             /*<! wether the number of capture ports registered should match the number of upstream physical capture ports */

            bool Initialize();                       /*<! establish upstream connection and register the client callbacks */

            int AllocPorts();                        /*<! register local and upstream ports */
            int FreePorts();                         /*<! unregister local ports */
            void ConnectPorts();                     /*<! connect upstream ports to physical ones */

            int CountIO(const char*, int);           /*<! get the number of upstream ports of a specific type */

            // client callbacks
            static int process_callback(jack_nframes_t, void*);
            static int bufsize_callback(jack_nframes_t, void*);
            static int srate_callback(jack_nframes_t, void*);
            static void connect_callback(jack_port_id_t, jack_port_id_t, int, void*);
            static void shutdown_callback(void*);

            // indirect member callbacks
            int bufsize_callback(jack_nframes_t);
            int srate_callback(jack_nframes_t);
            void connect_callback(jack_port_id_t, jack_port_id_t, int);

            JACK_HANDLE fHandle;                     /*<! handle to the jack client library */

            // map needed client library symbols as members to override those from the jackserver library
            PROXY_DEF_SYMBOL(jack_client_t*, jack_client_open, const char*, jack_options_t, jack_status_t*, ...);
            PROXY_DEF_SYMBOL(int, jack_set_process_callback, jack_client_t*, JackProcessCallback, void*);
            PROXY_DEF_SYMBOL(int, jack_set_buffer_size_callback, jack_client_t*, JackBufferSizeCallback, void*);
            PROXY_DEF_SYMBOL(int, jack_set_sample_rate_callback, jack_client_t*, JackSampleRateCallback, void*);
            PROXY_DEF_SYMBOL(int, jack_set_port_connect_callback, jack_client_t*, JackPortConnectCallback, void*);
            PROXY_DEF_SYMBOL(void, jack_on_shutdown, jack_client_t*, JackShutdownCallback, void*);
            PROXY_DEF_SYMBOL(jack_nframes_t, jack_get_buffer_size, jack_client_t*);
            PROXY_DEF_SYMBOL(jack_nframes_t, jack_get_sample_rate, jack_client_t*);
            PROXY_DEF_SYMBOL(int, jack_activate, jack_client_t*);
            PROXY_DEF_SYMBOL(int, jack_deactivate, jack_client_t*);
            PROXY_DEF_SYMBOL(jack_port_t*, jack_port_by_id, jack_client_t*, jack_port_id_t);
            PROXY_DEF_SYMBOL(int, jack_port_is_mine, const jack_client_t*, const jack_port_t*);
            PROXY_DEF_SYMBOL(const char**, jack_get_ports, jack_client_t*, const char*, const char*, unsigned long);
            PROXY_DEF_SYMBOL(void, jack_free, void*);
            PROXY_DEF_SYMBOL(jack_port_t*, jack_port_register, jack_client_t*, const char*, const char*, unsigned long, unsigned long);
            PROXY_DEF_SYMBOL(int, jack_port_unregister, jack_client_t*, jack_port_t*);
            PROXY_DEF_SYMBOL(void*, jack_port_get_buffer, jack_port_t*, jack_nframes_t);
            PROXY_DEF_SYMBOL(int, jack_connect, jack_client_t*, const char*, const char*);
            PROXY_DEF_SYMBOL(const char*, jack_port_name, const jack_port_t*);
            PROXY_DEF_SYMBOL(int, jack_client_close, jack_client_t*);

            /*! load the needed library symbols */
            void LoadSymbols()
            {
                PROXY_LOAD_SYMBOL(jack_client_t*, jack_client_open, const char*, jack_options_t, jack_status_t*, ...);
                PROXY_LOAD_SYMBOL(int, jack_set_process_callback, jack_client_t*, JackProcessCallback, void*);
                PROXY_LOAD_SYMBOL(int, jack_set_buffer_size_callback, jack_client_t*, JackBufferSizeCallback, void*);
                PROXY_LOAD_SYMBOL(int, jack_set_sample_rate_callback, jack_client_t*, JackSampleRateCallback, void*);
                PROXY_LOAD_SYMBOL(int, jack_set_port_connect_callback, jack_client_t*, JackPortConnectCallback, void*);
                PROXY_LOAD_SYMBOL(void, jack_on_shutdown, jack_client_t*, JackShutdownCallback, void*);
                PROXY_LOAD_SYMBOL(jack_nframes_t, jack_get_buffer_size, jack_client_t*);
                PROXY_LOAD_SYMBOL(jack_nframes_t, jack_get_sample_rate, jack_client_t*);
                PROXY_LOAD_SYMBOL(int, jack_activate, jack_client_t*);
                PROXY_LOAD_SYMBOL(int, jack_deactivate, jack_client_t*);
                PROXY_LOAD_SYMBOL(jack_port_t*, jack_port_by_id, jack_client_t*, jack_port_id_t);
                PROXY_LOAD_SYMBOL(int, jack_port_is_mine, const jack_client_t*, const jack_port_t*);
                PROXY_LOAD_SYMBOL(const char**, jack_get_ports, jack_client_t*, const char*, const char*, unsigned long);
                PROXY_LOAD_SYMBOL(void, jack_free, void*);
                PROXY_LOAD_SYMBOL(jack_port_t*, jack_port_register, jack_client_t*, const char*, const char*, unsigned long, unsigned long);
                PROXY_LOAD_SYMBOL(int, jack_port_unregister, jack_client_t*, jack_port_t*);
                PROXY_LOAD_SYMBOL(void*, jack_port_get_buffer, jack_port_t*, jack_nframes_t);
                PROXY_LOAD_SYMBOL(int, jack_connect, jack_client_t*, const char*, const char*);
                PROXY_LOAD_SYMBOL(const char*, jack_port_name, const jack_port_t*);
                PROXY_LOAD_SYMBOL(int, jack_client_close, jack_client_t*);
            }
            int LoadClientLib(); /*!< load the client library */

        public:

            JackProxyDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table,
                         const char* upstream, const char* promiscuous, char* client_name, bool auto_connect, bool auto_save);
            virtual ~JackProxyDriver();

            int Open(jack_nframes_t buffer_size,
                         jack_nframes_t samplerate,
                         bool capturing,
                         bool playing,
                         int inchannels,
                         int outchannels,
                         bool monitor,
                         const char* capture_driver_name,
                         const char* playback_driver_name,
                         jack_nframes_t capture_latency,
                         jack_nframes_t playback_latency);
            int Close();

            int Stop();

            int Attach();
            int Detach();

            int Read();
            int Write();

    };
}

#endif
