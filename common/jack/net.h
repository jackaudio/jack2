/*
  Copyright (C) 2009-2010 Grame

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

#ifndef __net_h__
#define __net_h__

#ifdef __cplusplus
extern "C"
{
#endif

#include <jack/systemdeps.h>
#include <jack/types.h>

#define DEFAULT_MULTICAST_IP    "225.3.19.154"
#define DEFAULT_PORT            19000
#define DEFAULT_MTU             1500
#define MASTER_NAME_SIZE        256

#define SOCKET_ERROR -1

enum JackNetEncoder {

    JackFloatEncoder = 0,   // samples are transmitted as float
    JackIntEncoder = 1,     // samples are transmitted as 16 bits integer
    JackCeltEncoder = 2,    // samples are transmitted using CELT codec (http://www.celt-codec.org/)
};

typedef struct {

    int audio_input;    // from master or to slave (-1 for get master audio physical outputs)
    int audio_output;   // to master or from slave (-1 for get master audio physical inputs)
    int midi_input;     // from master or to slave (-1 for get master MIDI physical outputs)
    int midi_output;    // to master or from slave (-1 for get master MIDI physical inputs)
    int mtu;            // network Maximum Transmission Unit
    int time_out;       // in second, -1 means in infinite
    int encoder;        // encoder type (one of JackNetEncoder)
    int kbps;           // KB per second for CELT encoder
    int latency;        // network latency

} jack_slave_t;

typedef struct {

    int audio_input;                    // master audio physical outputs
    int audio_output;                   // master audio physical inputs
    int midi_input;                     // master MIDI physical outputs
    int midi_output;                    // master MIDI physical inputs
    jack_nframes_t buffer_size;         // mater buffer size
    jack_nframes_t sample_rate;         // mater sample rate
    char master_name[MASTER_NAME_SIZE]; // master machine name

} jack_master_t;

/**
 *  jack_net_slave_t is an opaque type. You may only access it using the
 *  API provided.
 */
typedef struct _jack_net_slave jack_net_slave_t;

 /**
 * Open a network connection with the master machine.
 * @param ip the multicast address of the master
 * @param port the connection port
 * @param request a connection request structure
 * @param result a connection result structure
 *
 * @return Opaque net handle if successful or NULL in case of error.
 */
jack_net_slave_t* jack_net_slave_open(const char* ip, int port, const char* name, jack_slave_t* request, jack_master_t* result);

/**
 * Close the network connection with the master machine.
 * @param net the network connection to be closed
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_net_slave_close(jack_net_slave_t* net);

/**
 * Prototype for Process callback.
 * @param nframes buffer size
 * @param audio_input number of audio inputs
 * @param audio_input_buffer an array of audio input buffers (from master)
 * @param midi_input number of MIDI inputs
 * @param midi_input_buffer an array of MIDI input buffers (from master)
 * @param audio_output number of audio outputs
 * @param audio_output_buffer an array of audio output buffers (to master)
 * @param midi_output number of MIDI outputs
 * @param midi_output_buffer an array of MIDI output buffers (to master)
 * @param arg pointer to a client supplied structure supplied by jack_set_net_process_callback()
 *
 * @return zero on success, non-zero on error
 */
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

/**
 * Set network process callback.
 * @param net the network connection
 * @param net_callback the process callback
 * @param arg pointer to a client supplied structure
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_set_net_slave_process_callback(jack_net_slave_t * net, JackNetSlaveProcessCallback net_callback, void *arg);

/**
 * Start processing thread, the net_callback will start to be called.
 * @param net the network connection
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_net_slave_activate(jack_net_slave_t* net);

/**
 * Stop processing thread.
 * @param net the network connection
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_net_slave_deactivate(jack_net_slave_t* net);

/**
 * Prototype for BufferSize callback.
 * @param nframes buffer size
 * @param arg pointer to a client supplied structure supplied by jack_set_net_buffer_size_callback()
 *
 * @return zero on success, non-zero on error
 */
typedef int (*JackNetSlaveBufferSizeCallback)(jack_nframes_t nframes, void *arg);

/**
 * Prototype for SampleRate callback.
 * @param nframes sample rate
 * @param arg pointer to a client supplied structure supplied by jack_set_net_sample_rate_callback()
 *
 * @return zero on success, non-zero on error
 */
typedef int (*JackNetSlaveSampleRateCallback)(jack_nframes_t nframes, void *arg);

