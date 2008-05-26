/*
  Copyright (C) 2001 Paul Davis
  Copyright (C) 2004 Jack O'Quin
  
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

#ifndef __jack_types_h__
#define __jack_types_h__

#ifdef WIN32
#include <windows.h>
#define vsnprintf _vsnprintf
#define snprintf _snprintf
#ifndef __MINGW32__
	typedef long int32_t;
	typedef unsigned long uint32_t;
#else
	#include <stdint.h>
#endif
	typedef char int8_t;
	typedef unsigned char uint8_t;
	typedef short int16_t;
	typedef unsigned short uint16_t;
	typedef LONGLONG int64_t;
	typedef ULONGLONG uint64_t;
#else
	#include <inttypes.h>
#endif

typedef int32_t jack_shmsize_t;

/**
 * Type used to represent sample frame counts.
 */
typedef uint32_t	jack_nframes_t;

/**
 * Maximum value that can be stored in jack_nframes_t
 */
#define JACK_MAX_FRAMES (4294967295U)	/* This should be UINT32_MAX, but C++ has a problem with that. */

/**
 * Type used to represent the value of free running
 * monotonic clock with units of microseconds.
 */

#ifdef WIN32
typedef int64_t jack_time_t;
#else
typedef uint64_t jack_time_t;
#endif

/**
 *  Maximum size of @a load_init string passed to an internal client
 *  jack_initialize() function via jack_internal_client_load().
 */
#define JACK_LOAD_INIT_LIMIT 1024

/**
 *  jack_intclient_t is an opaque type representing a loaded internal
 *  client.  You may only access it using the API provided in @ref
 *  intclient.h "<jack/intclient.h>".
 */
typedef uint64_t jack_intclient_t;

/**
 *  jack_port_t is an opaque type.  You may only access it using the
 *  API provided.
 */
typedef struct _jack_port jack_port_t;

/**
 *  jack_client_t is an opaque type.  You may only access it using the
 *  API provided.
 */
typedef struct _jack_client jack_client_t;

/**
 *  Ports have unique ids. A port registration callback is the only
 *  place you ever need to know their value.
 */
#if defined(__x86_64__) || defined(__ppc64__)
typedef uint64_t jack_port_id_t;
#else
typedef uint32_t jack_port_id_t;
#endif

typedef uint32_t jack_port_type_id_t;

/**
 * Prototype for the client supplied function that is called 
 * by the engine anytime there is work to be done.
 *
 * @pre nframes == jack_get_buffer_size()
 * @pre nframes == pow(2,x)
 *
 * @param nframes number of frames to process
 * @param arg pointer to a client supplied structure
 *
 * @return zero on success, non-zero on error
 */
typedef int (*JackProcessCallback)(jack_nframes_t nframes, void *arg);

/**
 * Prototype for the client thread routine called 
 * by the engine when the client is inserted in the graph.
 *
 * @param arg pointer to a client supplied structure
 *
 */
typedef void *(*JackThreadCallback)(void* arg);

/**
 * Prototype for the client supplied function that is called 
 * once after the creation of the thread in which other
 * callbacks will be made. Special thread characteristics
 * can be set from this callback, for example. This is a
 * highly specialized callback and most clients will not
 * and should not use it.
 *
 * @param arg pointer to a client supplied structure
 *
 * @return void
 */
typedef void (*JackThreadInitCallback)(void *arg);

/**
 * Prototype for the client supplied function that is called 
 * whenever the processing graph is reordered.
 *
 * @param arg pointer to a client supplied structure
 *
 * @return zero on success, non-zero on error
 */
typedef int (*JackGraphOrderCallback)(void *arg);

/**
 * Prototype for the client-supplied function that is called whenever
 * an xrun has occured.
 *
 * @see jack_get_xrun_delayed_usecs()
 *
 * @param arg pointer to a client supplied structure
 *
 * @return zero on success, non-zero on error
 */
typedef int (*JackXRunCallback)(void *arg);

/**
 * Prototype for the @a bufsize_callback that is invoked whenever the
 * JACK engine buffer size changes.  Although this function is called
 * in the JACK process thread, the normal process cycle is suspended
 * during its operation, causing a gap in the audio flow.  So, the @a
 * bufsize_callback can allocate storage, touch memory not previously
 * referenced, and perform other operations that are not realtime
 * safe.
 *
 * @param nframes buffer size
 * @param arg pointer supplied by jack_set_buffer_size_callback().
 *
 * @return zero on success, non-zero on error
 */
typedef int (*JackBufferSizeCallback)(jack_nframes_t nframes, void *arg);

/**
 * Prototype for the client supplied function that is called 
 * when the engine sample rate changes.
 *
 * @param nframes new engine sample rate
 * @param arg pointer to a client supplied structure
 *
 * @return zero on success, non-zero on error
 */
typedef int (*JackSampleRateCallback)(jack_nframes_t nframes, void *arg);

/**
 * Prototype for the client supplied function that is called 
 * whenever a port is registered or unregistered.
 *
 * @param arg pointer to a client supplied structure
 */
typedef void (*JackPortRegistrationCallback)(jack_port_id_t port, int, void *arg);

/**
 * Prototype for the client supplied function that is called 
 * whenever a client is registered or unregistered.
 *
 * @param name a null-terminated string containing the client name 
 * @param register non-zero if the client is being registered,
 *                     zero if the client is being unregistered
 * @param arg pointer to a client supplied structure
 */ 
typedef void (*JackClientRegistrationCallback)(const char* name, int val, void *arg);

/**
 * Prototype for the client supplied function that is called 
 * whenever a client is registered or unregistered.
 *
 * @param a one of two ports connected or disconnected
 * @param b one of two ports connected or disconnected
 * @param connect non-zero if ports were connected
 *                    zero if ports were disconnected
 * @param arg pointer to a client supplied data
 */ 
