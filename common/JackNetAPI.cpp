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

#include <assert.h>
#include "JackNetInterface.h"
#include "JackPlatformPlug.h"
#include "JackError.h"
#include "JackTime.h"
#include "JackException.h"
#include "JackAudioAdapterInterface.h"

#ifdef __cplusplus
extern "C"
{
#endif
  
    // NetJack common API
    
    #define MASTER_NAME_SIZE 256

    enum JackNetMode {

        JackFastMode = 'f',
        JackNormalMode = 'n',
        JackSlowMode = 's',
    };

     typedef struct {
    
        int audio_input;
        int audio_output;
        int midi_input;
        int midi_ouput; 
        int mtu;
        int time_out;   // in millisecond, -1 means in infinite
        char mode;

    } jack_slave_t;

    typedef struct {
        
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

    SERVER_EXPORT jack_net_slave_t* jack_net_slave_open(const char* ip, int port, const char* name, jack_slave_t* request, jack_master_t* result);
    SERVER_EXPORT int jack_net_slave_close(jack_net_slave_t* net);
    
    SERVER_EXPORT int jack_net_slave_activate(jack_net_slave_t* net);
    SERVER_EXPORT int jack_net_slave_deactivate(jack_net_slave_t* net);
     
    SERVER_EXPORT int jack_set_net_slave_process_callback(jack_net_slave_t * net, JackNetSlaveProcessCallback net_callback, void *arg);
    SERVER_EXPORT int jack_set_net_slave_buffer_size_callback(jack_net_slave_t *net, JackNetSlaveBufferSizeCallback bufsize_callback, void *arg);
    SERVER_EXPORT int jack_set_net_slave_sample_rate_callback(jack_net_slave_t *net, JackNetSlaveSampleRateCallback samplerate_callback, void *arg);
    SERVER_EXPORT int jack_set_net_slave_shutdown_callback(jack_net_slave_t *net, JackNetSlaveShutdownCallback shutdown_callback, void *arg);
    
    // NetJack master API
    
    typedef struct _jack_net_master jack_net_master_t;
    
    SERVER_EXPORT jack_net_master_t* jack_net_master_open(const char* ip, int port, const char* name, jack_master_t* request, jack_slave_t* result);
    SERVER_EXPORT int jack_net_master_close(jack_net_master_t* net);
    
    SERVER_EXPORT int jack_net_master_recv(jack_net_master_t* net, int audio_input, float** audio_input_buffer, int midi_input, void** midi_input_buffer);
    SERVER_EXPORT int jack_net_master_send(jack_net_master_t* net, int audio_output, float** audio_output_buffer, int midi_output, void** midi_output_buffer);
  
    // NetJack adapter API
    
    typedef struct _jack_adapter jack_adapter_t;
    
    SERVER_EXPORT jack_adapter_t* jack_create_adapter(int input, int output,
                                                    jack_nframes_t host_buffer_size, 
                                                    jack_nframes_t host_sample_rate,
                                                    jack_nframes_t adapted_buffer_size,
                                                    jack_nframes_t adapted_sample_rate);
    SERVER_EXPORT int jack_destroy_adapter(jack_adapter_t* adapter);

    SERVER_EXPORT int jack_adapter_push_and_pull(jack_adapter_t* adapter, float** input, float** output, unsigned int frames);
    SERVER_EXPORT int jack_adapter_pull_and_push(jack_adapter_t* adapter, float** input, float** output, unsigned int frames);
         
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
    }
                
    virtual ~JackNetExtMaster()
    {}
    
    int Open(jack_slave_t* result)
    {
        // Init socket API (win32)
        if (SocketAPIInit() < 0) {
            fprintf(stderr, "Can't init Socket API, exiting...\n");
            return -1;
        }

        // Request socket
        if (fSocket.NewSocket() == SOCKET_ERROR) {
            fprintf(stderr, "Can't create the network management input socket : %s\n", StrError(NET_ERROR_CODE));
            return -1;
        }

        // Bind the socket to the local port
        if (fSocket.Bind() == SOCKET_ERROR) {
            fprintf(stderr, "Can't bind the network manager socket : %s\n", StrError(NET_ERROR_CODE));
            fSocket.Close();
            return -1;
        }

        // Join multicast group
        if (fSocket.JoinMCastGroup(fMulticastIP) == SOCKET_ERROR)
             fprintf(stderr, "Can't join multicast group : %s\n", StrError(NET_ERROR_CODE));

        // Local loop
        if (fSocket.SetLocalLoop() == SOCKET_ERROR)
            fprintf(stderr, "Can't set local loop : %s\n", StrError(NET_ERROR_CODE));

        // Set a timeout on the multicast receive (the thread can now be cancelled)
        if (fSocket.SetTimeOut(2000000) == SOCKET_ERROR)
            fprintf(stderr, "Can't set timeout : %s\n", StrError(NET_ERROR_CODE));
   
         //main loop, wait for data, deal with it and wait again
         //utility variables
        int attempt = 0;
        int rx_bytes = 0;
        
        do
        {
            session_params_t net_params;
            rx_bytes = fSocket.CatchHost(&net_params, sizeof(session_params_t), 0);
            SessionParamsNToH(&net_params, &fParams);
            
            if ((rx_bytes == SOCKET_ERROR) && (fSocket.GetError() != NET_NO_DATA)) {
                 fprintf(stderr, "Error in receive : %s\n", StrError(NET_ERROR_CODE));
                if (++attempt == 10) {
                    fprintf(stderr, "Can't receive on the socket, exiting net manager.\n" );
                    goto error;
                }
            }
            
            if (rx_bytes == sizeof(session_params_t ))  {
            
                switch (GetPacketType(&fParams)) {
                
                    case SLAVE_AVAILABLE:
                        if (MasterInit() == 0) {
                            SessionParamsDisplay(&fParams);
                            fRunning = false;
                        } else {
                            fprintf(stderr, "Can't init new net master...\n");
                            goto error;
                        }
                        break;
                        
                    case KILL_MASTER:
                         break;
                        
                    default:
                        break;
                }
            }
        }
        while (fRunning);
        
        // Set result paramaters
        result->audio_input = fParams.fSendAudioChannels;
        result->audio_output = fParams.fReturnAudioChannels;
        result->midi_input = fParams.fSendMidiChannels;
        result->midi_ouput = fParams.fReturnMidiChannels;
        result->midi_ouput = fParams.fMtu;
        result->mode = fParams.fNetworkMode;
        return 0;
        
    error:
        fSocket.Close();
        return -1;
    }
    
    int MasterInit()
    {
        // Check MASTER <==> SLAVE network protocol coherency
        if (fParams.fProtocolVersion != MASTER_PROTOCOL) {
            fprintf(stderr, "Error : slave is running with a different protocol %s\n", fParams.fName);
            return -1;
        }

        // Settings
        fSocket.GetName(fParams.fMasterNetName);
        fParams.fID = 1;
        fParams.fBitdepth = 0;
        fParams.fPeriodSize = fRequest.buffer_size;
        fParams.fSampleRate = fRequest.sample_rate;
     
        // Close request socket
        fSocket.Close();
        
        // Network slave init
        if (!JackNetMasterInterface::Init())
            return -1;

        // Set global parameters
        SetParams();
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
        unsigned int port_index;
     
        // Set buffers
        fAudioPlaybackBuffer = new float*[fParams.fSendAudioChannels];
        for (port_index = 0; port_index < fParams.fSendAudioChannels; port_index++) {
           fAudioPlaybackBuffer[port_index] = new float[fParams.fPeriodSize];
           fNetAudioPlaybackBuffer->SetBuffer(port_index, fAudioPlaybackBuffer[port_index]);
        }
        
        fMidiPlaybackBuffer = new JackMidiBuffer*[fParams.fSendMidiChannels];
        for (port_index = 0; port_index < fParams.fSendMidiChannels; port_index++) {
           fMidiPlaybackBuffer[port_index] = (JackMidiBuffer*)new float[fParams.fPeriodSize];
           fNetMidiPlaybackBuffer->SetBuffer(port_index, fMidiPlaybackBuffer[port_index]);
        }
      
        fAudioCaptureBuffer = new float*[fParams.fReturnAudioChannels];
        for (port_index = 0; port_index < fParams.fReturnAudioChannels; port_index++) {
           fAudioCaptureBuffer[port_index] = new float[fParams.fPeriodSize];
           fNetAudioCaptureBuffer->SetBuffer(port_index, fAudioCaptureBuffer[port_index]);
        }  
        
        fMidiCaptureBuffer = new JackMidiBuffer*[fParams.fReturnMidiChannels];
        for (port_index = 0; port_index < fParams.fReturnMidiChannels; port_index++) {
           fMidiCaptureBuffer[port_index] = (JackMidiBuffer*)new float[fParams.fPeriodSize];
           fNetMidiCaptureBuffer->SetBuffer(port_index, fMidiCaptureBuffer[port_index]);
        }
    }
    
    void FreePorts()
    {
        unsigned int port_index;
         
        if (fAudioPlaybackBuffer) {
            for (port_index = 0; port_index < fParams.fSendAudioChannels; port_index++)
                delete[] fAudioPlaybackBuffer[port_index];
            delete[] fAudioPlaybackBuffer;
            fAudioPlaybackBuffer = NULL;
        }
        
        if (fMidiPlaybackBuffer) {
            for (port_index = 0; port_index < fParams.fSendMidiChannels; port_index++)
                delete[] (fMidiPlaybackBuffer[port_index]);
            delete[] fMidiPlaybackBuffer;
            fMidiPlaybackBuffer = NULL;
        }

        if (fAudioCaptureBuffer) {
            for (port_index = 0; port_index < fParams.fReturnAudioChannels; port_index++)
                delete[] fAudioCaptureBuffer[port_index];
            delete[] fAudioCaptureBuffer;
            fAudioCaptureBuffer = NULL;
        }
        
        if (fMidiCaptureBuffer) {
            for (port_index = 0; port_index < fParams.fReturnMidiChannels; port_index++)
                delete[] fMidiCaptureBuffer[port_index];
            delete[] fMidiCaptureBuffer;
            fMidiCaptureBuffer = NULL;
        }
    }
    
    int Read(int audio_input, float** audio_input_buffer, int midi_input, void** midi_input_buffer)
     {
         assert((unsigned int)audio_input == fParams.fSendAudioChannels);
         int port_index;
         
         for (port_index = 0; port_index < audio_input; port_index++) {
             fNetAudioPlaybackBuffer->SetBuffer(port_index, audio_input_buffer[port_index]);
         }
         
         for (port_index = 0; port_index < midi_input; port_index++) {
             fNetMidiPlaybackBuffer->SetBuffer(port_index, ((JackMidiBuffer**)midi_input_buffer)[port_index]);
         }
        
         if (SyncRecv() == SOCKET_ERROR)
             return 0;

         DecodeSyncPacket();
         return DataRecv();
     }

     int Write(int audio_output, float** audio_output_buffer, int midi_output, void** midi_output_buffer)
     {
         assert((unsigned int)audio_output == fParams.fReturnAudioChannels);
         int port_index;
       
         for (port_index = 0;  port_index < audio_output; port_index++) {
             fNetAudioCaptureBuffer->SetBuffer(port_index, audio_output_buffer[port_index]);
         }
         
         for (port_index = 0;  port_index < midi_output; port_index++) {
             fNetMidiCaptureBuffer->SetBuffer(port_index, ((JackMidiBuffer**)midi_output_buffer)[port_index]);
         }
      
         EncodeSyncPacket();
     
         if (SyncSend() == SOCKET_ERROR)
             return SOCKET_ERROR;

         return DataSend();
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
        fParams.fReturnMidiChannels = request->midi_ouput;
        fParams.fNetworkMode = request->mode;
        fParams.fSlaveSyncMode = 1;
       
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
        // Init network connection
        if (!JackNetSlaveInterface::InitConnection()){
            return -1;
        }
             
        // Then set global parameters
        SetParams();
        
         // Set result
         if (result != NULL) {
            result->buffer_size = fParams.fPeriodSize;
            result->sample_rate = fParams.fSampleRate;
            strcpy(result->master_name, fParams.fMasterNetName);
        }
              
        AllocPorts();
        return 0;
    }
    
     int Restart()
    {
        // If shutdown cb is set, then call it
        if (fShutdownCallback)
            fShutdownCallback(fShutdownArg);
         
        // Init complete network connection
        if (!JackNetSlaveInterface::Init())
            return -1;
               
        // Then set global parameters
        SetParams();
     
        // We need to notify possibly new buffer size and sample rate (see Execute)
        if (fBufferSizeCallback) 
            fBufferSizeCallback(fParams.fPeriodSize, fBufferSizeArg);
            
        if (fSampleRateCallback) 
            fSampleRateCallback(fParams.fSampleRate, fSampleRateArg);
           
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
        unsigned int port_index;
     
        // Set buffers
        fAudioCaptureBuffer = new float*[fParams.fSendAudioChannels];
        for (port_index = 0; port_index < fParams.fSendAudioChannels; port_index++) {
           fAudioCaptureBuffer[port_index] = new float[fParams.fPeriodSize];
           fNetAudioCaptureBuffer->SetBuffer(port_index, fAudioCaptureBuffer[port_index]);
        }
        
        fMidiCaptureBuffer = new JackMidiBuffer*[fParams.fSendMidiChannels];
        for (port_index = 0; port_index < fParams.fSendMidiChannels; port_index++) {
           fMidiCaptureBuffer[port_index] = (JackMidiBuffer*)new float[fParams.fPeriodSize];
           fNetMidiCaptureBuffer->SetBuffer(port_index, fMidiCaptureBuffer[port_index]);
        }
      
        fAudioPlaybackBuffer = new float*[fParams.fReturnAudioChannels];
        for (port_index = 0; port_index < fParams.fReturnAudioChannels; port_index++) {
           fAudioPlaybackBuffer[port_index] = new float[fParams.fPeriodSize];
           fNetAudioPlaybackBuffer->SetBuffer(port_index, fAudioPlaybackBuffer[port_index]);
        }  
        
        fMidiPlaybackBuffer = new JackMidiBuffer*[fParams.fReturnMidiChannels];
        for (port_index = 0; port_index < fParams.fReturnMidiChannels; port_index++) {
           fMidiPlaybackBuffer[port_index] = (JackMidiBuffer*)new float[fParams.fPeriodSize];
           fNetMidiPlaybackBuffer->SetBuffer(port_index, fMidiPlaybackBuffer[port_index]);
        }
    }
    
    void FreePorts()
    {
        unsigned int port_index;
         
        if (fAudioCaptureBuffer) {
            for (port_index = 0; port_index < fParams.fSendAudioChannels; port_index++)
                delete[] fAudioCaptureBuffer[port_index];
            delete[] fAudioCaptureBuffer;
            fAudioCaptureBuffer = NULL;
        }
        
        if (fMidiCaptureBuffer) {
            for (port_index = 0; port_index < fParams.fSendMidiChannels; port_index++)
                delete[] (fMidiCaptureBuffer[port_index]);
            delete[] fMidiCaptureBuffer;
            fMidiCaptureBuffer = NULL;
        }

        if (fAudioPlaybackBuffer) {
            for (port_index = 0; port_index < fParams.fReturnAudioChannels; port_index++)
                delete[] fAudioPlaybackBuffer[port_index];
            delete[] fAudioPlaybackBuffer;
            fAudioPlaybackBuffer = NULL;
        }
        
        if (fMidiPlaybackBuffer) {
            for (port_index = 0; port_index < fParams.fReturnMidiChannels; port_index++)
                delete[] fMidiPlaybackBuffer[port_index];
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
        fThread.SetParams(float(fParams.fPeriodSize) / float(fParams.fSampleRate) * 1000000, 100 * 1000, 500 * 1000);  
        return (fThread.AcquireRealTime(80) == 0);      // TODO: get a value from the server
    }
    
    bool Execute()
    {
        try  {
            // Keep running even in case of error
            while (fThread.GetStatus() == JackThread::kRunning) {
                if (Process() == SOCKET_ERROR)
                    return false;
            }
            return false;
        } catch (JackNetException& e) {
        
            // Otherwise just restart...
            e.PrintMessage();
            fThread.DropRealTime();
            fThread.SetStatus(JackThread::kIniting);
            FreePorts();
            Restart();
            if (Init()) {
                fThread.SetStatus(JackThread::kRunning);
                return true;
            } else {
                return false;
            }
        }
    }
    
    int Read()
    {
        // Don't return -1 in case of sync recv failure
        // we need the process to continue for network error detection
        if (SyncRecv() == SOCKET_ERROR)
            return 0;

        DecodeSyncPacket();
        return DataRecv();
    }

    int Write()
    {
        EncodeSyncPacket();
    
        if (SyncSend() == SOCKET_ERROR)
            return SOCKET_ERROR;

        return DataSend();
    }
  
    int Process() 
    {
        // Read data from the network
        // in case of fatal network error, stop the process
        if (Read() == SOCKET_ERROR)
            return SOCKET_ERROR;
     
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
     
        // Then write data to network
        // in case of failure, stop process
        if (Write() == SOCKET_ERROR)
            return SOCKET_ERROR;

        return 0;
    }
    
    int Start()
    {
        // Finish connection.. 
        if (!JackNetSlaveInterface::InitRendering()) {
            return -1;
        }
   
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
        fCaptureRingBuffer = new JackResampler*[fCaptureChannels];
        fPlaybackRingBuffer = new JackResampler*[fPlaybackChannels];
        for (int i = 0; i < fCaptureChannels; i++ )
            fCaptureRingBuffer[i] = new JackResampler();
        for (int i = 0; i < fPlaybackChannels; i++ )
            fPlaybackRingBuffer[i] = new JackResampler();

        if (fCaptureChannels > 0)
            jack_log("ReadSpace = %ld", fCaptureRingBuffer[0]->ReadSpace());
        if (fPlaybackChannels > 0)
            jack_log("WriteSpace = %ld", fPlaybackRingBuffer[0]->WriteSpace());
    }

    virtual ~JackNetAdapter()
    {
        Destroy();
    }
     
};


} // end of namespace

