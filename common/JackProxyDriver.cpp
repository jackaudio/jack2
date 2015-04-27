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

#include "JackCompilerDeps.h"
#include "driver_interface.h"
#include "JackEngineControl.h"
#include "JackLockedEngine.h"
#include "JackWaitCallbackDriver.h"
#include "JackProxyDriver.h"

using namespace std;

namespace Jack
{
    JackProxyDriver::JackProxyDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table,
                                const char* upstream, const char* promiscuous,
                                char* client_name, bool auto_connect, bool auto_save)
            : JackRestarterDriver(name, alias, engine, table)
    {
        jack_log("JackProxyDriver::JackProxyDriver upstream %s", upstream);

        assert(strlen(upstream) < JACK_CLIENT_NAME_SIZE);
        strcpy(fUpstream, upstream);

        assert(strlen(client_name) < JACK_CLIENT_NAME_SIZE);
        strcpy(fClientName, client_name);

        if (promiscuous) {
            fPromiscuous = strdup(promiscuous);
        }

        fAutoConnect = auto_connect;
        fAutoSave = auto_save;
    }

    JackProxyDriver::~JackProxyDriver()
    {
        if (fHandle) {
            UnloadJackModule(fHandle);
        }
    }

    int JackProxyDriver::LoadClientLib()
    {
        // Already loaded
        if (fHandle) {
            return 0;
        }
        fHandle = LoadJackModule(JACK_PROXY_CLIENT_LIB);
        if (!fHandle) {
            return -1;
        }
        LoadSymbols();
        return 0;
    }

//open, close, attach and detach------------------------------------------------------

    int JackProxyDriver::Open(jack_nframes_t buffer_size,
                         jack_nframes_t samplerate,
                         bool capturing,
                         bool playing,
                         int inchannels,
                         int outchannels,
                         bool monitor,
                         const char* capture_driver_name,
                         const char* playback_driver_name,
                         jack_nframes_t capture_latency,
                         jack_nframes_t playback_latency)
    {
        fDetectPlaybackChannels = (outchannels == -1);
        fDetectCaptureChannels = (inchannels == -1);

        if (LoadClientLib() != 0) {
            jack_error("Cannot dynamically load client library !");
            return -1;
        }

        return JackWaiterDriver::Open(buffer_size, samplerate,
                                    capturing, playing,
                                    inchannels, outchannels,
                                    monitor,
                                    capture_driver_name, playback_driver_name,
                                    capture_latency, playback_latency);
    }

    int JackProxyDriver::Close()
    {
        FreePorts();
        return JackWaiterDriver::Close();
    }

    // Attach and Detach are defined as empty methods: port allocation is done when driver actually start (that is in Init)
    int JackProxyDriver::Attach()
    {
        return 0;
    }

    int JackProxyDriver::Detach()
    {
        return 0;
    }