typedef void (*JackPortConnectCallback)(jack_port_id_t a, jack_port_id_t b, int connect, void* arg);

/**
 * Prototype for the client supplied function that is called 
 * whenever jackd starts or stops freewheeling.
 *
 * @param starting non-zero if we start starting to freewheel, zero otherwise
 * @param arg pointer to a client supplied structure
 */
typedef void (*JackFreewheelCallback)(int starting, void *arg);


/**
 * Used for the type argument of jack_port_register() for default
 * audio ports and midi ports.
 */
#define JACK_DEFAULT_AUDIO_TYPE "32 bit float mono audio"
#define JACK_DEFAULT_MIDI_TYPE "8 bit raw midi"

/**
 * For convenience, use this typedef if you want to be able to change
 * between float and double. You may want to typedef sample_t to
 * jack_default_audio_sample_t in your application.
 */
typedef float jack_default_audio_sample_t;

/**
 *  A port has a set of flags that are formed by AND-ing together the
 *  desired values from the list below. The flags "JackPortIsInput" and
 *  "JackPortIsOutput" are mutually exclusive and it is an error to use
 *  them both.
 */
enum JackPortFlags {

    /**
     * if JackPortIsInput is set, then the port can receive
     * data.
     */
    JackPortIsInput = 0x1,

    /**
     * if JackPortIsOutput is set, then data can be read from
     * the port.
     */
    JackPortIsOutput = 0x2,

    /**
     * if JackPortIsPhysical is set, then the port corresponds
     * to some kind of physical I/O connector.
     */
    JackPortIsPhysical = 0x4,

    /**
     * if JackPortCanMonitor is set, then a call to
     * jack_port_request_monitor() makes sense.
     *
     * Precisely what this means is dependent on the client. A typical
     * result of it being called with TRUE as the second argument is
     * that data that would be available from an output port (with
     * JackPortIsPhysical set) is sent to a physical output connector
     * as well, so that it can be heard/seen/whatever.
     * 
     * Clients that do not control physical interfaces
     * should never create ports with this bit set.
     */
    JackPortCanMonitor = 0x8,

    /**
     * JackPortIsTerminal means:
     *
     *	for an input port: the data received by the port
     *                    will not be passed on or made
     *		           available at any other port
     *
     * for an output port: the data available at the port
     *                    does not originate from any other port
     *
     * Audio synthesizers, I/O hardware interface clients, HDR
     * systems are examples of clients that would set this flag for
     * their ports.
     */
    JackPortIsTerminal = 0x10
};

/**
 *  @ref jack_options_t bits
 */
enum JackOptions {

    /**
     * Null value to use when no option bits are needed.
     */
    JackNullOption = 0x00,

    /**
     * Do not automatically start the JACK server when it is not
     * already running.  This option is always selected if
     * \$JACK_NO_START_SERVER is defined in the calling process
     * environment.
     */
    JackNoStartServer = 0x01,

    /**
     * Use the exact client name requested.  Otherwise, JACK
     * automatically generates a unique one, if needed.
     */
    JackUseExactName = 0x02,

    /**
     * Open with optional <em>(char *) server_name</em> parameter.
     */
    JackServerName = 0x04,

    /**
     * Load internal client from optional <em>(char *)
     * load_name</em>.  Otherwise use the @a client_name.
     */
    JackLoadName = 0x08,

    /**
     * Pass optional <em>(char *) load_init</em> string to the
     * jack_initialize() entry point of an internal client.
     */
    JackLoadInit = 0x10
};

/** Valid options for opening an external client. */
#define JackOpenOptions (JackServerName|JackNoStartServer|JackUseExactName)

/** Valid options for loading an internal client. */
#define JackLoadOptions (JackLoadInit|JackLoadName|JackUseExactName)

/**
 *  Options for several JACK operations, formed by OR-ing together the
 *  relevant @ref JackOptions bits.
 */
typedef enum JackOptions jack_options_t;

/**
 *  @ref jack_status_t bits
 */
enum JackStatus {

    /**
     * Overall operation failed.
     */
    JackFailure = 0x01,

    /**
     * The operation contained an invalid or unsupported option.
     */
    JackInvalidOption = 0x02,

    /**
     * The desired client name was not unique.  With the @ref
     * JackUseExactName option this situation is fatal.  Otherwise,
     * the name was modified by appending a dash and a two-digit
     * number in the range "-01" to "-99".  The
     * jack_get_client_name() function will return the exact string
     * that was used.  If the specified @a client_name plus these
     * extra characters would be too long, the open fails instead.
     */
    JackNameNotUnique = 0x04,

    /**
     * The JACK server was started as a result of this operation.
     * Otherwise, it was running already.  In either case the caller
     * is now connected to jackd, so there is no race condition.
     * When the server shuts down, the client will find out.
     */
    JackServerStarted = 0x08,

    /**
     * Unable to connect to the JACK server.
     */
    JackServerFailed = 0x10,

    /**
     * Communication error with the JACK server.
     */
    JackServerError = 0x20,

    /**
     * Requested client does not exist.
     */
    JackNoSuchClient = 0x40,

    /**
     * Unable to load internal client
     */
    JackLoadFailure = 0x80,

    /**
     * Unable to initialize client
     */
    JackInitFailure = 0x100,

    /**
     * Unable to access shared memory
     */
    JackShmFailure = 0x200,

    /**
     * Client's protocol version does not match
     */
    JackVersionError = 0x400
};

/**
 *  Status word returned from several JACK operations, formed by
 *  OR-ing together the relevant @ref JackStatus bits.
 */
typedef enum JackStatus jack_status_t;

#endif /* __jack_types_h__ */