using namespace Jack;

SERVER_EXPORT jack_net_slave_t* jack_net_slave_open(const char* ip, int port, const char* name, jack_slave_t* request, jack_master_t* result)
{
    JackNetExtSlave* slave = new JackNetExtSlave(ip, port, name, request);    
    if (slave->Open(result) == 0) {
        return (jack_net_slave_t*)slave;
    } else {
        delete slave;
        return NULL;
    }
}

SERVER_EXPORT int jack_net_slave_close(jack_net_slave_t* net)
{
    JackNetExtSlave* slave = (JackNetExtSlave*)net;
    slave->Close();
    delete slave;
    return 0;
}
    
SERVER_EXPORT int jack_set_net_slave_process_callback(jack_net_slave_t* net, JackNetSlaveProcessCallback net_callback, void *arg)
{
     JackNetExtSlave* slave = (JackNetExtSlave*)net;
     return slave->SetProcessCallback(net_callback, arg);
}

SERVER_EXPORT int jack_net_slave_activate(jack_net_slave_t* net)
{
    JackNetExtSlave* slave = (JackNetExtSlave*)net;
    return slave->Start();
}

SERVER_EXPORT int jack_net_slave_deactivate(jack_net_slave_t* net)
{
    JackNetExtSlave* slave = (JackNetExtSlave*)net;
    return slave->Stop();
}

