/*
Copyright (C) 2001-2003 Paul Davis
Copyright (C) 2005 Jussi Laako
Copyright (C) 2004-2008 Grame
Copyright (C) 2018 Greg V

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

#include <time.h>
#include <unistd.h>

jack_time_t (*_jack_get_microseconds)(void) = 0;

static jack_time_t jack_get_microseconds_from_system (void)
{
	jack_time_t jackTime;
	struct timespec time;

	clock_gettime(CLOCK_MONOTONIC, &time);
	jackTime = (jack_time_t) time.tv_sec * 1e6 +
		(jack_time_t) time.tv_nsec / 1e3;
	return jackTime;
}


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
	_jack_get_microseconds = jack_get_microseconds_from_system;
}

const char* ClockSourceName(jack_timer_type_t source)
{
	return "system clock via clock_gettime";
}

SERVER_EXPORT jack_time_t GetMicroSeconds()
{
	return _jack_get_microseconds();
}

SERVER_EXPORT jack_time_t jack_get_microseconds()
{
	return _jack_get_microseconds();
}

