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

  $Id: transport_types.h,v 1.1.2.2 2006/06/20 14:44:00 letz Exp $
*/

#ifndef __jack_transport_aux_h__
#define __jack_transport_aux_h__

#ifdef __cplusplus
extern "C"
{
#endif

//#include "types.h"
#include <jack/types.h>

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

#ifdef __cplusplus
}
#endif

#endif /* __jack_transport_aux_h__ */