//init and restart--------------------------------------------------------------------

    /*
        JackProxyDriver is wrapped in a JackWaitCallbackDriver decorator that behaves
        as a "dummy driver, until Initialize method returns.
    */
    bool JackProxyDriver::Initialize()
    {
        jack_log("JackProxyDriver::Initialize");

        // save existing local connections if needed
        if (fAutoSave) {
            SaveConnections(0);
        }

        // new loading, but existing client, restart the driver
        if (fClient) {
            jack_info("JackProxyDriver restarting...");
            jack_client_close(fClient);
        }
        FreePorts();

        // display some additional infos
        jack_info("JackProxyDriver started in %s mode.",
                    (fEngineControl->fSyncMode) ? "sync" : "async");

        do {
            jack_status_t status;
            char *old = NULL;

            if (fPromiscuous) {
                // as we are fiddling with the environment variable content, save it
                const char* tmp = getenv("JACK_PROMISCUOUS_SERVER");
                if (tmp) {
                    old = strdup(tmp);
                }
                // temporary enable promiscuous mode
                if (setenv("JACK_PROMISCUOUS_SERVER", fPromiscuous, 1) < 0) {
                    free(old);
                    jack_error("Error allocating memory.");
                    return false;
                }
            }

            jack_info("JackProxyDriver connecting to %s", fUpstream);
            fClient = jack_client_open(fClientName, static_cast<jack_options_t>(JackNoStartServer|JackServerName), &status, fUpstream);

            if (fPromiscuous) {
                // restore previous environment variable content
                if (old) {
                    if (setenv("JACK_PROMISCUOUS_SERVER", old, 1) < 0) {
                        free(old);
                        jack_error("Error allocating memory.");
                        return false;
                    }
                    free(old);
                } else {
                    unsetenv("JACK_PROMISCUOUS_SERVER");
                }
            }

            // the connection failed, try again later
            if (!fClient) {
                JackSleep(1000000);
            }

        } while (!fClient);
        jack_info("JackProxyDriver connected to %s", fUpstream);

        // we are connected, let's register some callbacks

        jack_on_shutdown(fClient, shutdown_callback, this);

        if (jack_set_process_callback(fClient, process_callback, this) != 0) {
            jack_error("Cannot set process callback.");
            return false;
        }

        if (jack_set_buffer_size_callback(fClient, bufsize_callback, this) != 0) {
            jack_error("Cannot set buffer size callback.");
            return false;
        }

        if (jack_set_sample_rate_callback(fClient, srate_callback, this) != 0) {
            jack_error("Cannot set sample rate callback.");
            return false;
        }

        if (jack_set_port_connect_callback(fClient, connect_callback, this) != 0) {
            jack_error("Cannot set port connect callback.");
            return false;
        }

        // detect upstream physical playback ports if needed
        if (fDetectPlaybackChannels) {
            fPlaybackChannels = CountIO(JACK_DEFAULT_AUDIO_TYPE, JackPortIsPhysical | JackPortIsOutput);
        }

        // detect upstream physical capture ports if needed
        if (fDetectCaptureChannels) {
            fCaptureChannels = CountIO(JACK_DEFAULT_AUDIO_TYPE, JackPortIsPhysical | JackPortIsInput);
        }

        if (AllocPorts() != 0) {
            jack_error("Can't allocate ports.");
            return false;
        }

        bufsize_callback(jack_get_buffer_size(fClient));
        srate_callback(jack_get_sample_rate(fClient));

        // restore local connections if needed
        if (fAutoSave) {
            LoadConnections(0);
        }

        // everything is ready, start upstream processing
        if (jack_activate(fClient) != 0) {
            jack_error("Cannot activate jack client.");
            return false;
        }

        // connect upstream ports if needed
        if (fAutoConnect) {
            ConnectPorts();
        }

        return true;
    }

    int JackProxyDriver::Stop()
    {
        if (fClient && (jack_deactivate(fClient) != 0)) {
            jack_error("Cannot deactivate jack client.");
            return -1;
        }
        return 0;
    }

//client callbacks---------------------------------------------------------------------------

    int JackProxyDriver::process_callback(jack_nframes_t nframes, void* arg)
    {
        assert(static_cast<JackProxyDriver*>(arg));
        return static_cast<JackProxyDriver*>(arg)->Process();
    }

    int JackProxyDriver::bufsize_callback(jack_nframes_t nframes, void* arg)
    {
        assert(static_cast<JackProxyDriver*>(arg));
        return static_cast<JackProxyDriver*>(arg)->bufsize_callback(nframes);
    }
    int JackProxyDriver::bufsize_callback(jack_nframes_t nframes)
    {
        if (JackTimedDriver::SetBufferSize(nframes) == 0) {
            return -1;
        }
        JackDriver::NotifyBufferSize(nframes);
        return 0;
    }

    int JackProxyDriver::srate_callback(jack_nframes_t nframes, void* arg)
    {
        assert(static_cast<JackProxyDriver*>(arg));
        return static_cast<JackProxyDriver*>(arg)->srate_callback(nframes);
    }
    int JackProxyDriver::srate_callback(jack_nframes_t nframes)
    {
        if (JackTimedDriver::SetSampleRate(nframes) == 0) {
            return -1;
        }
        JackDriver::NotifySampleRate(nframes);
        return 0;
    }

    void JackProxyDriver::connect_callback(jack_port_id_t a, jack_port_id_t b, int connect, void* arg)
    {
        assert(static_cast<JackProxyDriver*>(arg));
        static_cast<JackProxyDriver*>(arg)->connect_callback(a, b, connect);
    }
    void JackProxyDriver::connect_callback(jack_port_id_t a, jack_port_id_t b, int connect)
    {
        jack_port_t* port;
        int i;

        // skip port if not our own
        port = jack_port_by_id(fClient, a);
        if (!jack_port_is_mine(fClient, port)) {
            port = jack_port_by_id(fClient, b);
            if (!jack_port_is_mine(fClient, port)) {
                return;
            }
        }

        for (i = 0; i < fCaptureChannels; i++) {
            if (fUpstreamPlaybackPorts[i] == port) {
                fUpstreamPlaybackPortConnected[i] = connect;
            }
        }

        for (i = 0; i < fPlaybackChannels; i++) {
            if (fUpstreamCapturePorts[i] == port) {
                fUpstreamCapturePortConnected[i] = connect;
            }
        }
    }

    void JackProxyDriver::shutdown_callback(void* arg)
    {
        assert(static_cast<JackProxyDriver*>(arg));
        static_cast<JackProxyDriver*>(arg)->RestartWait();
    }

