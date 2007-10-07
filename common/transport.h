/*
  Copyright (C) 2002 Paul Davis
  Copyright (C) 2003 Jack O'Quin
  
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

#ifndef __jack_transport_h__
#define __jack_transport_h__

#ifdef __cplusplus
extern "C"
{
#endif

//#include <jack/types.h>
#include "types.h"

    /**
     * Transport states.
     */
    typedef enum {

        /* the order matters for binary compatibility */
        JackTransportStopped = 0,  	/**< Transport halted */
        JackTransportRolling = 1,  	/**< Transport playing */
        JackTransportLooping = 2,  	/**< For OLD_TRANSPORT, now ignored */
        JackTransportStarting = 3, 	/**< Waiting for sync ready */
        JackTransportSynching = 4	/**< temporary*/

    } jack_transport_state_t;

    typedef uint64_t jack_unique_t;		/**< Unique ID (opaque) */

    /**
     * Optional struct jack_position_t fields.
     */
    typedef enum {

        JackPositionBBT = 0x10,  	/**< Bar, Beat, Tick */
        JackPositionTimecode = 0x20	/**< External timecode */

    } jack_position_bits_t;

    /** all valid position bits */
#define JACK_POSITION_MASK (JackPositionBBT|JackPositionTimecode)
#define EXTENDED_TIME_INFO

    /**
     * Struct for transport position information.
     */
    typedef struct {

        /* these four cannot be set from clients: the server sets them */
        jack_unique_t	unique_1;	/**< unique ID */
        jack_time_t	usecs;		/**< monotonic, free-rolling */
        jack_nframes_t	frame_rate;	/**< current frame rate (per second) */
        jack_nframes_t	frame;		/**< frame number, always present */

        jack_position_bits_t valid;		/**< which other fields are valid */

        /* JackPositionBBT fields: */
        int32_t	bar;		/**< current bar */
        int32_t	beat;		/**< current beat-within-bar */
        int32_t	tick;		/**< current tick-within-beat */
        double	bar_start_tick;

        float	beats_per_bar;	/**< time signature "numerator" */
        float	beat_type;	/**< time signature "denominator" */
        double	ticks_per_beat;
        double	beats_per_minute;

        /* JackPositionTimecode fields:	(EXPERIMENTAL: could change) */
        double	frame_time;	/**< current time in seconds */
        double	next_time;	/**< next sequential frame_time
                					     (unless repositioned) */

        /* For binary compatibility, new fields should be allocated from
         * this padding area with new valid bits controlling access, so
         * the existing structure size and offsets are preserved. */
        int32_t	padding[10];

        /* When (unique_1 == unique_2) the contents are consistent. */
        jack_unique_t	unique_2;	/**< unique ID */

    }
    jack_position_t;

    /**
     * Called by the timebase master to release itself from that
     * responsibility.
     *
     * If the timebase master releases the timebase or leaves the JACK
     * graph for any reason, the JACK engine takes over at the start of
     * the next process cycle.  The transport state does not change.  If
     * rolling, it continues to play, with frame numbers as the only
     * available position information.
     *
     * @see jack_set_timebase_callback
     *
     * @param client the JACK client structure.
     *
     * @return 0 on success, otherwise a non-zero error code.
     */

    int jack_release_timebase (jack_client_t *client);

    /**
     * Prototype for the @a sync_callback defined by slow-sync clients.
     * When the client is active, this callback is invoked just before
     * process() in the same thread.  This occurs once after registration,
     * then subsequently whenever some client requests a new position, or
     * the transport enters the ::JackTransportStarting state.  This
     * realtime function must not wait.
     *
     * The transport @a state will be:
     *
     *   - ::JackTransportStopped when a new position is requested;
     *   - ::JackTransportStarting when the transport is waiting to start;
     *   - ::JackTransportRolling when the timeout has expired, and the
     *   position is now a moving target.
     *
     * @param state current transport state.
     * @param pos new transport position.
     * @param arg the argument supplied by jack_set_sync_callback().
     *
     * @return TRUE (non-zero) when ready to roll.
     */
    typedef int (*JackSyncCallback)(jack_transport_state_t state,
                                    jack_position_t *pos,
                                    void *arg);

    /**
     * Register (or unregister) as a slow-sync client, one that cannot
     * respond immediately to transport position changes.
     *
     * The @a sync_callback will be invoked at the first available
     * opportunity after its registration is complete.  If the client is
     * currently active this will be the following process cycle,
     * otherwise it will be the first cycle after calling jack_activate().
     * After that, it runs according to the ::JackSyncCallback rules.
     * Clients that don't set a @a sync_callback are assumed to be ready
     * immediately any time the transport wants to start.
     *
     * @param client the JACK client structure.
     * @param sync_callback is a realtime function that returns TRUE when
     * the client is ready.  Setting @a sync_callback to NULL declares that
     * this client no longer requires slow-sync processing.
     * @param arg an argument for the @a sync_callback function.
     *
     * @return 0 on success, otherwise a non-zero error code.
     */

    int jack_set_sync_callback (jack_client_t *client,
                                JackSyncCallback sync_callback,
                                void *arg);
    /**
     * Set the timeout value for slow-sync clients.
     *
     * This timeout prevents unresponsive slow-sync clients from
     * completely halting the transport mechanism.  The default is two
     * seconds.  When the timeout expires, the transport starts rolling,
     * even if some slow-sync clients are still unready.  The @a
     * sync_callbacks of these clients continue being invoked, giving them
     * a chance to catch up.
     *
     * @see jack_set_sync_callback
     *
     * @param client the JACK client structure.
     * @param timeout is delay (in microseconds) before the timeout expires.
     *
     * @return 0 on success, otherwise a non-zero error code.
     */

    int jack_set_sync_timeout (jack_client_t *client,
                               jack_time_t timeout);
    /**
     * Prototype for the @a timebase_callback used to provide extended
     * position information.  Its output affects all of the following
     * process cycle.  This realtime function must not wait.
     *
     * This function is called immediately after process() in the same
     * thread whenever the transport is rolling, or when any client has
     * requested a new position in the previous cycle.  The first cycle
     * after jack_set_timebase_callback() is also treated as a new
     * position, or the first cycle after jack_activate() if the client
     * had been inactive.
     *
     * The timebase master may not use its @a pos argument to set @a
     * pos->frame.  To change position, use jack_transport_reposition() or
     * jack_transport_locate().  These functions are realtime-safe, the @a
     * timebase_callback can call them directly.
     *
     * @param state current transport state.
     * @param nframes number of frames in current period.
     * @param pos address of the position structure for the next cycle; @a
     * pos->frame will be its frame number.  If @a new_pos is FALSE, this
     * structure contains extended position information from the current
     * cycle.  If TRUE, it contains whatever was set by the requester.
     * The @a timebase_callback's task is to update the extended
     * information here.
     * @param new_pos TRUE (non-zero) for a newly requested @a pos, or for
     * the first cycle after the @a timebase_callback is defined.
     * @param arg the argument supplied by jack_set_timebase_callback().
     */
    typedef void (*JackTimebaseCallback)(jack_transport_state_t state,
                                         jack_nframes_t nframes,
                                         jack_position_t *pos,
                                         int new_pos,
                                         void *arg);

    /**
     * Register as timebase master for the JACK subsystem.
     *
     * The timebase master registers a callback that updates extended
     * position information such as beats or timecode whenever necessary.
     * Without this extended information, there is no need for this
     * function.
     *
     * There is never more than one master at a time.  When a new client
     * takes over, the former @a timebase_callback is no longer called.
     * Taking over the timebase may be done conditionally, so it fails if
     * there was a master already.
     *
     * @param client the JACK client structure.
     * @param conditional non-zero for a conditional request.
     * @param timebase_callback is a realtime function that returns
     * position information.
     * @param arg an argument for the @a timebase_callback function.
     *
     * @return
     *   - 0 on success;
     *   - EBUSY if a conditional request fails because there was already a
     *   timebase master;
     *   - other non-zero error code.
     */

    int jack_set_timebase_callback (jack_client_t *client,
                                    int conditional,
                                    JackTimebaseCallback timebase_callback,
                                    void *arg);
    /**
     * Reposition the transport to a new frame number.
     *
     * May be called at any time by any client.  The new position takes
     * effect in two process cycles.  If there are slow-sync clients and
     * the transport is already rolling, it will enter the
     * ::JackTransportStarting state and begin invoking their @a
     * sync_callbacks until ready.  This function is realtime-safe.
     *
     * @see jack_transport_reposition, jack_set_sync_callback
     * 
     * @param client the JACK client structure.
     * @param frame frame number of new transport position.
     *
     * @return 0 if valid request, non-zero otherwise.
     */

    int jack_transport_locate (jack_client_t *client,
                               jack_nframes_t frame);
    /**
     * Query the current transport state and position.
     *
     * This function is realtime-safe, and can be called from any thread.
     * If called from the process thread, @a pos corresponds to the first
     * frame of the current cycle and the state returned is valid for the
     * entire cycle.
     *
     * @param client the JACK client structure.
     * @param pos pointer to structure for returning current transport
     * position; @a pos->valid will show which fields contain valid data.
     * If @a pos is NULL, do not return position information.
     *
     * @return Current transport state.
     */

    jack_transport_state_t jack_transport_query (const jack_client_t *client,
            jack_position_t *pos);
    /**
     * Return an estimate of the current transport frame,
     * including any time elapsed since the last transport
     * positional update.
     *
     * @param client the JACK client structure
     */

    jack_nframes_t jack_get_current_transport_frame (const jack_client_t *client);
    /**
     * Request a new transport position.
     *
     * May be called at any time by any client.  The new position takes
     * effect in two process cycles.  If there are slow-sync clients and
     * the transport is already rolling, it will enter the
     * ::JackTransportStarting state and begin invoking their @a
     * sync_callbacks until ready.  This function is realtime-safe.
     *
     * @see jack_transport_locate, jack_set_sync_callback
     * 
     * @param client the JACK client structure.
     * @param pos requested new transport position.
     *
     * @return 0 if valid request, EINVAL if position structure rejected.
     */

    int jack_transport_reposition (jack_client_t *client,
                                   jack_position_t *pos);
    /**
     * Start the JACK transport rolling.
     *
     * Any client can make this request at any time.  It takes effect no
     * sooner than the next process cycle, perhaps later if there are
     * slow-sync clients.  This function is realtime-safe.
     *
     * @see jack_set_sync_callback
     *
     * @param client the JACK client structure.
     */

    void jack_transport_start (jack_client_t *client);
    /**
     * Stop the JACK transport.
     *
     * Any client can make this request at any time.  It takes effect on
     * the next process cycle.  This function is realtime-safe.
     *
     * @param client the JACK client structure.
     */

    void jack_transport_stop (jack_client_t *client);

    /*********************************************************************
     * The following interfaces are DEPRECATED.  They are only provided
     * for compatibility with the earlier JACK transport implementation.
     *********************************************************************/

    /**
     * Optional struct jack_transport_info_t fields.
     *
     * @see jack_position_bits_t.
     */
    typedef enum {

        JackTransportState = 0x1,  	/**< Transport state */
        JackTransportPosition = 0x2,  	/**< Frame number */
        JackTransportLoop = 0x4,  	/**< Loop boundaries (ignored) */
        JackTransportSMPTE = 0x8,  	/**< SMPTE (ignored) */
        JackTransportBBT = 0x10	/**< Bar, Beat, Tick */

    } jack_transport_bits_t;

    /**
     * Deprecated struct for transport position information.
     *
     * @deprecated This is for compatibility with the earlier transport
     * interface.  Use the jack_position_t struct, instead.
     */
    typedef struct {

        /* these two cannot be set from clients: the server sets them */

        jack_nframes_t frame_rate;		/**< current frame rate (per second) */
        jack_time_t usecs;		/**< monotonic, free-rolling */

        jack_transport_bits_t valid;	/**< which fields are legal to read */
        jack_transport_state_t transport_state;
        jack_nframes_t frame;
        jack_nframes_t loop_start;
        jack_nframes_t loop_end;

        long smpte_offset;	/**< SMPTE offset (from frame 0) */
        float smpte_frame_rate;	/**< 29.97, 30, 24 etc. */

        int bar;
        int beat;
        int tick;
        double bar_start_tick;

        float beats_per_bar;
        float beat_type;
        double ticks_per_beat;
        double beats_per_minute;

    }
    jack_transport_info_t;

    /**
     * Gets the current transport info structure (deprecated).
     *
     * @param client the JACK client structure.
     * @param tinfo current transport info structure.  The "valid" field
     * describes which fields contain valid data.
     *
     * @deprecated This is for compatibility with the earlier transport
     * interface.  Use jack_transport_query(), instead.
     *
     * @pre Must be called from the process thread.
     */

    void jack_get_transport_info (jack_client_t *client,
                                  jack_transport_info_t *tinfo);
    /**
      * Set the transport info structure (deprecated).
      *
      * @deprecated This function still exists for compatibility with the
      * earlier transport interface, but it does nothing.  Instead, define
      * a ::JackTimebaseCallback.
      */

    void jack_set_transport_info (jack_client_t *client,
                                  jack_transport_info_t *tinfo);

#ifdef __cplusplus
}
#endif

#endif /* __jack_transport_h__ */