SERVER_EXPORT int jack_set_net_slave_buffer_size_callback(jack_net_slave_t *net, JackNetSlaveBufferSizeCallback bufsize_callback, void *arg)
{
    JackNetExtSlave* slave = (JackNetExtSlave*)net;
    return slave->SetBufferSizeCallback(bufsize_callback, arg);
}

SERVER_EXPORT int jack_set_net_slave_sample_rate_callback(jack_net_slave_t *net, JackNetSlaveSampleRateCallback samplerate_callback, void *arg)
{
    JackNetExtSlave* slave = (JackNetExtSlave*)net;
    return slave->SetSampleRateCallback(samplerate_callback, arg);
}

SERVER_EXPORT int jack_set_net_slave_shutdown_callback(jack_net_slave_t *net, JackNetSlaveShutdownCallback shutdown_callback, void *arg)
{
    JackNetExtSlave* slave = (JackNetExtSlave*)net;
    return slave->SetShutdownCallback(shutdown_callback, arg);
}

// Master API

SERVER_EXPORT jack_net_master_t* jack_net_master_open(const char* ip, int port, const char* name, jack_master_t* request, jack_slave_t* result)
{
    JackNetExtMaster* master = new JackNetExtMaster(ip, port, name, request);    
    if (master->Open(result) == 0) {
        return (jack_net_master_t*)master;
    } else {
        delete master;
        return NULL;
    }
}