//jack ports and buffers--------------------------------------------------------------

    int JackProxyDriver::CountIO(const char* type, int flags)
    {
        int count = 0;
        const char** ports = jack_get_ports(fClient, NULL, type, flags);
        if (ports != NULL) {
            while (ports[count]) { count++; }
            jack_free(ports);
        }
        return count;
    }

    int JackProxyDriver::AllocPorts()
    {
        jack_log("JackProxyDriver::AllocPorts fBufferSize = %ld fSampleRate = %ld", fEngineControl->fBufferSize, fEngineControl->fSampleRate);

        char proxy[REAL_JACK_PORT_NAME_SIZE];
        int i;

        fUpstreamPlaybackPorts = new jack_port_t* [fCaptureChannels];
        fUpstreamPlaybackPortConnected = new int [fCaptureChannels];
        for (i = 0; i < fCaptureChannels; i++) {
            snprintf(proxy, sizeof(proxy), "%s:to_client_%d", fClientName, i + 1);
            fUpstreamPlaybackPorts[i] = jack_port_register(fClient, proxy, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput | JackPortIsTerminal, 0);
            if (fUpstreamPlaybackPorts[i] == NULL) {
                jack_error("driver: cannot register upstream port %s", proxy);
                return -1;
            }
            fUpstreamPlaybackPortConnected[i] = 0;
        }

        fUpstreamCapturePorts = new jack_port_t* [fPlaybackChannels];
        fUpstreamCapturePortConnected = new int [fPlaybackChannels];
        for (i = 0; i < fPlaybackChannels; i++) {
            snprintf(proxy, sizeof(proxy), "%s:from_client_%d", fClientName, i + 1);
            fUpstreamCapturePorts[i] = jack_port_register(fClient, proxy, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput | JackPortIsTerminal, 0);
            if (fUpstreamCapturePorts[i] == NULL) {
                jack_error("driver: cannot register upstream port %s", proxy);
                return -1;
            }
            fUpstreamCapturePortConnected[i] = 0;
        }

        // local ports are registered here
        return JackAudioDriver::Attach();
    }

    int JackProxyDriver::FreePorts()
    {
        jack_log("JackProxyDriver::FreePorts");

        int i;

        for (i = 0; i < fCaptureChannels; i++) {
            if (fCapturePortList[i] > 0) {
                fEngine->PortUnRegister(fClientControl.fRefNum, fCapturePortList[i]);
                fCapturePortList[i] = 0;
            }
            if (fUpstreamPlaybackPorts && fUpstreamPlaybackPorts[i]) {
                fUpstreamPlaybackPorts[i] = NULL;
            }
        }

        for (i = 0; i < fPlaybackChannels; i++) {
            if (fPlaybackPortList[i] > 0) {
                fEngine->PortUnRegister(fClientControl.fRefNum, fPlaybackPortList[i]);
                fPlaybackPortList[i] = 0;
            }
            if (fUpstreamCapturePorts && fUpstreamCapturePorts[i]) {
                fUpstreamCapturePorts[i] = NULL;
            }
        }

        delete[] fUpstreamPlaybackPorts;
        delete[] fUpstreamPlaybackPortConnected;
        delete[] fUpstreamCapturePorts;
        delete[] fUpstreamCapturePortConnected;

        fUpstreamPlaybackPorts = NULL;
        fUpstreamPlaybackPortConnected = NULL;
        fUpstreamCapturePorts = NULL;
        fUpstreamCapturePortConnected = NULL;

        return 0;
    }

    void JackProxyDriver::ConnectPorts()
    {
        jack_log("JackProxyDriver::ConnectPorts");
        const char** ports = jack_get_ports(fClient, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsPhysical | JackPortIsOutput);
        if (ports != NULL) {
            for (int i = 0; i < fCaptureChannels && ports[i]; i++) {
                jack_connect(fClient, ports[i], jack_port_name(fUpstreamPlaybackPorts[i]));
            }
            jack_free(ports);
        }

        ports = jack_get_ports(fClient, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsPhysical | JackPortIsInput);
        if (ports != NULL) {
            for (int i = 0; i < fPlaybackChannels && ports[i]; i++) {
                jack_connect(fClient, jack_port_name(fUpstreamCapturePorts[i]), ports[i]);
            }
            jack_free(ports);
        }
    }

