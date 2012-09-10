/*
Copyright (C) 2009-2011 Grame

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

#include <assert.h>
#include <stdarg.h>

#include "JackNetInterface.h"
#include "JackAudioAdapterInterface.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // NetJack common API

    #define MASTER_NAME_SIZE 256

    enum JackNetEncoder {

        JackFloatEncoder = 0,
        JackIntEncoder = 1,
        JackCeltEncoder = 2,
        JackOpusEncoder = 3
    };

    typedef struct {

        int audio_input;
        int audio_output;
        int midi_input;
        int midi_output;
        int mtu;
        int time_out;   // in millisecond, -1 means in infinite
        int encoder;    // one of JackNetEncoder
        int kbps;       // KB per second for CELT encoder
        int latency;    // network cycles

    } jack_slave_t;

    typedef struct {

        int audio_input;
        int audio_output;
        int midi_input;
        int midi_output;
        jack_nframes_t buffer_size;
        jack_nframes_t sample_rate;
        char master_name[MASTER_NAME_SIZE];

    } jack_master_t;

    // NetJack slave API

    typedef struct _jack_net_slave jack_net_slave_t;

    typedef int (* JackNetSlaveProcessCallback) (jack_nframes_t buffer_size,
                                            int audio_input,
                                            float** audio_input_buffer,
                                            int midi_input,
                                            void** midi_input_buffer,
                                            int audio_output,
                                            float** audio_output_buffer,
                                            int midi_output,
                                            void** midi_output_buffer,
                                            void* data);

    typedef int (*JackNetSlaveBufferSizeCallback) (jack_nframes_t nframes, void *arg);
    typedef int (*JackNetSlaveSampleRateCallback) (jack_nframes_t nframes, void *arg);
    typedef void (*JackNetSlaveShutdownCallback) (void* data);

    LIB_EXPORT jack_net_slave_t* jack_net_slave_open(const char* ip, int port, const char* name, jack_slave_t* request, jack_master_t* result);
    LIB_EXPORT int jack_net_slave_close(jack_net_slave_t* net);

    LIB_EXPORT int jack_net_slave_activate(jack_net_slave_t* net);
    LIB_EXPORT int jack_net_slave_deactivate(jack_net_slave_t* net);

    LIB_EXPORT int jack_set_net_slave_process_callback(jack_net_slave_t* net, JackNetSlaveProcessCallback net_callback, void *arg);
    LIB_EXPORT int jack_set_net_slave_buffer_size_callback(jack_net_slave_t* net, JackNetSlaveBufferSizeCallback bufsize_callback, void *arg);
    LIB_EXPORT int jack_set_net_slave_sample_rate_callback(jack_net_slave_t* net, JackNetSlaveSampleRateCallback samplerate_callback, void *arg);
    LIB_EXPORT int jack_set_net_slave_shutdown_callback(jack_net_slave_t* net, JackNetSlaveShutdownCallback shutdown_callback, void *arg);

    // NetJack master API

    typedef struct _jack_net_master jack_net_master_t;

    LIB_EXPORT jack_net_master_t* jack_net_master_open(const char* ip, int port, const char* name, jack_master_t* request, jack_slave_t* result);
    LIB_EXPORT int jack_net_master_close(jack_net_master_t* net);

    LIB_EXPORT int jack_net_master_recv(jack_net_master_t* net, int audio_input, float** audio_input_buffer, int midi_input, void** midi_input_buffer);
    LIB_EXPORT int jack_net_master_send(jack_net_master_t* net, int audio_output, float** audio_output_buffer, int midi_output, void** midi_output_buffer);

    // NetJack adapter API

    typedef struct _jack_adapter jack_adapter_t;

    LIB_EXPORT jack_adapter_t* jack_create_adapter(int input, int output,
                                                    jack_nframes_t host_buffer_size,
                                                    jack_nframes_t host_sample_rate,
                                                    jack_nframes_t adapted_buffer_size,
                                                    jack_nframes_t adapted_sample_rate);
    LIB_EXPORT int jack_destroy_adapter(jack_adapter_t* adapter);
    LIB_EXPORT void jack_flush_adapter(jack_adapter_t* adapter);

    LIB_EXPORT int jack_adapter_push_and_pull(jack_adapter_t* adapter, float** input, float** output, unsigned int frames);
    LIB_EXPORT int jack_adapter_pull_and_push(jack_adapter_t* adapter, float** input, float** output, unsigned int frames);

    #define LOG_LEVEL_INFO   1
    #define LOG_LEVEL_ERROR  2

    LIB_EXPORT void jack_error(const char *fmt, ...);
    LIB_EXPORT void jack_info(const char *fmt, ...);
    LIB_EXPORT void jack_log(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

namespace Jack
{

struct JackNetExtMaster : public JackNetMasterInterface {

    // Data buffers
    float** fAudioCaptureBuffer;
    float** fAudioPlaybackBuffer;

    JackMidiBuffer** fMidiCaptureBuffer;
    JackMidiBuffer** fMidiPlaybackBuffer;

    jack_master_t fRequest;

    JackNetExtMaster(const char* ip,
                    int port,
                    const char* name,
                    jack_master_t* request)
    {
        fRunning = true;
        assert(strlen(ip) < 32);
        strcpy(fMulticastIP, ip);
        fSocket.SetPort(port);
        fRequest.buffer_size = request->buffer_size;
        fRequest.sample_rate = request->sample_rate;
        fRequest.audio_input = request->audio_input;
        fRequest.audio_output = request->audio_output;
        fAudioCaptureBuffer = NULL;
        fAudioPlaybackBuffer = NULL;
        fMidiCaptureBuffer = NULL;
        fMidiPlaybackBuffer = NULL;
    }

    virtual ~JackNetExtMaster()
    {}

    int Open(jack_slave_t* result)
    {
        // Init socket API (win32)
        if (SocketAPIInit() < 0) {
            jack_error("Can't init Socket API, exiting...");
            return -1;
        }

        // Request socket
        if (fSocket.NewSocket() == SOCKET_ERROR) {
            jack_error("Can't create the network management input socket : %s", StrError(NET_ERROR_CODE));
            return -1;
        }

        // Bind the socket to the local port
        if (fSocket.Bind() == SOCKET_ERROR) {
            jack_error("Can't bind the network manager socket : %s", StrError(NET_ERROR_CODE));
            fSocket.Close();
            return -1;
        }

        // Join multicast group
        if (fSocket.JoinMCastGroup(fMulticastIP) == SOCKET_ERROR) {
            jack_error("Can't join multicast group : %s", StrError(NET_ERROR_CODE));
        }

        // Local loop
        if (fSocket.SetLocalLoop() == SOCKET_ERROR) {
            jack_error("Can't set local loop : %s", StrError(NET_ERROR_CODE));
        }

        // Set a timeout on the multicast receive (the thread can now be cancelled)
        if (fSocket.SetTimeOut(MANAGER_INIT_TIMEOUT) == SOCKET_ERROR) {
            jack_error("Can't set timeout : %s", StrError(NET_ERROR_CODE));
        }

         // Main loop, wait for data, deal with it and wait again
        int attempt = 0;
        int rx_bytes = 0;

        do
        {
            session_params_t net_params;
            rx_bytes = fSocket.CatchHost(&net_params, sizeof(session_params_t), 0);
            SessionParamsNToH(&net_params, &fParams);

            if ((rx_bytes == SOCKET_ERROR) && (fSocket.GetError() != NET_NO_DATA)) {
                jack_error("Error in receive : %s", StrError(NET_ERROR_CODE));
                if (++attempt == 10) {
                    jack_error("Can't receive on the socket, exiting net manager" );
                    goto error;
                }
            }

            if (rx_bytes == sizeof(session_params_t ))  {
                switch (GetPacketType(&fParams)) {

                    case SLAVE_AVAILABLE:
                        if (InitMaster(result) == 0) {
                            SessionParamsDisplay(&fParams);
                            fRunning = false;
                        } else {
                            jack_error("Can't init new net master...");
                            goto error;
                        }
                        jack_info("Waiting for a slave...");
                        break;

                    case KILL_MASTER:
                         break;

                    default:
                        break;
                }
            }
        }
        while (fRunning);
 
        // Set result parameters
        result->audio_input = fParams.fSendAudioChannels;
        result->audio_output = fParams.fReturnAudioChannels;
        result->midi_input = fParams.fSendMidiChannels;
        result->midi_output = fParams.fReturnMidiChannels;
        result->mtu = fParams.fMtu;
        result->latency = fParams.fNetworkLatency;
        return 0;

    error:
        fSocket.Close();
        return -1;
    }

    int InitMaster(jack_slave_t* result)
    {
        // Check MASTER <==> SLAVE network protocol coherency
        if (fParams.fProtocolVersion != MASTER_PROTOCOL) {
            jack_error("Error : slave is running with a different protocol %s", fParams.fName);
            return -1;
        }

        // Settings
        fSocket.GetName(fParams.fMasterNetName);
        fParams.fID = 1;
        fParams.fPeriodSize = fRequest.buffer_size;
        fParams.fSampleRate = fRequest.sample_rate;
        
        if (fRequest.audio_input == -1) {
            if (fParams.fSendAudioChannels == -1) {
                jack_error("Error : master and slave use -1 for wanted inputs...");
                return -1;
            } else {
                result->audio_input = fParams.fSendAudioChannels;
                jack_info("Takes slave %d inputs", fParams.fSendAudioChannels);
            }
        } else if (fParams.fSendAudioChannels == -1) {
            fParams.fSendAudioChannels = fRequest.audio_input;
            jack_info("Takes master %d inputs", fRequest.audio_input);
        } else if (fParams.fSendAudioChannels != fRequest.audio_input) {
            jack_error("Error : master wants %d inputs and slave wants %d inputs...", fRequest.audio_input, fParams.fSendAudioChannels);
            return -1;
        } 
                
        if (fRequest.audio_output == -1) {
            if (fParams.fReturnAudioChannels == -1) {
                jack_error("Error : master and slave use -1 for wanted outputs...");
                return -1;
            } else {
                result->audio_output = fParams.fReturnAudioChannels;
                jack_info("Takes slave %d outputs", fParams.fReturnAudioChannels);
            }
        } else if (fParams.fReturnAudioChannels == -1) {
            fParams.fReturnAudioChannels = fRequest.audio_output;
            jack_info("Takes master %d outputs", fRequest.audio_output);
        } else if (fParams.fReturnAudioChannels != fRequest.audio_output) {
            jack_error("Error : master wants %d outputs and slave wants %d outputs...", fRequest.audio_output, fParams.fReturnAudioChannels);
            return -1;
        }
        
        // Close request socket
        fSocket.Close();

        /// Network init
        if (!JackNetMasterInterface::Init()) {
            return -1;
        }

        // Set global parameters
        if (!SetParams()) {
            return -1;
        }

        AllocPorts();
        return 0;
    }

    int Close()
    {
        fSocket.Close();
        FreePorts();
        return 0;
    }

    void AllocPorts()
    {
        // Set buffers
        if (fParams.fSendAudioChannels > 0) {
            fAudioCaptureBuffer = new float*[fParams.fSendAudioChannels];
            for (int audio_port_index = 0; audio_port_index < fParams.fSendAudioChannels; audio_port_index++) {
                fAudioCaptureBuffer[audio_port_index] = new float[fParams.fPeriodSize];
                fNetAudioCaptureBuffer->SetBuffer(audio_port_index, fAudioCaptureBuffer[audio_port_index]);
            }
        }

        if (fParams.fSendMidiChannels > 0) {
            fMidiCaptureBuffer = new JackMidiBuffer*[fParams.fSendMidiChannels];
            for (int midi_port_index = 0; midi_port_index < fParams.fSendMidiChannels; midi_port_index++) {
                fMidiCaptureBuffer[midi_port_index] = (JackMidiBuffer*)new float[fParams.fPeriodSize];
                fNetMidiCaptureBuffer->SetBuffer(midi_port_index, fMidiCaptureBuffer[midi_port_index]);
            }
        }

        if (fParams.fReturnAudioChannels > 0) {
            fAudioPlaybackBuffer = new float*[fParams.fReturnAudioChannels];
            for (int audio_port_index = 0; audio_port_index < fParams.fReturnAudioChannels; audio_port_index++) {
                fAudioPlaybackBuffer[audio_port_index] = new float[fParams.fPeriodSize];
                fNetAudioPlaybackBuffer->SetBuffer(audio_port_index, fAudioPlaybackBuffer[audio_port_index]);
            }
        }

        if (fParams.fReturnMidiChannels > 0) {
            fMidiPlaybackBuffer = new JackMidiBuffer*[fParams.fReturnMidiChannels];
            for (int midi_port_index = 0; midi_port_index < fParams.fReturnMidiChannels; midi_port_index++) {
                fMidiPlaybackBuffer[midi_port_index] = (JackMidiBuffer*)new float[fParams.fPeriodSize];
                fNetMidiPlaybackBuffer->SetBuffer(midi_port_index, fMidiPlaybackBuffer[midi_port_index]);
            }
        }
    }

    void FreePorts()
    {
        if (fAudioPlaybackBuffer) {
            for (int audio_port_index = 0; audio_port_index < fParams.fSendAudioChannels; audio_port_index++)
                delete[] fAudioPlaybackBuffer[audio_port_index];
            delete[] fAudioPlaybackBuffer;
            fAudioPlaybackBuffer = NULL;
        }

        if (fMidiPlaybackBuffer) {
            for (int midi_port_index = 0; midi_port_index < fParams.fSendMidiChannels; midi_port_index++)
                delete[] (fMidiPlaybackBuffer[midi_port_index]);
            delete[] fMidiPlaybackBuffer;
            fMidiPlaybackBuffer = NULL;
        }

        if (fAudioCaptureBuffer) {
            for (int audio_port_index = 0; audio_port_index < fParams.fReturnAudioChannels; audio_port_index++)
                delete[] fAudioCaptureBuffer[audio_port_index];
            delete[] fAudioCaptureBuffer;
            fAudioCaptureBuffer = NULL;
        }

        if (fMidiCaptureBuffer) {
            for (int midi_port_index = 0; midi_port_index < fParams.fReturnMidiChannels; midi_port_index++)
                delete[] fMidiCaptureBuffer[midi_port_index];
            delete[] fMidiCaptureBuffer;
            fMidiCaptureBuffer = NULL;
        }
    }

    int Read(int audio_input, float** audio_input_buffer, int midi_input, void** midi_input_buffer)
    {
        
        try {
           
            assert(audio_input == fParams.fReturnAudioChannels);

            for (int audio_port_index = 0; audio_port_index < audio_input; audio_port_index++) {
                fNetAudioPlaybackBuffer->SetBuffer(audio_port_index, audio_input_buffer[audio_port_index]);
            }

            for (int midi_port_index = 0; midi_port_index < midi_input; midi_port_index++) {
                fNetMidiPlaybackBuffer->SetBuffer(midi_port_index, ((JackMidiBuffer**)midi_input_buffer)[midi_port_index]);
            }
         
            //receive sync
            int res = SyncRecv();
            if ((res == 0) || (res == SOCKET_ERROR)) {
                return res;
            }
       
            DecodeSyncPacket();
            return DataRecv();

        } catch (JackNetException& e) {
            jack_error("Connection lost.");
            return -1;
        }
     }

     int Write(int audio_output, float** audio_output_buffer, int midi_output, void** midi_output_buffer)
     {
        try {
            
            assert(audio_output == fParams.fSendAudioChannels);

            for (int audio_port_index = 0; audio_port_index < audio_output; audio_port_index++) {
                fNetAudioCaptureBuffer->SetBuffer(audio_port_index, audio_output_buffer[audio_port_index]);
            }

            for (int midi_port_index = 0; midi_port_index < midi_output; midi_port_index++) {
                fNetMidiCaptureBuffer->SetBuffer(midi_port_index, ((JackMidiBuffer**)midi_output_buffer)[midi_port_index]);
            }
            
            
            if (IsSynched()) {  // only send if connection is "synched"
            
                EncodeSyncPacket();

                if (SyncSend() == SOCKET_ERROR) {
                    return SOCKET_ERROR;
                }

                //send data
                if (DataSend() == SOCKET_ERROR) {
                    return SOCKET_ERROR;
                }
                
            } else {
                jack_info("Connection is not synched, skip cycle...");
            }
            
            return 0;

        } catch (JackNetException& e) {
            jack_error("Connection lost.");
            return -1;
        }
    }

    // Transport
    void EncodeTransportData()
    {}

    void DecodeTransportData()
    {}

};

struct JackNetExtSlave : public JackNetSlaveInterface, public JackRunnableInterface {

    JackThread fThread;

    JackNetSlaveProcessCallback fProcessCallback;
    void* fProcessArg;

    JackNetSlaveShutdownCallback fShutdownCallback;
    void* fShutdownArg;

    JackNetSlaveBufferSizeCallback fBufferSizeCallback;
    void* fBufferSizeArg;

    JackNetSlaveSampleRateCallback fSampleRateCallback;
    void* fSampleRateArg;

    //sample buffers
    float** fAudioCaptureBuffer;
    float** fAudioPlaybackBuffer;

    JackMidiBuffer** fMidiCaptureBuffer;
    JackMidiBuffer** fMidiPlaybackBuffer;

    int fConnectTimeOut;

    JackNetExtSlave(const char* ip,
                    int port,
                    const char* name,
                    jack_slave_t* request)
        :fThread(this),
        fProcessCallback(NULL),fProcessArg(NULL),
        fShutdownCallback(NULL), fShutdownArg(NULL),
        fBufferSizeCallback(NULL), fBufferSizeArg(NULL),
        fSampleRateCallback(NULL), fSampleRateArg(NULL),
        fAudioCaptureBuffer(NULL), fAudioPlaybackBuffer(NULL),
        fMidiCaptureBuffer(NULL), fMidiPlaybackBuffer(NULL)
    {
        char host_name[JACK_CLIENT_NAME_SIZE];

        // Request parameters
        assert(strlen(ip) < 32);
        strcpy(fMulticastIP, ip);
        fParams.fMtu = request->mtu;
        fParams.fTransportSync = 0;
        fParams.fSendAudioChannels = request->audio_input;
        fParams.fReturnAudioChannels = request->audio_output;
        fParams.fSendMidiChannels = request->midi_input;
        fParams.fReturnMidiChannels = request->midi_output;
        fParams.fNetworkLatency = request->latency;
        fParams.fSampleEncoder = request->encoder;
        fParams.fKBps = request->kbps;
        fParams.fSlaveSyncMode = 1;
        fConnectTimeOut = request->time_out;

        // Create name with hostname and client name
        GetHostName(host_name, JACK_CLIENT_NAME_SIZE);
        snprintf(fParams.fName, JACK_CLIENT_NAME_SIZE, "%s_%s", host_name, name);
        fSocket.GetName(fParams.fSlaveNetName);

        // Set the socket parameters
        fSocket.SetPort(port);
        fSocket.SetAddress(fMulticastIP, port);
    }

    virtual ~JackNetExtSlave()
    {}

    int Open(jack_master_t* result)
    {
        // Check CELT encoder parameters
        if ((fParams.fSampleEncoder == JackCeltEncoder) && (fParams.fKBps == 0)) {
            jack_error("CELT encoder with 0 for kps...");
            return -1;
        }
        
        if ((fParams.fSampleEncoder == JackOpusEncoder) && (fParams.fKBps == 0)) {
            jack_error("Opus encoder with 0 for kps...");
            return -1;
        }

        // Check latency
        if (fParams.fNetworkLatency > NETWORK_MAX_LATENCY) {
            jack_error("Error : network latency is limited to %d", NETWORK_MAX_LATENCY);
            return -1;
        }

        // Init network connection
        if (!JackNetSlaveInterface::InitConnection(fConnectTimeOut)) {
            jack_error("Initing network fails...");
            return -1;
        }

        // Finish connection...
        if (!JackNetSlaveInterface::InitRendering()) {
            jack_error("Starting network fails...");
            return -1;
        }

        // Then set global parameters
        if (!SetParams()) {
            jack_error("SetParams error...");
            return -1;
        }

         // Set result
         if (result != NULL) {
            result->buffer_size = fParams.fPeriodSize;
            result->sample_rate = fParams.fSampleRate;
            result->audio_input = fParams.fSendAudioChannels;
            result->audio_output = fParams.fReturnAudioChannels;
            result->midi_input = fParams.fSendMidiChannels;
            result->midi_output = fParams.fReturnMidiChannels;
            strcpy(result->master_name, fParams.fMasterNetName);
        }

        AllocPorts();
        return 0;
    }

    int Restart()
    {
        // If shutdown cb is set, then call it
        if (fShutdownCallback) {
            fShutdownCallback(fShutdownArg);
        }

        // Init network connection
        if (!JackNetSlaveInterface::InitConnection(fConnectTimeOut)) {
            jack_error("Initing network fails...");
            return -1;
        }

         // Finish connection
        if (!JackNetSlaveInterface::InitRendering()) {
            jack_error("Starting network fails...");
            return -1;
        }

        // Then set global parameters
        if (!SetParams()) {
            jack_error("SetParams error...");
            return -1;
        }

        // We need to notify possibly new buffer size and sample rate (see Execute)
        if (fBufferSizeCallback) {
            fBufferSizeCallback(fParams.fPeriodSize, fBufferSizeArg);
        }

        if (fSampleRateCallback) {
            fSampleRateCallback(fParams.fSampleRate, fSampleRateArg);
        }

        AllocPorts();
        return 0;
    }

    int Close()
    {
        fSocket.Close();
        FreePorts();
        return 0;
    }

    void AllocPorts()
    {
        // Set buffers
        fAudioCaptureBuffer = new float*[fParams.fSendAudioChannels];
        for (int audio_port_index = 0; audio_port_index < fParams.fSendAudioChannels; audio_port_index++) {
            fAudioCaptureBuffer[audio_port_index] = new float[fParams.fPeriodSize];
            fNetAudioCaptureBuffer->SetBuffer(audio_port_index, fAudioCaptureBuffer[audio_port_index]);
        }

        fMidiCaptureBuffer = new JackMidiBuffer*[fParams.fSendMidiChannels];
        for (int midi_port_index = 0; midi_port_index < fParams.fSendMidiChannels; midi_port_index++) {
            fMidiCaptureBuffer[midi_port_index] = (JackMidiBuffer*)new float[fParams.fPeriodSize];
            fNetMidiCaptureBuffer->SetBuffer(midi_port_index, fMidiCaptureBuffer[midi_port_index]);
        }

        fAudioPlaybackBuffer = new float*[fParams.fReturnAudioChannels];
        for (int audio_port_index = 0; audio_port_index < fParams.fReturnAudioChannels; audio_port_index++) {
            fAudioPlaybackBuffer[audio_port_index] = new float[fParams.fPeriodSize];
            fNetAudioPlaybackBuffer->SetBuffer(audio_port_index, fAudioPlaybackBuffer[audio_port_index]);
        }

        fMidiPlaybackBuffer = new JackMidiBuffer*[fParams.fReturnMidiChannels];
        for (int midi_port_index = 0; midi_port_index < fParams.fReturnMidiChannels; midi_port_index++) {
            fMidiPlaybackBuffer[midi_port_index] = (JackMidiBuffer*)new float[fParams.fPeriodSize];
            fNetMidiPlaybackBuffer->SetBuffer(midi_port_index, fMidiPlaybackBuffer[midi_port_index]);
        }
    }

    void FreePorts()
    {
        if (fAudioCaptureBuffer) {
            for (int audio_port_index = 0; audio_port_index < fParams.fSendAudioChannels; audio_port_index++) {
                delete[] fAudioCaptureBuffer[audio_port_index];
            }
            delete[] fAudioCaptureBuffer;
            fAudioCaptureBuffer = NULL;
        }

        if (fMidiCaptureBuffer) {
            for (int midi_port_index = 0; midi_port_index < fParams.fSendMidiChannels; midi_port_index++) {
                delete[] (fMidiCaptureBuffer[midi_port_index]);
            }
            delete[] fMidiCaptureBuffer;
            fMidiCaptureBuffer = NULL;
        }

        if (fAudioPlaybackBuffer) {
            for (int audio_port_index = 0; audio_port_index < fParams.fReturnAudioChannels; audio_port_index++) {
                delete[] fAudioPlaybackBuffer[audio_port_index];
            }
            delete[] fAudioPlaybackBuffer;
            fAudioPlaybackBuffer = NULL;
        }

        if (fMidiPlaybackBuffer) {
            for (int midi_port_index = 0; midi_port_index < fParams.fReturnMidiChannels; midi_port_index++) {
                delete[] fMidiPlaybackBuffer[midi_port_index];
            }
            delete[] fMidiPlaybackBuffer;
            fMidiPlaybackBuffer = NULL;
        }
    }

    // Transport
    void EncodeTransportData()
    {}

    void DecodeTransportData()
    {}

    bool Init()
    {
        // Will do "something" on OSX only...
        UInt64 period, constraint;
        period = constraint = UInt64(1000000000.f * (float(fParams.fPeriodSize) / float(fParams.fSampleRate)));
        UInt64 computation = JackTools::ComputationMicroSec(fParams.fPeriodSize) * 1000;
        fThread.SetParams(period, computation, constraint);

        return (fThread.AcquireSelfRealTime(80) == 0);      // TODO: get a value from the server
    }

    bool Execute()
    {
        try  {
            // Keep running even in case of error
            while (fThread.GetStatus() == JackThread::kRunning) {
                if (Process() == SOCKET_ERROR) {
                    return false;
                }
            }
            return false;
        } catch (JackNetException& e) {
            // Otherwise just restart...
            e.PrintMessage();
            jack_info("NetSlave is restarted");
            fThread.DropRealTime();
            fThread.SetStatus(JackThread::kIniting);
            FreePorts();
            if (Restart() == 0 && Init()) {
                fThread.SetStatus(JackThread::kRunning);
                return true;
            } else {
                return false;
            }
        }
    }

    int Read()
    {
        //receive sync (launch the cycle)
        if (SyncRecv() == SOCKET_ERROR) {
            return SOCKET_ERROR;
        }

        DecodeSyncPacket();
        return DataRecv();
    }

    int Write()
    {
        EncodeSyncPacket();

        if (SyncSend() == SOCKET_ERROR) {
            return SOCKET_ERROR;
        }

        return DataSend();
    }

    int Process()
    {
        // Read data from the network, throw JackNetException in case of network error...
        if (Read() == SOCKET_ERROR) {
            return SOCKET_ERROR;
        }

        fProcessCallback(fParams.fPeriodSize,
                        fParams.fSendAudioChannels,
                        fAudioCaptureBuffer,
                        fParams.fSendMidiChannels,
                        (void**)fMidiCaptureBuffer,
                        fParams.fReturnAudioChannels,
                        fAudioPlaybackBuffer,
                        fParams.fReturnMidiChannels,
                        (void**)fMidiPlaybackBuffer,
                        fProcessArg);

        // Then write data to network, throw JackNetException in case of network error...
        if (Write() == SOCKET_ERROR) {
            return SOCKET_ERROR;
        }

        return 0;
    }

    int Start()
    {
        return (fProcessCallback == 0) ? -1 : fThread.StartSync();
    }

    int Stop()
    {
        return (fProcessCallback == 0) ? -1 : fThread.Kill();
    }

    // Callback
    int SetProcessCallback(JackNetSlaveProcessCallback net_callback, void *arg)
    {
        if (fThread.GetStatus() == JackThread::kRunning) {
            return -1;
        } else {
            fProcessCallback = net_callback;
            fProcessArg = arg;
            return 0;
        }
    }

    int SetShutdownCallback(JackNetSlaveShutdownCallback shutdown_callback, void *arg)
    {
        if (fThread.GetStatus() == JackThread::kRunning) {
            return -1;
        } else {
            fShutdownCallback = shutdown_callback;
            fShutdownArg = arg;
            return 0;
        }
    }

    int SetBufferSizeCallback(JackNetSlaveBufferSizeCallback bufsize_callback, void *arg)
    {
        if (fThread.GetStatus() == JackThread::kRunning) {
            return -1;
        } else {
            fBufferSizeCallback = bufsize_callback;
            fBufferSizeArg = arg;
            return 0;
        }
    }

    int SetSampleRateCallback(JackNetSlaveSampleRateCallback samplerate_callback, void *arg)
    {
        if (fThread.GetStatus() == JackThread::kRunning) {
            return -1;
        } else {
            fSampleRateCallback = samplerate_callback;
            fSampleRateArg = arg;
            return 0;
        }
    }

};

struct JackNetAdapter : public JackAudioAdapterInterface {

    JackNetAdapter(int input, int output,
                    jack_nframes_t host_buffer_size,
                    jack_nframes_t host_sample_rate,
                    jack_nframes_t adapted_buffer_size,
                    jack_nframes_t adapted_sample_rate)
        :JackAudioAdapterInterface(host_buffer_size, host_sample_rate, adapted_buffer_size, adapted_sample_rate)
    {
        fCaptureChannels = input;
        fPlaybackChannels = output;
        Create();
    }

    void Create()
    {
        //ringbuffers

        if (fCaptureChannels > 0) {
            fCaptureRingBuffer = new JackResampler*[fCaptureChannels];
        }
        if (fPlaybackChannels > 0) {
            fPlaybackRingBuffer = new JackResampler*[fPlaybackChannels];
        }

        if (fAdaptative) {
            AdaptRingBufferSize();
            jack_info("Ringbuffer automatic adaptative mode size = %d frames", fRingbufferCurSize);
        } else {
            if (fRingbufferCurSize > DEFAULT_RB_SIZE) {
                fRingbufferCurSize = DEFAULT_RB_SIZE;
            }
            jack_info("Fixed ringbuffer size = %d frames", fRingbufferCurSize);
        }

        for (int i = 0; i < fCaptureChannels; i++ ) {
            fCaptureRingBuffer[i] = new JackResampler();
            fCaptureRingBuffer[i]->Reset(fRingbufferCurSize);
        }
        for (int i = 0; i < fPlaybackChannels; i++ ) {
            fPlaybackRingBuffer[i] = new JackResampler();
            fPlaybackRingBuffer[i]->Reset(fRingbufferCurSize);
        }

        if (fCaptureChannels > 0) {
            jack_log("ReadSpace = %ld", fCaptureRingBuffer[0]->ReadSpace());
        }
        if (fPlaybackChannels > 0) {
            jack_log("WriteSpace = %ld", fPlaybackRingBuffer[0]->WriteSpace());
        }
    }

    virtual ~JackNetAdapter()
    {
        Destroy();
    }

    void Flush()
    {
        for (int i = 0; i < fCaptureChannels; i++ ) {
            fCaptureRingBuffer[i]->Reset(fRingbufferCurSize);
        }
        for (int i = 0; i < fPlaybackChannels; i++ ) {
            fPlaybackRingBuffer[i]->Reset(fRingbufferCurSize);
        }
    }

};


} // end of namespace

using namespace Jack;

LIB_EXPORT jack_net_slave_t* jack_net_slave_open(const char* ip, int port, const char* name, jack_slave_t* request, jack_master_t* result)
{
    JackNetExtSlave* slave = new JackNetExtSlave(ip, port, name, request);
    if (slave->Open(result) == 0) {
        return (jack_net_slave_t*)slave;
    } else {
        delete slave;
        return NULL;
    }
}

LIB_EXPORT int jack_net_slave_close(jack_net_slave_t* net)
{
    JackNetExtSlave* slave = (JackNetExtSlave*)net;
    slave->Close();
    delete slave;
    return 0;
}

LIB_EXPORT int jack_set_net_slave_process_callback(jack_net_slave_t* net, JackNetSlaveProcessCallback net_callback, void *arg)
{
     JackNetExtSlave* slave = (JackNetExtSlave*)net;
     return slave->SetProcessCallback(net_callback, arg);
}

LIB_EXPORT int jack_net_slave_activate(jack_net_slave_t* net)
{
    JackNetExtSlave* slave = (JackNetExtSlave*)net;
    return slave->Start();
}

LIB_EXPORT int jack_net_slave_deactivate(jack_net_slave_t* net)
{
    JackNetExtSlave* slave = (JackNetExtSlave*)net;
    return slave->Stop();
}

LIB_EXPORT int jack_set_net_slave_buffer_size_callback(jack_net_slave_t *net, JackNetSlaveBufferSizeCallback bufsize_callback, void *arg)
{
    JackNetExtSlave* slave = (JackNetExtSlave*)net;
    return slave->SetBufferSizeCallback(bufsize_callback, arg);
}

LIB_EXPORT int jack_set_net_slave_sample_rate_callback(jack_net_slave_t *net, JackNetSlaveSampleRateCallback samplerate_callback, void *arg)
{
    JackNetExtSlave* slave = (JackNetExtSlave*)net;
    return slave->SetSampleRateCallback(samplerate_callback, arg);
}

LIB_EXPORT int jack_set_net_slave_shutdown_callback(jack_net_slave_t *net, JackNetSlaveShutdownCallback shutdown_callback, void *arg)
{
    JackNetExtSlave* slave = (JackNetExtSlave*)net;
    return slave->SetShutdownCallback(shutdown_callback, arg);
}

// Master API

LIB_EXPORT jack_net_master_t* jack_net_master_open(const char* ip, int port, const char* name, jack_master_t* request, jack_slave_t* result)
{
    JackNetExtMaster* master = new JackNetExtMaster(ip, port, name, request);
    if (master->Open(result) == 0) {
        return (jack_net_master_t*)master;
    } else {
        delete master;
        return NULL;
    }
}

LIB_EXPORT int jack_net_master_close(jack_net_master_t* net)
{
    JackNetExtMaster* master = (JackNetExtMaster*)net;
    master->Close();
    delete master;
    return 0;
}

LIB_EXPORT int jack_net_master_recv(jack_net_master_t* net, int audio_input, float** audio_input_buffer, int midi_input, void** midi_input_buffer)
{
    JackNetExtMaster* master = (JackNetExtMaster*)net;
    return master->Read(audio_input, audio_input_buffer, midi_input, midi_input_buffer);
}

LIB_EXPORT int jack_net_master_send(jack_net_master_t* net, int audio_output, float** audio_output_buffer, int midi_output, void** midi_output_buffer)
{
    JackNetExtMaster* master = (JackNetExtMaster*)net;
    return master->Write(audio_output, audio_output_buffer, midi_output, midi_output_buffer);
}

// Adapter API

LIB_EXPORT jack_adapter_t* jack_create_adapter(int input, int output,
                                                jack_nframes_t host_buffer_size,
                                                jack_nframes_t host_sample_rate,
                                                jack_nframes_t adapted_buffer_size,
                                                jack_nframes_t adapted_sample_rate)
{
    try {
        return (jack_adapter_t*)new JackNetAdapter(input, output, host_buffer_size, host_sample_rate, adapted_buffer_size, adapted_sample_rate);
    } catch (...) {
        return NULL;
    }
}

LIB_EXPORT int jack_destroy_adapter(jack_adapter_t* adapter)
{
    delete((JackNetAdapter*)adapter);
    return 0;
}

LIB_EXPORT void jack_flush_adapter(jack_adapter_t* adapter)
{
    JackNetAdapter* slave = (JackNetAdapter*)adapter;
    slave->Flush();
}

LIB_EXPORT int jack_adapter_push_and_pull(jack_adapter_t* adapter, float** input, float** output, unsigned int frames)
{
    JackNetAdapter* slave = (JackNetAdapter*)adapter;
    return slave->PushAndPull(input, output, frames);
}

LIB_EXPORT int jack_adapter_pull_and_push(jack_adapter_t* adapter, float** input, float** output, unsigned int frames)
{
    JackNetAdapter* slave = (JackNetAdapter*)adapter;
    return slave->PullAndPush(input, output, frames);
}

static void jack_format_and_log(int level, const char *prefix, const char *fmt, va_list ap)
{
    static const char* netjack_log = getenv("JACK_NETJACK_LOG");
    static bool is_netjack_log = (netjack_log) ? atoi(netjack_log) : 0;

    if (is_netjack_log) {
        char buffer[300];
        size_t len;

        if (prefix != NULL) {
            len = strlen(prefix);
            memcpy(buffer, prefix, len);
        } else {
            len = 0;
        }

        vsnprintf(buffer + len, sizeof(buffer) - len, fmt, ap);
        printf("%s", buffer);
        printf("\n");
    }
}

LIB_EXPORT void jack_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    jack_format_and_log(LOG_LEVEL_INFO, "Jack: ", fmt, ap);
    va_end(ap);
}

LIB_EXPORT void jack_info(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    jack_format_and_log(LOG_LEVEL_INFO, "Jack: ", fmt, ap);
    va_end(ap);
}

LIB_EXPORT void jack_log(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    jack_format_and_log(LOG_LEVEL_INFO, "Jack: ", fmt, ap);
    va_end(ap);
}