/**
 * Set network buffer size callback.
 * @param net the network connection
 * @param bufsize_callback the buffer size callback
 * @param arg pointer to a client supplied structure
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_set_net_slave_buffer_size_callback(jack_net_slave_t *net, JackNetSlaveBufferSizeCallback bufsize_callback, void *arg);

/**
 * Set network sample rate callback.
 * @param net the network connection
 * @param samplerate_callback the sample rate callback
 * @param arg pointer to a client supplied structure
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_set_net_slave_sample_rate_callback(jack_net_slave_t *net, JackNetSlaveSampleRateCallback samplerate_callback, void *arg);

/**
 * Prototype for server Shutdown callback (if not set, the client will just restart, waiting for an available master again).
 * @param arg pointer to a client supplied structure supplied by jack_set_net_shutdown_callback()
 */
typedef void (*JackNetSlaveShutdownCallback)(void* data);

/**
 * Set network shutdown callback.
 * @param net the network connection
 * @param shutdown_callback the shutdown callback
 * @param arg pointer to a client supplied structure
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_set_net_slave_shutdown_callback(jack_net_slave_t *net, JackNetSlaveShutdownCallback shutdown_callback, void *arg);

/**
 *  jack_net_master_t is an opaque type, you may only access it using the API provided.
 */
typedef struct _jack_net_master jack_net_master_t;

 /**
 * Open a network connection with the slave machine.
 * @param ip the multicast address of the master
 * @param port the connection port
 * @param request a connection request structure
 * @param result a connection result structure
 *
 * @return Opaque net handle if successful or NULL in case of error.
 */
jack_net_master_t* jack_net_master_open(const char* ip, int port, const char* name, jack_master_t* request, jack_slave_t* result);

/**
 * Close the network connection with the slave machine.
 * @param net the network connection to be closed
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_net_master_close(jack_net_master_t* net);

/**
 * Receive sync and data from the network.
 * @param net the network connection
 * @param audio_input number of audio inputs
 * @param audio_input_buffer an array of audio input buffers
 * @param midi_input number of MIDI inputs
 * @param midi_input_buffer an array of MIDI input buffers
 *
 * @return zero on success, non-zero on error
 */
int jack_net_master_recv(jack_net_master_t* net, int audio_input, float** audio_input_buffer, int midi_input, void** midi_input_buffer);

/**
 * Send sync and data to the network.
 * @param net the network connection
 * @param audio_output number of audio outputs
 * @param audio_output_buffer an array of audio output buffers
 * @param midi_output number of MIDI ouputs
 * @param midi_output_buffer an array of MIDI output buffers
 *
 * @return zero on success, non-zero on error
 */
int jack_net_master_send(jack_net_master_t* net, int audio_output, float** audio_output_buffer, int midi_output, void** midi_output_buffer);

// Experimental Adapter API

/**
 *  jack_adapter_t is an opaque type, you may only access it using the API provided.
 */
typedef struct _jack_adapter jack_adapter_t;

/**
 * Create an adapter.
 * @param input number of audio inputs
 * @param output of audio outputs
 * @param host_buffer_size the host buffer size in frames
 * @param host_sample_rate the host buffer sample rate
 * @param adapted_buffer_size the adapted buffer size in frames
 * @param adapted_sample_rate the adapted buffer sample rate
 *
 * @return 0 on success, otherwise a non-zero error code
 */
jack_adapter_t* jack_create_adapter(int input, int output,
                                    jack_nframes_t host_buffer_size,
                                    jack_nframes_t host_sample_rate,
                                    jack_nframes_t adapted_buffer_size,
                                    jack_nframes_t adapted_sample_rate);

/**
 * Destroy an adapter.
 * @param adapter the adapter to be destroyed
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_destroy_adapter(jack_adapter_t* adapter);

/**
 * Flush internal state of an adapter.
 * @param adapter the adapter to be flushed
 *
 * @return 0 on success, otherwise a non-zero error code
 */
void jack_flush_adapter(jack_adapter_t* adapter);

/**
 * Push input to and pull output from adapter ringbuffer.
 * @param adapter the adapter
 * @param input an array of audio input buffers
 * @param output an array of audio ouput buffers
 * @param frames number of frames
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_adapter_push_and_pull(jack_adapter_t* adapter, float** input, float** output, unsigned int frames);

/**
 * Pull input to and push output from adapter ringbuffer.
 * @param adapter the adapter
 * @param input an array of audio input buffers
 * @param output an array of audio ouput buffers
 * @param frames number of frames
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_adapter_pull_and_push(jack_adapter_t* adapter, float** input, float** output, unsigned int frames);

#ifdef __cplusplus
}
#endif

#endif /* __net_h__ */