SERVER_EXPORT int jack_net_master_close(jack_net_master_t* net)
{
    JackNetExtMaster* master = (JackNetExtMaster*)net;
    master->Close();
    delete master;
    return 0;
}
SERVER_EXPORT int jack_net_master_recv(jack_net_master_t* net, int audio_input, float** audio_input_buffer, int midi_input, void** midi_input_buffer)
{
    JackNetExtMaster* slave = (JackNetExtMaster*)net;
    return slave->Read(audio_input, audio_input_buffer, midi_input, midi_input_buffer);
}

SERVER_EXPORT int jack_net_master_send(jack_net_master_t* net, int audio_output, float** audio_output_buffer, int midi_output, void** midi_output_buffer)
{
    JackNetExtMaster* slave = (JackNetExtMaster*)net;
    return slave->Write(audio_output, audio_output_buffer, midi_output, midi_output_buffer);
}

// Adapter API

SERVER_EXPORT jack_adapter_t* jack_create_adapter(int input, int output,
                                                jack_nframes_t host_buffer_size, 
                                                jack_nframes_t host_sample_rate,
                                                jack_nframes_t adapted_buffer_size,
                                                jack_nframes_t adapted_sample_rate)
{
    return (jack_adapter_t*)new JackNetAdapter(input, output, host_buffer_size, host_sample_rate, adapted_buffer_size, adapted_sample_rate);
}