//driver processes--------------------------------------------------------------------

    int JackProxyDriver::Read()
    {
        // take the time at the beginning of the cycle
        JackDriver::CycleTakeBeginTime();

        int i;
        void *from, *to;
        size_t buflen = sizeof(jack_default_audio_sample_t) * fEngineControl->fBufferSize;

        for (i = 0; i < fCaptureChannels; i++) {
            if (fUpstreamPlaybackPortConnected[i]) {
                from = jack_port_get_buffer(fUpstreamPlaybackPorts[i], fEngineControl->fBufferSize);
                to = GetInputBuffer(i);
                memcpy(to, from, buflen);
            }
        }

        return 0;
    }

    int JackProxyDriver::Write()
    {
        int i;
        void *from, *to;
        size_t buflen = sizeof(jack_default_audio_sample_t) * fEngineControl->fBufferSize;

        for (i = 0; i < fPlaybackChannels; i++) {
            if (fUpstreamCapturePortConnected[i]) {
                to = jack_port_get_buffer(fUpstreamCapturePorts[i], fEngineControl->fBufferSize);
                from = GetOutputBuffer(i);
                memcpy(to, from, buflen);
            }
        }

        return 0;
    }

//driver loader-----------------------------------------------------------------------

#ifdef __cplusplus
    extern "C"
    {
#endif

        SERVER_EXPORT jack_driver_desc_t* driver_get_descriptor()
        {
            jack_driver_desc_t * desc;
            jack_driver_desc_filler_t filler;
            jack_driver_param_value_t value;

            desc = jack_driver_descriptor_construct("proxy", JackDriverMaster, "proxy backend", &filler);

            strcpy(value.str, DEFAULT_UPSTREAM);
            jack_driver_descriptor_add_parameter(desc, &filler, "upstream", 'u', JackDriverParamString, &value, NULL, "Name of the upstream jack server", NULL);

            strcpy(value.str, "");
            jack_driver_descriptor_add_parameter(desc, &filler, "promiscuous", 'p', JackDriverParamString, &value, NULL, "Promiscuous group", NULL);

            value.i = -1;
            jack_driver_descriptor_add_parameter(desc, &filler, "input-ports", 'C', JackDriverParamInt, &value, NULL, "Number of audio input ports", "Number of audio input ports. If -1, audio physical input from the master");
            jack_driver_descriptor_add_parameter(desc, &filler, "output-ports", 'P', JackDriverParamInt, &value, NULL, "Number of audio output ports", "Number of audio output ports. If -1, audio physical output from the master");

            strcpy(value.str, "proxy");
            jack_driver_descriptor_add_parameter(desc, &filler, "client-name", 'n', JackDriverParamString, &value, NULL, "Name of the jack client", NULL);

            value.i = false;
            jack_driver_descriptor_add_parameter(desc, &filler, "use-username", 'U', JackDriverParamBool, &value, NULL, "Use current username as client name", NULL);

            value.i = false;
            jack_driver_descriptor_add_parameter(desc, &filler, "auto-connect", 'c', JackDriverParamBool, &value, NULL, "Auto connect proxy to upstream system ports", NULL);

            value.i = false;
            jack_driver_descriptor_add_parameter(desc, &filler, "auto-save", 's', JackDriverParamBool, &value, NULL, "Save/restore connection state when restarting", NULL);

            return desc;
        }

        SERVER_EXPORT Jack::JackDriverClientInterface* driver_initialize(Jack::JackLockedEngine* engine, Jack::JackSynchro* table, const JSList* params)
        {
            char upstream[JACK_CLIENT_NAME_SIZE + 1];
            char promiscuous[JACK_CLIENT_NAME_SIZE + 1] = {0};
            char client_name[JACK_CLIENT_NAME_SIZE + 1];
            jack_nframes_t period_size = 1024;  // to be used while waiting for master period_size
            jack_nframes_t sample_rate = 48000; // to be used while waiting for master sample_rate
            int capture_ports = -1;
            int playback_ports = -1;
            const JSList* node;
            const jack_driver_param_t* param;
            bool auto_connect = false;
            bool auto_save = false;
            bool use_promiscuous = false;

            // Possibly use env variable for upstream name
            const char* default_upstream = getenv("JACK_PROXY_UPSTREAM");
            strcpy(upstream, (default_upstream) ? default_upstream : DEFAULT_UPSTREAM);

            // Possibly use env variable for upstream promiscuous
            const char* default_promiscuous = getenv("JACK_PROXY_PROMISCUOUS");
            strcpy(promiscuous, (default_promiscuous) ? default_promiscuous : "");

            // Possibly use env variable for client name
            const char* default_client_name = getenv("JACK_PROXY_CLIENT_NAME");
            strcpy(client_name, (default_client_name) ? default_client_name : DEFAULT_CLIENT_NAME);

#ifdef WIN32
            const char* username = getenv("USERNAME");
#else
            const char* username = getenv("LOGNAME");
#endif

            for (node = params; node; node = jack_slist_next(node)) {
                param = (const jack_driver_param_t*) node->data;
                switch (param->character)
                {
                    case 'u' :
                        assert(strlen(param->value.str) < JACK_CLIENT_NAME_SIZE);
                        strcpy(upstream, param->value.str);
                        break;
                    case 'p':
                        assert(strlen(param->value.str) < JACK_CLIENT_NAME_SIZE);
                        use_promiscuous = true;
                        strcpy(promiscuous, param->value.str);
                        break;
                    case 'C':
                        capture_ports = param->value.i;
                        break;
                    case 'P':
                        playback_ports = param->value.i;
                        break;
                    case 'n' :
                        assert(strlen(param->value.str) < JACK_CLIENT_NAME_SIZE);
                        strncpy(client_name, param->value.str, JACK_CLIENT_NAME_SIZE);
                        break;
                    case 'U' :
                        if (username && *username) {
                            assert(strlen(username) < JACK_CLIENT_NAME_SIZE);
                            strncpy(client_name, username, JACK_CLIENT_NAME_SIZE);
                        }
                    case 'c':
                        auto_connect = true;
                        break;
                    case 's':
                        auto_save = true;
                        break;
                }
            }

            try {

                Jack::JackDriverClientInterface* driver = new Jack::JackWaitCallbackDriver(
                        new Jack::JackProxyDriver("system", "proxy_pcm", engine, table, upstream, use_promiscuous ? promiscuous : NULL, client_name, auto_connect, auto_save));
                if (driver->Open(period_size, sample_rate, 1, 1, capture_ports, playback_ports, false, "capture_", "playback_", 0, 0) == 0) {
                    return driver;
                } else {
                    delete driver;
                    return NULL;
                }

            } catch (...) {
                return NULL;
            }
        }

#ifdef __cplusplus
    }
#endif
}
