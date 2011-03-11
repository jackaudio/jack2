/*
  Copyright (C) 2001 Paul Davis

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

  $Id: JackTypes.h,v 1.2.2.1 2006/06/20 14:44:00 letz Exp $
*/

#ifndef __JackTypes__
#define __JackTypes__

#include "JackCompilerDeps.h"

typedef unsigned short UInt16;
#if __LP64__
typedef unsigned int UInt32;
typedef signed int   SInt32;
#else
typedef unsigned long UInt32;
typedef signed long   SInt32;
#endif

#include "JackTypes_os.h"

/**
 * Type used to represent the value of free running
 * monotonic clock with units of microseconds.
 */
typedef uint64_t jack_time_t;

typedef uint16_t jack_int_t;  // Internal type for ports and refnum

typedef enum {
	JACK_TIMER_SYSTEM_CLOCK,
	JACK_TIMER_CYCLE_COUNTER,
	JACK_TIMER_HPET,
} jack_timer_type_t;

typedef enum {
    NotTriggered,
    Triggered,
    Running,
    Finished,
} jack_client_state_t;

#endif