SERVER_EXPORT int jack_destroy_adapter(jack_adapter_t* adapter)
{
    delete((JackNetAdapter*)adapter);
    return 0;
}

SERVER_EXPORT int jack_adapter_push_and_pull(jack_adapter_t* adapter, float** input, float** output, unsigned int frames)
{
    JackNetAdapter* slave = (JackNetAdapter*)adapter;
    return slave->PushAndPull(input, output, frames);
}

SERVER_EXPORT int jack_adapter_pull_and_push(jack_adapter_t* adapter, float** input, float** output, unsigned int frames)
{
    JackNetAdapter* slave = (JackNetAdapter*)adapter;
    return slave->PullAndPush(input, output, frames);
}


#ifdef MY_TARGET_OS_IPHONE

static void jack_format_and_log(int level, const char *prefix, const char *fmt, va_list ap)
{
    char buffer[300];
    size_t len;
    
    if (prefix != NULL) {
        len = strlen(prefix);
        memcpy(buffer, prefix, len);
    } else {
        len = 0;
    }
    
    vsnprintf(buffer + len, sizeof(buffer) - len, fmt, ap);
    printf(buffer);
    printf("\n");
}

SERVER_EXPORT void jack_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    jack_format_and_log(LOG_LEVEL_INFO, "Jack: ", fmt, ap);
    va_end(ap);}

SERVER_EXPORT void jack_info(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    jack_format_and_log(LOG_LEVEL_INFO, "Jack: ", fmt, ap);
    va_end(ap);
}

SERVER_EXPORT void jack_log(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    jack_format_and_log(LOG_LEVEL_INFO, "Jack: ", fmt, ap);
    va_end(ap);
}

#else

// Empty code for now..

SERVER_EXPORT void jack_error(const char *fmt, ...)
{}

SERVER_EXPORT void jack_info(const char *fmt, ...)
{}

SERVER_EXPORT void jack_log(const char *fmt, ...)
{}

#endif
