/*
Copyright (C) 2001-2003 Paul Davis
Copyright (C) 2005 Jussi Laako
Copyright (C) 2004-2008 Grame

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

#include "JackConstants.h"
#include "JackTime.h"
#include "JackTypes.h"
#include "JackError.h"

#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>

jack_time_t (*_jack_get_microseconds)(void) = 0;

#if defined(__gnu_linux__) && (defined(__i386__) || defined(__x86_64__))
#define HPET_SUPPORT
#define HPET_MMAP_SIZE		1024
#define HPET_CAPS			0x000
#define HPET_PERIOD			0x004
#define HPET_COUNTER		0x0f0
#define HPET_CAPS_COUNTER_64BIT	(1 << 13)
#if defined(__x86_64__)
typedef uint64_t hpet_counter_t;
#else
typedef uint32_t hpet_counter_t;
#endif
static int hpet_fd;
static unsigned char *hpet_ptr;
static uint32_t hpet_period; /* period length in femto secs */
static uint64_t hpet_offset = 0;
static uint64_t hpet_wrap;
static hpet_counter_t hpet_previous = 0;
#endif /* defined(__gnu_linux__) && (__i386__ || __x86_64__) */

#ifdef HPET_SUPPORT

static int jack_hpet_init ()
{
	uint32_t hpet_caps;

	hpet_fd = open("/dev/hpet", O_RDONLY);
	if (hpet_fd < 0) {
		jack_error ("This system has no accessible HPET device (%s)", strerror (errno));
		return -1;
	}

	hpet_ptr = (unsigned char *) mmap(NULL, HPET_MMAP_SIZE,
					  PROT_READ, MAP_SHARED, hpet_fd, 0);
	if (hpet_ptr == MAP_FAILED) {
		jack_error ("This system has no mappable HPET device (%s)", strerror (errno));
		close (hpet_fd);
		return -1;
	}

	/* this assumes period to be constant. if needed,
	   it can be moved to the clock access function
	*/
	hpet_period = *((uint32_t *) (hpet_ptr + HPET_PERIOD));
	hpet_caps = *((uint32_t *) (hpet_ptr + HPET_CAPS));
	hpet_wrap = ((hpet_caps & HPET_CAPS_COUNTER_64BIT) &&
		(sizeof(hpet_counter_t) == sizeof(uint64_t))) ?
		0 : ((uint64_t) 1 << 32);

	return 0;
}

static jack_time_t jack_get_microseconds_from_hpet (void)
{
	hpet_counter_t hpet_counter;
	long double hpet_time;

	hpet_counter = *((hpet_counter_t *) (hpet_ptr + HPET_COUNTER));
	if (hpet_counter < hpet_previous)
		hpet_offset += hpet_wrap;
	hpet_previous = hpet_counter;
	hpet_time = (long double) (hpet_offset + hpet_counter) *
		(long double) hpet_period * (long double) 1e-9;
	return ((jack_time_t) (hpet_time + 0.5));
}

#else

static int jack_hpet_init ()
{
	jack_error ("This version of JACK or this computer does not have HPET support.\n"
		    "Please choose a different clock source.");
	return -1;
}

static jack_time_t jack_get_microseconds_from_hpet (void)
{
	/* never called */
	return 0;
}

#endif /* HPET_SUPPORT */

#define HAVE_CLOCK_GETTIME 1

#ifndef HAVE_CLOCK_GETTIME

static jack_time_t jack_get_microseconds_from_system (void)
{
	jack_time_t jackTime;
	struct timeval tv;

	gettimeofday (&tv, NULL);
	jackTime = (jack_time_t) tv.tv_sec * 1000000 + (jack_time_t) tv.tv_usec;
	return jackTime;
}

#else

static jack_time_t jack_get_microseconds_from_system (void)
{
	jack_time_t jackTime;
	struct timespec time;

	clock_gettime(CLOCK_MONOTONIC, &time);
	jackTime = (jack_time_t) time.tv_sec * 1e6 +
		(jack_time_t) time.tv_nsec / 1e3;
	return jackTime;
}

#endif /* HAVE_CLOCK_GETTIME */


SERVER_EXPORT void JackSleep(long usec)
{
	usleep(usec);
}

SERVER_EXPORT void InitTime()
{
	/* nothing to do on a generic system - we use the system clock */
}

SERVER_EXPORT void EndTime()
{}

void SetClockSource(jack_timer_type_t source)
{
    jack_log("Clock source : %s", ClockSourceName(source));

	switch (source)
	{
        case JACK_TIMER_HPET:
            if (jack_hpet_init () == 0) {
                _jack_get_microseconds = jack_get_microseconds_from_hpet;
            } else {
                _jack_get_microseconds = jack_get_microseconds_from_system;
            }
            break;

        case JACK_TIMER_SYSTEM_CLOCK:
            default:
            _jack_get_microseconds = jack_get_microseconds_from_system;
            break;
	}
}

const char* ClockSourceName(jack_timer_type_t source)
{
	switch (source) {
        case JACK_TIMER_HPET:
            return "hpet";
        case JACK_TIMER_SYSTEM_CLOCK:
        #ifdef HAVE_CLOCK_GETTIME
            return "system clock via clock_gettime";
        #else
            return "system clock via gettimeofday";
        #endif
	}

	/* what is wrong with gcc ? */
	return "unknown";
}

SERVER_EXPORT jack_time_t GetMicroSeconds()
{
	return _jack_get_microseconds();
}

SERVER_EXPORT jack_time_t jack_get_microseconds()
{
	return _jack_get_microseconds();
}

